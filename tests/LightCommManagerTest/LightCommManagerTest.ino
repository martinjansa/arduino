#include <RF24Network.h>
#include <RF24Network_config.h>
#include <Sync.h>

#include <SPI.h>

#include <serialassert.h>
#include <testdriver.h>

#include <assert.h>


#include <nRF24L01.h>
#include <RF24.h>
#include <RF24_config.h>

#include <RF24NetworkWrapper.h>
#include <RF24NetworkWrapper_Mock-Loopback.h>

#include <LightCommManager.h>
#include <LightCommMessage.h>



class LightCommMessageHandlerControllerLoopbackVerifier: public ILightCommControllerMessageHandler {
private:
  RF24CommMessageType m_expectedType;
  LightCommMessage_LiDropAlertDetected *m_liDropAlertDetected;
  LightCommMessage_LiLowLightDetected *m_liLowLightDetected;
  LightCommMessage_LiHighLightDetected *m_liHighLightDetected;
  bool m_unexpectedMessageReceived;
  bool m_expectedMessageReceived;
  LightCommError m_respondWithResult;
  
  void ClearExpected()
  {
    m_expectedType = RF24CMT_UNDEFINED;
    m_liDropAlertDetected = 0;
    m_liLowLightDetected = 0;
    m_liHighLightDetected = 0;
    m_respondWithResult = 0;

    m_unexpectedMessageReceived = false;
    m_expectedMessageReceived = false;
  }
  
  void EvaluateExpected(bool rightMessage)
  {
    if (rightMessage) {
      if (m_expectedMessageReceived) {
        m_unexpectedMessageReceived = true;
      } else {
        m_expectedMessageReceived = true;
      }
    } else {
       m_unexpectedMessageReceived = true;
    }
  }
  
public:

  LightCommMessageHandlerControllerLoopbackVerifier()
  {
    ClearExpected();
  }
  
  void Expect(RF24CommMessageType expectedType, LightCommError respondWithResult = 0)
  {
    ClearExpected();
    m_expectedType = expectedType;
  }
  void Expect(LightCommMessage_LiDropAlertDetected &liDropAlertDetected, LightCommError respondWithResult = 0)
  {
    ClearExpected();
    m_liDropAlertDetected = &liDropAlertDetected; 
    m_respondWithResult = respondWithResult;
  }
  void Expect(LightCommMessage_LiLowLightDetected &liLowLightDetected, LightCommError respondWithResult = 0)
  {
    ClearExpected();
    m_liLowLightDetected = &liLowLightDetected; 
    m_respondWithResult = respondWithResult;
  }
  void Expect(LightCommMessage_LiHighLightDetected &liHighLightDetected, LightCommError respondWithResult = 0)
  {
    ClearExpected();
    m_liHighLightDetected = &liHighLightDetected; 
    m_respondWithResult = respondWithResult;
  }

  bool ExpectedMessageReceived()
  {
    return m_expectedMessageReceived && !m_unexpectedMessageReceived;
  }
  
  virtual LightCommError HandleLiDropAlertDetected(uint16_t senderNode, const LightCommMessage_LiDropAlertDetected &message)
  {
    EvaluateExpected(m_liDropAlertDetected && m_liDropAlertDetected->GetBefore() == message.GetBefore() && m_liDropAlertDetected->GetAfter() == message.GetAfter());
    return m_respondWithResult;
  }

  virtual LightCommError HandleLiLowLightDetected(uint16_t senderNode, const LightCommMessage_LiLowLightDetected &message)
  {
    EvaluateExpected(m_liLowLightDetected && m_liLowLightDetected->GetIntensity() == message.GetIntensity());
    return m_respondWithResult;
  }
  
  virtual LightCommError HandleLiHighLightDetected(uint16_t senderNode, const LightCommMessage_LiHighLightDetected &message)
  {
    EvaluateExpected(m_liHighLightDetected && m_liHighLightDetected->GetIntensity() == message.GetIntensity());
    return m_respondWithResult;
  }

};


class LightCommMessageLightDriverHandlerLoopbackVerifier: public ILightCommLightDriverMessageHandler {
private:
  RF24CommMessageType m_expectedType;
  LightCommMessage_QueryInputs *m_queryInputs;
  LightCommMessage_SetRGB *m_setRGB;
  LightCommMessage_LIConfigDropAlert *m_lIConfigDropAlert;
  LightCommMessage_LiConfigGradualAlert *m_liConfigGradualAlert;
  bool m_unexpectedMessageReceived;
  bool m_expectedMessageReceived;
  LightCommError m_respondWithResult;
  
