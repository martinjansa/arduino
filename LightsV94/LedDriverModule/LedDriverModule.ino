#include <EEPROM.h>

#include <RF24Network.h>
#include <RF24Network_config.h>
#include <Sync.h>

#include <SPI.h>

#include <serialassert.h>
#include <testdriver.h>
#include <PersistentConfig.h>

#include <assert.h>


#include <nRF24L01.h>
#include <RF24.h>
#include <RF24_config.h>

#include <RF24NetworkWrapper.h>
#include <RF24NetworkWrapper_Mock-Loopback.h>

#include <LightCommManager.h>
#include <LightCommMessage.h> 


// transactions durations
static const unsigned long WAKING_UP_DURATION        = 50;
static const unsigned long RGB_TRANSITION_DURATION   = 500;


// analog pins
static const int LIGHT_SENSOR_PIN       = A0; 

// digital pins
static const int SERIAL_RX_PIN          =  0;  // (used by the Serial port)
static const int SERIAL_TX_PIN          =  1;  // (used by the Serial port)
static const int LED_POWER_RELAY_PIN    =  2;
static const int RED_PWM_PIN            =  3;
//               available                 4
static const int GREEN_PWM_PIN          =  5;
static const int BLUE_PWM_PIN           =  6;
//               available                 7
//               available                 8
static const int RF24_CHIP_ENABLE_PIN   =  9;
static const int RF24_CHIP_SELECT_PIN   = 10;
static const int RF24_MOSI_PIN          = 11;  // (cannot be changed probably, see RF24 if you consider this options)
static const int RF24_MISO_PIN          = 12;  // (cannot be changed probably, see RF24 if you consider this options)
static const int RF24_SCK_PIN           = 13;  // (cannot be changed probably, see RF24 if you consider this options)

// persistent configuration
PersistentConfig ee_config;
PersistentConfigItem<byte> ee_ledDriverId(ee_config, 0);     // 1 byte, valid values are 1, 2, 3, 4
PersistentConfigItem<uint8_t> ee_rf24Channel(ee_config, 1);  // 1 byte, RF24 channel
PersistentConfigItem<uint16_t> ee_rf24Address(ee_config, 2); // 2 bytes, RF24 node address
PersistentConfigItem<uint16_t> ee_rf24ControllerAddress(ee_config, 4); // 2 bytes, RF24 controller node address

// initialize radio
RF24 rf24Radio(RF24_CHIP_ENABLE_PIN, RF24_CHIP_SELECT_PIN);
RF24Network rf24network(rf24Radio);
RF24NetworkWrapper network(rf24network);

// init the comm manager
LightCommManager commManager(network); 


/* 
 * The controller controls the operation of the LED driver module. It repetitively updates the RF24 network and handles the incomming messages
 * and monitors the inputs, detects and reports the alerts into the controller device.
 *
 * States of the controller:
 *   SLEEPING:         the LEDs are not operating and the LED power source is switched off
 *   STANDBY:          the LEDs are not operating but the LED power source is switched on
 *   ON_TRANSITION:    the LEDs are in transition from one RGB to another, while source or target can also be 0,0,0 (off)
 *   ON:               the LEDs are operating according to the required RGB settings
 *   RESET_PENDING:    the reset was required and the device wait for it to be done
 *
 * Transitions:
 *   SLEEPING -> STANDBY                       - WAKE command received, the power source is switched on
 *   (*) -> SLEEPING                           - SLEEP command received, the power source is switched off and the RGB to {0, 0, 0}
 *   STANDBY|ON|ON_TRANSITION -> ON_TRANSITION - SetRGB command received
 *   ON_TRANSITION -> STANDBY                  - automatically after transition duration expires and if new RGB is {0, 0, 0}
 *   ON_TRANSITION -> ON                       - automatically after transition duration expires and if new RGB is non zero
 *   (*) -> RESET_PENDING                      - the machine will be reset after the network update (response sent to the controller)
 */
class LedDriverController: public ILightCommLightDriverMessageHandler {
public:
  enum Status {
    S_SLEEPING           = 1,
    S_STANDBY            = 2,
    S_ON_TRANSITION      = 3,
    S_ON                 = 4,
    S_RESET_PENDING      = 5
  };
  
  enum LightSensorAlertStatus {
    LSSA_ALERT_INACTIVE    = 0,  // initial status, the alert is deactivated
    LSSA_ALERT_ACTIVE      = 1,  // the alert has been configured
    LSSA_ABOVE_HIGH_ALERT  = 2,  // the intensity is above the configured limit, reported to controller. It will not be repeared untill the decrease bellow lower limit  
    LSSA_BELLOW_LOW_ALERT  = 3,  // the intensity is above the configured limit, reported to controller. It will not be repeared untill the decrease bellow lower limit  
  };
    
private:
  Status m_status;
  
  // light intensity drop alert status and configuration
  LightSensorAlertStatus m_liDropAlertStatus;
  word m_liDropAlertHigh;
  word m_liDropAlertLow;
  word m_liDropAlertMs;
  unsigned long m_liDropAlertLastTimeAboveHigh;
  
