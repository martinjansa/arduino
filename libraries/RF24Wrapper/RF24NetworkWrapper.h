#ifndef RF24_NETWORK_WRAPPER__H
#define RF24_NETWORK_WRAPPER__H

#include <RF24Network.h>

class IRF24Network {
public:

  virtual void update(void) = 0;

  virtual bool available(void) = 0;

  virtual void peek(RF24NetworkHeader& header) = 0;

  virtual size_t read(RF24NetworkHeader& header, void* message, size_t maxlen) = 0;

  virtual bool write(RF24NetworkHeader& header,const void* message, size_t len) = 0;  

  static void DumpHeader(const RF24NetworkHeader &header, const char *prefix = 0, bool newLine = true)
  {
	if (prefix) {
	  Serial.print(prefix);
	}
	Serial.print(F("{type: "));
	Serial.print(header.type, DEC);
	Serial.print(F(", to_node: "));
	Serial.print(header.to_node, DEC);
	Serial.print(F(", from_node: "));
	Serial.print(header.from_node, DEC);
	Serial.print(F(", id: "));
	Serial.print(header.id, DEC);
	Serial.print(F("}"));
	if (newLine) {
	  Serial.println(F(""));
	}
  }
  
};


class RF24NetworkWrapper: public IRF24Network {
public:

  RF24NetworkWrapper(RF24Network &network):
    m_network(network)
  {
  }

  virtual void update(void)
  {
    m_network.update();
  }

  virtual bool available(void)
  {
    return m_network.available();
  }

  virtual void peek(RF24NetworkHeader& header)
  {
    m_network.peek(header);
  }

  virtual size_t read(RF24NetworkHeader& header, void* message, size_t maxlen)
  {
    size_t result = m_network.read(header, message, maxlen);
    IRF24Network::DumpHeader(header, "Message received ");
	return result;
  }

  virtual bool write(RF24NetworkHeader& header,const void* message, size_t len)
  {
    IRF24Network::DumpHeader(header, "Sending message ");
    return m_network.write(header, message, len);
  }

private:
  RF24Network &m_network;

};


#endif  // RF24_NETWORK_WRAPPER__H
