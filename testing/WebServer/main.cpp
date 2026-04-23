// A simple server implementation showing how to:
//  * serve static messages
//  * read GET and POST parameters
//  * handle missing pages / 404s
//

#include <Arduino.h>
#include <WiFi.h>
#include <DNSServer.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include <esp-fs-webserver.h>
#include <FS.h>
#include <LittleFS.h>
#include <git_revision.h>

#define FILESYSTEM LittleFS

DNSServer dnsServer;
WebServer server(80);

FSWebServer myWebServer(FILESYSTEM, server);

//DynamicJsonDocument doc(1000);//da evitare in tutti i modi!!!!

bool AP_MODE = false;
String tempSsid = "";
String tempPass = "";
uint32_t newConnectionIstant = 0;
bool newConnection = false;


//Funzioni
String splitString(String str, char sep, int index);

//LittleFS
bool FSStarted = false;\

const byte buttonPin = 0;
#define N_LED 2
const byte ledPins[N_LED] = {2, 27};

////////////////////////////////  Filesystem  /////////////////////////////////////////
void startFilesystem() {
  // FILESYSTEM INIT
  if ( FILESYSTEM.begin()) {
    FSStarted = true;
    File root = FILESYSTEM.open("/", "r");
    File file = root.openNextFile();
    while (file) {
      const char* fileName = file.name();
      size_t fileSize = file.size();
      Serial.printf("FS File: %s, size: %lu\n", fileName, (long unsigned)fileSize);
      file = root.openNextFile();
    }
    Serial.println();
    File configFile;
    if (FILESYSTEM.exists("/app_config.json")) {
      Serial.println("Restoring config");
      configFile = FILESYSTEM.open("/app_config.json", FILE_READ);
    } else {
      Serial.println("Createing config");
      configFile = FILESYSTEM.open("/app_config.json", FILE_WRITE);
    }
    //Apertura del documento
    if (configFile) {
      Serial.println("File opened!");
      StaticJsonDocument<200> doc;
      deserializeJson(doc, configFile);
      Serial.println("\nConfig file: ");
      serializeJson(doc, Serial);
      Serial.println();

      JsonArray leds;
      if (FILESYSTEM.exists("/app_config.json")) {
        if (doc.containsKey("leds")) {
          Serial.println("Led ARR is present!");
          leds = doc["leds"];
          for (JsonObject led : leds) {
            digitalWrite(led["pin"], led["state"]);
          }
        } else {
          Serial.println("Doc not contains leds arr");
        }
      } else {
        doc.clear();
        leds = doc.createNestedArray("leds");
        leds.clear();
        for (auto pin : ledPins) {
          JsonObject led = leds.createNestedObject();
          led["pin"] = pin;
          led["state"] = 0;
        }
        Serial.print("Config file created:");
        serializeJson(doc, configFile);
        serializeJson(doc, Serial);
        Serial.println();
      }
    } else {
      Serial.println("Error opening config file");
    }
    configFile.close();
  }
  else {
    Serial.println("ERROR on mounting filesystem. It will be formatted!");
    FILESYSTEM.format();
    ESP.restart();
  }
}

void handleMainEndpoint() {
  String msg = "";
  StaticJsonDocument<150> doc;
  JsonArray ledArr = doc.createNestedArray("leds");
  for (int pin : ledPins) {
    JsonObject led = ledArr.createNestedObject();
    //Serial.printf("led: {pin: %d, state: %d}\n\n", pin, digitalRead(pin));
    led["pin"] = pin;
    led["state"] = digitalRead(pin);
  }
  serializeJson(doc, msg);
  server.send(200, "application/json", msg);
}

void handleLedStates() {
  Serial.println("LEDs");
  int argCount = server.args();
  String jsonData =  server.arg(0);
  Serial.println(jsonData);
  if (argCount) {
    String jsonData =  server.arg(0);
    StaticJsonDocument<150> doc;
    deserializeJson(doc, jsonData);
    Serial.print("Des document: ");
    serializeJson(doc, Serial); Serial.println();
    if (doc.containsKey("leds")) {
      JsonArray ledArr = doc["leds"];
      for (JsonObject led : ledArr) {
        digitalWrite(led["pin"], led["state"]);
      }
      server.send(200, "text/plain", "OK");
      return;
    }
  }
  server.send(400, "text/plain", "Bad request");
}

