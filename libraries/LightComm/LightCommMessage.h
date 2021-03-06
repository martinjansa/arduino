#ifndef LIGHT_COMM_MESSAGE__H
#define LIGHT_COMM_MESSAGE__H

#define __ASSERT_USE_STDERR

#include <assert.h>
#include <arduino.h>
#include <RF24NetworkWrapper.h>

typedef byte LightCommError;

enum RF24CommMessageType {
  
  RF24CMT_UNDEFINED                =  0, // invalid message type

  // OK/error responses
  RF24CMT_OK                       =  1, // OK response
  RF24CMT_ERROR                    =  2, // (byte, byte) - error response with two reserved arguments

  // reset
  RF24CMT_RESET                    =  5, // (void) - reset the device

  // low level functions - use with care and preferably only for a diagnostics!
  RF24CMT_QUERY_INPUTS             =  6, // (byte analog, word digital) - asks the device to send the current values of the analog and digital inputs specified in the individual bits of the analog byte (bit 0 = A0, bit 1 = A1 and so on) and diginal word (bit 0 = D0, etc.). The response is a sequence of messages, first one RF24CMT_ANALOG_INPUT_VALUE for for each queried analog input and then one RF24CMT_DIGITAL_INPUT_VALUE for each digital input
  RF24CMT_ANALOG_INPUT_VALUE       =  7, // (byte analogInput, word value)
  RF24CMT_DIGITAL_INPUT_VALUE      =  8, // (byte digitalInput, bool value)

  // status
  RF24CMT_SLEEP                    = 10, // (void) - put the device into the low power sleep mode (switches off the external power supply for LEDs, etc.)
  RF24CMT_WAKE                     = 11, // (void) - awake the device, get ready for immediate operation (activate external power supply, etc.)
  RF24CMT_QUERY_STATUS             = 12, // (void) - ask the device to send it's current status. The device responds with RF24CMT_STATUS message (see bellow).
  RF24CMT_STATUS                   = 13, // (byte status) - send as a response to the RF24CMT_QUERY_STATUS query. Status is a bitmask, where bit 0: 0 = sleeping, 1 = awake; bit 1: 0 = light off, 1 = ligh on; bits 2 to 7: reserved.

  // Set HSB color messages
  RF24CMT_SET_HSB_COLOR            = 15, // (word hue, byte sat, byte bri, word ms) - Sets the requested color and intesity. If ms is not 0, it specifies the duration of the change from the current color to the requested 0. If 0, the change is immediate.

  // light sensor alert configuration and notification messages
  //
  // There are several different types of alerts that can be reported by the light sensor equipped device:
  //   - sudden light intensity drop  - the room got suddenly dark, probably the last light in the other rooms was switched off.
  //                                    The controller device might react by a temporary activation of some orientation lighting using either main or orientation LEDs.
  //                                    This alert is configured by RF24CMT_LI_CONFIG_DROP_ALERT, see bellow.
  //   - gradual light intensity change - the room intensity has gradully changed bellow or above some given limit
  //                                    The controller might de/activate the orientation lighting and correlate its intensity to the light intensity in the room.
  //                                    This alert is configured by RF24CMT_LI_CONFIG_GRADUAL_ALERT.
  //                                    
  // Note: The light sensor is active only if the allowed and the LEDs of the device are off (R,G,B = 0,0,0).
  RF24CMT_LI_CONFIG_DROP_ALERT     = 20, // (byte high, byte low, byte ms) - configures sudden light intensity drop alert. High and low are light intensity sensor threshold values in the range 0..1023 and ms is the maximal time for the intensity change to be considered sudden/fast in the range 0..65535. All values 0 means the alert is disabled.
  RF24CMT_LI_DROP_ALERT_DETECTED   = 21, // () - reports that the sudden light intensity drop was detected by the device with light sensor.
  RF24CMT_LI_CONFIG_GRADUAL_ALERT  = 22, // (byte high, byte low) - configures gradual alert by the high and low threasholds (byte 0..255 multipled by 4) used in a hysteresis. A combination of 0, 0 means alert is disabled. If the light intensity gets bellow the low threshold the 
  RF24CMT_LI_LOW_LIGHT_DETECTED    = 23, // () - reports a gradual decrease of the light intensity bellow the given threshold. Reported only if the threshold is reached for the first time after the alert activation or since the last report of the RF24CMT_LI_HIGH_LIGHT_DETECTED message. 
  RF24CMT_LI_HIGH_LIGHT_DETECTED   = 24, // () - reports a gradual increase of the light intensity above the given threshold. Reported only if the threshold is reached for the first time after the alert activation or since the last report of the RF24CMT_LI_LOW_LIGHT_DETECTED message. 

};