  void ClearExpected()
  {
    m_expectedType = RF24CMT_UNDEFINED;
    m_queryInputs = 0;
    m_setRGB = 0;
    m_lIConfigDropAlert = 0;
    m_liConfigGradualAlert = 0;
    m_respondWithResult = 0;

    m_unexpectedMessageReceived = false;
    m_expectedMessageReceived = false;
  }
  
  void EvaluateExpected(bool rightMessage)
  {
    if (rightMessage) {
      if (m_expectedMessageReceived) {
        m_unexpectedMessageReceived = true;
      } else {
        m_expectedMessageReceived = true;
      }
    } else {
       m_unexpectedMessageReceived = true;
    }
  }
  
public:

  LightCommMessageLightDriverHandlerLoopbackVerifier()
  {
    ClearExpected();
  }
  
  void Expect(RF24CommMessageType expectedType, LightCommError respondWithResult = 0)
  {
    ClearExpected();
    m_expectedType = expectedType;
    m_respondWithResult = respondWithResult;
  }
  void Expect(LightCommMessage_QueryInputs &queryInputs, LightCommError respondWithResult = 0)
  {
    ClearExpected();
    m_queryInputs = &queryInputs;
    m_respondWithResult = respondWithResult;
  }
  void Expect(LightCommMessage_SetRGB &setRGB, LightCommError respondWithResult = 0)
  {
    ClearExpected();
    m_setRGB = &setRGB; 
    m_respondWithResult = respondWithResult;
  }
  void Expect(LightCommMessage_LIConfigDropAlert &lIConfigDropAlert, LightCommError respondWithResult = 0)
  {
    ClearExpected();
    m_lIConfigDropAlert = &lIConfigDropAlert; 
    m_respondWithResult = respondWithResult;
  }
  void Expect(LightCommMessage_LiConfigGradualAlert &liConfigGradualAlert, LightCommError respondWithResult = 0)
  {
    ClearExpected();
    m_liConfigGradualAlert = &liConfigGradualAlert; 
    m_respondWithResult = respondWithResult;
  }

  bool ExpectedMessageReceived()
  {
    return m_expectedMessageReceived && !m_unexpectedMessageReceived;
  }
  
  virtual LightCommError HandleReset(uint16_t senderNode)
  {
    EvaluateExpected(m_expectedType == RF24CMT_RESET);
    return m_respondWithResult;
  }


  virtual void HandleQueryInputs(uint16_t senderNode, const LightCommMessage_QueryInputs &message, RF24NetworkWrapper &network)
  {
    EvaluateExpected(m_queryInputs && m_queryInputs->GetAnalog() == message.GetAnalog() && m_queryInputs->GetDigital() == message.GetDigital());
  }

  virtual LightCommError HandleSleep(uint16_t senderNode)
  {
    EvaluateExpected(m_expectedType == RF24CMT_SLEEP);
    return m_respondWithResult;
  }
  
  virtual LightCommError HandleWake(uint16_t senderNode)
  {
    EvaluateExpected(m_expectedType == RF24CMT_WAKE);
    return m_respondWithResult;
  }
 
//  virtual LightCommError HandleQueryStatus(uint16_t senderNode) = 0;

  virtual LightCommError HandleSetRGB(uint16_t senderNode, const LightCommMessage_SetRGB &message)
  {
    EvaluateExpected(m_setRGB && m_setRGB->GetRed() == message.GetRed() && m_setRGB->GetGreen() == message.GetGreen() && m_setRGB->GetBlue() == message.GetBlue());
    return m_respondWithResult;
  }


  virtual LightCommError HandleLiConfigDropAlert(uint16_t senderNode, const LightCommMessage_LIConfigDropAlert &message)
  {
    EvaluateExpected(m_lIConfigDropAlert && m_lIConfigDropAlert->GetHigh() == message.GetHigh() && m_lIConfigDropAlert->GetLow() == message.GetLow() && m_lIConfigDropAlert->GetMs() == message.GetMs());
    return m_respondWithResult;
  }
  
  virtual LightCommError HandleLiConfigGradualAlert(uint16_t senderNode, const LightCommMessage_LiConfigGradualAlert &message)
  {
    EvaluateExpected(m_liConfigGradualAlert && m_liConfigGradualAlert->GetHigh() == message.GetHigh() && m_liConfigGradualAlert->GetLow() == message.GetLow());
    return m_respondWithResult;
  }

};

