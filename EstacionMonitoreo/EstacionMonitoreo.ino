#include <Arduino.h>

#define WIFI_SSID "wifi ssid"
#define WIFI_PASS "wifi password"
#define MQTT_SERVER "test.mosquitto.org"
#define MQTT_PORT 1883
#define MQTT_TOPIC "rcr/station01"
#define MQTT_CLIENTID "RCR_f00dda31-cb50-4eb8-b4f7-7b33920c39c2"
#define DHT_PIN_DATA D5
#define SDS_PIN_RX D1
#define SDS_PIN_TX D2
#define SLEEP_MS 2000

#include <ESP8266WiFi.h>
WiFiClient net;

#include <PubSubClient.h>
PubSubClient mqtt(net);

#include <ArduinoJson.h>
StaticJsonDocument<256> jsonDoc;

#include "DHT.h"
DHT dht(DHT_PIN_DATA, DHT22);

#include "SdsDustSensor.h"
SdsDustSensor sds(SDS_PIN_TX, SDS_PIN_RX);

//---

void setup() {
  Serial.begin(9600);
  while (!Serial) delay(50);
  Serial.println();
  Serial.println();

  WiFi.mode(WIFI_STA);
  WiFi.setAutoConnect(true);
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  dht.begin();
  sds.begin();
  mqtt.setServer(MQTT_SERVER, MQTT_PORT);
  jsonDoc["NodeID"] = MQTT_CLIENTID;
}

bool wifiReconnect() {
  if (WiFi.status() == WL_CONNECTED)
    return true;

  Serial.print("Conectando a la WiFi: .");
  for (int i = 0; i < 10; i++) {
    if (WiFi.status() == WL_CONNECTED) {
      Serial.println(" Ok");
      return true;
    }
    Serial.print(".");
    delay(500);
  }
  Serial.println(" Timeout");
  return false;
}

bool mqttReconnect() {
  if (mqtt.connected())
    return true;

  Serial.print("Conectando a MQTT: .");
  for (int i = 0; i < 10; i++) {
    if (mqtt.connect(MQTT_CLIENTID)) {
      mqtt.subscribe(MQTT_TOPIC, 0);
      Serial.println(" Ok");
      return true;
    }
    Serial.print(".");
    delay(500);
  }
  Serial.println(" Timeout");
  return false;
}

void loop() {
  mqtt.loop();
  if (!wifiReconnect()) return;
  if (!mqttReconnect()) return;

  // DHT22
  float temp = dht.readTemperature();
  float hum = dht.readHumidity();

  jsonDoc["temp"] = isnan(temp) ? -99999 : temp;
  jsonDoc["hum"] = isnan(hum) ? -99999 : hum;

  // SDS011
  PmResult pm = sds.readPm();
  if (pm.isOk()) {
    jsonDoc["pm25"] = pm.pm25;
    jsonDoc["pm10"] = pm.pm10;
  } else {
    jsonDoc["pm25"] = -99999;
    jsonDoc["pm10"] = -99999;
  }

  // publish
  char buffer[256];
  serializeJson(jsonDoc, buffer);
  Serial.println(buffer);
  mqtt.publish(MQTT_TOPIC, buffer);
  Serial.flush();

  // pause
  delay(SLEEP_MS);
}
