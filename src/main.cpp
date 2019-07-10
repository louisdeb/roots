#include <Arduino.h>

#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>

const char *AP_NAME = "roots";
const char *AP_PASS = "password";

void setup() {
  Serial.begin(9600);

  WiFi.disconnect(); // Removes saved WiFi credentials

  const char* authToken = (char*)malloc(24 * sizeof(char));
  int tokenLength = 24;
  WiFiManagerParameter authParam("auth", "canopy user auth", authToken, tokenLength);

  WiFiManager wifiManager;
  wifiManager.addParameter(&authParam);
  wifiManager.autoConnect(AP_NAME, AP_PASS);

  authToken = authParam.getValue();

  Serial.println("connected");
  Serial.println(authToken);
}

void loop() {
}
