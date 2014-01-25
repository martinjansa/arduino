#ifndef LIGHT_COMM_MANAGER__H
#define LIGHT_COMM_MANAGER__H

#define __ASSERT_USE_STDERR

#include <LightCommMessage.h>


class ILightCommControllerMessageHandler {
public:

  virtual LightCommError HandleLiDropAlertDetected(uint16_t senderNode) = 0;

  virtual LightCommError HandleLiLowLightDetected(uint16_t senderNode) = 0;

  virtual LightCommError HandleLiHighLightDetected(uint16_t senderNode) = 0;

};


class ILightCommLightDriverMessageHandler {
public:

  virtual LightCommError HandleReset(uint16_t senderNode) = 0;

  virtual void HandleQueryInputs(uint16_t senderNode, const LightCommMessage_QueryInputs &message, RF24NetworkWrapper &network) = 0;

  virtual LightCommError HandleSleep(uint16_t senderNode) = 0;
  virtual LightCommError HandleWake(uint16_t senderNode) = 0 ;

//  virtual LightCommError HandleQueryStatus(uint16_t senderNode) = 0;

  virtual LightCommError HandleSetHSBColor(uint16_t senderNode, const LightCommMessage_SetHSBColor &message) = 0;

  virtual LightCommError HandleLiConfigDropAlert(uint16_t senderNode, const LightCommMessage_LIConfigDropAlert &message) = 0;

  virtual LightCommError HandleLiConfigGradualAlert(uint16_t senderNode, const LightCommMessage_LiConfigGradualAlert &message) = 0;

};


/*
  RF24 rf24Radio(_cepin, _cspin);
  RF24Network rf24network(rf24Radio);
  RF24NetworkWrapper network(rf24network);
  LightCommManager commManager(network, nodeAddress);
  
  rf24network.begin(channel, nodeAddress);
*/

class LightCommManager {
private:

  RF24NetworkWrapper &m_network;
  uint16_t m_nodeAddress;

private:

  bool SendMessage(uint16_t recepientNode, const ILightCommMessage &message)
  {
    RF24NetworkHeader header;
	header.to_node = recepientNode;
	header.from_node = m_nodeAddress;
	header.id = RF24NetworkHeader::next_id++;
    return message.write(header, m_network);
  }

  bool SendMessage(uint16_t recepientNode, RF24CommMessageType messageType)
  {
    RF24NetworkHeader header;
	header.to_node = recepientNode;
	header.from_node = m_nodeAddress;
	header.id = RF24NetworkHeader::next_id++;
    return ILightCommMessage::writeNoParams(header, m_network, messageType);
  }


  void SendResult(uint16_t recepientNode, LightCommError result)
  {
    bool sendResult = false;
	
    // if succeeded
    if (result == 0) {

      Serial.print(F("Sending OK message to node "));
	  Serial.println(recepientNode);
	  
      // send OK message
      sendResult = SendMessage(recepientNode, RF24CMT_OK);

    } else {

      Serial.print(F("Sending error "));
	  Serial.print(result);
	  Serial.print(F(" message to node "));
	  Serial.println(recepientNode);

      // send error message with error code
      sendResult = SendMessage(recepientNode, LightCommMessage_Error(result));
    }
	
	if (sendResult) {
	
      Serial.print(F("Sending succeeded."));

    } else {
	
      Serial.print(F("Sending failed."));
	}
  }

  bool ReceiveResult(uint16_t senderNode, LightCommError &receivedResult, RF24CommMessageType &receivedMessageType, word timeOut = 200)
  {
    unsigned long start_time = millis();
	const unsigned long timeOutLong = timeOut;

	Serial.print(F("ReceiveResult: (0 ms) waiting for the response from "));
	Serial.println(senderNode);
	
    while ((millis() - start_time) < timeOutLong) {

	  Serial.print(F("ReceiveResult: ("));
	  Serial.print(millis() - start_time);
	  Serial.println(F(" ms) updating network."));

      // update the network
      m_network.update();

      if (m_network.available()) {

	    Serial.print(F("ReceiveResult: ("));
	    Serial.print(millis() - start_time);
	    Serial.println(F(" ms) message available."));
	  
        // get the header of the current message, but leave it in the queue
        RF24NetworkHeader header;
        m_network.peek(header);    

        IRF24Network::DumpHeader(header, "ReceiveResult: Message in the head of the network queue ");
		
        // if the message is for this node from the expected sender
        if (header.to_node == m_nodeAddress && header.from_node == senderNode) {

          // get the type of the received message
          receivedMessageType = RF24CommMessageType(header.type);

          // switch according to the received message type
          switch (receivedMessageType) {

            case RF24CMT_OK:

	          Serial.println(F("ReceiveResult: OK message received."));
			  
			  // remove the OK message from the queue
			  ILightCommMessage::readNoParams(header, m_network, RF24CMT_OK);

			  // OK is expected
              receivedResult = 0;

              // report success
              return true;

            case RF24CMT_ERROR:

              {
                // error message is expected, receive the parameters
                LightCommMessage_Error errorMessage;
                if (errorMessage.read(header, m_network)) {

                  // get the received error code from the message
                  receivedResult = errorMessage.GetErrorCode();

	              Serial.print(F("ReceiveResult: error message received with error code "));
				  Serial.println(receivedResult);

			      // report successful reception of the error
                  return true;
				  
                } else {
				
	              Serial.println(F("ReceiveResult: receiving of the error message FAILED."));
				}
              }

              // report failure
              return false;

            default:

	          Serial.println(F("ReceiveResult: ERROR: unexpected message received."));
			  
              // report failure
              return false;
          }
        }
      }
    }

	Serial.print(F("ReceiveResult: ("));
	Serial.print(millis() - start_time);
	Serial.println(F(" ms) TIMEOUT ERROR."));
	
    // report failure (timeout)
    return false;
  }

