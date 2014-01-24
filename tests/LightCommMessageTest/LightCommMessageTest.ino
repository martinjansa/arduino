
#include <arduino.h>

#include <SPI.h>

#include <assert.h>

//#include <serialassert.h>
#include <testdriver.h>

#include <nRF24L01.h>
#include <RF24.h>
#include <RF24_config.h>

#include <RF24Network.h>
#include <RF24Network_config.h>
//#include <Sync.h>

#include <RF24NetworkWrapper.h>
#include <RF24NetworkWrapper_Mock-Loopback.h>

//#include <LightCommManager.h>
#include <LightCommMessage.h>


TestDriver testDriver(13);
  

void SendAndReceive(TestDriver &testDriver, LightCommMessage_Error &message)
{
  
  RF24NetworkLoopback loopback;
  
  RF24NetworkHeader header;
  
  TEST_ASSERT(message.write(header, loopback));
  loopback.MakeSentDataAvailableForReading();

  LightCommMessage_Error receivedMessage;
  
  TEST_ASSERT(receivedMessage.read(header, loopback));

  TEST_ASSERT(receivedMessage.GetErrorCode() == message.GetErrorCode());
  TEST_ASSERT(receivedMessage.GetReserved() == message.GetReserved());
}

void SendAndReceive(TestDriver &testDriver, LightCommMessage_QueryInputs &message)
{
  
  RF24NetworkLoopback loopback;
  
  RF24NetworkHeader header;
  
  TEST_ASSERT(message.write(header, loopback));
  loopback.MakeSentDataAvailableForReading();

  LightCommMessage_QueryInputs receivedMessage;
  
  TEST_ASSERT(receivedMessage.read(header, loopback));

  TEST_ASSERT(receivedMessage.GetAnalog() == message.GetAnalog());
  TEST_ASSERT(receivedMessage.GetDigital() == message.GetDigital());
}

void SendAndReceive(TestDriver &testDriver, LightCommMessage_SetRGB &message)
{
  
  RF24NetworkLoopback loopback;
  
  RF24NetworkHeader header;
  
  TEST_ASSERT(message.write(header, loopback));
  loopback.MakeSentDataAvailableForReading();

  LightCommMessage_SetRGB receivedMessage;
  
  TEST_ASSERT(receivedMessage.read(header, loopback));

  TEST_ASSERT(receivedMessage.GetRed() == message.GetRed());
  TEST_ASSERT(receivedMessage.GetGreen() == message.GetGreen());
  TEST_ASSERT(receivedMessage.GetBlue() == message.GetBlue());
}


void SendAndReceive(TestDriver &testDriver, LightCommMessage_LIConfigDropAlert &message)
{
  RF24NetworkLoopback loopback;
  
  RF24NetworkHeader header;
  
  TEST_ASSERT(message.write(header, loopback));

  loopback.MakeSentDataAvailableForReading();

  LightCommMessage_LIConfigDropAlert receivedMessage;
  
  TEST_ASSERT(receivedMessage.read(header, loopback));

  TEST_ASSERT(receivedMessage.GetHigh() == message.GetHigh());
  TEST_ASSERT(receivedMessage.GetLow() == message.GetLow());
  TEST_ASSERT(receivedMessage.GetMs() == message.GetMs());
}

void SendAndReceive(TestDriver &testDriver, LightCommMessage_LiDropAlertDetected &message)
{
  
  RF24NetworkLoopback loopback;
  
  RF24NetworkHeader header;
  
  TEST_ASSERT(message.write(header, loopback));
  loopback.MakeSentDataAvailableForReading();

  LightCommMessage_LiDropAlertDetected receivedMessage;
  
  TEST_ASSERT(receivedMessage.read(header, loopback));

  TEST_ASSERT(receivedMessage.GetBefore() == message.GetBefore());
  TEST_ASSERT(receivedMessage.GetAfter() == message.GetAfter());
}

void SendAndReceive(TestDriver &testDriver, LightCommMessage_LiConfigGradualAlert &message)
{
  
  RF24NetworkLoopback loopback;
  
  RF24NetworkHeader header;
  
  TEST_ASSERT(message.write(header, loopback));
  loopback.MakeSentDataAvailableForReading();

  LightCommMessage_LiConfigGradualAlert receivedMessage;
  
  TEST_ASSERT(receivedMessage.read(header, loopback));

  TEST_ASSERT(receivedMessage.GetHigh() == message.GetHigh());
  TEST_ASSERT(receivedMessage.GetLow() == message.GetLow());
}

void SendAndReceive(TestDriver &testDriver, LightCommMessage_LiLowLightDetected &message)
{
  
  RF24NetworkLoopback loopback;
  
  RF24NetworkHeader header;
  
  TEST_ASSERT(message.write(header, loopback));
  loopback.MakeSentDataAvailableForReading();

  LightCommMessage_LiLowLightDetected receivedMessage;
  
  TEST_ASSERT(receivedMessage.read(header, loopback));

  TEST_ASSERT(receivedMessage.GetIntensity() == message.GetIntensity());
}

