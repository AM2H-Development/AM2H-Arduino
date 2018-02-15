#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>

#define LED_PIN 0  // 3 für Nano
#include <SPI.h>
#include "RF24.h"

const char* ssid = "SSID";
const char* password = "WLAN-PW";
ESP8266WebServer server(80);

const char* host = "api.thingspeak.com";
const char* apiKey0 = "TS-API-KEY1";
const char* apiKey1 = "TS-API-KEY2";

void handleRoot() {
  String message = "hello from esp8266!\n\n";

}

void handleNotFound() {
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
}


/* Hardware configuration: Set up nRF24L01 radio on SPI bus plus pins 7 & 8 */
RF24 radio(4, 15);   // 9,10 für ArduinoNano
/**********************************************************/

byte addresses[][6] = {"1Node", "2Node"};

struct NRFpayload {
  int16_t temperature;
  int16_t humidity;
  uint8_t id;
} ;


void setup() {
  Serial.begin(115200);

  WiFi.begin(ssid, password);
  Serial.println("");

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  server.on("/", handleRoot);

  server.on("/inline", []() {
    server.send(200, "text/plain", "this works as well");
  });

  server.onNotFound(handleNotFound);

  server.begin();
  Serial.println("HTTP server started");


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
  radio.setRetries(15, 15);

  // Open a writing and reading pipe on each radio, with opposite addresses
  radio.openReadingPipe(1, addresses[0]);
  radio.openReadingPipe(2, addresses[1]);
  radio.startListening();
}

void updateExternalServer(const char *apiKey, double temperature, double humidity) {
  // Use WiFiClient class to create TCP connections
  WiFiClient client;
  const int httpPort = 80;
  if (!client.connect(host, httpPort)) {
    Serial.println("connection failed");
    return;
  }

  // We now create a URI for the request
  String url = "/update/";
  url += "?api_key=";
  url += apiKey;
  url += "&field1=";
  url += temperature;
  url += "&field2=";
  url += humidity;

  Serial.print("Requesting URL: ");
  Serial.println(url);

  // This will send the request to the server
  client.print(String("GET ") + url + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" +
               "Connection: close\r\n\r\n");
  delay(10);

  // Read all the lines of the reply from server and print them to Serial
  while (client.available()) {
    String line = client.readStringUntil('\r');
    Serial.print(line);
  }

  Serial.println();
  Serial.println("closing connection");
}


void loop() {


  /****************** Ping Out Role ***************************/
  if ( radio.available() ) {
    NRFpayload payload;                                 // Grab the response, compare, and send to debugging spew

    Serial.print( "sizeof(payload):");
    Serial.print( sizeof(payload), HEX);
    delay(1000);

    radio.read( &payload, sizeof(payload) );
    Serial.print(F("Got response "));
    Serial.print(payload.id);
    Serial.print(": ");
    Serial.print(payload.temperature);
    Serial.print(" / ");
    Serial.println(payload.humidity);

    double temperature = payload.temperature;
    temperature /= 10;

    double humidity = payload.humidity;
    humidity /= 10;

    if (payload.id == 0) {
      updateExternalServer(apiKey0, temperature, humidity);
    }

    if (payload.id == 1) {
      updateExternalServer(apiKey1, temperature, humidity);
    }


  }

} // Loop

