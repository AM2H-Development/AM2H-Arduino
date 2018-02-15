#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <dhtESP.h>
#define DHT22_PIN 2
#define DHT22_2_PIN 0


dhtESP DHT;

// 2. Sensor für Test Einbaugerät
dhtESP DHT2;

const char* ssid = "SSID";
const char* password = "WLAN-PW";

ESP8266WebServer server(80);

const char* host = "api.thingspeak.com";  // Thingspeak Host
const char* apiKey = "TS-API-KEY";  // Write API Key Temperatur4
const unsigned long updateThingspeakInt = 120 * 1000; // Update Intervall in ms
unsigned long timestamp;

// Kalman Filter Variablen
float x_est_last;
float P_last; 
float Q = 0.005;
float R = 0.7;
float K; 
float P; 
float P_temp; 
float x_temp_est; 
float x_est; 
float z_measured; //the 'noisy' value we measured


float kalman(float x){

        z_measured=x;
            //do a prediction 
        x_temp_est = x_est_last; 
        P_temp = P_last + Q; 
        //calculate the Kalman gain 
        K = P_temp * (1.0/(P_temp + R));
        //measure 
        // z_measured = z_real; //  + frand()*0.09; //the real measurement plus noise
        //correct 
        x_est = x_temp_est + K * (z_measured - x_temp_est);  
        P = (1- K) * P_temp; 
        //we have our new system 
         
         
        //update our last's 
        P_last = P; 
        x_est_last = x_est; 
        
        return x_est;

    
}


void setup(void) {
  delay(10000);
  
  pinMode(DHT22_PIN, INPUT);

  pinMode(DHT22_2_PIN, INPUT);     // Nur für 2. Sensor

  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  Serial.println("Starte WiFi Verbindung:");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Verbunden mit ");
  Serial.println(ssid);
  Serial.print("IP-Adresse: ");
  Serial.println(WiFi.localIP());

  server.on("/", handleRoot);
  server.onNotFound(handleNotFound);

  server.begin();
  Serial.println("HTTP Server gestartet");
  timestamp = millis();
}

void loop(void) {
  server.handleClient();
  updateExternalServer();
}

void updateExternalServer(void) {
  if ( timestamp+updateThingspeakInt > millis() ){
    delay (10);
    return;
  }
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

  url += "&field3=";
  url += DHT2.temperature;
  url += "&field4=";
  url += DHT2.humidity;

  Serial.print("Requesting URL: ");
  Serial.println(url);

  // This will send the request to the server
  client.print(String("GET ") + url + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" +
               "Connection: close\r\n\r\n");
  delay(100);

  // Read all the lines of the reply from server and print them to Serial
  while (client.available()) {
    String line = client.readStringUntil('\r');
    Serial.print(line);
  }

  Serial.println();
  Serial.println("closing connection");
}

bool getTemp(void) {
  int chk = DHT.read22(DHT22_PIN);
  delay(2000);
  if (chk != DHTLIB_OK) {Serial.print(chk); Serial.println(" getTemp failed"); return false;}
  return true;
}


void handleRoot() {
  // {"created_at":"2016-03-26T14:14:26Z","entry_id":19709,"field1":"18.80","field2":"50.80"}
  String message = "{\"Temp\":";
  message += DHT.temperature;
  message += ",\"Hum\":";
  message += DHT.humidity;
  message += ",\"Temp2\":";
  message += DHT.temperature;
  message += ",\"Hum2\":";
  message += DHT.humidity;
  message += "}";

  server.send(200, "application/json", message);
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

