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

#include <RGBlink.h>

#include <LightCommManager.h>
#include <LightCommMessage.h> 


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
PersistentConfig ee_config(false);

  // ee_ledDriverId = 2;
  // ee_rf24Channel = 90;
  // ee_rf24Address = 2;
  // ee_rf24ControllerAddress = 0;

PersistentConfigItem<byte>      ee_ledDriverId              (ee_config, 0x00,     2);  // valid values are 1, 2, 3, 4
PersistentConfigItem<uint8_t>   ee_rf24Channel              (ee_config, 0x01,    90);  // RF24 channel
PersistentConfigItem<uint16_t>  ee_rf24Address              (ee_config, 0x02,     2);  // RF24 node address
PersistentConfigItem<uint16_t>  ee_rf24ControllerAddress    (ee_config, 0x04,     0);  // RF24 controller node address

PersistentConfigItem<word>      ee_LedPowerStandbyDuration  (ee_config, 0x06, 10000);  // 10 seconds

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
 */
class LedDriverController: public ILightCommLightDriverMessageHandler {
private:

  // handles the LEDs PWM with the transitions, etc.
  class LEDPowerHandler {
  private:
    enum PowerMode { PM_OFF, PM_STANDBY, PM_ON };
  
    LED rgbLedDriver;
  
    HSB m_originalColor;
    HSB m_targetColor;
    HSB m_currentColor;
    unsigned long m_transitionStartTime;
    unsigned long m_transitionDuration;
    bool m_transitionFinished;
    
    PowerMode m_powerMode;    
    unsigned long m_standByStartTime;
        
  public:
  
    LEDPowerHandler():
      rgbLedDriver(RED_PWM_PIN, GREEN_PWM_PIN, BLUE_PWM_PIN),
      m_originalColor(),
      m_targetColor(),
      m_currentColor(),
      m_transitionStartTime(millis()),   
      m_transitionDuration(0),
      m_transitionFinished(true),
      m_powerMode(PM_OFF),
      m_standByStartTime(0)
    {
      rgbLedDriver.off();
      m_targetColor.hue = 0;
      m_targetColor.sat = 0;
      m_targetColor.bri = 0;
      m_currentColor.hue = 0;
      m_currentColor.sat = 0;
      m_currentColor.bri = 0;
    }

    bool LightsOn() const
    {
      // the current color is not black or a transition is in progress
      return (m_currentColor.hue != 0 || m_currentColor.sat != 0 || m_currentColor.bri != 0 || !m_transitionFinished);  
    }
    
    void Update()
    {
      // if we are handling the light transitions
      if (!m_transitionFinished) {

        unsigned long now = millis();
      
        bool colorChanged = false;
      
        // if we are with the scheduled transition time
        if ((now - m_transitionStartTime) < m_transitionDuration) {
       
          // calculate the current step
          uint8_t step = uint8_t(255 * float(now - m_transitionStartTime) / float(m_transitionDuration));
        
          // calculate the current color
          HSB newColor = mix(m_originalColor, m_targetColor, step);
        
          // if the new color is different from the original current color
          if (m_currentColor.hue != newColor.hue || m_currentColor.sat != newColor.sat || m_currentColor.bri != newColor.bri) {
          
            // keep the calculated color
            m_currentColor = newColor;
 
            // request output update
            colorChanged = true;
          }
        
        } else {
        
          // the transition has just finished
          m_transitionFinished = true;
          
          // set the target color as the new current
          m_currentColor = m_targetColor;
          colorChanged = true;

          // if we are changing the color to black (0, 0, 0)
          if (m_currentColor.hue == 0 && m_currentColor.sat == 0 && m_currentColor.bri == 0) {
         
            // let's start the power off transition 
            m_powerMode = PM_STANDBY;

            m_standByStartTime = millis();            
          }
        }
      
        // if the color has been changed
        if (colorChanged) {

          RGB rgb = HSBtoRGB(m_currentColor);
        
          Serial.print(F("HSB color update { hue: "));
          Serial.print(m_currentColor.hue);
          Serial.print(F(", sat: "));
          Serial.print(m_currentColor.sat);
          Serial.print(F(", bri: "));
          Serial.print(m_currentColor.bri);
          Serial.print(F("}, RGB = { red: "));
          Serial.print(rgb.red);
          Serial.print(F(", green: "));
          Serial.print(rgb.green);
          Serial.print(F(", blue: "));
          Serial.print(rgb.blue);
          Serial.println(F("}."));

          // update the PWM outputs 
          rgbLedDriver.writeRGB(rgb);
        }
        
      } else {
        
        // if we are in the standby mode
        if (m_powerMode == PM_STANDBY) {
          
          // if it has elapsed
          if ((millis() - m_standByStartTime) > ee_LedPowerStandbyDuration) {
            
            Serial.println(F("Turning LED power OFF."));
          
            // power the LED off
            digitalWrite(LED_POWER_RELAY_PIN, LOW);
          
            // mark the status
            m_powerMode = PM_OFF;
          }
        }
      }
    }
    
