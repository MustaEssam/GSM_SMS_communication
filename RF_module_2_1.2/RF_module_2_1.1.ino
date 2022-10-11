/* 
 *  This is the code for the RF module 2 version 2.1
 *  
 */

#include <SoftwareSerial.h>

String incomingString;
#define led 6
#define swt24 7
#define swt5 8

SoftwareSerial RF_module = SoftwareSerial(4, 5);

void setup() {
  Serial.begin(9600);
  // start RF_module port at 9600 bps:
  RF_module.begin(9600);
  // initialize digital pins as an output.
  pinMode(led, OUTPUT);
  pinMode(swt24, OUTPUT);
  pinMode(swt5, OUTPUT);
  while (!RF_module) {
    ; // wait for RF_module port to connect. Needed for native USB port only
  }
  Serial.println("RF Module 2 initialized");
  digitalWrite(led, LOW);
  digitalWrite(swt24, LOW);
  digitalWrite(swt5, LOW);
}

void loop() {
  // check if data is available
  if (RF_module.available() > 0) {
    // read the incoming string:
    incomingString = RF_module.readStringUntil('\n');
    if(incomingString == "S2"){
      digitalWrite(led, HIGH);
      digitalWrite(swt24, HIGH);
      digitalWrite(swt5, HIGH);
      Serial.println("Sw2 Opened");
      RF_module.write(45);
    } else if(incomingString == "s2"){
      digitalWrite(led, LOW);
      digitalWrite(swt24, LOW);
      digitalWrite(swt5, LOW);
      Serial.println("Sw2 Closed");
      RF_module.write(40);
    } 
  }
}