void SendAndReceive(TestDriver &testDriver, LightCommMessage_LiHighLightDetected &message)
{
  
  RF24NetworkLoopback loopback;
  
  RF24NetworkHeader header;
  
  TEST_ASSERT(message.write(header, loopback));
  loopback.MakeSentDataAvailableForReading();

  LightCommMessage_LiHighLightDetected receivedMessage;
  
  TEST_ASSERT(receivedMessage.read(header, loopback));

  TEST_ASSERT(receivedMessage.GetIntensity() == message.GetIntensity());
}


DECLARE_TEST_CASE(Error_default)
{
  LightCommMessage_Error defaultError;
  TEST_ASSERT(defaultError.GetErrorCode() == 0);
  TEST_ASSERT(defaultError.GetReserved() == 0);
  SendAndReceive(testDriver, defaultError);
}

DECLARE_TEST_CASE(Error_zero)
{
  LightCommMessage_Error zeroError(0, 0);
  TEST_ASSERT(zeroError.GetErrorCode() == 0);
  TEST_ASSERT(zeroError.GetReserved() == 0);
  SendAndReceive(testDriver, zeroError);
}



DECLARE_TEST_CASE(QueryInputs_default)
{
  LightCommMessage_QueryInputs defaultQueryInputs;
  TEST_ASSERT(defaultQueryInputs.GetAnalog() == 0);
  TEST_ASSERT(defaultQueryInputs.GetDigital() == 0);
  SendAndReceive(testDriver, defaultQueryInputs);
}

DECLARE_TEST_CASE(QueryInputs_zero)
{
  LightCommMessage_QueryInputs zeroQueryInputs(0, 0);
  TEST_ASSERT(zeroQueryInputs.GetAnalog() == 0);
  TEST_ASSERT(zeroQueryInputs.GetDigital() == 0);
  SendAndReceive(testDriver, zeroQueryInputs);
}


DECLARE_TEST_CASE(QueryInputs_max)
{
  LightCommMessage_QueryInputs maxQueryInputs(0xff, 0xffff);
  TEST_ASSERT(maxQueryInputs.GetAnalog() == 0xff);
  TEST_ASSERT(maxQueryInputs.GetDigital() == 0xffff);
  SendAndReceive(testDriver, maxQueryInputs);
}


DECLARE_TEST_CASE(QueryInputs_middle)
{
  LightCommMessage_QueryInputs middleQueryInputs(0x03, 0x0507);
  TEST_ASSERT(middleQueryInputs.GetAnalog() == 0x03);
  TEST_ASSERT(middleQueryInputs.GetDigital() == 0x0507);
  SendAndReceive(testDriver, middleQueryInputs);
}

DECLARE_TEST_CASE(SetRGB_default)
  {
    LightCommMessage_SetRGB defaultSetRGB;
    TEST_ASSERT(defaultSetRGB.GetRed() == 0);
    TEST_ASSERT(defaultSetRGB.GetGreen() == 0);
    TEST_ASSERT(defaultSetRGB.GetBlue() == 0);
    SendAndReceive(testDriver, defaultSetRGB);
  }

DECLARE_TEST_CASE(SetRGB_zero)
  {
    LightCommMessage_SetRGB zeroSetRGB(0, 0, 0);
    TEST_ASSERT(zeroSetRGB.GetRed() == 0);
    TEST_ASSERT(zeroSetRGB.GetGreen() == 0);
    TEST_ASSERT(zeroSetRGB.GetBlue() == 0);
    SendAndReceive(testDriver, zeroSetRGB);
  }

DECLARE_TEST_CASE(SetRGB_max)
  {
    LightCommMessage_SetRGB maxSetRGB(0xff, 0xff, 0xff);
    TEST_ASSERT(maxSetRGB.GetRed() == 0xff);
    TEST_ASSERT(maxSetRGB.GetGreen() == 0xff);
    TEST_ASSERT(maxSetRGB.GetBlue() == 0xff);
    SendAndReceive(testDriver, maxSetRGB);
  }

DECLARE_TEST_CASE(SetRGB_middle)
  {
    LightCommMessage_SetRGB middleSetRGB(0x78, 0x89, 0x95);
    TEST_ASSERT(middleSetRGB.GetRed() == 0x78);
    TEST_ASSERT(middleSetRGB.GetGreen() == 0x89);
    TEST_ASSERT(middleSetRGB.GetBlue() == 0x95);
    SendAndReceive(testDriver, middleSetRGB);
  }

