/*--------------------------- Erfassung Gasverbrauch --------------------------------*/
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>

#define IMPULSE_ISR_PIN 2                        // Interrupt auf GPIO 2

const char* ssid = "SSID";
const char* password = "WLAN-PW";

const char* host = "api.thingspeak.com";
const char* apiWriteKey = "TS-API-KEY";

const char* mqttServerIp = "192.168.178.37";
const char* stateTopic = "mqtt/devices/gas1/state";
const char* counterTopic = "mqtt/location/gasmeter/state/counter";
const char* powerTopic = "mqtt/location/gasmeter/state/power";

unsigned long lastImpulseMillis;
unsigned long timespan;
boolean dataAvailable = false;
boolean dataSent = true;

unsigned long counter = 0; // 0,1m³
unsigned long power = 0; // in W
unsigned long powerMillis = 0; // timestamp

WiFiClient mqttWifiClient;
PubSubClient mqttClient(mqttWifiClient);
long lastMsg = 0;
char msg[50];
int value = 0;

ESP8266WebServer server(80);

String status;

void handleRoot() {
  server.send(200, "text/plain", status );
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

void setup()
{
  Serial.begin(115200);
  attachInterrupt(IMPULSE_ISR_PIN, impulseISR, FALLING);

  WiFi.mode(WIFI_AP);
  WiFi.begin(ssid, password);

  status = String("WIFI Connecting:");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    status += ".";
  }

  status += "\r\n";
  server.on("/", handleRoot);
  server.onNotFound(handleNotFound);
  server.begin();

  status += "Connected to ";
  status += ssid;
  status += " IP address: ";
  status += WiFi.localIP();
  status += "\r\n";

  mqttClient.setServer(mqttServerIp, 1883);
  mqttClient.setCallback(mqttCallback);
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  String s;
  status += "Message arrived [";
  status += topic;
  status += "] ";
  for (int i = 0; i < length; i++) {
    status += (char)payload[i];
    s += (char)payload[i];
  }
  status += " #\r\n";

  counter = s.toInt();
}

void mqttReconnect() {
  // Loop until we're reconnected
  while (!mqttClient.connected()) {
    status += "Attempting MQTT connection...";
    // Attempt to connect
    // boolean connect(const char* id, const char* willTopic, uint8_t willQos, boolean willRetain, const char* willMessage);
    if (mqttClient.connect("gas1", stateTopic, 2, true, "offline")) {
      status += "connected\r\n";
      mqttClient.publish(stateTopic, "online");
      mqttClient.subscribe(counterTopic);
    } else {
      status += "failed, rc=";
      status += mqttClient.state();
      status += " try again in 1 second\r\n";
      // Wait 1 second before retrying
      delay(1000);
    }
  }
}

void calcPower() {
  if (powerMillis == 0) {
    powerMillis = millis();
    return;
  }
  // 1 Impuls = 0,1 m³  => 0,1 m³=1,07311 kWh => 0,1 m³ = 1073 Wh = 3863196 Ws
  // 0,1 m³ in 8min (=480s) (=0,133h) => 3863196 / 480 = 8048 W ##
  power = 3863196 / ((millis() - powerMillis) / 1000);
  powerMillis = millis();
}

void impulseISR()                                    // Interrupt service routine
{
  if ((millis() - lastImpulseMillis >= 50) && dataSent)            // 50ms Entprellzeit
  {
    lastImpulseMillis = millis();                    // Zeit merken
    counter++;
    dataAvailable = true;                            // Daten zum Senden vorhanden
    dataSent = false;
  }
}

void sendToTS()
{
  WiFiClient client;
  const int httpPort = 80;
  if (!client.connect(host, httpPort)) {
    status += "HTTP connection to TS failed\r\n";
    return;
  }

  // We now create a URI for the request
  String url = "/update/";
  url += "?api_key=";
  url += apiWriteKey;
  url += "&field1=";
  url += counter;
  url += "&field2=";
  url += power;
  status += "Requesting URL: ";
  status += url;
  status += "\r\n";

  // This will send the request to the server
  client.print(String("GET ") + url + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" +
               "Connection: close\r\n\r\n");
  status += "closing connection\r\n";
  status += "Timpestamps: ";
  status += lastImpulseMillis;
  status += " ";
  status += millis();
  status += "\r\n";
}

void sendToMqtt() {
  if (mqttClient.connected()) {
    char buf [10];
    sprintf (buf, "%u", counter);
    mqttClient.publish(counterTopic, buf, true );
    sprintf (buf, "%u", power);
    mqttClient.publish(powerTopic, buf );
  }
}

void mqtt() {
  if (!mqttClient.connected()) {
    mqttReconnect();
  }
  mqttClient.loop();
}

void isr() {
  if (dataAvailable) {
    timespan = millis() - lastImpulseMillis;
    if ( timespan >= 50 ) { // wait to debounce;
      if ( digitalRead(IMPULSE_ISR_PIN) == HIGH ) { // wait for switch to open or 0,5s
        if (timespan >= 500) {
          calcPower();
          sendToTS();
          sendToMqtt();
        }
        dataSent = true;
        dataAvailable = false;
      }
    }
  }
}

void checkZeroPower() {
  if ( (power > 0) && ((millis() - lastImpulseMillis) >= 32 * 60 * 1000) ) {
    power = 0;
    sendToTS();
    sendToMqtt();
  }
}

void loop()
{
  server.handleClient();
  mqtt();
  isr();
  checkZeroPower();
  if (status.length() > 2000) status = status.substring(500);
}