DECLARE_TEST_CASE(ResetMessage_OK)
{
  // loopback mockup network  
  RF24NetworkLoopback loopback;

  // write a message to be handled by the handler
  RF24NetworkHeader header;
  header.from_node = 1;
  header.to_node = 0;
  header.id = header.next_id++;
  TEST_ASSERT(ILightCommMessage::writeNoParams(header, loopback, RF24CMT_RESET));
  loopback.MakeSentDataAvailableForReading();

  // init the comm manager
  LightCommManager manager((RF24NetworkWrapper &) loopback, 0);
    
  // configure the handler to receive the right expected message
  LightCommMessageLightDriverHandlerLoopbackVerifier messageHandler;
  messageHandler.Expect(RF24CMT_RESET);
  
  // handle the incoming messages and check if the expected one arrives
  manager.HandleLightDriverIncommingMessages(messageHandler);

  // check if the expected message has arrived
  TEST_ASSERT(messageHandler.ExpectedMessageReceived());

  // loopback the OK response
  loopback.MakeSentDataAvailableForReading();
  RF24NetworkHeader receivedHeader;
  TEST_ASSERT(ILightCommMessage::readNoParams(receivedHeader, loopback, RF24CMT_UNDEFINED));
  
  TEST_ASSERT(receivedHeader.from_node == 0);
  TEST_ASSERT(receivedHeader.to_node == 1);
  TEST_ASSERT(receivedHeader.type == RF24CMT_OK);
}

DECLARE_TEST_CASE(ResetMessage_FAIL)
{
  // loopback mockup network  
  RF24NetworkLoopback loopback;

  // write a message to be handled by the handler
  RF24NetworkHeader header;
  header.from_node = 1;
  header.to_node = 0;
  header.id = header.next_id++;
  TEST_ASSERT(ILightCommMessage::writeNoParams(header, loopback, RF24CMT_RESET));
  loopback.MakeSentDataAvailableForReading();

  // init the comm manager
  LightCommManager manager((RF24NetworkWrapper &) loopback, 0);
    
  // configure the handler to receive the right expected message
  LightCommMessageLightDriverHandlerLoopbackVerifier messageHandler;
  messageHandler.Expect(RF24CMT_RESET, 100);
  
  // handle the incoming messages and check if the expected one arrives
  manager.HandleLightDriverIncommingMessages(messageHandler);

  // check if the expected message has arrived
  TEST_ASSERT(messageHandler.ExpectedMessageReceived());

  // loopback the OK response
  loopback.MakeSentDataAvailableForReading();
  RF24NetworkHeader receivedHeader;
  TEST_ASSERT(ILightCommMessage::readNoParams(receivedHeader, loopback, RF24CMT_UNDEFINED));
  
  TEST_ASSERT(receivedHeader.from_node == 0);
  TEST_ASSERT(receivedHeader.to_node == 1);
  TEST_ASSERT(receivedHeader.type == RF24CMT_ERROR);
}

DECLARE_TEST_CASE(SleepMessage_OK)
{
  // loopback mockup network  
  RF24NetworkLoopback loopback;

  // write a message to be handled by the handler
  RF24NetworkHeader header;
  header.from_node = 1;
  header.to_node = 0;
  header.id = header.next_id++;
  TEST_ASSERT(ILightCommMessage::writeNoParams(header, loopback, RF24CMT_SLEEP));
  loopback.MakeSentDataAvailableForReading();

  // init the comm manager
  LightCommManager manager((RF24NetworkWrapper &) loopback, 0);
    
  // configure the handler to receive the right expected message
  LightCommMessageLightDriverHandlerLoopbackVerifier messageHandler;
  messageHandler.Expect(RF24CMT_SLEEP);
  
  // handle the incoming messages and check if the expected one arrives
  manager.HandleLightDriverIncommingMessages(messageHandler);

  // check if the expected message has arrived
  TEST_ASSERT(messageHandler.ExpectedMessageReceived());

  // loopback the OK response
  loopback.MakeSentDataAvailableForReading();
  RF24NetworkHeader receivedHeader;
  TEST_ASSERT(ILightCommMessage::readNoParams(receivedHeader, loopback, RF24CMT_UNDEFINED));
  
  TEST_ASSERT(receivedHeader.from_node == 0);
  TEST_ASSERT(receivedHeader.to_node == 1);
  TEST_ASSERT(receivedHeader.type == RF24CMT_OK);
}