class ILightCommMessage {
public:

  virtual bool read(RF24NetworkHeader &header, IRF24Network &network) = 0;

  virtual bool write(RF24NetworkHeader &header, IRF24Network &network) const = 0;

  static bool readNoParams(RF24NetworkHeader &header, IRF24Network &network, RF24CommMessageType expectedMessageType)
  {
    byte buf = 0;
    bool result = (network.read(header, &buf, 0) == 0);
    IRF24Network::DumpHeader(header, "Message received without params ");
    assert(header.type == expectedMessageType || expectedMessageType == RF24CMT_UNDEFINED);
    return result;
  }

  static bool writeNoParams(RF24NetworkHeader &header, IRF24Network &network, RF24CommMessageType messageType)
  {
    byte buf = 0;
    header.type = messageType;
    IRF24Network::DumpHeader(header, "Sending message without params ");
    return network.write(header, &buf, 0);
  }

};

// RF24CMT_ERROR
class LightCommMessage_Error: public ILightCommMessage {
private:
  LightCommError m_errorCode;
  byte m_reserved;

public:
  LightCommMessage_Error(): m_errorCode(0), m_reserved(0) {}  
  LightCommMessage_Error(LightCommError errorCode, byte reserved = 0): m_errorCode(errorCode), m_reserved(reserved) {}  

  LightCommError GetErrorCode() const {
    return m_errorCode;
  };
  byte GetReserved() const {
    return m_reserved;
  }

  virtual bool read(RF24NetworkHeader &header, IRF24Network &network)
  {
    byte buf[2];
    size_t size = network.read(header, buf, 2);
    if (size != 2) return false;
    assert(header.type = RF24CMT_ERROR);
    m_errorCode = buf[0];
    m_reserved = buf[1];
    return true;
  }

  virtual bool write(RF24NetworkHeader &header, IRF24Network &network) const
  {
    byte buf[2];
    buf[0] = m_errorCode;
    buf[1] = m_reserved;
    header.type = RF24CMT_ERROR;
    return network.write(header, buf, 2);
  }

};


// RF24CMT_QUERY_INPUTS
class LightCommMessage_QueryInputs: public ILightCommMessage {
private:
  byte m_analog;
  word m_digital;

public:
  LightCommMessage_QueryInputs(): m_analog(0), m_digital(0) {}  
  LightCommMessage_QueryInputs(byte analog, word digital): m_analog(analog), m_digital(digital) {}  

  byte GetAnalog() const {
    return m_analog;
  };
  word GetDigital() const {
    return m_digital;
  }

  virtual bool read(RF24NetworkHeader &header, IRF24Network &network)
  {
    byte buf[3];
    size_t size = network.read(header, buf, 3);
    if (size != 3) return false;
    assert(header.type = RF24CMT_QUERY_INPUTS);
    m_analog = buf[0];
    m_digital = *((word *) &buf[1]);
    return true;
  }

  virtual bool write(RF24NetworkHeader &header, IRF24Network &network) const
  {
    byte buf[3];
    buf[0] = m_analog;
    *((word *) &buf[1]) = m_digital;
    header.type = RF24CMT_QUERY_INPUTS;
    return network.write(header, buf, 3);
  }

};


