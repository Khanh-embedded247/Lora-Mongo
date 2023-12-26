#include <SPI.h>
#include <LoRa.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <WebServer.h>  // Sử dụng WebServer thay vì ESPAsyncWebServer

const char *ssid = "F11_A10";
const char *password = "88888888";
const char *mqtt_server = "192.168.68.147";
const int mqtt_port = 1883;
const char *mqtt_topic = "status";
const char *mqtt_username = "khanh";
const char *mqtt_password = "1";
WiFiClient espClient;
PubSubClient client(espClient);
WebServer server(80);
DynamicJsonDocument jsonDoc(200); // Giảm kích thước bộ nhớ cần thiết

const String masterId = "MASTER_1";


struct DeviceInfo {
  String id;
  bool isConnected;
};

const int maxDevices = 10;
DeviceInfo connectedDevices[maxDevices];
int numConnectedDevices = 0;

void setup() {
  Serial.begin(115200);
  connectToWiFi();
  client.setServer(mqtt_server, mqtt_port);
  connectToMQTT();
  setupLoRa();
  setupWebServer();
}

void loop() {
  handleLoRaData();
  
  server.handleClient();
  
}

void connectToWiFi() {
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");
  Serial.println(WiFi.localIP());
}

void connectToMQTT() {
  while (!client.connected()) {
    if (client.connect(masterId.c_str(), mqtt_username, mqtt_password)) {
      Serial.println("Connected to MQTT");
    } else {
      Serial.println("Failed to connect to MQTT. Retrying in 5 seconds...");
      delay(5000);
    }
  }
}

void setupLoRa() {
  LoRa.setPins(5, 4, 23);

  // Sửa đổi tần số, băng thông, spreading factor, và coding rate
  if (!LoRa.begin(433E6)) {
    Serial.println("LoRa initialization failed. Check your connections.");
    while (1);
  }

  // Đặt các tham số LoRa khác nếu cần
  LoRa.setSignalBandwidth(125E3);
  LoRa.setSpreadingFactor(12);
  LoRa.setCodingRate4(5);

  Serial.println("LoRa initialization successful");
}

void setupWebServer() {
  server.on("/control", HTTP_POST, [](){
    String requestBody = server.arg("plain");
    if (verifyAndForward(requestBody)) {
      server.send(200, "text/plain", "OK");
    } else {
      server.send(400, "text/plain", "Bad Request");
    }
  });

  server.begin();
}

bool verifyAndForward(String requestBody) {
  DeserializationError error = deserializeJson(jsonDoc, requestBody);
  if (!error) {
    String master = jsonDoc["chủ"];
    String slave = jsonDoc["con"];
    String fanControl = jsonDoc["fan"];
    String lightControl = jsonDoc["light"];

    // Kiểm tra nếu yêu cầu đúng và gửi qua LoRa tới bộ con tương ứng
    if (master == masterId) {
      forwardControl(slave, fanControl, lightControl);
      return true;
    }
  }
  return false;
}

void forwardControl(String deviceId, String fanControl, String lightControl) {
  StaticJsonDocument<200> jsonDoc;
  jsonDoc["id"] = masterId;
  jsonDoc["fan"] = fanControl;
  jsonDoc["light"] = lightControl;

  char buffer[256];
  size_t n = serializeJson(jsonDoc, buffer);

  LoRa.beginPacket();
  LoRa.write(reinterpret_cast<const uint8_t*>(buffer), n);
  LoRa.endPacket();

  Serial.println("Forwarded control to device: " + deviceId);
}

// Cấu trúc dữ liệu cho một thiết bị phụ
struct DeviceData {
  String id;
  String temp;
  String hum;
  int mq;
  bool fan;
  bool light;
};
DeviceData devices[maxDevices]; // Mảng chứa dữ liệu từ các thiết bị phụ

void handleLoRaData() {
  int packetSize = LoRa.parsePacket();
  if (packetSize) {
    char buffer[1024];
    int i = 0;

    while (LoRa.available()) {
      char c = LoRa.read();
      if (c != '\r' && c != '\n') {
        buffer[i++] = c;
      }
    }

    buffer[i] = '\0';

    DeserializationError error = deserializeJson(jsonDoc, buffer);
    if (error) {
      Serial.println("Failed to parse LoRa data");
      return;
    }

    String senderId = jsonDoc["id"];
    bool deviceExists = false;
    int deviceIndex = 0;
    
    for (int i = 0; i < numConnectedDevices; ++i) {
      if (connectedDevices[i].id == senderId) {
        deviceExists = true;
        deviceIndex = i;
        break;
      }
    }

    if (!deviceExists && numConnectedDevices < maxDevices) {
      connectedDevices[numConnectedDevices].id = senderId;
      connectedDevices[numConnectedDevices].isConnected = true;
      deviceIndex = numConnectedDevices;
      ++numConnectedDevices;
    }

     // Cập nhật dữ liệu cho thiết bị phụ
    devices[deviceIndex].id = senderId;
    devices[deviceIndex].temp = jsonDoc["temp"].as<String>();
    devices[deviceIndex].hum = jsonDoc["hum"].as<String>();
    devices[deviceIndex].mq = jsonDoc["mq"];
    devices[deviceIndex].fan = jsonDoc["fan"];
    devices[deviceIndex].light = jsonDoc["light"];

// Chuẩn bị dữ liệu JSON để gửi lên MQTT
    DynamicJsonDocument mqttJson(1024);
    mqttJson["master"] = masterId;
    JsonArray deviceArray = mqttJson.createNestedArray("devices");

   for (int i = 0; i < numConnectedDevices; ++i) {
      JsonObject deviceJson = deviceArray.createNestedObject();
      deviceJson["id"] = devices[i].id;
      deviceJson["temp"] = devices[i].temp;
      deviceJson["hum"] = devices[i].hum;
      deviceJson["mq"] = devices[i].mq;
      deviceJson["fan"] = devices[i].fan;
      deviceJson["light"] = devices[i].light;
    }


    // Nếu có các thay đổi giá trị khác, cập nhật chúng ở đây

    jsonDoc["id"] = senderId;

    String mqttString;
    serializeJson(mqttJson, mqttString);

    // Check MQTT connection
    if (!client.connected()) {
      // Reconnect if not connected
      connectToMQTT();
    }

    // Publish data to MQTT
    if (client.connected()) {
      client.publish(mqtt_topic, mqttString.c_str());
      Serial.println("Published to MQTT");
    }

    jsonDoc.clear();
  }
}
