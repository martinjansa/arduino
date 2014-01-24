/*
  Blink
  Turns on an LED on for one second, then off for one second, repeatedly.
 
  This example code is in the public domain.
 */

// the light sensor analog pin
int light_sensor = A0;
 
// Pin 3 is connected to the Red LED chain
// give it a name:
int redLedPin = 3;
int greenLedPin = 5;
int blueLedPin = 6;

int light_status = 0;

// the setup routine runs once when you press reset:
void setup() {         
  
  // initialize the serial communication to the PC
  Serial.begin(9600);  
  
  // initialize the digital pin as an output.
  pinMode(redLedPin, OUTPUT);
  pinMode(greenLedPin, OUTPUT);
  pinMode(blueLedPin, OUTPUT);
}

static const int LIGHT_INTENSITY_RANGE = 1023;

const int SI_COMMAND = 1;
const int SI_RGB_1 = 2;
const int SI_RGB_2 = 3;
const int SI_RGB_3 = 4;
const int SI_RGB_4 = 5;
const int SI_RGB_5 = 6;
const int SI_RGB_6 = 7;

int serialInputStatus = SI_COMMAND;
int serialInputColorRedHi = 0;
int serialInputColorRedLo = 0;
int serialInputColorGreenHi = 0;
int serialInputColorGreenLo = 0;
int serialInputColorBlueHi = 0;
int serialInputColorBlueLo = 0;

int redValue = 255;
int greenValue = 255;
int blueValue = 255;

int ConvertCharacter(int c) {
  if (c >= '0' && c <= '9') {
    return (c - '0');
  } else {
    if (c >= 'a' && c <= 'f') {
      return (10 + c - 'a');
    } else {
      Serial.println("Invalid character.");
      return 0;
    }
  }
}

void HandleSerialInput() {
  
  while (Serial.available() > 0) {
    
    // read the incoming byte:
    int c = Serial.read();

    switch (serialInputStatus) {
      
      case SI_COMMAND:
        if (c == 'c') {
          serialInputStatus = SI_RGB_1;
        } else if (c == '\r' || c == '\n') {
          // ignore character
        } else {
          Serial.println("Invalid command.");
        }
        break;
        
      case SI_RGB_1:
        serialInputColorRedHi = ConvertCharacter(c);
        serialInputStatus = SI_RGB_2;
        break;
        
      case SI_RGB_2:
        serialInputColorRedLo = ConvertCharacter(c);
        serialInputStatus = SI_RGB_3;
        break;
        
      case SI_RGB_3:
        serialInputColorGreenHi = ConvertCharacter(c);
        serialInputStatus = SI_RGB_4;
        break;
        
      case SI_RGB_4:
        serialInputColorGreenLo = ConvertCharacter(c);
        serialInputStatus = SI_RGB_5;
        break;
        
      case SI_RGB_5:
        serialInputColorBlueHi = ConvertCharacter(c);
        serialInputStatus = SI_RGB_6;
        break;
        
      case SI_RGB_6:
        serialInputColorBlueLo = ConvertCharacter(c);
        
        // calculate the RGB values from the read characters
        redValue = (serialInputColorRedHi << 4) + serialInputColorRedLo;
        greenValue = (serialInputColorGreenHi << 4) + serialInputColorGreenLo;
        blueValue = (serialInputColorBlueHi << 4) + serialInputColorBlueLo;
        
        Serial.print("New RGB: ");
        Serial.print(redValue);
        Serial.print(", ");
        Serial.print(greenValue);
        Serial.print(", ");
        Serial.println(blueValue);

        serialInputStatus = SI_COMMAND;
        break;
        
      default:
        Serial.println("Internal state machine error.");
        break;
    }
  }
}

// the loop routine runs over and over again forever:
void loop() {

  // read the data from serial
  HandleSerialInput();
  
  // read the light sensor input :
  int sensorValue = analogRead(light_sensor);
  
  // print out the value you read:
  Serial.print(sensorValue);
  Serial.print(", duty value (");
  
/*  // limit the maximal value
  if (sensorValue > LIGHT_INTENSITY_RANGE - 1) {
   
    sensorValue = LIGHT_INTENSITY_RANGE - 1;
  } 
  */
  int redDutyValue = int((long(redValue) * long(sensorValue)) / long(LIGHT_INTENSITY_RANGE));
  int greenDutyValue = int((long(greenValue) * long(sensorValue)) / long(LIGHT_INTENSITY_RANGE));
  int blueDutyValue = int((long(blueValue) * long(sensorValue)) / long(LIGHT_INTENSITY_RANGE));
  
  Serial.print(redDutyValue);
  Serial.print(", ");
  Serial.print(greenDutyValue);
  Serial.print(", ");
  Serial.print(blueDutyValue);
  Serial.println(")");

  // turn the ligh on
  analogWrite(redLedPin, redDutyValue);   // activate the red LEDs
  analogWrite(greenLedPin, greenDutyValue);   // activate the red LEDs
  analogWrite(blueLedPin, blueDutyValue);   // activate the red LEDs

  delay(100);               // wait for a second
}

