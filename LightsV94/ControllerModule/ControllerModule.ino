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
static const unsigned long INITIALIZE_LED_DRIVERS_DELAY  = 1000;
static const unsigned long ON_SWITCH_DEBOUNCING_FILTER_DURATION = 100;
//static const unsigned long RGB_TRANSITION_DURATION        = 500;


// analog pins
//static const int LIGHT_SENSOR_PIN       = A0; 

// digital pins
static const int SERIAL_RX_PIN          =  0;  // (used by the Serial port)
static const int SERIAL_TX_PIN          =  1;  // (used by the Serial port)
static const int ON_SWITCH_PIN          =  2;  // (light controlling switch pin)
//static const int RED_PWM_PIN            =  3;
//               available                 4
//static const int GREEN_PWM_PIN          =  5;
//static const int BLUE_PWM_PIN           =  6;
//               available                 7
//               available                 8
static const int RF24_CHIP_ENABLE_PIN   =  9;
static const int RF24_CHIP_SELECT_PIN   = 10;
static const int RF24_MOSI_PIN          = 11;  // (cannot be changed probably, see RF24 if you consider this options)
static const int RF24_MISO_PIN          = 12;  // (cannot be changed probably, see RF24 if you consider this options)
static const int RF24_SCK_PIN           = 13;  // (cannot be changed probably, see RF24 if you consider this options)

// persistent configuration
PersistentConfig ee_config(true);

// network configuration [0x00-x0f]
PersistentConfigItem<uint8_t> ee_rf24Channel            (ee_config, 0x00,  90);  // RF24 channel
PersistentConfigItem<uint16_t> ee_rf24Address           (ee_config, 0x01,   0);  // RF24 node address
PersistentConfigItem<uint16_t> ee_rf24LedDriver1Address (ee_config, 0x03,   1);  // LED driver #1 RF24 node address
PersistentConfigItem<uint16_t> ee_rf24LedDriver2Address (ee_config, 0x05,   2);  // LED driver #2 RF24 node address
PersistentConfigItem<uint16_t> ee_rf24LedDriver3Address (ee_config, 0x07,   3);  // LED driver #3 RF24 node address
PersistentConfigItem<uint16_t> ee_rf24LedDriver4Address (ee_config, 0x09,   4);  // LED driver #4 RF24 node address

// colors configuration

// full strength mode [0x10-0x1f]  - Warm white: RGB = [255, 255, 200], HSB = [60, 22, 100]
PersistentConfigItem<word> ee_FullStrenghtColor_Hue     (ee_config, 0x10,   60);
PersistentConfigItem<byte> ee_FullStrenghtColor_Sat     (ee_config, 0x12,   56);
PersistentConfigItem<byte> ee_FullStrenghtColor_Bri     (ee_config, 0x13,  255);
PersistentConfigItem<word> ee_FullStrenghtOnSpeed       (ee_config, 0x14,  2000);   // power on time in ms
PersistentConfigItem<word> ee_FullStrenghtOffSpeed      (ee_config, 0x16,  2000);   // power off time in ms

// night mode [0x20-0x2f]  - Warm white 50%: RGB = [128, 128, 100], HSB = [60, 22, 50]
PersistentConfigItem<word> ee_NightModeColor_Hue        (ee_config, 0x20,   60);
PersistentConfigItem<byte> ee_NightModeColor_Sat        (ee_config, 0x22,   56);
PersistentConfigItem<byte> ee_NightModeColor_Bri        (ee_config, 0x23,  128);
PersistentConfigItem<word> ee_NightModeOnSpeed          (ee_config, 0x24, 2000);   // power on time in ms
PersistentConfigItem<word> ee_NightModeOffSpeed         (ee_config, 0x26, 2000);   // power off time in ms
PersistentConfigItem<word> ee_NightModeTime_End         (ee_config, 0x28,  420);   // minute of the end of the night mode (7:00)
PersistentConfigItem<word> ee_NightModeTime_Start       (ee_config, 0x2a, 1230);   // minute of the start of the night mode (20:30)