DECLARE_TEST_CASE(SleepMessage_FAIL)
{
  // loopback mockup network  
  RF24NetworkLoopback loopback;

  // write a message to be handled by the handler
  RF24NetworkHeader header;
  header.from_node = 1;
  header.to_node = 0;
  header.id = header.next_id++;
  TEST_ASSERT(ILightCommMessage::writeNoParams(header, loopback, RF24CMT_SLEEP));
  loopback.MakeSentDataAvailableForReading();

  // init the comm manager
  LightCommManager manager((RF24NetworkWrapper &) loopback, 0);
    
  // configure the handler to receive the right expected message
  LightCommMessageLightDriverHandlerLoopbackVerifier messageHandler;
  messageHandler.Expect(RF24CMT_SLEEP, 100);
  
  // handle the incoming messages and check if the expected one arrives
  manager.HandleLightDriverIncommingMessages(messageHandler);

  // check if the expected message has arrived
  TEST_ASSERT(messageHandler.ExpectedMessageReceived());

  // loopback the OK response
  loopback.MakeSentDataAvailableForReading();
  RF24NetworkHeader receivedHeader;
  TEST_ASSERT(ILightCommMessage::readNoParams(receivedHeader, loopback, RF24CMT_UNDEFINED));
  
  TEST_ASSERT(receivedHeader.from_node == 0);
  TEST_ASSERT(receivedHeader.to_node == 1);
  TEST_ASSERT(receivedHeader.type == RF24CMT_ERROR);
}

DECLARE_TEST_CASE(WakeMessage_OK)
{
  // loopback mockup network  
  RF24NetworkLoopback loopback;

  // write a message to be handled by the handler
  RF24NetworkHeader header;
  header.from_node = 1;
  header.to_node = 0;
  header.id = header.next_id++;
  TEST_ASSERT(ILightCommMessage::writeNoParams(header, loopback, RF24CMT_WAKE));
  loopback.MakeSentDataAvailableForReading();

  // init the comm manager
  LightCommManager manager((RF24NetworkWrapper &) loopback, 0);
    
  // configure the handler to receive the right expected message
  LightCommMessageLightDriverHandlerLoopbackVerifier messageHandler;
  messageHandler.Expect(RF24CMT_WAKE);
  
  // handle the incoming messages and check if the expected one arrives
  manager.HandleLightDriverIncommingMessages(messageHandler);

  // check if the expected message has arrived
  TEST_ASSERT(messageHandler.ExpectedMessageReceived());

  // loopback the OK response
  loopback.MakeSentDataAvailableForReading();
  RF24NetworkHeader receivedHeader;
  TEST_ASSERT(ILightCommMessage::readNoParams(receivedHeader, loopback, RF24CMT_UNDEFINED));
  
  TEST_ASSERT(receivedHeader.from_node == 0);
  TEST_ASSERT(receivedHeader.to_node == 1);
  TEST_ASSERT(receivedHeader.type == RF24CMT_OK);
}

DECLARE_TEST_CASE(WakeMessage_FAIL)
{
  // loopback mockup network  
  RF24NetworkLoopback loopback;

  // write a message to be handled by the handler
  RF24NetworkHeader header;
  header.from_node = 1;
  header.to_node = 0;
  header.id = header.next_id++;
  TEST_ASSERT(ILightCommMessage::writeNoParams(header, loopback, RF24CMT_WAKE));
  loopback.MakeSentDataAvailableForReading();

  // init the comm manager
  LightCommManager manager((RF24NetworkWrapper &) loopback, 0);
    
  // configure the handler to receive the right expected message
  LightCommMessageLightDriverHandlerLoopbackVerifier messageHandler;
  messageHandler.Expect(RF24CMT_WAKE, 100);
  
  // handle the incoming messages and check if the expected one arrives
  manager.HandleLightDriverIncommingMessages(messageHandler);

  // check if the expected message has arrived
  TEST_ASSERT(messageHandler.ExpectedMessageReceived());

  // loopback the OK response
  loopback.MakeSentDataAvailableForReading();
  RF24NetworkHeader receivedHeader;
  TEST_ASSERT(ILightCommMessage::readNoParams(receivedHeader, loopback, RF24CMT_UNDEFINED));
  
  TEST_ASSERT(receivedHeader.from_node == 0);
  TEST_ASSERT(receivedHeader.to_node == 1);
  TEST_ASSERT(receivedHeader.type == RF24CMT_ERROR);
}

TestDriver testDriver(13);
  
void setup()
{
  // initialize the serial communication to the PC
  Serial.begin(57600);   

  testDriver.begin();
  testDriver.StartTests();
  
  RUN_TEST_CASE(testDriver, ResetMessage_OK);
  RUN_TEST_CASE(testDriver, ResetMessage_FAIL);
  RUN_TEST_CASE(testDriver, SleepMessage_OK);
  RUN_TEST_CASE(testDriver, SleepMessage_FAIL);
  RUN_TEST_CASE(testDriver, WakeMessage_OK);
  RUN_TEST_CASE(testDriver, WakeMessage_FAIL);
  
  testDriver.FinishTests();
}

void loop()
{
  testDriver.SignalTestsStatusViaLED();
}


