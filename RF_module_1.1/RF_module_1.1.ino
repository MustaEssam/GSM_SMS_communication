/* 
 *  This is the code for the RF module version 1.1
 *  
 */

#include <SoftwareSerial.h>

String incomingString;
#define led 5
#define swt12 7
#define swt5 6

SoftwareSerial RF_module = SoftwareSerial(3, 4);

void setup() {
  Serial.begin(9600);
  // start RF_module port at 9600 bps:
  RF_module.begin(9600);
  // initialize digital pins as an output.
  pinMode(led, OUTPUT);
  pinMode(swt12, OUTPUT);
  pinMode(swt5, OUTPUT);
  while (!RF_module) {
    ; // wait for RF_module port to connect. Needed for native USB port only
  }
  Serial.println("RF Module 1 initialized");
  digitalWrite(led, LOW);
  digitalWrite(swt12, LOW);
  digitalWrite(swt5, LOW);
}

void loop() {
  // check if data is available
  if (RF_module.available() > 0) {
    // read the incoming string:
    incomingString = RF_module.readStringUntil('\n');
    if(incomingString == "S"){
      digitalWrite(led, HIGH);
      digitalWrite(swt12, HIGH);
      digitalWrite(swt5, HIGH);
      Serial.println("Sw1 Opened");
      RF_module.write(45);
    } else if(incomingString == "s"){
      digitalWrite(led, LOW);
      digitalWrite(swt12, LOW);
      digitalWrite(swt5, LOW);
      Serial.println("Sw1 Closed");
      RF_module.write(40);
    } 
  }
}