// orientation mode [0x30-0x3f] - Warm white 20%: RGB = [51, 51, 40], HSB = [60, 22, 20]
PersistentConfigItem<word> ee_OrientationModeColor_Hue  (ee_config, 0x30,   60);
PersistentConfigItem<byte> ee_OrientationModeColor_Sat  (ee_config, 0x32,   56);
PersistentConfigItem<byte> ee_OrientationModeColor_Bri  (ee_config, 0x33,   51);
PersistentConfigItem<word> ee_OrientationModeOnSpeed    (ee_config, 0x34,  200);   // power on time in ms
PersistentConfigItem<word> ee_OrientationModeOffSpeed   (ee_config, 0x36, 5000);   // power off time in ms
PersistentConfigItem<word> ee_OrientationModeDuration   (ee_config, 0x38, 5000);   // mode duration in ms

// alerts configuration [0x40-0x4f]
PersistentConfigItem<word> ee_DropAlert_High            (ee_config, 0x40,  100);   // drop alert configuration - high (0 mean inactive)
PersistentConfigItem<word> ee_DropAlert_Low             (ee_config, 0x42,   20);   // drop alert configuration - low
PersistentConfigItem<word> ee_DropAlert_Ms              (ee_config, 0x44,  100);   // drop alert configuration - ms
PersistentConfigItem<word> ee_GradualAlert_High         (ee_config, 0x46,  100);   // gradual alert configuration - high (0 mean inactive)
PersistentConfigItem<word> ee_GradualAlert_Low          (ee_config, 0x48,   50);   // gradual alert configuration - low

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
 *   OFF:                   the LEDs are not operating and the LED power source is switched off
 *   ON:                    the LEDs are operating according to the required RGB settings for full strenght
 *   ON_NIGHT_MODE          the LEDs are operating according to the required RGB settings for night mode
 *   S_LI_DROP_ORIENTATION  the orientation leds are on
 *
 * Transitions (TODO):
 *   SLEEPING -> WAKING_UP                     - WAKE command received, the power source is switched on
 *   WAKING_UP -> STANDBY                      - automatically after 50ms
 *   (*) -> SLEEPING                           - SLEEP command received, the power source is switched off and the RGB to {0, 0, 0}
 *   STANDBY|ON|ON_TRANSITION -> ON_TRANSITION - SetRGB command received
 *   ON_TRANSITION -> STANDBY                  - automatically after transition duration expires and if new RGB is {0, 0, 0}
 *   ON_TRANSITION -> ON                       - automatically after transition duration expires and if new RGB is non zero
 *   (*) -> RESET_PENDING                      - the machine will be reset after the network update (response sent to the controller)
 */
class LightController: public ILightCommControllerMessageHandler {
public:
  enum Status {
    S_UNINITIALIZED              = 1,
    S_OFF                        = 2,
    S_OFF_ORIENTATION_REQUESTED  = 3,
    S_OFF_ORIENTATION            = 4,
    S_ON                         = 5,
    S_ON_NIGHT_MODE              = 6,
    S_ON_ORIENTATION             = 7
  };
      
private:
  Status m_status;
  
  // light intensity drop alert status
  unsigned long m_startupTime;
  unsigned long m_liDropOrientationModeStart;

  // on/off switch debouncing filter status (the switch is simulated to be kept in the first changed status during 100ms)
  bool m_onSwitchStatus;
  unsigned long m_onSwitchChangeStart;
  
public:
  
  LightController(): 
    m_status(S_UNINITIALIZED),
    m_startupTime(millis()),
    m_liDropOrientationModeStart(0),
    m_onSwitchStatus(false),
    m_onSwitchChangeStart(0)
  {
  }

  bool LightsActive()
  {
    switch (m_status) {
      case S_UNINITIALIZED:    return false;
      case S_OFF:              return false;
      case S_OFF_ORIENTATION:  return false;
      case S_ON:               return true;
      case S_ON_NIGHT_MODE:    return true;
      case S_ON_ORIENTATION:   return true;
      default: assert(false);
    }
  };
  // returns true during the night mode time
  bool InNightModeTime()
  {
    return false;  // TODO: fix, so far hard coded day mode    
  }
  
