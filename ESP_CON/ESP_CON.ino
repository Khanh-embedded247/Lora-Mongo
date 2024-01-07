#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <MQ135.h>
#include <LoRa.h>
#include <ArduinoJson.h>

#define DHT_PIN 15
#define MQ135_PIN 12
#define FAN_PIN 32//int2
#define LIGHT_PIN 14//int1

#define LORA_SS 5
#define LORA_RST 4
#define LORA_DIO0 21
#define LORA_FREQUENCY 433E6

struct SensorData {
  String id;
  String master;
  float temp;
  float hum;
  int mq;
  bool fan;
  bool light;
};

DHT dht(DHT_PIN, DHT11);
MQ135 mq135(MQ135_PIN);

SensorData sensorData;  // Giữ cấu trúc dữ liệu để tránh tạo lại
unsigned long previousMillis = 0;
const unsigned long interval = 3000;  // Thời gian giữa các lần đọc và gửi dữ liệu

void setup() {
  Serial.begin(115200);

  LoRa.setPins(LORA_SS, LORA_RST, LORA_DIO0);
  if (!LoRa.begin(LORA_FREQUENCY)) {
    Serial.println("LoRa initialization failed. Check your connections.");
    while (1)
      ;
  }

  dht.begin();
  LoRa.setSignalBandwidth(125E3);
  LoRa.setSpreadingFactor(12);
  LoRa.setCodingRate4(5);
  pinMode(FAN_PIN, OUTPUT);
  // digitalWrite(FAN_PIN, LOW);
  pinMode(LIGHT_PIN, OUTPUT);
  // digitalWrite(LIGHT_PIN, LOW);
}

void loop() {
  unsigned long currentMillis = millis();

  if (currentMillis - previousMillis >= interval) {
    readAndSendData();
    previousMillis = currentMillis;
  }

  handleControlSignal();
}

void readAndSendData() {
  readSensorData();

  if (sensorData.temp > 30) {
    sensorData.light = true;
    digitalWrite(LIGHT_PIN, HIGH);
  } else {
    sensorData.light = false;
    digitalWrite(LIGHT_PIN, LOW);
  }

  bool previousFanStatus = sensorData.fan;

  if (sensorData.mq > 10) {
    sensorData.fan = true;
    digitalWrite(FAN_PIN, HIGH);  // This might be the issue
  } else {
    sensorData.fan = false;
    digitalWrite(FAN_PIN, LOW);  // This might be the issue
  }


  // // Kiểm tra xem có yêu cầu bật quạt và đèn không
  // digitalWrite(FAN_PIN, sensorData.fan ? LOW : HIGH);
  // digitalWrite(LIGHT_PIN, sensorData.light ? LOW : HIGH);

  // Tạo JSON để gửi lên bộ chủ
  StaticJsonDocument<200> jsonDoc;
  jsonDoc["master"] = sensorData.master;
  jsonDoc["id"] = sensorData.id;
  jsonDoc["temp"] = sensorData.temp;
  jsonDoc["hum"] = sensorData.hum;
  jsonDoc["mq"] = sensorData.mq;
  jsonDoc["fan"] = sensorData.fan;
  jsonDoc["light"] = sensorData.light;

  char buffer[256];
  size_t n = serializeJson(jsonDoc, buffer);
  Serial.println(buffer);

  // Gửi dữ liệu lên bộ chủ qua LoRa
  sendToMaster(buffer, n);
}

void readSensorData() {
  sensorData.id = "ESP_SLAVE_1";
  sensorData.master = "MASTER_1";
  sensorData.temp = dht.readTemperature();
  sensorData.hum = dht.readHumidity();
  sensorData.mq = mq135.getPPM();
}

void handleControlSignal() {
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

    // Kiểm tra tín hiệu JSON từ bộ chủ
    DynamicJsonDocument jsonDoc(200);
    DeserializationError error = deserializeJson(jsonDoc, buffer);
    if (error) {
      Serial.println("Failed to parse LoRa control signal");
      return;
    }

    // Lấy thông tin từ tín hiệu JSON
    const char* slaveId = jsonDoc["id"];
    if (strcmp(slaveId, "ESP_SLAVE_2") == 0) {
      // Đúng bộ con nhận tín hiệu
      sensorData.fan = jsonDoc["fan"];
      sensorData.light = jsonDoc["light"];

      // Gửi trạng thái của thiết bị lên lại bộ chủ
      sendStatusToMaster();
    }
  }
}

void sendToMaster(const char* buffer, size_t length) {
  LoRa.beginPacket();
  LoRa.write(reinterpret_cast<const uint8_t*>(buffer), length);
  LoRa.endPacket();
}

void sendStatusToMaster() {
  readSensorData();  // Cập nhật trạng thái của thiết bị vào cấu trúc dữ liệu

  // Tạo JSON để gửi lên bộ chủ
  StaticJsonDocument<200> jsonDoc;
  jsonDoc["id"] = sensorData.id;
  jsonDoc["temp"] = sensorData.temp;
  jsonDoc["hum"] = sensorData.hum;
  jsonDoc["mq"] = (sensorData.mq/1000000);
  jsonDoc["fan"] = sensorData.fan;
  jsonDoc["light"] = sensorData.light;

  char buffer[256];
  size_t n = serializeJson(jsonDoc, buffer);

  // Gửi dữ liệu lên bộ chủ qua LoRa
  sendToMaster(buffer, n);

  Serial.println("Sent status to master");
}