  bool ReceiveResult(uint16_t senderNode, LightCommError &receivedResult, word timeOut = 200)
  { 
    RF24CommMessageType receivedMessageType = RF24CMT_UNDEFINED;
    return ReceiveResult(senderNode, receivedResult, receivedMessageType, timeOut);
  }

  bool ReceiveOkErrorResult(uint16_t senderNode, word timeOut = 200)
  {
    // receive processing result from the recepient  
    LightCommError receivedError = 0;
    bool result = ReceiveResult(senderNode, receivedError, timeOut);

    if (result) {

	  Serial.print(F("ReceiveOkErrorResult: receive result succeeded, received error: "));
	  Serial.println(receivedError);
	  
      result = (receivedError == 0);
	  
    } else {
	
	  Serial.println(F("ReceiveOkErrorResult: receive result FAILED."));
	}

    return result;
  }

public:

  LightCommManager(RF24NetworkWrapper &network): m_network(network), m_nodeAddress(0)
  {
  }

  void begin(uint16_t nodeAddress)
  {
    m_nodeAddress = nodeAddress;
  }  
 
  void HandleControllerIncommingMessages(ILightCommControllerMessageHandler &handler)
  {
    m_network.update();

    while (m_network.available()) {
  
      // get the header of the current message, but leave it in the queue
      RF24NetworkHeader header;
      m_network.peek(header);    

	  IRF24Network::DumpHeader(header, "Message in the head of light driver handler queue ");

      // if the message is for this node
      if (header.to_node == m_nodeAddress) {

        // switch according to the received message type
        switch (RF24CommMessageType(header.type)) {

          case RF24CMT_LI_DROP_ALERT_DETECTED:
            {
              if (ILightCommMessage::readNoParams(header, m_network, RF24CMT_LI_DROP_ALERT_DETECTED)) {
                LightCommError result = handler.HandleLiDropAlertDetected(header.from_node);
                
                // send the handling result
                SendResult(header.from_node, result);
              }
            }
            break;
 
          case RF24CMT_LI_LOW_LIGHT_DETECTED:
            {
              if (ILightCommMessage::readNoParams(header, m_network, RF24CMT_LI_LOW_LIGHT_DETECTED)) {
                LightCommError result = handler.HandleLiLowLightDetected(header.from_node);
                
                // send the handling result
                SendResult(header.from_node, result);
              }
            }
            break;

          case RF24CMT_LI_HIGH_LIGHT_DETECTED:
            {
              if (ILightCommMessage::readNoParams(header, m_network, RF24CMT_LI_HIGH_LIGHT_DETECTED)) {
                LightCommError result = handler.HandleLiHighLightDetected(header.from_node);
                
                // send the handling result
                SendResult(header.from_node, result);
              }
            }
            break;
 
          default:
		    // unexpected message type arrived
		    IRF24Network::DumpHeader(header, "ERROR: Unexpected message received ");
			
			// remove any unexpected messages from the network queue
			ILightCommMessage::readNoParams(header, m_network, RF24CMT_UNDEFINED);
		}
      }
    }
  }