  void UpdateOnOffSwitch()
  {
    // switch connects to the ground, so if the pin reads LOW, it is active
    bool currentSwitchStatus = (digitalRead(ON_SWITCH_PIN) == LOW);
       
    // if the de-bouncing filter is active
    if ((millis() - m_onSwitchChangeStart) <= ON_SWITCH_DEBOUNCING_FILTER_DURATION) {
    
      // ignore the current status and use the stored one
      currentSwitchStatus = m_onSwitchStatus;
      
    } else {
      
      // if the switch status has changed right now
      if (m_onSwitchStatus != currentSwitchStatus) {
        
        // keep the current time for de-bouncing filter
        m_onSwitchChangeStart = millis();
        
        // keep the value
        m_onSwitchStatus = currentSwitchStatus;
        
        // the message sending result
        bool result = false;
        word hue = 0;
        byte sat = 0;
        byte bri = 0;
        word ms = 0;
        
        // if the switch has just been switched ON
        if (m_onSwitchStatus) {

          // if we are not in the night mode time
          if (!InNightModeTime()) { 
          
            Serial.println(F("UpdateOnOffSwitch: switch has just turned ON, activation LEDs in full strenght."));
     
            hue = ee_FullStrenghtColor_Hue;
            sat = ee_FullStrenghtColor_Sat;
            bri = ee_FullStrenghtColor_Bri;
            ms = ee_FullStrenghtOnSpeed;
    
            m_status = S_ON;
            
          } else {
            
            Serial.println(F("UpdateOnOffSwitch: switch has just turned ON, activation LEDs in night mode."));
     
            hue = ee_NightModeColor_Hue;
            sat = ee_NightModeColor_Sat;
            bri = ee_NightModeColor_Bri;
            ms = ee_NightModeOnSpeed;

            m_status = S_ON_NIGHT_MODE;
          }
          
        } else {
          
          Serial.println(F("UpdateOnOffSwitch: switch has just turned OFF, deactivating LEDs."));

          hue = 0;
          sat = 0;
          bri = 0;
          
          m_status = S_OFF;

          // if we are not in the night mode time
          if (!InNightModeTime()) { 
          
            ms = ee_FullStrenghtOffSpeed;
            
          } else {
            
            ms = ee_NightModeOffSpeed;
          }
        }

        // TODO: send the HSB request to all the LED drivers
        result = commManager.SetLightDriverHSBColor(ee_rf24LedDriver2Address, hue, sat, bri, ms);
        if (result) {
          Serial.println(F("UpdateOnOffSwitch: message sending succeeded."));
        } else {
          Serial.println(F("UpdateOnOffSwitch: message sending FAILED."));
        }
      }
    }
  }
  
  void Update()
  {
    // Serial.println("Update: handle incomming messages.");

    // handle network messages
    commManager.HandleControllerIncommingMessages(*this);

    // if uninitialized and it is already time to initialize led drivers
    if (m_status == S_UNINITIALIZED && (millis() - m_startupTime) >= INITIALIZE_LED_DRIVERS_DELAY) {

      Serial.println("Update: initializing LED drivers...");

      // initialize the LED driver 2
      bool result = commManager.ConfigLightDriverDropAlert(ee_rf24LedDriver2Address, ee_DropAlert_High, ee_DropAlert_Low, ee_DropAlert_Ms);
      
      if (result) {
        
        Serial.println(F("Update: initializing LED drivers: drop alert succeeded."));

      } else {
        
        Serial.println(F("Update: initializing LED drivers: drop alert FAILED."));
      }

      // initialize the LED driver 2
      result = commManager.ConfigLightDriverGradualAlert(ee_rf24LedDriver2Address, ee_GradualAlert_High, ee_GradualAlert_Low);
      
      if (result) {
        
        Serial.println(F("Update: initializing LED drivers: gradual alert succeeded."));

      } else {
        
        Serial.println(F("Update: initializing LED drivers: gradual alert FAILED."));
      }

      m_status = S_OFF;
    }

    // handle the orientation mode start (from the li drop command)
    if (m_status == S_OFF_ORIENTATION_REQUESTED) {
      
      // reset the flag
      m_status = S_ON_ORIENTATION;
      m_liDropOrientationModeStart = millis();
    
      // TODO: send the HSB request to all the LED drivers
      bool result = commManager.SetLightDriverHSBColor(ee_rf24LedDriver2Address, ee_OrientationModeColor_Hue, ee_OrientationModeColor_Sat, ee_OrientationModeColor_Bri, ee_OrientationModeOnSpeed);
      if (result) {
        Serial.println(F("UpdateOrientationMode: message sending succeeded."));
      } else {
        Serial.println(F("UpdateOrientationMode: message sending FAILED."));
      }
      
    } else {
      
      // if we are in the S_ON_ORIENTATION mode
      if (m_status == S_ON_ORIENTATION) {
        
        // check it's expiration
        if ((millis() - m_liDropOrientationModeStart) > ee_OrientationModeDuration) {
         
          // TODO: send the HSB request to all the LED drivers
          bool result = commManager.SetLightDriverHSBColor(ee_rf24LedDriver2Address, 0, 0, 0, ee_OrientationModeOffSpeed);
          if (result) {
            Serial.println(F("UpdateOrientationMode: message sending succeeded."));
          } else {
            Serial.println(F("UpdateOrientationMode: message sending FAILED."));
          }
          m_status = S_OFF;          
        }
      }
    }
    
    // handle light intensity sensor input changes
    UpdateOnOffSwitch();
  }
  
