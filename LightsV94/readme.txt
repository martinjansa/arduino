The lights are driven by 5 Arduino modules, that communicate to each other via the RF24 network.

The modules have following RF24 addresses:
  000: controller (base node of the network)
  001: LED driver #1 (kitchen, WC, mail door)
  002: LED driver #2 (longer corridor - south), light intensity sensor, IR proximity sensor
  003: LED driver #3 (longer corridor - middle)
  004: LED driver #4 (longer corridor - north)
  
The Controller runs the sketch ControllerModule, which uses LightComm library, the individual LED drivers run the sketch LedDriverModule with the same version. The distinguishing of the individual nodes is done via ID in the EEPROM.

