#include <WiFi.h>
#include <PubSubClient.h>
#include "DHT.h"
 
#define WIFI_SSID      "P-403"
#define WIFI_PASSWORD  "12345678"
 

#define TOKEN               "8e6RngbV7MnkKEjGNnbu"
#define THINGSBOARD_SERVER  "eu.thingsboard.cloud"
#define THINGSBOARD_PORT    1883
 

#define DHTPIN   4
#define DHTTYPE  DHT22
 

#define TEMP_OFFSET         -2.0
#define HUMIDITY_OFFSET      0.0
 

#define TEMP_HIGH_LIMIT      35.0
#define HUMIDITY_HIGH_LIMIT  85.0
 

#define TELEMETRY_INTERVAL   5000   
 
WiFiClient espClient;
PubSubClient client(espClient);
DHT dht(DHTPIN, DHTTYPE);
 
unsigned long lastSendTime = 0;
 
void connectWiFi() {
  Serial.println("Dang ket noi WiFi...");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
 
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
 
  Serial.println();
  Serial.println("WiFi connected!");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());
}
 
void sendTelemetry(String payload) {
  bool result = client.publish("v1/devices/me/telemetry", payload.c_str());
 
  if (result) {
    Serial.println("Da gui du lieu len ThingsBoard");
  } else {
    Serial.println("Gui du lieu that bai!");
  }
 
  Serial.println(payload);
  Serial.println("------------------------------------");
}
 
void sendStatus(String status, String alarm, String alarmMessage) {
  String payload = "{";
  payload += "\"status\":\"" + status + "\",";
  payload += "\"alarm\":\"" + alarm + "\",";
  payload += "\"alarmMessage\":\"" + alarmMessage + "\"";
  payload += "}";
 
  sendTelemetry(payload);
}
 
void connectThingsBoard() {
  client.setServer(THINGSBOARD_SERVER, THINGSBOARD_PORT);
  client.setBufferSize(512);
 
  while (!client.connected()) {
    Serial.println("Dang ket noi ThingsBoard...");
 
    const char* willTopic = "v1/devices/me/telemetry";
    const char* willMessage =
      "{\"status\":\"OFFLINE\",\"alarm\":\"WARNING\",\"alarmMessage\":\"DEVICE_OFFLINE\"}";
 
    bool connected = client.connect(
      "ESP32_DHT22_Client",
      TOKEN,
      NULL,
      willTopic,
      1,
      false,
      willMessage
    );
 
    if (connected) {
      Serial.println("Da ket noi ThingsBoard!");
      sendStatus("ONLINE", "NORMAL", "BINH_THUONG");
    } else {
      Serial.print("Ket noi ThingsBoard that bai, rc=");
      Serial.println(client.state());
      delay(2000);
    }
  }
}
 
void readAndSendSensorData() {
  float rawHumidity = dht.readHumidity();
  float rawTemperature = dht.readTemperature();
 
  if (isnan(rawHumidity) || isnan(rawTemperature)) {
    Serial.println("Doc DHT22 loi!");
 
    String errorPayload = "{";
    errorPayload += "\"status\":\"SENSOR_ERROR\",";
    errorPayload += "\"alarm\":\"WARNING\",";
    errorPayload += "\"alarmMessage\":\"DHT22_READ_ERROR\"";
    errorPayload += "}";
 
    sendTelemetry(errorPayload);
    return;
  }
 
  // Áp dụng hiệu chỉnh
  float temperature = rawTemperature + TEMP_OFFSET;
  float humidity = rawHumidity + HUMIDITY_OFFSET;
  humidity = constrain(humidity, 0.0, 100.0);
 
  // Kiểm tra ngưỡng cảnh báo
  bool tempHigh = temperature >= TEMP_HIGH_LIMIT;
  bool humidityHigh = humidity >= HUMIDITY_HIGH_LIMIT;
  bool hasAlarm = tempHigh || humidityHigh;
 
  String status = "ONLINE";
  String alarm;
  String alarmMessage;
 
  if (hasAlarm) {
    alarm = "WARNING";
 
    if (tempHigh && humidityHigh) {
      alarmMessage = "TEMP_AND_HUMIDITY_HIGH";
    } else if (tempHigh) {
      alarmMessage = "TEMPERATURE_HIGH";
    } else {
      alarmMessage = "HUMIDITY_HIGH";
    }
  } else {
    alarm = "NORMAL";
    alarmMessage = "BINH_THUONG";
  }
 
  Serial.printf("Nhiet do : %.1f C  raw: %.1f C\n", temperature, rawTemperature);
  Serial.printf("Do am    : %.1f %% raw: %.1f %%\n", humidity, rawHumidity);
  Serial.print("Trang thai: ");
  Serial.println(status);
  Serial.print("Canh bao  : ");
  Serial.println(alarmMessage);
 
  String payload = "{";
  payload += "\"temperature\":" + String(temperature, 1) + ",";
  payload += "\"humidity\":" + String(humidity, 1) + ",";
  payload += "\"rawTemperature\":" + String(rawTemperature, 1) + ",";
  payload += "\"rawHumidity\":" + String(rawHumidity, 1) + ",";
  payload += "\"status\":\"" + status + "\",";
  payload += "\"alarm\":\"" + alarm + "\",";
  payload += "\"alarmMessage\":\"" + alarmMessage + "\",";
  payload += "\"tempHigh\":" + String(tempHigh ? "true" : "false") + ",";
  payload += "\"humidityHigh\":" + String(humidityHigh ? "true" : "false") + ",";
  payload += "\"rssi\":" + String(WiFi.RSSI()) + ",";
  payload += "\"uptimeSec\":" + String(millis() / 1000);
  payload += "}";
 
  sendTelemetry(payload);
}
 
void setup() {
  Serial.begin(115200);
  delay(1000);
 
  Serial.println();
  Serial.println("ESP32 DHT22 ThingsBoard Telemetry Start");
 
  dht.begin();
  connectWiFi();
  connectThingsBoard();
}
 
void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    connectWiFi();
  }
 
  if (!client.connected()) {
    connectThingsBoard();
  }
 
  client.loop();
 
  unsigned long currentTime = millis();
 
  if (currentTime - lastSendTime >= TELEMETRY_INTERVAL) {
    lastSendTime = currentTime;
    readAndSendSensorData();
  }
}
