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
PersistentConfig ee_config;
PersistentConfigItem<uint8_t> ee_rf24Channel(ee_config, 0);             // 1 byte, RF24 channel
PersistentConfigItem<uint16_t> ee_rf24Address(ee_config, 1);            // 2 bytes, RF24 node address
PersistentConfigItem<uint16_t> ee_rf24LedDriver1Address(ee_config, 3);  // 2 bytes, RF24 LED driver #1 node address
PersistentConfigItem<uint16_t> ee_rf24LedDriver2Address(ee_config, 5);  // 2 bytes, RF24 LED driver #2 node address
PersistentConfigItem<uint16_t> ee_rf24LedDriver3Address(ee_config, 7);  // 2 bytes, RF24 LED driver #3 node address
PersistentConfigItem<uint16_t> ee_rf24LedDriver4Address(ee_config, 9);  // 2 bytes, RF24 LED driver #4 node address
PersistentConfigItem<byte> ee_FullStrenghtColor_Red(ee_config, 11);     // 1 byte, 100% power red color
PersistentConfigItem<byte> ee_FullStrenghtColor_Green(ee_config, 12);   // 1 byte, 100% power green color
PersistentConfigItem<byte> ee_FullStrenghtColor_Blue(ee_config, 13);    // 1 byte, 100% power blue color
PersistentConfigItem<word> ee_NightModeTime_End(ee_config, 14);         // 2 bytes, minute of the end of the night mode
PersistentConfigItem<word> ee_NightModeTime_Start(ee_config, 16);       // 2 bytes, minute of the start of the night mode
PersistentConfigItem<byte> ee_NightModeColor_Red(ee_config, 18);        // 1 byte, night mode power red color
PersistentConfigItem<byte> ee_NightModeColor_Green(ee_config, 19);      // 1 byte, night mode power green color
PersistentConfigItem<byte> ee_NightModeColor_Blue(ee_config, 20);       // 1 byte, night mode power blue color
PersistentConfigItem<word> ee_DropAlert_High(ee_config, 21);            // 2 bytes, drop alert configuration - high (0 mean inactive)
PersistentConfigItem<word> ee_DropAlert_Low(ee_config, 23);             // 2 bytes, drop alert configuration - low
PersistentConfigItem<word> ee_DropAlert_Ms(ee_config, 25);              // 2 bytes, drop alert configuration - ms
PersistentConfigItem<word> ee_GradualAlert_High(ee_config, 27);         // 2 bytes, gradual alert configuration - high (0 mean inactive)
PersistentConfigItem<word> ee_GradualAlert_Low(ee_config, 29);          // 2 bytes, gradual alert configuration - low

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
    S_UNINITIALIZED        = 1,
    S_OFF                  = 2,
    S_ON                   = 3,
    S_ON_NIGHT_MODE        = 4,
    S_LI_DROP_ORIENTATION  = 5
  };
      
private:
  Status m_status;
  
  // light intensity drop alert status
  unsigned long m_startupTime;
  unsigned long m_liDropOrientationModeStart;
      
public:
  
  LightController(): 
    m_status(S_UNINITIALIZED),
    m_startupTime(millis()),
    m_liDropOrientationModeStart(0)
  {
  }

  // returns true during the night mode time
  bool InNightModeTime()
  {
    return false;  // TODO: fix, so far hard coded day mode    
  }
  
  void SetLedPower(bool on)
  {

  }
  
  void UpdateOnOffSwitch()
  {
    // get the status of the ON/OFF switch
    int onStatus = digitalRead(ON_SWITCH_PIN);

    // if the light switch is ON
    if (onStatus == HIGH) {
     
      // if the light is not active
      if (m_status == S_OFF || m_status == S_LI_DROP_ORIENTATION) {
       
        // if weare in the night mode time
        if (InNightModeTime()) {
          
          // activate the night mode
          // TODO: 
        } else {
          
          // activate the light in full strenght
          // TODO: 
        }
      }
      
    } else {
      
      // switch is off
      
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

    // Serial.println("Update: ON/OFF switch status.");

    // handle light intensity sensor input changes
    UpdateOnOffSwitch();
  }
  
  virtual LightCommError HandleLiDropAlertDetected(uint16_t senderNode)
  {
    Serial.println(F("Light intensity drop alert received."));
    
    // TODO:
    
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
  
  // uncomment to write initial values into the EEPROM
  /*
  ee_rf24Channel = 90;
  ee_rf24Address = 0;
  ee_rf24LedDriver1Address = 1;
  ee_rf24LedDriver2Address = 2;
  ee_rf24LedDriver3Address = 3;
  ee_rf24LedDriver4Address = 4;
  ee_FullStrenghtColor_Red = 255;
  ee_FullStrenghtColor_Green = 128;
  ee_FullStrenghtColor_Blue = 128;
  ee_NightModeTime_End = 420;     //  7:00
  ee_NightModeTime_Start = 1230;  // 20:30
  ee_NightModeColor_Red = 128;
  ee_NightModeColor_Green = 64;
  ee_NightModeColor_Blue = 64;
  ee_DropAlert_High = 100;
  ee_DropAlert_Low = 20;
  ee_DropAlert_Ms = 100;
  ee_GradualAlert_High = 100;
  ee_GradualAlert_Low = 50;
  /**/
  
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
  Serial.print("  Full streght power (red: ");
  Serial.print(ee_FullStrenghtColor_Red);
  Serial.print(", green: ");
  Serial.print(ee_FullStrenghtColor_Green);
  Serial.print(", blue: ");
  Serial.print(ee_FullStrenghtColor_Blue);
  Serial.println("}");
  Serial.print("  Night mode power (red: ");
  Serial.print(ee_NightModeColor_Red);
  Serial.print(", green: ");
  Serial.print(ee_NightModeColor_Green);
  Serial.print(", blue: ");
  Serial.print(ee_NightModeColor_Blue);
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
