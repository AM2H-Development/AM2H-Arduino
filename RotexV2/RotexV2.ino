#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>

const char* ssid = "SSID";
const char* password = "WLAN-PW";

const char* host = "api.thingspeak.com";
const char* apiWriteKey = "TS-API-KEY";

const byte k01[] = {0x60, 0x20, 0x10, 0x21, 0x01};
const byte k03[] = {0x41, 0x20, 0x09, 0x21, 0x03};
const byte k04[] = {0x41, 0x20, 0x09, 0x21, 0x04};
const byte k05[] = {0x41, 0x20, 0x09, 0x21, 0x05};
const byte k5f[] = {0x5f, 0x5f, 0x5f};
boolean skipFirst = true;

byte pointerK01 = 0;
byte pointerK03 = 0;
byte pointerK04 = 0;
byte pointerK05 = 0;
byte pointerK5f = 0;

byte st; // Betriebsart
byte sw; // Sollwert Temperatur
byte vl; // Vorlauftemperatur
byte rl; // Rücklauftemperatur;
byte sp; // Speichertemperatur;
int8_t at; // Außentemperatur
int luftr; // Lüfterdrehzahl
byte bs; //Betriebsstatus
byte up; // Umwälzpumpe

// int tVlSoll, tVlIst, tRlIst, tSpeicher, tAussen, betriebsart, luefterdrehzahl, betriebsstatus;

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

  Serial.begin(9600, SERIAL_8N2);

  WiFi.mode(WIFI_AP);
  WiFi.begin(ssid, password);
  status = String("Connecting:");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    status += ".";
  }
  status += "\r\n";
  server.on("/", handleRoot);
  server.onNotFound(handleNotFound);
  server.begin();

}

void parseK01() {
  byte b[17];

  if (Serial.readBytes(b, 17) == 17) {
    for (int i = 0; i < 17 ; i++) {
      status += String(b[i], DEC);
      status += " ";
    }

    byte crc[sizeof(b) + 3];
    for (int i = 0; i < 5; i++) {
      crc[i] = k01[i];
    }
    for (int i = 5; i < sizeof(crc); i++) {
      crc[i] = b[i - 5];
    }
    unsigned int crc1 = crc16k(crc, sizeof(crc));
    unsigned int crc2 = (b[sizeof(b)-2] << 8) + b[sizeof(b)-1];

    status += String(crc1, HEX);
    status += " ";
    status += String(crc2, HEX);
    status += " ";

    if (crc1==crc2){
      st = b[1];
      sw = b[2];
    }
    status += "\r\n";
  }
}

void parseK03() {
  byte b[10];

  if (Serial.readBytes(b, 10) == 10) {
    for (int i = 0; i < 10; i++) {
      status += String(b[i], DEC);
      status += " ";
    }

    byte crc[sizeof(b) + 3];
    for (int i = 0; i < 5; i++) {
      crc[i] = k03[i];
    }
    for (int i = 5; i < sizeof(crc); i++) {
      crc[i] = b[i - 5];
    }
    unsigned int crc1 = crc16k(crc, sizeof(crc));
    unsigned int crc2 = (b[sizeof(b)-2] << 8) + b[sizeof(b)-1];

    status += String(crc1, HEX);
    status += " ";
    status += String(crc2, HEX);
    status += " ";

    if (crc1==crc2){
      vl = b[0];
      rl = b[1];
      sp = b[2];
      at = b[3];
    }
    status += "\r\n";
  }
}

void parseK04() {
  byte b[10];

  if (Serial.readBytes(b, 10) == 10) {
    for (int i = 0; i < 10; i++) {
      status += String(b[i], DEC);
      status += " ";
    }

    byte crc[sizeof(b) + 3];
    for (int i = 0; i < 5; i++) {
      crc[i] = k04[i];
    }
    for (int i = 5; i < sizeof(crc); i++) {
      crc[i] = b[i - 5];
    }
    unsigned int crc1 = crc16k(crc, sizeof(crc));
    unsigned int crc2 = (b[sizeof(b)-2] << 8) + b[sizeof(b)-1];

    status += String(crc1, HEX);
    status += " ";
    status += String(crc2, HEX);
    status += " ";

    if (crc1==crc2){
      bs = b[6];
      luftr = int(b[4] << 8) + int(b[5]);
    }

    status += "\r\n";
  }
}

