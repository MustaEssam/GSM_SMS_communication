
/******* ORIGINAL ADAFRUIT FONA LIBRARY TEXT *******/
/***************************************************
  This is an example for our Adafruit FONA Cellular Module

  Designed specifically to work with the Adafruit FONA
  ----> http://www.adafruit.com/products/1946
  ----> http://www.adafruit.com/products/1963
  ----> http://www.adafruit.com/products/2468
  ----> http://www.adafruit.com/products/2542

  These cellular modules use TTL Serial to communicate, 2 pins are
  required to interface
  Adafruit invests time and resources providing this open source code,
  please support Adafruit and open-source hardware by purchasing
  products from Adafruit!

  Written by Limor Fried/Ladyada for Adafruit Industries.
  BSD license, all text above must be included in any redistribution
 ****************************************************/

#include "Adafruit_FONA.h"

// Define *one* of the following lines:
#define SIMCOM_7000

#if defined(SIMCOM_7000) || defined(SIMCOM_7070)
  // For botletics SIM7000/7070 shield
  #define FONA_PWRKEY 6
  #define FONA_RST 7 // No RST pin for SIM7070
  //#define FONA_DTR 8 // Connect with solder jumper
  //#define FONA_RI 9 // Need to enable via AT commands
  #define FONA_TX 10 // Microcontroller RX
  #define FONA_RX 11 // Microcontroller TX
  //#define T_ALERT 12 // Connect with solder jumper
#endif

// define LEDs
#define swt12 2
#define led 3
#define swt5 4

// We default to using software serial. If you want to use hardware serial
// (because softserial isnt supported) comment out the following three lines 
// and uncomment the HardwareSerial line
#include <SoftwareSerial.h>
SoftwareSerial fonaSS = SoftwareSerial(FONA_TX, FONA_RX);

// Altsoft Serial communication for the RF module becaouse we can't use 2 software serial on arduino nano
#include <AltSoftSerial.h>

// AltSoftSerial always uses these pins:
//
// Board          Transmit  Receive   PWM Unusable
// -----          --------  -------   ------------
// Arduino Uno        9         8         10
// Arduino Leonardo   5        13       (none)
// Arduino Mega      46        48       44, 45

AltSoftSerial RF_module;

// Use the following line for ESP8266 instead of the line above (comment out the one above)
//SoftwareSerial fonaSS = SoftwareSerial(FONA_TX, FONA_RX, false, 256); // TX, RX, inverted logic, buffer size
SoftwareSerial *fonaSerial = &fonaSS;

// Hardware serial is also possible!
//HardwareSerial *fonaSerial = &Serial1;

Adafruit_FONA_LTE fona = Adafruit_FONA_LTE();

char fonaNotificationBuffer[64];  //for notifications from the FONA
//Sending Buffer
char smsBuffer[50];
//reveiving Buffer
char recBuffer[250];

char callerIDbuffer[32];  //we'll store the SMS sender number in here
char myNumber[32] = "+201091944905";  //My Number
uint8_t slot;            //this will be the slot number of the SMS
// varibles
byte storage[30];
byte i = 0;
uint8_t send_count = 0;   //Sending message counter
static unsigned long lastTime = 0;
static const unsigned int REFRESH_INTERVAL = 10000; // ms
static const unsigned int waitingTime = 10000; // ms
uint8_t history[10] = {0,0,0,0,0,0,0,0,0,0};

void read_SMS(uint8_t smsn);
void sendText(const char* textMessage);
bool receiveRF();