DECLARE_TEST_CASE(LIConfigDropAlert_default)
  {
    LightCommMessage_LIConfigDropAlert defaultCfg;
    TEST_ASSERT(defaultCfg.GetHigh() == 0);
    TEST_ASSERT(defaultCfg.GetLow() == 0);
    TEST_ASSERT(defaultCfg.GetMs() == 0);
    SendAndReceive(testDriver, defaultCfg);
  }

DECLARE_TEST_CASE(LIConfigDropAlert_zero)
  {
    LightCommMessage_LIConfigDropAlert zeroCfg(0, 0, 0);
    TEST_ASSERT(zeroCfg.GetHigh() == 0);
    TEST_ASSERT(zeroCfg.GetLow() == 0);
    TEST_ASSERT(zeroCfg.GetMs() == 0);
    SendAndReceive(testDriver, zeroCfg);
  }


DECLARE_TEST_CASE(LIConfigDropAlert_max)
  {
    LightCommMessage_LIConfigDropAlert maxCfg(0x03ff, 0x03ff, 0x03ff);
    TEST_ASSERT(maxCfg.GetHigh() == 0x03ff);
    TEST_ASSERT(maxCfg.GetLow() == 0x03ff);
    TEST_ASSERT(maxCfg.GetMs() == 0x03ff);
    SendAndReceive(testDriver, maxCfg);
  }

DECLARE_TEST_CASE(LIConfigDropAlert_middle)
  {
    LightCommMessage_LIConfigDropAlert middleCfg(0x178, 0x289, 0x395);
    TEST_ASSERT(middleCfg.GetHigh() == 0x0178);
    TEST_ASSERT(middleCfg.GetLow() == 0x0289);
    TEST_ASSERT(middleCfg.GetMs() == 0x0395);
    SendAndReceive(testDriver, middleCfg);
  }
 
DECLARE_TEST_CASE(LiDropAlertDetected_default)
  {
    LightCommMessage_LiDropAlertDetected defaultAlert;
    TEST_ASSERT(defaultAlert.GetBefore() == 0);
    TEST_ASSERT(defaultAlert.GetAfter() == 0);
    SendAndReceive(testDriver, defaultAlert);
  }

DECLARE_TEST_CASE(LiDropAlertDetected_zero)
  {
    LightCommMessage_LiDropAlertDetected zeroAlert(0, 0);
    TEST_ASSERT(zeroAlert.GetBefore() == 0);
    TEST_ASSERT(zeroAlert.GetAfter() == 0);
    SendAndReceive(testDriver, zeroAlert);
  }

DECLARE_TEST_CASE(LiDropAlertDetected_max)
  {
    LightCommMessage_LiDropAlertDetected maxAlert(0x03ff, 0x03ff);
    TEST_ASSERT(maxAlert.GetBefore() == 0x03ff);
    TEST_ASSERT(maxAlert.GetAfter() == 0x03ff);
    SendAndReceive(testDriver, maxAlert);
  }

DECLARE_TEST_CASE(LiDropAlertDetected_middle)
  {
    LightCommMessage_LiDropAlertDetected middleAlert(0x145, 0x275);
    TEST_ASSERT(middleAlert.GetBefore() == 0x0145);
    TEST_ASSERT(middleAlert.GetAfter() == 0x0275);
    SendAndReceive(testDriver, middleAlert);
  }
  

DECLARE_TEST_CASE(LiConfigGradualAlert_default)
  {
    LightCommMessage_LiConfigGradualAlert defaultCfg;
    TEST_ASSERT(defaultCfg.GetHigh() == 0);
    TEST_ASSERT(defaultCfg.GetLow() == 0);
    SendAndReceive(testDriver, defaultCfg);
  }

DECLARE_TEST_CASE(LiConfigGradualAlert_zero)
  {
    LightCommMessage_LiConfigGradualAlert zeroCfg(0, 0);
    TEST_ASSERT(zeroCfg.GetHigh() == 0);
    TEST_ASSERT(zeroCfg.GetLow() == 0);
    SendAndReceive(testDriver, zeroCfg);
  }

DECLARE_TEST_CASE(LiConfigGradualAlert_max)
  {
    LightCommMessage_LiConfigGradualAlert maxCfg(0x03ff, 0x03ff);
    TEST_ASSERT(maxCfg.GetHigh() == 0x03ff);
    TEST_ASSERT(maxCfg.GetLow() == 0x03ff);
    SendAndReceive(testDriver, maxCfg);
  }

DECLARE_TEST_CASE(LiConfigGradualAlert_middle)
  {
    LightCommMessage_LiConfigGradualAlert middleCfg(0x123, 0x265);
    TEST_ASSERT(middleCfg.GetHigh() == 0x0123);
    TEST_ASSERT(middleCfg.GetLow() == 0x0265);
    SendAndReceive(testDriver, middleCfg);
  }

