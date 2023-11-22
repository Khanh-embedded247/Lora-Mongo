#include <SPI.h>
#include <LoRa.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

const char *ssid = "Dang Thi Hoai";
const char *password = "Etzetkhong9";
const char *mqtt_server = "192.168.1.12";
const int mqtt_port = 1883;
const char *mqtt_topic = "status";

WiFiClient espClient;
PubSubClient client(espClient);

DynamicJsonDocument jsonDoc(1024);  //tạo mảng kích thước json 1024 kí tự

const String masterId = "ESP_MASTER_1";

struct DeviceInfo {
  String id;
  bool isConnected;
};

const int maxDevices = 10;  // số lượng tối đa mà 1 con chủ nhận được
DeviceInfo connectedDevices[maxDevices];

int numConnectedDevices = 0;

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }

  Serial.println("Connected to WiFi");

  LoRa.setPins(5, 4, 2);    // cấu hình chân cho lora esp32
  if (!LoRa.begin(433E6)) {  //~ 433mhz
    Serial.println("LoRa initialization failed. Check your connections.");
    while (1)
      ;
  }

  client.setServer(mqtt_server, mqtt_port);
}

void loop() {
  // Receive LoRa data
  int packetSize = LoRa.parsePacket();
  if (packetSize) {
    while (LoRa.available()) {
      char c = LoRa.read();
      if (c != '\r' && c != '\n') {
        Serial.write(c);
      }
    }
    Serial.println();

    DeserializationError error = deserializeJson(jsonDoc, LoRa);
    if (error) {
      Serial.println("Failed to parse LoRa data");
      return;
    }

    String senderId = jsonDoc["id"];
    bool deviceExists = false;

    for (int i = 0; i < numConnectedDevices; ++i) {
      if (connectedDevices[i].id == senderId) {
        deviceExists = true;
        break;
      }
    }

    if (!deviceExists && numConnectedDevices < maxDevices) {
      connectedDevices[numConnectedDevices].id = senderId;
      connectedDevices[numConnectedDevices].isConnected = true;
      ++numConnectedDevices;
    }

    JsonArray devices = jsonDoc["devices"].as<JsonArray>();
    for (JsonArray::iterator it = devices.begin(); it != devices.end(); ++it) {
      JsonObject device = *it;
      if (!device.containsKey("id")) {
        device["id"] = senderId;
      }

      String deviceId = device["id"];
      bool deviceIsConnected = false;

      for (int i = 0; i < numConnectedDevices; ++i) {
        if (connectedDevices[i].id == deviceId) {
          deviceIsConnected = true;
          break;
        }
      }

      if (!deviceIsConnected) {
        removeDevice(deviceId);
      }
    }

    jsonDoc["id"] = masterId;  // trường master là tên của chủ

    // thêm dl vào mảng devieces
    JsonObject newDevice = jsonDoc["devices"].createNestedObject();
    newDevice["id"] = senderId;
    newDevice["temp"] = jsonDoc["temp"];
    newDevice["hum"] = jsonDoc["hum"];
    newDevice["mq"] = jsonDoc["mq"];
    newDevice["fan"] = jsonDoc["fan"];
    newDevice["light"] = jsonDoc["light"];

    String jsonString;
    serializeJson(jsonDoc, jsonString);

    if (client.connect("esp32-client")) {
      client.publish(mqtt_topic, jsonString.c_str());
      Serial.println("Published to MQTT");
      client.disconnect();
    }

    jsonDoc.clear();
  }

  delay(1000);
}

void removeDevice(String deviceId) {
  for (int i = 0; i < numConnectedDevices; ++i) {
    if (connectedDevices[i].id == deviceId) {
      // Remove the device by shifting remaining devices
      for (int j = i; j < numConnectedDevices - 1; ++j) {
        connectedDevices[j] = connectedDevices[j + 1];
      }
      --numConnectedDevices;
      return;
    }
  }
}