void handleSaveConfig() {
  Serial.println("Saving actual config");
  if (FSStarted) {
    File configFile = FILESYSTEM.open("/app_config.json", FILE_WRITE);
    if (configFile) {
      StaticJsonDocument<200> doc;
      deserializeJson(doc, configFile);
      Serial.println("Config file: ");
      serializeJson(doc, Serial);
      Serial.println();
      JsonArray leds;
      if (doc.containsKey("leds")) {
        Serial.println("LED ARR is present");
        leds = doc["leds"];
      } else {
        leds = doc.createNestedArray("leds");
        Serial.println("Creating LED ARR");
      }
      leds.clear();
      for (auto pin : ledPins) {
        JsonObject led = leds.createNestedObject();
        led["pin"] = pin;
        led["state"] = digitalRead(pin);
      }
      serializeJson(doc, configFile);
      Serial.println("Final document: ");
      serializeJson(doc, Serial);
      Serial.println();
    } else {
      Serial.println("Error opening config file");
    }
    configFile.close();
  }
  server.sendHeader("Location", "/");
  server.send(307, "Temporary Redirect", "");
}

void handleWiFiConnect() {
  if (server.hasArg("ssid")) {
    Serial.println("ssid present");
    if (server.hasArg("pass")) {
      Serial.println("pass present");
      tempSsid = server.arg("ssid");
      tempPass = server.arg("pass");
      server.sendHeader("Location", "/WiFiPortal/WiFiPortal.html");
      server.send(307, "Temporary Redirect", "");
      newConnectionIstant = millis();
      newConnection = true;
    } else {
      Serial.println("pass NOT present");
      byte a = server.args();
      Serial.printf("Args count: %d", a);
      for (int i = 0; i < a; ++i) {
        Serial.printf("arg[%d].name: %s,\t arg[%d].value: %s", i, server.argName(i).c_str(), i, server.arg(i).c_str());
      }
    }
  } else {
    Serial.println("ssid NOT present");
    byte a = server.args();
    Serial.printf("Args count: %d", a);
    for (int i = 0; i < a; ++i) {
      Serial.printf("arg[%d].name: %s,\t arg[%d].value: %s", i, server.argName(i).c_str(), i, server.arg(i).c_str());
    }
  }
}

void newConnectionHandler() {
  if (newConnection && millis() - newConnectionIstant > 1500) {
    newConnection = false;
    if (tempSsid != "" && tempPass != "") {
      if (AP_MODE) {
        Serial.printf("Stopping AP and \n");
        WiFi.softAPdisconnect();
        AP_MODE = false;
      } else if (WiFi.status() == WL_CONNECTED) {
        WiFi.disconnect();
      }
      Serial.print("[NEW CONN HANDLER] Connecting to: "); Serial.println(tempSsid);
      WiFi.begin(tempSsid.c_str(), tempPass.c_str());
      uint32_t t0 = millis();
      while (WiFi.status() != WL_CONNECTED) {
        Serial.printf(".");
        delay(500);
        if (millis() - t0 > (10 * 1000)) {
          Serial.println("connection timeout!!");
          break;
        }
      }
      Serial.println();
      if (WiFi.status() != WL_CONNECTED) {
        WiFi.mode(WIFI_AP);
        Serial.print("Reactivacting AP!! AP NAME: "); Serial.println("ESP32_AP");
        WiFi.softAP("ESP32_AP", "12345678");
        Serial.print("AP IP Address: ");
        Serial.println(WiFi.softAPIP());
        AP_MODE = true;
      } else {
        Serial.print("Connected to: ");
        Serial.println(WiFi.SSID());
        Serial.print("IP Address: ");
        Serial.println(WiFi.localIP());
      }
    } else {
      Serial.println("Connection ERROR; empty parameter");
    }
  }
}

void setup() {
  Serial.begin(115200);
  startFilesystem();
  Serial.printf("Git Remote: %s, Git Commit: %s\n", __GIT_REMOTE_URL__, __GIT_COMMIT__);

  for (const auto& pin : ledPins) {
    pinMode(pin, OUTPUT);
  }
  pinMode(buttonPin, INPUT);

  IPAddress myIP = myWebServer.startWiFi(15000, "ESP32_AP", "12345678" );

  //ENDPOINT MAIN
  myWebServer.addHandler("/mainPageEndpoint", HTTP_GET, handleMainEndpoint);
  myWebServer.addHandler("/saveConfig", HTTP_GET, handleSaveConfig);
  myWebServer.addHandler("/ledState", HTTP_POST, handleLedStates);
  myWebServer.addHandler("/WiFiConnect", HTTP_POST, handleWiFiConnect);

  if (myWebServer.begin()) {
    Serial.print(F("ESP Web Server started on IP Address: "));
    Serial.println(myIP);
    Serial.println(F("Open /setup page to configure optional parameters"));
    Serial.println(F("Open /edit page to view and edit files"));
    Serial.println(F("Open /update page to upload firmware and filesystem updates"));
  }
}

bool buttonStateP = 0;

void loop() {
  myWebServer.run();
  newConnectionHandler();
  bool buttonState = !digitalRead(buttonPin);
  if (buttonState && !buttonStateP) {
    digitalWrite(2, !digitalRead(2));
  }
  buttonStateP = buttonState;
}