  void HandleLightDriverIncommingMessages(ILightCommLightDriverMessageHandler &handler)
  {
    m_network.update();

    while (m_network.available()) {
  	  
      // get the header of the current message, but leave it in the queue
      RF24NetworkHeader header;
      m_network.peek(header);    

	  IRF24Network::DumpHeader(header, "Message in the head of light driver handler queue ");
	  
      // if the message is for this node
      if (header.to_node == m_nodeAddress) {

        // switch according to the received message type
        switch (RF24CommMessageType(header.type)) {
          case RF24CMT_RESET:
            if (ILightCommMessage::readNoParams(header, m_network, RF24CMT_RESET)) {
              LightCommError result = handler.HandleReset(header.from_node);
                
              // send the handling result
              SendResult(header.from_node, result);
            }
            break;

          case RF24CMT_QUERY_INPUTS:
            {
              LightCommMessage_QueryInputs message;
              if (message.read(header, m_network)) {
                handler.HandleQueryInputs(header.from_node, message, m_network);
              }
            }
            break;

          case RF24CMT_SLEEP:
            if (ILightCommMessage::readNoParams(header, m_network, RF24CMT_SLEEP)) {
              LightCommError result = handler.HandleSleep(header.from_node);
                
              // send the handling result
              SendResult(header.from_node, result);
            }
            break;

          case RF24CMT_WAKE:
            if (ILightCommMessage::readNoParams(header, m_network, RF24CMT_WAKE)) {
              LightCommError result = handler.HandleWake(header.from_node);
                
              // send the handling result
              SendResult(header.from_node, result);
            }
            break;

//        case RF24CMT_QUERY_STATUS:
//          HandleQueryStatus(header.from_node)

          case RF24CMT_SET_HSB_COLOR:
            {
              LightCommMessage_SetHSBColor message;
              if (message.read(header, m_network)) {
                LightCommError result = handler.HandleSetHSBColor(header.from_node, message);
                
                // send the handling result
                SendResult(header.from_node, result);
              }
            }

          case RF24CMT_LI_CONFIG_DROP_ALERT:
            {
              LightCommMessage_LIConfigDropAlert message;
              if (message.read(header, m_network)) {
                LightCommError result = handler.HandleLiConfigDropAlert(header.from_node, message);

                // send the handling result
                SendResult(header.from_node, result);
              }
            }
            break;

          case RF24CMT_LI_CONFIG_GRADUAL_ALERT:
            {
              LightCommMessage_LiConfigGradualAlert message;
              if (message.read(header, m_network)) {
                LightCommError result = handler.HandleLiConfigGradualAlert(header.from_node, message);

                // send the handling result
                SendResult(header.from_node, result);
              }
            }
            break;
 
 
          default:
		    // unexpected message type arrived
		    IRF24Network::DumpHeader(header, "ERROR: Unexpected message received ");
			
			// remove any unexpected messages from the network queue
			ILightCommMessage::readNoParams(header, m_network, RF24CMT_UNDEFINED);
        }
      }
    }
  }

  bool ResetLightDriver(uint16_t recepientNode)
  {
    // send the reset message
    bool result = SendMessage(recepientNode, RF24CMT_RESET);

    // wait for the response (only OK or error expected)
    return (result && ReceiveOkErrorResult(recepientNode));
  }

  bool SetLightDriverPower(uint16_t recepientNode, bool powerActive)
  {
    // send the wake or sleep message
    bool result = SendMessage(recepientNode, (powerActive? RF24CMT_WAKE: RF24CMT_SLEEP));

    // wait for the response (only OK or error expected)
    return (result && ReceiveOkErrorResult(recepientNode));
  }

  bool SetLightDriverHSBColor(uint16_t recepientNode, word hue, byte sat, byte bri, word ms)
  {
    // send the set RGB reset message
    LightCommMessage_SetHSBColor setHSBColor(hue, sat, bri, ms);
    bool result = SendMessage(recepientNode, setHSBColor);

    // wait for the response (only OK or error expected)
    return (result && ReceiveOkErrorResult(recepientNode));
  }

  bool ConfigLightDriverDropAlert(uint16_t recepientNode, word high, word low, word ms)
  {
    // send the config message
    LightCommMessage_LIConfigDropAlert message(high, low, ms);
    bool result = SendMessage(recepientNode, message);

    if (result) {
	  Serial.println(F("ConfigLightDriverDropAlert: sending config message succeeded."));
	} else {
	  Serial.println(F("ConfigLightDriverDropAlert: sending config message FAILED."));
	}
	
    // wait for the response (only OK or error expected)
    return (result && ReceiveOkErrorResult(recepientNode));
  }

  bool ReportLightIntensityDropDected(uint16_t recepientNode)
  {
    // send the RF24CMT_LI_DROP_ALERT_DETECTED message
    bool result = SendMessage(recepientNode, RF24CMT_LI_DROP_ALERT_DETECTED);

    // wait for the response (only OK or error expected)
    return (result && ReceiveOkErrorResult(recepientNode));
  }

  bool ConfigLightDriverGradualAlert(uint16_t recepientNode, word high, word low)
  {
    // send the config message
    LightCommMessage_LiConfigGradualAlert message(high, low);
    bool result = SendMessage(recepientNode, message);

    if (result) {
	  Serial.println(F("ConfigLightDriverGradualAlert: sending config message succeeded."));
	} else {
	  Serial.println(F("ConfigLightDriverGradualAlert: sending config message FAILED."));
	}
	
    // wait for the response (only OK or error expected)
    return (result && ReceiveOkErrorResult(recepientNode));
  }

  bool ReportLowLightIntensityDected(uint16_t recepientNode)
  {
    // send the RF24CMT_LI_LOW_LIGHT_DETECTED message
    bool result = SendMessage(recepientNode, RF24CMT_LI_LOW_LIGHT_DETECTED);

    // wait for the response (only OK or error expected)
    return (result && ReceiveOkErrorResult(recepientNode));
  }

  bool ReportHighLightIntensityDected(uint16_t recepientNode)
  {
    // send the RF24CMT_LI_HIGH_LIGHT_DETECTED message
    bool result = SendMessage(recepientNode, RF24CMT_LI_HIGH_LIGHT_DETECTED);

    // wait for the response (only OK or error expected)
    return (result && ReceiveOkErrorResult(recepientNode));
  }
  
};



#endif // LIGHT_COMM_MANAGER__H