// RF24CMT_SET_HSB_COLOR
class LightCommMessage_SetHSBColor: public ILightCommMessage {
private:
  word m_hue;
  byte m_sat;
  byte m_bri;
  word m_ms;

public:
  LightCommMessage_SetHSBColor(): m_hue(0), m_sat(0), m_bri(0), m_ms(0) {}  
  LightCommMessage_SetHSBColor(word hue, byte sat, byte bri, word ms): m_hue(hue), m_sat(sat), m_bri(bri), m_ms(ms) {}  

  word GetHue() const {
    return m_hue;
  };
  byte GetSat() const {
    return m_sat;
  }
  byte GetBri() const {
    return m_bri;
  }
  word GetMs() const {
    return m_ms;
  }

  virtual bool read(RF24NetworkHeader &header, IRF24Network &network)
  {
    byte buf[6];
    size_t size = network.read(header, buf, 6);
    if (size != 6) return false;
    assert(header.type = RF24CMT_SET_HSB_COLOR);
    m_hue = *((word *)&buf[0]);
    m_sat = buf[2];
    m_bri = buf[3];
	m_ms = *((word *)&buf[4]);
    return true;
  }

  virtual bool write(RF24NetworkHeader &header, IRF24Network &network) const
  {
    byte buf[6];
    *((word *)&buf[0]) = m_hue;
    buf[2] = m_sat;
    buf[3] = m_bri;
	*((word *)&buf[4]) = m_ms;
    header.type = RF24CMT_SET_HSB_COLOR;
    return network.write(header, buf, 6);
  }

};

  
// RF24CMT_LI_CONFIG_DROP_ALERT
class LightCommMessage_LIConfigDropAlert: public ILightCommMessage {
private:
  word m_high, m_low, m_ms;

public:
  LightCommMessage_LIConfigDropAlert(): m_high(0), m_low(0), m_ms(0) {}
  LightCommMessage_LIConfigDropAlert(word high, word low, word ms): m_high(high), m_low(low), m_ms(ms) {}

  word GetHigh() const {
    return m_high;
  }
  word GetLow() const {
    return m_low;
  }
  word GetMs() const {
    return m_ms;
  }

  virtual bool read(RF24NetworkHeader &header, IRF24Network &network)
  {
    word buf[3];
    size_t size = network.read(header, &(buf[0]), 6);
    if (size != 6) return false;
    assert(header.type = RF24CMT_LI_CONFIG_DROP_ALERT);
    m_high = buf[0];
    m_low = buf[1];
    m_ms = buf[2];
    return true;
  }

  virtual bool write(RF24NetworkHeader &header, IRF24Network &network) const 
  {
    word buf[3];
    buf[0] = m_high;
    buf[1] = m_low;
    buf[2] = m_ms;
    header.type = RF24CMT_LI_CONFIG_DROP_ALERT;
    return network.write(header, &(buf[0]), 6);
  }

};


// RF24CMT_LI_CONFIG_GRADUAL_ALERT
class LightCommMessage_LiConfigGradualAlert: public ILightCommMessage {
private:
  word m_high, m_low;

public:
  LightCommMessage_LiConfigGradualAlert(): m_high(0), m_low(0) {}
  LightCommMessage_LiConfigGradualAlert(word high, word low): m_high(high), m_low(low) {}

  word GetHigh() const {
    return m_high;
  }
  word GetLow() const {
    return m_low;
  }

  virtual bool read(RF24NetworkHeader &header, IRF24Network &network)
  {
    word buf[2];
    size_t size = network.read(header, buf, 4);
    if (size != 4) return false;
    assert(header.type = RF24CMT_LI_CONFIG_GRADUAL_ALERT);
    m_high = buf[0];
    m_low = buf[1];
    return true;
  }

  virtual bool write(RF24NetworkHeader &header, IRF24Network &network) const 
  {
    word buf[2];
    buf[0] = m_high;
    buf[1] = m_low;
    header.type = RF24CMT_LI_CONFIG_GRADUAL_ALERT;
    return network.write(header, buf, 4);
  }

};


#endif // LIGHT_COMM_MESSAGE__H