  // light intensity gradual alert status and configuration
  LightSensorAlertStatus m_liGradualAlertStatus;
  word m_liGradualAlertHigh;
  word m_liGradualAlertLow;
    
public:
  
  LedDriverController(): 
    m_status(S_SLEEPING),
    m_liDropAlertStatus(LSSA_ALERT_INACTIVE),
    m_liDropAlertHigh(0),
    m_liDropAlertLow(0),
    m_liDropAlertMs(0),
    m_liDropAlertLastTimeAboveHigh(),
    m_liGradualAlertStatus(LSSA_ALERT_INACTIVE),
    m_liGradualAlertHigh(0),
    m_liGradualAlertLow(0)
  {
  }

  void SetLedPower(bool on)
  {
    Serial.print(F("Setting LED power relay "));
    if (on) {
      Serial.println(F("ON."));
    } else {
      Serial.println(F("OFF."));
    }      
    // write to the pin that handles the relay
    digitalWrite(LED_POWER_RELAY_PIN, (on? HIGH: LOW));
  }
  
  void UpdateLISensor()
  {
    // if the LEDs are not on (and don't disturb the light sensor)
    if (m_status == S_SLEEPING || m_status == S_STANDBY) {
    
      // read the current light intensity
      int lightIntensity = analogRead(LIGHT_SENSOR_PIN);    
      
      assert (0 <= lightIntensity && lightIntensity < 1024);

      // check the light drop alert first
      
      // if the drop alert is configured (active)
      if (m_liDropAlertStatus != LSSA_ALERT_INACTIVE) {
        
        // if we are above high limit
        if (m_liDropAlertHigh <= lightIntensity) {

          // if we are entering this status
          if (m_liDropAlertStatus != LSSA_ABOVE_HIGH_ALERT) {
            
            Serial.println(F("Light intensity drop alert: above high."));

            // update the status
            m_liDropAlertStatus = LSSA_ABOVE_HIGH_ALERT;
          }            
         
          // remember the time stamp to calculate the drop speed if the alert gets bellow the lower limit
          m_liDropAlertLastTimeAboveHigh = millis();
          
        } else {
          
          // if the we are bellow the low limit
          if (lightIntensity <= m_liDropAlertLow) {
            
            // did we switch from LSSA_ABOVE_HIGH_ALERT status?
            if (m_liDropAlertStatus == LSSA_ABOVE_HIGH_ALERT) {
             
              // calculate the transition duration
              unsigned long ms = (millis() - m_liDropAlertLastTimeAboveHigh);
              
              Serial.print(F("Light intensity drop alert: intensity has falen within "));
              Serial.print(ms);
              Serial.println(F(" ms."));
  
              // if the transition quick enough?
              if (ms <= m_liDropAlertMs) {
  
                Serial.println(F("Light intensity drop alert: drop detected!"));
                
                // send the intensity drop alert
                bool result = commManager.ReportLightIntensityDropDected(ee_rf24ControllerAddress);
                
                if (!result) {
                  
                  Serial.print(F("Light intensity drop alert: command sending FAILED!"));
                }
              }
            }
          
            // now we are bellow low
            m_liDropAlertStatus = LSSA_BELLOW_LOW_ALERT;
          }
        }
      }

      // check the gradual light decrease/increase alert first
      
      // if the gradual alert is configured (active)
      if (m_liGradualAlertStatus != LSSA_ALERT_INACTIVE) {
        
        // if we are above high limit
        if (m_liGradualAlertHigh <= lightIntensity) {

          // if we are entering this status
          if (m_liGradualAlertStatus != LSSA_ABOVE_HIGH_ALERT) {
            
            Serial.println(F("Light intensity gradual alert: high detected."));

            // send the intensity drop alert
            bool result = commManager.ReportHighLightIntensityDected(ee_rf24ControllerAddress);
                
            if (!result) {
                  
              Serial.print(F("Light intensity gradual alert: command sending FAILED!"));
            }

            // update the status
            m_liGradualAlertStatus = LSSA_ABOVE_HIGH_ALERT;
          }            
                
        } else {
          
          // if the we are bellow the low limit
          if (lightIntensity <= m_liGradualAlertLow) {
            
            // are we entering the LOW mode for the first time since last change?
            if (m_liGradualAlertStatus != LSSA_BELLOW_LOW_ALERT) {
                           
              Serial.println(F("Light intensity gradual alert: low detected!"));
                
              // send the intensity drop alert
              bool result = commManager.ReportLowLightIntensityDected(ee_rf24ControllerAddress);
                
              if (!result) {
                
                Serial.print(F("Light intensity gradual alert: command sending FAILED!"));
              }
          
              // now we are bellow
              m_liGradualAlertStatus = LSSA_BELLOW_LOW_ALERT;
            }
          }
        }
      }
    }
  }
  
