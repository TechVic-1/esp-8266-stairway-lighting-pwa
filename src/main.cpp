#include "config.h"
#include "led_functions.h"
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <LittleFS.h>
#include <NeoPixelBus.h>
#include <WebSocketsServer.h>

using namespace globalconstants;

String WIFI_SSID = "Tyko-Net 2.4GHz";
String WIFI_PASS = "#gjnwb55439#";
#define WIFI_HOSTNAME "Stairway-Led"

// function prototypes for HTTP handlers
void handleNotFound(AsyncWebServerRequest *request);
void serverRouting();
void webSocketEvent(uint8_t num, WStype_t type, uint8_t *payload,
                    size_t length);

// LED, server instances and global variables
NeoPixelBus<NeoGrbFeature, Neo800KbpsMethod> leds(NUM_LEDS, DATA_PIN);
AsyncWebServer server(80);
WebSocketsServer webSocket(81);
LedFunctions led_fcn(&leds);
static int Fkt = 0;
static bool On = false;

void connectWifi() {
  if (WiFi.status() == WL_CONNECTED) return;
  //Manual Wifi
  Serial.print("Connecting to ");
  Serial.print(WIFI_SSID);
  Serial.println(" ...");
  WiFi.disconnect();
  WiFi.mode(WIFI_STA);
  WiFi.hostname(WIFI_HOSTNAME);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  int i = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    if (i > 80) i = 0;
    Serial.print(++i);
    Serial.print(' ');
  }
  Serial.println('\n');
  Serial.println("Connection established!");
  Serial.print("IP address:\t"); 
  Serial.println(WiFi.localIP()); //Get ip and subnet mask
  Serial.print("MAC address:\t"),
  Serial.println(WiFi.macAddress());  //Get the local mac address
}

void setup() {
  // WIFI and server setup
  Serial.begin(115200);
  delay(10);
  Serial.println('\n');

  //WiFi.setSleepMode(WIFI_NONE_SLEEP);

  connectWifi();

  serverRouting();

  webSocket.begin();
  webSocket.onEvent(webSocketEvent);

  server.begin();
  Serial.println("HTTP server started");

  // file system
  if (!LittleFS.begin()) {
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }

  // NeoPixel setup
  leds.Begin();
  leds.Show();

  // Ultrasonic sensors setup
  pinMode(ULTRASONIC_TOP, OUTPUT);
  pinMode(ULTRASONIC_BOTTOM, INPUT);
  digitalWrite(ULTRASONIC_TOP, LOW);
  digitalWrite(ULTRASONIC_BOTTOM, LOW);
}

void loop() {
  webSocket.loop();
  if (On) {
    switch (Fkt) {
    case 0:
      led_fcn.sensingmode();
      break;
    case 1:
      led_fcn.resetState();
      led_fcn.steadymode();
      break;
    case 2:
      led_fcn.resetState();
      led_fcn.nightmode();
      break;
    case 3:
      led_fcn.resetState();
      led_fcn.rainbowmode();
      break;
    case 4:
      led_fcn.resetState();
      led_fcn.rainbowmoderunning();
      break;
    case 5:
      led_fcn.resetState();
      led_fcn.sparkle();
      break;
    case 6:
      led_fcn.resetState();
      led_fcn.twinkle();
      break;
    case 7:
      led_fcn.resetState();
      led_fcn.fire();
      break;
    case 8:
      led_fcn.resetState();
      led_fcn.turnOffLight();
      break;
    default:
      led_fcn.resetState();
      led_fcn.turnOffLight();
      break;
    }
  }
  led_fcn.resetState();
  led_fcn.turnOffLight();
}

// server handler and helper functions
String getContentType(String filename) {
  if (filename.endsWith(".htm"))
    return "text/html";
  else if (filename.endsWith(".html"))
    return "text/html";
  else if (filename.endsWith(".css"))
    return "text/css";
  else if (filename.endsWith(".js"))
    return "application/javascript";
  else if (filename.endsWith(".png"))
    return "image/png";
  else if (filename.endsWith(".gif"))
    return "image/gif";
  else if (filename.endsWith(".jpg"))
    return "image/jpeg";
  else if (filename.endsWith(".ico"))
    return "image/x-icon";
  else if (filename.endsWith(".xml"))
    return "text/xml";
  else if (filename.endsWith(".pdf"))
    return "application/x-pdf";
  else if (filename.endsWith(".zip"))
    return "application/x-zip";
  else if (filename.endsWith(".gz"))
    return "application/x-gzip";
  return "text/plain";
}

bool handleFileRead(AsyncWebServerRequest *request, String path) {
  Serial.print(F("handleFileRead: "));
  Serial.println(path);

  if (path.endsWith("/"))
    path += F("index.html");
  String contentType = getContentType(path);
  String pathWithGz = path + F(".gz");
  if (LittleFS.exists(pathWithGz) || LittleFS.exists(path)) {
    bool gzipped = false;

    if (LittleFS.exists(pathWithGz)) {
      gzipped = true;
    }
    AsyncWebServerResponse *response =
        request->beginResponse(LittleFS, path, contentType);
    if (gzipped) {
      response->addHeader("Content-Encoding", "gzip");
    }
    Serial.print("Real file path: ");
    Serial.println(path);

    request->send(response);

    return true;
  }
  Serial.println(String(F("\tFile Not Found: ")) + path);
  return false;
}

void handleNotFound(AsyncWebServerRequest *request) {
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += request->url();
  message += "\nMethod: ";
  message += (request->method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += request->args();
  message += "\n";

  for (uint8_t i = 0; i < request->args(); i++) {
    message += " " + request->argName(i) + ": " + request->arg(i) + "\n";
  }

  request->send(200, "text/plain", message);
}

void serverRouting() {
  server.onNotFound([](AsyncWebServerRequest *request) {
    Serial.println(F("On not found\n"));
    if (!handleFileRead(request, request->url())) {
      handleNotFound(request);
    }
  });
}

void webSocketEvent(uint8_t num, WStype_t type, uint8_t *payload,
                    size_t length) {
  switch (type) {
  case WStype_DISCONNECTED:
    Serial.printf("Disconnected!");
    break;
  case WStype_CONNECTED:
    Serial.printf("Connected!");
    break;
  case WStype_TEXT:
    Serial.printf("[%u] get Text: %s\n", num, payload);
    if (payload[0] == 'O') {
      if (length == 2) {
        On = true;
      } else if (length == 3) {
        On = false;
      }
    } else if (payload[0] == 'E') {
      String velocity = "";
      for (int i = 1; i < length; i++) {
        velocity.concat(payload[i]);
      }
      led_fcn.setVelocity(velocity.toInt());
    } else if (payload[0] == '#') {
      Fkt =
          payload[1] - 48; // converting the char number to the same int number
    } else if (payload[0] == 'H') {
      String hue = "";
      for (int i = 1; i < length; i++) {
        hue.concat(payload[i]);
      }
      led_fcn.setHue(hue.toFloat());
    } else if (payload[0] == 'S') {
      String sat = "";
      for (int i = 1; i < length; i++) {
        sat.concat(payload[i]);
      }
      led_fcn.setSat(sat.toFloat());
    } else if (payload[0] == 'V') {
      String val = "";
      for (int i = 1; i < length; i++) {
        val.concat(payload[i]);
      }
      led_fcn.setVal(val.toFloat());
    }
    break;
  }
}