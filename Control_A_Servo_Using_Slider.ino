#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include "LittleFS.h"
#include <Arduino_JSON.h>
#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>

const char* ssid = "DNS";
const char* password = "01234567";

Adafruit_PWMServoDriver board1 = Adafruit_PWMServoDriver();

String message = "";
String sliderValue1 = "0";


int dutyCycle1;

JSONVar sliderValues;

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

String getSliderValues(){
  sliderValues["sliderValue1"] = String(sliderValue1);
  String jsonString = JSON.stringify(sliderValues);
  return jsonString;
}
void handleWebSocketMessage(void *arg, uint8_t *data, size_t len) {
  AwsFrameInfo *info = (AwsFrameInfo*)arg;
  if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
    data[len] = 0;
    message = (char*)data;
    if (message.indexOf("1s") >= 0) {
      sliderValue1 = message.substring(2);
      dutyCycle1 = sliderValue1.toInt();
      Serial.println(getSliderValues());
    }
  }
}
void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type,
             void *arg, uint8_t *data, size_t len) {
  switch (type) {
    case WS_EVT_CONNECT:
      Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
      break;
    case WS_EVT_DISCONNECT:
      Serial.printf("WebSocket client #%u disconnected\n", client->id());
      break;
    case WS_EVT_DATA:
      handleWebSocketMessage(arg, data, len);
      break;
    case WS_EVT_PONG:
    case WS_EVT_ERROR:
      break;
  }
}
void initWebSocket() {
  ws.onEvent(onEvent);
  server.addHandler(&ws);
}
void initFS() {
  if (!LittleFS.begin()) {
    Serial.println("An error has occurred while mounting LittleFS");
  }
  else{
   Serial.println("LittleFS mounted successfully");
  }
}
void initWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi ..");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    delay(1000);
  }
  Serial.println(WiFi.localIP());
}
int angleToPulse (int ang) {
  Serial.println(ang);
  int pulse = map(ang, 0, 180, 125, 625);
  Serial.println(pulse);
  return pulse;
}
void setup(){
  Serial.begin(115200);
  Wire.begin();
  board1.begin();
  board1.setPWMFreq(60);
  initFS();
  initWiFi();
  initWebSocket();
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(LittleFS, "/index.html", "text/html");
  });
  server.serveStatic("/", LittleFS, "/");
  server.begin();

  board1.setPWM(0, 0, 125);

}
void loop() {
  ws.cleanupClients();
  board1.setPWM(0, 0, angleToPulse(dutyCycle1));
  delay(100);
}