  virtual LightCommError HandleLiDropAlertDetected(uint16_t senderNode)
  {
    Serial.println(F("Light intensity drop alert received."));
    
    // requst the orientation mode
    m_status = S_OFF_ORIENTATION_REQUESTED;
    
    return 0;
  }

  virtual LightCommError HandleLiLowLightDetected(uint16_t senderNode)
  {
    Serial.println(F("Low light intensity alert received."));

    // TODO:   

    return 0;
  }

  virtual LightCommError HandleLiHighLightDetected(uint16_t senderNode) 
  {
    Serial.println(F("High light intensity alert received."));
    
    // TODO:
    
    return 0;
  }

};

LightController lightController;

void setup()
{
  // initialize the serial communication to the PC
  Serial.begin(57600);    

  // check the potential overlaps in the persisten configuration
  assert(ee_config.CheckOverlaps());
    
  Serial.println("Light Controller");

  Serial.print("RF24 configuration: {channel: ");
  Serial.print(ee_rf24Channel);
  Serial.print(", node_address: ");
  Serial.print(ee_rf24Address);
  Serial.print(", led_driver_1_node_address: ");
  Serial.print(ee_rf24LedDriver1Address);
  Serial.print(", led_driver_2_node_address: ");
  Serial.print(ee_rf24LedDriver2Address);
  Serial.print(", led_driver_3_node_address: ");
  Serial.print(ee_rf24LedDriver3Address);
  Serial.print(", led_driver_4_node_address: ");
  Serial.print(ee_rf24LedDriver4Address);
  Serial.println("}");

  Serial.println("Light configuration: ");
  
  Serial.print("  Full streght power (hue: ");
  Serial.print(ee_FullStrenghtColor_Hue);
  Serial.print(", sat: ");
  Serial.print(ee_FullStrenghtColor_Sat);
  Serial.print(", bri: ");
  Serial.print(ee_FullStrenghtColor_Bri);
  Serial.print(", on: ");
  Serial.print(ee_FullStrenghtOnSpeed);
  Serial.print(", off: ");
  Serial.print(ee_FullStrenghtOffSpeed);
  Serial.println("}");
  
  Serial.print("  Night mode power (hue: ");
  Serial.print(ee_NightModeColor_Hue);
  Serial.print(", sat: ");
  Serial.print(ee_NightModeColor_Sat);
  Serial.print(", bri: ");
  Serial.print(ee_NightModeColor_Bri);
  Serial.print(", on: ");
  Serial.print(ee_NightModeOnSpeed);
  Serial.print(", off: ");
  Serial.print(ee_NightModeOffSpeed);
  Serial.print("}, night mode time {end: ");
  Serial.print(ee_NightModeTime_End);
  Serial.print(", start: ");
  Serial.print(ee_NightModeTime_Start);
  Serial.println("}");
  
  Serial.print("  Light intensity drop alert (high: ");
  Serial.print(ee_DropAlert_High);
  Serial.print(", low: ");
  Serial.print(ee_DropAlert_Low);
  Serial.print(", milliseconds: ");
  Serial.print(ee_DropAlert_Ms);
  Serial.println("}");
  
  Serial.print("  Light intensity gradual alert (high: ");
  Serial.print(ee_GradualAlert_High);
  Serial.print(", low: ");
  Serial.print(ee_GradualAlert_Low);
  Serial.println("}");

  // configure the ON/OFF switch pin as input
  pinMode(ON_SWITCH_PIN, INPUT);

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
  lightController.Update();
}