void setup() {
//  while (!Serial);
  pinMode(FONA_RST, OUTPUT);
  digitalWrite(FONA_RST, HIGH); // Default state

  // LEDs 
  pinMode(led, OUTPUT);
  pinMode(swt12, OUTPUT);
  pinMode(swt5, OUTPUT);
  
  // Turn on the module by pulsing PWRKEY low for a little bit
  // This amount of time depends on the specific module that's used
  fona.powerOn(FONA_PWRKEY);

  Serial.begin(115200);
  Serial.println(F("Roconts Gateway Module"));

  // RF Module Start
  RF_module.begin(9600);

  // SIM7000 takes about 3s to turn on and SIM7500 takes about 15s
  // Press Arduino reset button if the module is still turning on and the board doesn't find it.
  // When the module is on it should communicate right after pressing reset

  // Software serial:
  fonaSS.begin(115200); // Default SIM7000 shield baud rate

  Serial.println(F("Configuring to 9600 baud"));
  fonaSS.println("AT+IPR=9600"); // Set baud rate
  delay(100); // Short pause to let the command run
  fonaSS.begin(9600);
  if (! fona.begin(fonaSS)) {
    Serial.println(F("Couldn't find FONA"));
    //while (1); // Don't proceed if it couldn't find the device
    while(true){
      digitalWrite(led, HIGH);
      delay(300);
      digitalWrite(led, LOW);
      delay(300);
    }
  }

  // Set modem to full functionality
  // AT+CFUN=1
  if(fona.setFunctionality(1)){
    Serial.println("GSM Enabled");
    
  }else{
    Serial.print("Please restart");
    while(true){
      digitalWrite(led, HIGH);
      delay(300);
      digitalWrite(led, LOW);
      delay(300);
    }
  }

  /*
  // Other examples of some things you can set:
  fona.setPreferredMode(38); // Use LTE only, not 2G
  fona.setPreferredLTEMode(1); // Use LTE CAT-M only, not NB-IoT
  fona.setOperatingBand("CAT-M", 12); // AT&T uses band 12
//  fona.setOperatingBand("CAT-M", 13); // Verizon uses band 13
  fona.enableRTC(true);
  
  fona.enableSleepMode(true);
  fona.set_eDRX(1, 4, "0010");
  fona.enablePSM(true);

  // Set the network status LED blinking pattern while connected to a network (see AT+SLEDS command)
  fona.setNetLED(true, 2, 64, 3000); // on/off, mode, timer_on, timer_off
  fona.setNetLED(false); // Disable network status LED
  */
  // Get connection type, cellular band, carrier name, etc.
  fona.getNetworkInfo();
  // Delete all stored messages before starting
  deleteAllMsgs();

// Everything is good 
  digitalWrite(led, HIGH);
  delay(2000);
  digitalWrite(led, LOW);
  delay(100);
  digitalWrite(led, HIGH);
  delay(2000);
  digitalWrite(led, LOW);
}


void loop(){
  // read an SMS
  char* bufPtr = fonaNotificationBuffer;    //handy buffer pointer

  if (fona.available())      //any data available from the FONA?
  {
    int charCount = 0;
    //Read the notification into fonaInBuffer
    do  {
      *bufPtr = fona.read();
      Serial.write(*bufPtr);
      delay(1);
    } while ((*bufPtr++ != '\n') && (fona.available()) && (++charCount < (sizeof(fonaNotificationBuffer)-1)));
    
    //Add a terminal NULL to the notification string
    *bufPtr = 0;

    //Scan the notification string for an SMS received notification.
    //  If it's an SMS message, we'll get the slot number in 'slot'
    if (1 == sscanf(fonaNotificationBuffer, "+CMTI: " FONA_PREF_SMS_STORAGE ",%d", &slot)) {
      Serial.print("slot: "); Serial.println(slot);
//      delay(1000);
//      read_SMS(slot);      
//
//      send_all();
//      if(smsnum - i){
//        read_SMS(smsnum);
//        slot = smsnum ;
//        deleteAllMsgs();
//      }
    }
  }
  if(millis() - lastTime >= REFRESH_INTERVAL){
    // Check if we excuted all commands
    int8_t smsnum = fona.getNumSMS();
    if(i < smsnum){
      for( ; i<smsnum; ){
        read_SMS(i);
        smsnum = fona.getNumSMS();
      }
      send_all();
      if (i > 10){
        deleteAllMsgs();
        i = 0;
      }
    }
    lastTime = millis();
  }
}

