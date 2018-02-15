/*
  Getting Started example sketch for nRF24L01+ radios
  This is a very basic example of how to send data from one node to another
  Updated: Dec 2014 by TMRh20
*/

#define LED_PIN 0  // 3 für Nano
#define SENSOR_NUMBER 1
#include <SPI.h>
#include "RF24.h"
#include <dhtESP.h>

dhtESP DHT;
#define DHT22_PIN 4

/* Hardware configuration: Set up nRF24L01 radio on SPI bus plus pins 7 & 8 */
RF24 radio(9, 10);   // 9,10 für ArduinoNano 
/**********************************************************/

byte addresses[][6] = {"1Node", "2Node"};
unsigned long start_time = SENSOR_NUMBER;

unsigned long timestamp;
const unsigned long updateInt = 120000;

struct NRFpayload {
      int16_t temperature;
      int16_t humidity;
      uint8_t id;
    } ;

void setup() {
  
  Serial.begin(115200);
  Serial.println(F("RF24/examples/GettingStarted"));
  Serial.println(F("*** PRESS 'T' to begin transmitting to the other node"));
  Serial.println(sizeof(int16_t));
  Serial.println(sizeof(uint8_t));  
  pinMode(3, OUTPUT);
  radio.begin();

  // Set the PA Level low to prevent power supply related issues since this is a
  // getting_started sketch, and the likelihood of close proximity of the devices. RF24_PA_MAX is default.
  radio.setPALevel(RF24_PA_MAX);
  radio.setDataRate(RF24_250KBPS);
  radio.setRetries(15,15);

  // Open a writing and reading pipe on each radio, with opposite addresses
  radio.openWritingPipe(addresses[SENSOR_NUMBER]);
  radio.powerDown();
  timestamp = millis();
}

bool getTemp(void) {
  int chk = DHT.read22(DHT22_PIN);
  delay(2000);
  if (chk != DHTLIB_OK) {Serial.print(chk); Serial.println(" getTemp failed"); return false;}
  return true;
}


void loop() {
  if ( timestamp+updateInt > millis() ){
    Serial.print(F("Hallo "));
    Serial.print(timestamp+updateInt);    
    Serial.print(F("  "));
    Serial.print(timestamp);    
    Serial.print(F("  "));
    Serial.print(updateInt);    
    Serial.print(F("  "));
    Serial.println(millis());    
    delay(5000);
    return;
  }
    timestamp=millis();

  if (!getTemp()){
      Serial.println(F("DHT failed"));    
    return;
  }

  /****************** Ping Out Role ***************************/
    radio.powerUp();
    Serial.println(F("Now sending"));

    struct NRFpayload payload ={DHT.temperature*10,DHT.humidity*10, SENSOR_NUMBER};
    
    if (!radio.write( &payload, sizeof(payload) )) {
      Serial.println(F("failed"));
    }
    radio.powerDown();
    Serial.print( "sizeof(payload):");      

    Serial.print( sizeof(payload), HEX);      
    Serial.print( " # ");      

    /*for (byte i=0; i<sizeof(payload);i++){
      byte tmp;
      memcpy (&tmp,&payload+i,1);

      Serial.print( i, HEX);      

      Serial.print( ":");      
      Serial.print( tmp, HEX);      
      Serial.print( " , ");      
    }

      double temperat=DHT.temperature;
     for (byte i=0; i<sizeof(temperat);i++){
      byte tmp;
      memcpy (&tmp,&temperat+i,1);

      Serial.print( i, HEX);      

      Serial.print( ":");      
      Serial.print( tmp, HEX);      
      Serial.print( " , ");      
    }

      temperat=DHT.humidity;
     for (byte i=0; i<sizeof(temperat);i++){
      byte tmp;
      memcpy (&tmp,&temperat+i,1);

      Serial.print( i, HEX);      

      Serial.print( ":");      
      Serial.print( tmp, HEX);      
      Serial.print( " , ");      
    }*/

    Serial.print("ID");    
    Serial.print(payload.id);
    Serial.print(" : ");
    Serial.print(payload.temperature);
    Serial.print(" / ");
    Serial.println(payload.humidity);

} // Loop