    void StartTransition(const HSB &newTargetColor, unsigned long transitionDuration)
    {
      Serial.print(F("Starting transition for target color { hue: "));
      Serial.print(newTargetColor.hue);
      Serial.print(F(", sat: "));
      Serial.print(newTargetColor.sat);
      Serial.print(F(", bri: "));
      Serial.print(newTargetColor.bri);
      Serial.print(F("} withing "));
      Serial.print(transitionDuration);
      Serial.println(F(" ms."));

      // if new color is not black, set and keep the power ON
      if (newTargetColor.hue != 0 || newTargetColor.sat != 0 || newTargetColor.bri != 0) {
       
        // if the power is OFF
        if (m_powerMode == PM_OFF) {
          
          Serial.println(F("Turning LED power ON."));
        
          // power the LEDs on
          digitalWrite(LED_POWER_RELAY_PIN, HIGH);
        }
 
        // set the power status
        m_powerMode = PM_ON; 
      }
      
      // start a new transition
      m_originalColor = m_currentColor;
      m_targetColor = newTargetColor;         
      m_transitionStartTime = millis();
      m_transitionDuration = transitionDuration;
      m_transitionFinished = false;
    }
    
  };
  
public:
  
  enum LightSensorAlertStatus {
    LSSA_ALERT_INACTIVE    = 0,  // initial status, the alert is deactivated
    LSSA_ALERT_ACTIVE      = 1,  // the alert has been configured
    LSSA_ABOVE_HIGH_ALERT  = 2,  // the intensity is above the configured limit, reported to controller. It will not be repeared untill the decrease bellow lower limit  
    LSSA_BELLOW_LOW_ALERT  = 3,  // the intensity is above the configured limit, reported to controller. It will not be repeared untill the decrease bellow lower limit  
  };
    
private:
  
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

  // LED power handler
  LEDPowerHandler m_ledPowerHandler;

  bool m_ResetIsPending;
  
public:
  
  LedDriverController(): 
    m_liDropAlertStatus(LSSA_ALERT_INACTIVE),
    m_liDropAlertHigh(0),
    m_liDropAlertLow(0),
    m_liDropAlertMs(0),
    m_liDropAlertLastTimeAboveHigh(),
    m_liGradualAlertStatus(LSSA_ALERT_INACTIVE),
    m_liGradualAlertHigh(0),
    m_liGradualAlertLow(0),
    m_ledPowerHandler(),
    m_ResetIsPending(false)
  {
  }

  void UpdateLISensor()
  {
    // if the LEDs are not on (and don't disturb the light sensor)
    if (!m_ledPowerHandler.LightsOn()) {
    
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
    if (m_ResetIsPending) {
    
      // TODO:  
    }
    
    // handle network messages
    commManager.HandleLightDriverIncommingMessages(*this);
    
    // handle light intensity sensor input changes
    UpdateLISensor();
    
    // update the LED handler and power status
    m_ledPowerHandler.Update();
  }
  
  virtual LightCommError HandleReset(uint16_t senderNode)
  {
    Serial.println("Reset command received.");
    
    // register reset request
    m_ResetIsPending = true;
    
    return 0;
  }

  virtual void HandleQueryInputs(uint16_t senderNode, const LightCommMessage_QueryInputs &message, RF24NetworkWrapper &network)
  {
    Serial.println("Query inputs command received.");
    
    // TODO: there should be some response in here
  }

  virtual LightCommError HandleSleep(uint16_t senderNode)
  {/*
    Serial.println("Sleep command received.");
    
    // the reset pending has a higher priority, otherwise let's deactivate the power relay
    if (m_state != S_RESET_PENDING) {

      SetLedPower(false);

      m_state = S_SLEEPING;   
    }
    */
    return 0;
  }

  virtual LightCommError HandleWake(uint16_t senderNode)
  {/*
    Serial.println("Wake command received.");

    // the reset pending has a higher priority, otherwise let's activate the power relay
    if (m_state != S_RESET_PENDING) {

      SetLedPower(true);

      m_state = S_STANDBY;
      // TODO: LEDs?   
    }*/
    
    return 0;
  }

//  virtual LightCommError HandleQueryStatus(uint16_t senderNode) {}

  virtual LightCommError HandleSetHSBColor(uint16_t senderNode, const LightCommMessage_SetHSBColor &message)
  {
    Serial.print("Set HSB command received { hue: ");
    Serial.print(message.GetHue(), DEC);
    Serial.print(", sat: ");
    Serial.print(message.GetSat(), DEC);
    Serial.print(", bri: ");
    Serial.print(message.GetBri(), DEC);
    Serial.print(", ms: ");
    Serial.print(message.GetMs(), DEC);
    Serial.println("}");

    HSB newTargetColor;
    newTargetColor.hue = message.GetHue();
    newTargetColor.sat = message.GetSat();
    newTargetColor.bri = message.GetBri();
    
    // start a new transition in the LED handler
    m_ledPowerHandler.StartTransition(newTargetColor, message.GetMs());
    
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
  
  Serial.print(F("LED driver #"));
  Serial.println(ee_ledDriverId);
  Serial.print(F("RF24 configuration: {channel: "));
  Serial.print(ee_rf24Channel);
  Serial.print(F(", node_address: "));
  Serial.print(ee_rf24Address);
  Serial.print(F(", controller_node_address: "));
  Serial.print(ee_rf24ControllerAddress);
  Serial.println(F("}"));
  
  Serial.print(F("LED power standby duration "));
  Serial.println(ee_LedPowerStandbyDuration);

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