  void Update()
  {
    // if the reset has been required
    if (m_status == S_RESET_PENDING) {
    
      // TODO:  
    }
    
    // handle network messages
    commManager.HandleLightDriverIncommingMessages(*this);
    
    // handle light intensity sensor input changes
    UpdateLISensor();
  }
  
  virtual LightCommError HandleReset(uint16_t senderNode)
  {
    Serial.println("Reset command received.");
    
    // register reset request
    m_status = S_RESET_PENDING;
    
    return 0;
  }

  virtual void HandleQueryInputs(uint16_t senderNode, const LightCommMessage_QueryInputs &message, RF24NetworkWrapper &network)
  {
    Serial.println("Query inputs command received.");
    
    // there should be some response in here
  }

  virtual LightCommError HandleSleep(uint16_t senderNode)
  {
    Serial.println("Sleep command received.");
    
    //if (state == 
    return 0;
  }

  virtual LightCommError HandleWake(uint16_t senderNode)
  {
    Serial.println("Wake command received.");
    return 0;
  }

//  virtual LightCommError HandleQueryStatus(uint16_t senderNode) {}

  virtual LightCommError HandleSetHSBColor(uint16_t senderNode, const LightCommMessage_SetHSBColor &message)
  {
    Serial.print("Set HSB color command received { hue: ");
    Serial.print(message.GetHue(), DEC);
    Serial.print(", sat: ");
    Serial.print(message.GetSat(), DEC);
    Serial.print(", bri: ");
    Serial.print(message.GetBri(), DEC);
    Serial.print(", ms: ");
    Serial.print(message.GetMs(), DEC);
    Serial.println("}");
    
    // TODO: hand over to the LED PWM driver
    // TODO: activate power adaptor for LEDs
    
    return 0;
  }

  virtual LightCommError HandleLiConfigDropAlert(uint16_t senderNode, const LightCommMessage_LIConfigDropAlert &message)
  {
    Serial.print("Config light intensity drop alert command received { high: ");
    Serial.print(message.GetHigh(), DEC);
    Serial.print(", low: ");
    Serial.print(message.GetLow(), DEC);
    Serial.print(", milliseconds: ");
    Serial.print(message.GetMs(), DEC);
    Serial.println("}");

    // if the drop alert should be enabled
    if (message.GetHigh() != 0 || message.GetLow() != 0 || message.GetMs() != 0) {
      
      // activate the alert and restart it
      m_liDropAlertStatus = LSSA_ALERT_ACTIVE;
      m_liDropAlertHigh = message.GetHigh();
      m_liDropAlertLow = message.GetLow();
      m_liDropAlertMs = message.GetMs();
      m_liDropAlertLastTimeAboveHigh = 0;
      
    } else {
  
      // deactivate the alert    
      m_liDropAlertStatus = LSSA_ALERT_INACTIVE;
    }
    
    return 0;
  }

  virtual LightCommError HandleLiConfigGradualAlert(uint16_t senderNode, const LightCommMessage_LiConfigGradualAlert &message)
  {
    Serial.print("Config light intensity gradual alert command received { high: ");
    Serial.print(message.GetHigh(), DEC);
    Serial.print(", low: ");
    Serial.print(message.GetLow(), DEC);
    Serial.println("}");
    
    // if the gradual alert should be enabled
    if (message.GetHigh() != 0 || message.GetLow() != 0) {
      
      // activate the alert and restart it
      m_liGradualAlertStatus = LSSA_ALERT_ACTIVE;
      m_liGradualAlertHigh = message.GetHigh();
      m_liGradualAlertLow = message.GetLow();
      
    } else {
  
      // deactivate the alert    
      m_liGradualAlertStatus = LSSA_ALERT_INACTIVE;
    }

    return 0;
  }
  
};

LedDriverController ledDriverController;

void setup()
{
  // initialize the serial communication to the PC
  Serial.begin(57600);    

  // check the potential overlaps in the persisten configuration
  assert(ee_config.CheckOverlaps());
  
  // uncomment to write initial values into the EEPROM
  // ee_ledDriverId = 2;
  // ee_rf24Channel = 90;
  // ee_rf24Address = 2;
  // ee_rf24ControllerAddress = 0;
  
  Serial.print("LED driver #");
  Serial.println(ee_ledDriverId);
  Serial.print("RF24 configuration: {channel: ");
  Serial.print(ee_rf24Channel);
  Serial.print(", node_address: ");
  Serial.print(ee_rf24Address);
  Serial.print(", controller_node_address: ");
  Serial.print(ee_rf24ControllerAddress);
  Serial.println("}");

  // initialize the network
  Serial.println("Initializing SPI");
  SPI.begin(); 
  Serial.println("Initializing radio");
  rf24Radio.begin();
  Serial.println("Initializing network");
  rf24network.begin(ee_rf24Channel, ee_rf24Address);
  Serial.println("Initializing Comm Manager");
  commManager.begin(ee_rf24Address);

  Serial.println("Initialization done.");
}


void loop()
{
  ledDriverController.Update();
}

