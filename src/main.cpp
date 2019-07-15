#include <Arduino.h>

#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>

#include <ArduinoJson.h>

const char* AP_NAME = "roots";
const char* AP_PASS = "password";

const int API_KEY_LENGTH = 16;
const int TOKEN_LENGTH = 24;

const int CANOPY_PORT = 80;
const char* CANOPY_HOST = "grow.v2.vapor.cloud";
const char* SENSOR_ANNOUNCE_ENDPOINT = "/api/box";
const char* SENSOR_DATA_UPLOAD_ENDPOINT = "/api/sensordata";

const char* CANOPY_API_KEY = (char*)malloc(API_KEY_LENGTH * sizeof(char));

// token and box id used for data upload before server uses API key auth instead
char* USER_AUTH_TOKEN = (char*)malloc(TOKEN_LENGTH * sizeof(char));
const char* BOX_ID;

const int ANALOG_INPUT_PIN = A0;

// When converting to HTTPS, we want to use WiFiClientSecure (on port 443)
WiFiClient client;

void setup() {
  Serial.begin(9600);

  WiFi.disconnect(); // Removes saved WiFi credentials
  
  const char* authToken = (char*)malloc(TOKEN_LENGTH * sizeof(char));
  WiFiManagerParameter authParam("auth", "canopy user auth token", authToken, TOKEN_LENGTH);

  WiFiManager wifiManager;
  wifiManager.addParameter(&authParam);
  wifiManager.autoConnect(AP_NAME, AP_PASS);

  authToken = authParam.getValue();
  USER_AUTH_TOKEN = strncpy(USER_AUTH_TOKEN, authToken, TOKEN_LENGTH);

  Serial.println("connected to wifi");
  Serial.println(authToken);

  if (client.connect(CANOPY_HOST, CANOPY_PORT)) {
    Serial.println("connected to canopy");

    client.print("POST ");
    client.print(SENSOR_ANNOUNCE_ENDPOINT);
    client.println(" HTTP/1.1");
    client.print("Host: ");
    client.println(CANOPY_HOST);
    client.println("Connection: close");
    client.print("Authorization: Bearer ");
    client.println(authToken);
    client.println();

    while (client.connected() || client.available()) {
      if (client.available()) {
        String line = client.readStringUntil('\n');
        Serial.println(line);

        // https://arduinojson.org/v6/doc/deserialization/ §3.2.5
        const int capacity = 300; // extraneous size, may reduce server return
        StaticJsonDocument<capacity> doc;
        DeserializationError err = deserializeJson(doc, line);

        if (err) {
          Serial.print("deserialization failed with err code: ");
          Serial.println(err.c_str());
        } else {
          CANOPY_API_KEY = doc["apiKey"];
          Serial.print("got api key: ");
          Serial.println(CANOPY_API_KEY);

          BOX_ID = doc["id"];
          Serial.print("got box id: ");
          Serial.println(BOX_ID);
        }
      }
    }
  } else {
    Serial.println("failed to connect to canopy");
  }

  client.stop();
  
  pinMode(ANALOG_INPUT_PIN, INPUT);
}

void loop() {
  /* 
    A multiplexing circuit will be required to read different sensor readings 
    from the same A0 pin.
    https://www.instructables.com/id/ESP8266-with-Multiple-Analog-Sensors/
  */

  int sensorData = analogRead(ANALOG_INPUT_PIN);

  Serial.print("Sensor reading: ");
  Serial.println(sensorData);

  const int capacity = 500;
  StaticJsonDocument<capacity> doc;

  JsonObject sensorDataJson = doc.to<JsonObject>();
  sensorDataJson["type"] = "SoilHumidity"; // placeholder, conforming to server
  sensorDataJson["value"] = sensorData;
  sensorDataJson["time"] = 5; // placeholder, may want to generate this data on the server
  sensorDataJson["boxID"] = BOX_ID;

  serializeJsonPretty(sensorDataJson, Serial);
  Serial.println();

  if (client.connect(CANOPY_HOST, CANOPY_PORT)) {
    Serial.print("Authorization token: ");
    Serial.println(USER_AUTH_TOKEN);

    // POST request headers
    client.print("POST ");
    client.print(SENSOR_DATA_UPLOAD_ENDPOINT);
    client.println(" HTTP/1.1");
    client.print("Host: ");
    client.println(CANOPY_HOST);
    client.println("Connection: close");
    client.print("Authorization: Bearer ");
    client.println(USER_AUTH_TOKEN);
    client.println("Content-Type: application/json");
    client.print("Content-Length: ");
    client.println(measureJson(sensorDataJson));

    // Terminate headers
    client.println();

    // Send JSON object in the body
    serializeJson(sensorDataJson, client);

    while (client.connected() || client.available()) {
      if (client.available()) {
        String line = client.readStringUntil('\n');
        Serial.println(line);
      }
    }

    Serial.println();
  } else {
    Serial.println("(data upload) failed to connect to canopy");
  }

  client.stop();

  delay(60000); // 60s
}