void parseK05() {
  byte b[10];

  if (Serial.readBytes(b, 10) == 10) {
    for (int i = 0; i < 10; i++) {
      status += String(b[i], DEC);
      status += " ";
    }
    byte crc[sizeof(b) + 3];
    for (int i = 0; i < 5; i++) {
      crc[i] = k05[i];
    }
    for (int i = 5; i < sizeof(crc); i++) {
      crc[i] = b[i - 5];
    }
    unsigned int crc1 = crc16k(crc, sizeof(crc));
    unsigned int crc2 = (b[sizeof(b)-2] << 8) + b[sizeof(b)-1];

    status += String(crc1, HEX);
    status += " ";
    status += String(crc2, HEX);
    status += " ";

    if (crc1==crc2){
      up = b[5];
    }
    status += "\r\n";
  }
}

void parseK5f() {
  status += String(st, DEC); // Betriebsart
  status += " ";
  status += String(sw, DEC); // Sollwert Temperatur
  status += " ";
  status += String(vl, DEC); // Vorlauftemperatur
  status += " ";
  status += String(rl, DEC); // Rücklauftemperatur
  status += " ";
  status += String(sp, DEC); // Speichertemperatur
  status += " ";
  status += String(at, DEC); // Außentemperatur
  status += " ";
  status += String(luftr, DEC); // Lüfterdrehzahl
  status += " ";
  status += String(bs, DEC); //Betriebsstatus
  status += " ";
  status += String(up, DEC); // Umwälzpumpe
  status += "\r\n";
}

void sendToTS()
{
  WiFiClient client;
  const int httpPort = 80;
  if (!client.connect(host, httpPort)) {
    status += "connection failed";
    return;
  }
 String url = "/update/";
  url += "?api_key=";
  url += apiWriteKey;
  url += "&field1=" + String(sw, DEC);
  url += "&field2=" + String(vl, DEC);
  url += "&field3=" + String(rl, DEC);
  url += "&field4=" + String(sp, DEC);
  url += "&field5=" + String(at, DEC);
  url += "&field6=" + String(st, DEC);
  url += "&field7=" + String(luftr, DEC);
  url += "&field8=" + String(bs, DEC);
  

  status += "Requesting URL: " +url + "\r\n";
  client.print(String("GET ") + url + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" +
               "Connection: close\r\n\r\n");
}

void parseInput(int n, Stream &s)
{
  byte b;
  if (status.length() > 2000) status = "";

  if (s.available() > 0) {
    b = s.read();
  } else {
    return;
  }

  if (b == k01[pointerK01++]) {
    if (pointerK01 == sizeof(k01)) {
      pointerK01 = 0;
      status += " k01### ";
      parseK01();
    }
  } else {
    pointerK01 = 0;
  }

  if (b == k03[pointerK03++]) {
    if (pointerK03 == sizeof(k03)) {
      pointerK03 = 0;
      status += " k03### ";
      parseK03();
    }
  } else {
    pointerK03 = 0;
  }

  if (b == k04[pointerK04++]) {
    if (pointerK04 == sizeof(k04)) {
      pointerK04 = 0;
      status += " k04### ";
      parseK04();
    }
  } else {
    pointerK04 = 0;
  }

  if (b == k05[pointerK05++]) {
    if (pointerK05 == sizeof(k05)) {
      pointerK05 = 0;
      status += " k05### ";
      parseK05();
    }
  } else {
    pointerK05 = 0;
  }

  if (b == k5f[pointerK5f++]) {
    if (pointerK5f == sizeof(k5f)) {
      pointerK5f = 0;
      status += " k5f### ";
      parseK5f();
      if (!skipFirst) sendToTS();
      skipFirst=false;
    }
  } else {
    pointerK5f = 0;
  }
}

unsigned int crc16k(byte b[], int arrSize) {
  unsigned int current_crc_value = 0;
  for (int i = 0; i < arrSize; i++) {
    current_crc_value ^= b[i] & 0xFF;
    for (int j = 0; j < 8; j++) {
      if ((current_crc_value & 1) != 0) {
        current_crc_value = (current_crc_value >> 1) ^ 0x8408;
      } else {
        current_crc_value = current_crc_value >> 1;
      }
    }
  }
  return ((current_crc_value & 0xFF) << 8) + ((current_crc_value & 0xFF00) >> 8);
}

void loop()
{
  server.handleClient();
  parseInput(0, Serial);
}
