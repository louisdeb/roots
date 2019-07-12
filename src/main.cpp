#include <Arduino.h>

#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>

#include <ArduinoJson.h>

const char* AP_NAME = "roots";
const char* AP_PASS = "password";

const int CANOPY_PORT = 80;
const char* CANOPY_HOST = "grow.v2.vapor.cloud";
const char* CANOPY_BOX_ANNOUNCE = "/api/box";

const char* CANOPY_API_KEY = (char*)malloc(16 * sizeof(char));

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

  Serial.println("connected to wifi");
  Serial.println(authToken);

  // When converting to HTTPS, we want to use WiFiClientSecure (on port 443)
  WiFiClient client;
  if (client.connect(CANOPY_HOST, CANOPY_PORT)) {
    Serial.println("connected to canopy");

    client.print(String("POST ") + CANOPY_BOX_ANNOUNCE + " HTTP/1.1\r\n" + 
                   "Host: " + CANOPY_HOST + "\r\n" +
                   "Connection: close\r\n" +
                   "Authorization: Bearer " + authToken + "\r\n" +
                   "\r\n");

    while (client.connected() || client.available()) {
      if (client.available()) {
        String line = client.readStringUntil('\n');
        Serial.println(line);

        // https://arduinojson.org/v6/doc/deserialization/ §3.2.5
        const int capacity = 300; // extraneous size, may reduce server return
        StaticJsonDocument<capacity> doc;
        DeserializationError err = deserializeJson(doc, line);

        if (err) {
          Serial.print(F("deserialization failed with err code: "));
          Serial.println(err.c_str());
        } else {
          CANOPY_API_KEY = doc["apiKey"];
          Serial.print("got api key ");
          Serial.println(CANOPY_API_KEY);
        }
      }
    }

    client.stop();
    Serial.println("disconnected from canopy");
  } else {
    Serial.println("failed to connect to canopy");
    client.stop();
  }
}

void loop() {
}
