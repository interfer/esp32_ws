#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
// HTML 
#include "index.h"
// HTML ENDS

//TIMER MACRO
#define SLEEP_IN(sec) if(ticks == sec) ledOff();

hw_timer_t * timer = NULL;
volatile long ticks = 0;

void IRAM_ATTR onTimerHandler() {
    //This code will be executed every 1000000 ticks, 1sec
    ticks++;
}
uint8_t get_minutes() {
  return ticks/60;
}
void print_seconds() {
  Serial.println(String(ticks) + "- seconds have passed");
}
void init_timer() {
    // Prescaler 80 is for 80MHz clock. One tick of the timer is 1us
    timer = timerBegin(0, 80, true); 

    //Set the handler for the timer
    timerAttachInterrupt(timer, &onTimerHandler, true); 

    // How often handler should be triggered? 1000 means every 1000 ticks, 1ms
    timerAlarmWrite(timer, 1000000, true); 

    //And enable the timer
    timerAlarmEnable(timer); 
}

// Replace with your network credentials
const char* ssid = "Microwave Gorenje";
const char* password = "88888888";

bool ledState = 0;
const int ledPin = 2;
bool messageSent = 0;

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

void notifyClients() {
  ws.textAll(String(ledState));
}

void handleWebSocketMessage(void *arg, uint8_t *data, size_t len) {
  AwsFrameInfo *info = (AwsFrameInfo*)arg;
  if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
    data[len] = 0;
    if (strcmp((char*)data, "toggle") == 0) {
      ledState = !ledState;
      digitalWrite(ledPin, ledState);
      notifyClients();
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

String processor(const String& var){
  Serial.println(var);
  if(var == "STATE"){
    if (ledState){
      return "ON";
    }
    else{
      return "OFF";
    }
  }
  return String();
}

void setup(){
  // Serial port for debugging purposes
  Serial.begin(115200);

  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, LOW);

  init_timer();

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to " + String(ssid) + "...");
  }

  // Print ESP Local IP Address
  Serial.println(WiFi.localIP());

  initWebSocket();

  // Route for root / web page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html, processor);
  });

  // Start server
  server.begin();
}

void ledOff() {
    if (!messageSent) {
    Serial.println("time to sleep");
      ws.textAll("OFF");
      digitalWrite(ledPin, false);
    }
    messageSent = true;
}

void loop() {
  ws.cleanupClients();
  SLEEP_IN(25);
}