DECLARE_TEST_CASE(LiLowLightDetected_default)
  {
    LightCommMessage_LiLowLightDetected defaultAlert;
    TEST_ASSERT(defaultAlert.GetIntensity() == 0);
    SendAndReceive(testDriver, defaultAlert);
  }

DECLARE_TEST_CASE(LiLowLightDetected_zero)
  {
    LightCommMessage_LiLowLightDetected zeroAlert(0);
    TEST_ASSERT(zeroAlert.GetIntensity() == 0);
    SendAndReceive(testDriver, zeroAlert);
  }

DECLARE_TEST_CASE(LiLowLightDetected_max)
  {
    LightCommMessage_LiLowLightDetected maxAlert(0x03ff);
    TEST_ASSERT(maxAlert.GetIntensity() == 0x03ff);
    SendAndReceive(testDriver, maxAlert);
  }

DECLARE_TEST_CASE(LiLowLightDetected_middle)
  {
    LightCommMessage_LiLowLightDetected middleAlert(0x121);
    TEST_ASSERT(middleAlert.GetIntensity() == 0x0121);
    SendAndReceive(testDriver, middleAlert);
  }
  
DECLARE_TEST_CASE(LiHighLightDetected_default)
  {
    LightCommMessage_LiHighLightDetected defaultAlert;
    TEST_ASSERT(defaultAlert.GetIntensity() == 0);
    SendAndReceive(testDriver, defaultAlert);
  }

DECLARE_TEST_CASE(LiHighLightDetected_zero)
  {
    LightCommMessage_LiHighLightDetected zeroAlert(0);
    TEST_ASSERT(zeroAlert.GetIntensity() == 0);
    SendAndReceive(testDriver, zeroAlert);
  }

DECLARE_TEST_CASE(LiHighLightDetected_max)
  {
    LightCommMessage_LiHighLightDetected maxAlert(0x03ff);
    TEST_ASSERT(maxAlert.GetIntensity() == 0x03ff);
    SendAndReceive(testDriver, maxAlert);
  }

DECLARE_TEST_CASE(LiHighLightDetected_middle)
  {
    LightCommMessage_LiHighLightDetected middleAlert(0x256);
    TEST_ASSERT(middleAlert.GetIntensity() == 0x0256);
    SendAndReceive(testDriver, middleAlert);
  }

void setup()
{
  // initialize the serial communication to the PC
  Serial.begin(57600);   

  testDriver.begin();
  testDriver.StartTests();
  
  RUN_TEST_CASE(testDriver, Error_default);
  RUN_TEST_CASE(testDriver, Error_zero);
  RUN_TEST_CASE(testDriver, QueryInputs_default);
  RUN_TEST_CASE(testDriver, QueryInputs_zero);
  RUN_TEST_CASE(testDriver, QueryInputs_max);
  RUN_TEST_CASE(testDriver, QueryInputs_middle);
  RUN_TEST_CASE(testDriver, SetRGB_default);
  RUN_TEST_CASE(testDriver, SetRGB_zero);
  RUN_TEST_CASE(testDriver, SetRGB_max);
  RUN_TEST_CASE(testDriver, SetRGB_middle);
  RUN_TEST_CASE(testDriver, LIConfigDropAlert_default);
  RUN_TEST_CASE(testDriver, LIConfigDropAlert_zero);
  RUN_TEST_CASE(testDriver, LIConfigDropAlert_max);
  RUN_TEST_CASE(testDriver, LIConfigDropAlert_middle);
  RUN_TEST_CASE(testDriver, LiDropAlertDetected_default);
  RUN_TEST_CASE(testDriver, LiDropAlertDetected_zero);
  RUN_TEST_CASE(testDriver, LiDropAlertDetected_max);
  RUN_TEST_CASE(testDriver, LiDropAlertDetected_middle);
  RUN_TEST_CASE(testDriver, LiConfigGradualAlert_default);
  RUN_TEST_CASE(testDriver, LiConfigGradualAlert_zero);
  RUN_TEST_CASE(testDriver, LiConfigGradualAlert_max);
  RUN_TEST_CASE(testDriver, LiConfigGradualAlert_middle);
  RUN_TEST_CASE(testDriver, LiLowLightDetected_default);
  RUN_TEST_CASE(testDriver, LiLowLightDetected_zero);
  RUN_TEST_CASE(testDriver, LiLowLightDetected_max);
  RUN_TEST_CASE(testDriver, LiLowLightDetected_middle);
  RUN_TEST_CASE(testDriver, LiHighLightDetected_default);
  RUN_TEST_CASE(testDriver, LiHighLightDetected_zero);
  RUN_TEST_CASE(testDriver, LiHighLightDetected_max);
  RUN_TEST_CASE(testDriver, LiHighLightDetected_middle);
  
  testDriver.FinishTests();
}

void loop()
{
  testDriver.SignalTestsStatusViaLED();
}


