#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <dhtESP.h>

dhtESP DHT;
#define DHT22_PIN 2

const char* ssid = "SSID";
const char* password = "WLAN-PW";
MDNSResponder mdns;
ESP8266WebServer server(80);

const char* host = "api.thingspeak.com";
const char* apiKey = "TS-API-KEY";
const unsigned long updateInt = 120 * 1000;

unsigned long timestamp;

void handleRoot() {
  String message = "hello from esp8266!\n\n";

  server.send(200, "text/plain", message + DHT.temperature );
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

void setup(void) {
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

  if (mdns.begin("dht22temp2", WiFi.localIP())) {
    Serial.println("MDNS responder started");
  }

  server.on("/", handleRoot);

  server.on("/inline", []() {
    server.send(200, "text/plain", "this works as well");
  });

  server.onNotFound(handleNotFound);

  server.begin();
  Serial.println("HTTP server started");
  timestamp = millis();
}

bool getTemp(void) {
  int chk = DHT.read22(DHT22_PIN);
  delay(2000);
  if (chk != DHTLIB_OK) {Serial.print(chk); Serial.println(" getTemp failed"); return false;}
  return true;
}

void updateExternalServer(void) {
  if ( timestamp+updateInt > millis() ) return;
  if (!getTemp()) return;
  
  timestamp=millis();
  
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
  url += DHT.temperature;
  url += "&field2=";
  url += DHT.humidity;

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


void loop(void) {
  server.handleClient();
  updateExternalServer();
}
