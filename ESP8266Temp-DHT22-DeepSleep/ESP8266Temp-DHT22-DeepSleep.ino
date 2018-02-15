#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <dhtESP.h>
#include <Wire.h>
#include <ACROBOTIC_SSD1306.h>

dhtESP DHT;
#define DHT22_PIN 2

const char* ssid = "SSID";
const char* password = "WLAN-PW";

const char* host = "api.thingspeak.com";
const char* apiKey = "TS-API-KEY";
const unsigned long updateInt = 30; // in Sekunden

void setup(void) {
  Serial.begin(115200);

  Serial.println("Init OLED");

  Wire.begin(1,3);  // RX=SCL=3, TX=SDA=1
  oled.init();                      // Initialze SSD1306 OLED display
  oled.clearDisplay();              // Clear screen
  oled.setTextXY(0,0);              // Set cursor position, start of line 0

  WiFi.begin(ssid, password);
  Serial.println("Connecting:");
  oled.putString("Connecting:");

  // Wait for connection
 int curY=0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    oled.setTextXY(1,curY++);
    oled.putString(".");
    
  }
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  oled.setTextXY(0,0);
  
  String localIP = "IP: ";
  // localIP += WiFi.localIP();
  oled.putString(localIP);

}

bool getTemp(void) {
  int chk = DHT.read22(DHT22_PIN);
  delay(2000);
  // if (chk != DHTLIB_OK) {Serial.print(chk); Serial.println(" getTemp failed"); return false;}
  return true;
}

void updateExternalServer(void) {
  if (!getTemp()) return;
    
  oled.setTextXY(5,0);
  String temperature = "T: ";
  temperature += DHT.temperature;
  temperature += "°C";
  oled.putString(temperature);
  
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
  Serial.println("closing connection // Deep Sleep");

  ESP.deepSleep(updateInt * 1000000, WAKE_RF_DEFAULT);
  delay(100);
  
}


void loop(void) {
  updateExternalServer();
}