// Reading SMS 
void read_SMS(uint8_t smsn){
  String command = "";
  // Retrieve SMS sender address/phone number.
  if (! fona.getSMSSender(smsn, callerIDbuffer, 31)) {
    Serial.println("Failed!");
  }
  Serial.print(F("FROM: ")); Serial.println(callerIDbuffer);
  
  // Retrieve SMS value.
  uint16_t smslen;
  if (! fona.readSMS(smsn, recBuffer, 250, &smslen)) { // pass in buffer and max len!
    Serial.println("Failed!");
  }
  Serial.println(recBuffer);
  // Check for the command 
  command = recBuffer;
  command.trim();
  command.toLowerCase();
  // Take and action with the command
  action(command);
  // increment the counter
  i++;
  // Check if we excuted all commands
  int8_t smsnum = fona.getNumSMS();
  Serial.println(i);
  for( ; i<smsnum;){
    read_SMS(i);
    smsnum = fona.getNumSMS();
  }
}

/// Take an action for the sent message
void action(String command){
  uint8_t store = 0;
   if (command == "open switch") {
    // open blue led
    digitalWrite(led, HIGH);
    digitalWrite(swt5, HIGH);
    digitalWrite(swt12, HIGH);
    // send feedback
    store = 50;

  }else if (command == "close switch") {
    // close blue led
    digitalWrite(led, LOW);
    digitalWrite(swt12, LOW);
    digitalWrite(swt5, LOW);
    // send feedback
    store = 40;
  
  }else if (command == "open switch 1") {
    // send RF signal to open the blue led.
    RF_module.write("S\n");     // .print("S\n"); will be better if problem happened later
    // receive feedback
    if(receiveRF()){
      store = 51;
    }else{
      store = 10;
    }
  }else if (command == "close switch 1") {
    // Send RF signal to close the blue led.
    RF_module.write("s\n");
    // receive feedback
    if(receiveRF()){
      store = 41;
    }else{
      store = 10;
    }

  }else if (command == "open switch 2") {
    // send RF signal to open the blue led.
    RF_module.write("S2\n");
    // receive feedback
    if(receiveRF()){
      store = 52;
    }else{
      store = 10;
    }

  }else if (command == "close switch 2") {
    // Send RF signal to close the blue led.
    RF_module.write("s2\n");
    // receive feedback
    if(receiveRF()){
      store = 42;
    }else{
      store = 10;
    }

  }else{
    // responed with wrong command
    Serial.println("Wrong command");
    store = 0;
  }
  Serial.println(store);
  history[i] = store;
}

/// Send response for all messages at one time
void send_all(){
  String text = "";
  for(; send_count<i; send_count++){
    switch (history[send_count]){
    case 0:
      text = "Wrong command";
      break;
    case 50:
      text = "Switch opened";
      break;
    case 40:
      text = "Switch closed";
      break;
    case 51:
      text = "Switch 1 opened";
      break;
    case 41:
      text = "Switch 1 closed";
      break;
    case 52:
      text = "Switch 2 opened";
      break;
    case 42:
      text = "Switch 2 closed";
      break;
    case 10:
      text = "Couldn't reach.\nPlease try again.";  
    }
    text.toCharArray(smsBuffer, 250);
    sendText(smsBuffer);
  }
}

// Receive RF Feedback
bool receiveRF(){
  int res;
  unsigned long last = millis();
  while(RF_module.available() == 0){
    // wait till feedback received
    if(millis() - last >= waitingTime){
      break;
    }
  }
  res = RF_module.read();
  Serial.println(res);
  if(res == 45 || res == 40){
    return true;
  }else{
    return false;
  }
}

// Send an SMS
void sendText(const char* textMessage) {
  Serial.println("Sending reponse...");
  
  if (!fona.sendSMS(callerIDbuffer, textMessage)) {
    Serial.println(F("Failed"));
  } else {
    Serial.println(F("Sent!"));
  }
}

// Delete all saved messages to free space
void deleteAllMsgs(){
  for(int i=0; i<30; i++){
    delete_msg(i);
  }
}
/// delete one messae code
void delete_msg(uint8_t i){
  if (fona.deleteSMS(i)) {
      Serial.println(F("OK!"));
    } else {
      Serial.print(F("Couldn't delete SMS in slot ")); Serial.println(i);
      fona.print(F("AT+CMGD=?\r\n"));
      while(true){
        digitalWrite(led, HIGH);
        delay(300);
        digitalWrite(led, LOW);
        delay(300);
      }
    }
}
