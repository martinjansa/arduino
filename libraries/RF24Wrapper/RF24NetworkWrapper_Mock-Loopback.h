#ifndef RF24_NETWORK_WRAPPER__MOCK_LOOPBACK_H
#define RF24_NETWORK_WRAPPER__MOCK_LOOPBACK_H

#include "RF24NetworkWrapper.h"
#include <serialassert.h>

class RF24NetworkLoopback: public IRF24Network {
private:
  bool dataAvailable;
  RF24NetworkHeader internalHeader;
  byte data[16];
  size_t dataLen;
  
public:
  RF24NetworkLoopback(): dataAvailable(false), internalHeader(), dataLen(0) {}

  void MakeSentDataAvailableForReading()
  {
    dataAvailable = true;
  }
  
  virtual void update(void)
  {
  }

  virtual bool available(void)
  {
    return dataAvailable;
  };

  virtual void peek(RF24NetworkHeader& header)
  {
    header = internalHeader;
  }

  virtual size_t read(RF24NetworkHeader& header, void* message, size_t maxlen)
  {
    assert(dataAvailable);
    header = internalHeader;
    size_t count = min(maxlen, dataLen);
    memcpy(message, &data[0], count);
    dataAvailable = false;
    IRF24Network::DumpHeader(header, "Message received ");
    return count;
  }

  virtual bool write(RF24NetworkHeader& header, const void* message, size_t len)
  {
    IRF24Network::DumpHeader(header, "Sending message ");
    assert(len <= 16);
    internalHeader = header;
    memcpy(&data[0], message, len);
    dataLen = len;
    for (size_t i = 0; i < len; i++) {
      Serial.print(data[i], HEX);
      Serial.print(F(" "));
    }
    Serial.println(F(""));
    return true;
  }
};


#endif  // RF24_NETWORK_WRAPPER__MOCK_LOOPBACK_H

