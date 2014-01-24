/*
  Blink
  Turns on an LED on for one second, then off for one second, repeatedly.
 
  This example code is in the public domain.
 */

// the light sensor analog pin
int light_sensor = A0;
 
// Pin 13 has an LED connected on most Arduino boards.
// give it a name:
int led = 13;

int light_status = 0;

// the setup routine runs once when you press reset:
void setup() {         
  
  // initialize the serial communication to the PC
  Serial.begin(9600);  
  
  // initialize the digital pin as an output.
  pinMode(led, OUTPUT);
  
}

// the loop routine runs over and over again forever:
void loop() {

  // read the light sensor input :
  int sensorValue = analogRead(light_sensor);
  
  // print out the value you read:
  Serial.print(sensorValue);
  Serial.print(" ");
  
  int new_light_status = 0;
  
  if (sensorValue < 300) {
    
    new_light_status = 1;    
  }
  
  if (light_status != new_light_status) {
    
    light_status = new_light_status;

    if (light_status) {
      
      // turn the ligh on
      digitalWrite(led, HIGH);   // turn the LED on (HIGH is the voltage level)
      
      Serial.println("light on");
      
    } else {
    
      digitalWrite(led, LOW);    // turn the LED off by making the voltage LOW
      
      Serial.println("light off");
    }
  } else {
      Serial.println("");
  }
  delay(500);               // wait for a second
}

