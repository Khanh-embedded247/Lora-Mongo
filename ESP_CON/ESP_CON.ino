#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <MQ135.h>
#include <LoRa.h>
#include <ArduinoJson.h>

// Pin configuration
#define DHT_PIN 25      // DHT11 data pin
#define MQ135_PIN 34    // MQ135 analog pin
#define FAN_PIN 14      // Pin controlling the fan
#define LIGHT_PIN 12    // Pin controlling the light

// LoRa configuration
#define LORA_SS 5       // LoRa chip select
#define LORA_RST 14     // LoRa reset
#define LORA_DIO0 2      // LoRa DIO0
#define LORA_FREQUENCY 433E6  // LoRa frequency

// LoRa packet structure
struct SensorData {
  String id;
  float temp;
  float hum;
  int mq;
  bool fan;
  bool light;
};

// Create instances of the sensors
DHT dht(DHT_PIN, DHT11);
MQ135 mq135(MQ135_PIN);

void setup() {
  Serial.begin(115200);

  // Initialize LoRa
  LoRa.setPins(LORA_SS, LORA_RST, LORA_DIO0);
  if (!LoRa.begin(LORA_FREQUENCY)) {
    Serial.println("LoRa initialization failed. Check your connections.");
    while (1);
  }

  // Initialize sensors
  dht.begin();

  // Set pin modes for controlling fan and light
  pinMode(FAN_PIN, OUTPUT);
  pinMode(LIGHT_PIN, OUTPUT);
}

void loop() {
  // Read sensor data
  float temperature = dht.readTemperature();
  float humidity = dht.readHumidity();
  int airQuality = mq135.getPPM(); // Use getPPM to get CO2 concentration

  // Read fan and light status
  bool fanStatus = digitalRead(FAN_PIN) == HIGH;
  bool lightStatus = digitalRead(LIGHT_PIN) == HIGH;

  // Prepare LoRa packet
  SensorData sensorData;
  sensorData.id = "ESP_SLAVE_1";  // Set the device ID
  sensorData.temp = temperature;
  sensorData.hum = humidity;
  sensorData.mq = airQuality;
  sensorData.fan = fanStatus;
  sensorData.light = lightStatus;

  // Send data via LoRa
  StaticJsonDocument<200> jsonDoc;
  jsonDoc["id"] = sensorData.id;
  jsonDoc["temp"] = sensorData.temp;
  jsonDoc["hum"] = sensorData.hum;
  jsonDoc["mq"] = sensorData.mq;
  jsonDoc["fan"] = sensorData.fan;
  jsonDoc["light"] = sensorData.light;

  char buffer[256];
  size_t n = serializeJson(jsonDoc, buffer);
  Serial.println(buffer);
  LoRa.beginPacket();
  LoRa.write(reinterpret_cast<const uint8_t*>(buffer), n);
  LoRa.endPacket();

  delay(5000); // Delay for 5 seconds before sending the next packet
}
