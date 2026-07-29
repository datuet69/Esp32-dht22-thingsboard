#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>
#include "secrets.h"
#define DHT_PIN 4
#define DHT_TYPE DHT22

constexpr float TEMPERATURE_OFFSET = -2.0F;
constexpr float HUMIDITY_OFFSET = 0.0F;

constexpr float TEMPERATURE_HIGH_LIMIT = 35.0F;
constexpr float HUMIDITY_HIGH_LIMIT = 85.0F;

constexpr unsigned long SEND_INTERVAL_MS = 5000UL;


constexpr char THINGSBOARD_SERVER[] = "eu.thingsboard.cloud";
constexpr uint16_t THINGSBOARD_PORT = 1883;

constexpr char TELEMETRY_TOPIC[] =
    "v1/devices/me/telemetry";


WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);
DHT dht(DHT_PIN, DHT_TYPE);

unsigned long previousSendTime = 0;

// Kết nối Wi-Fi

bool connectWiFi()
{
    if (WiFi.status() == WL_CONNECTED)
    {
        return true;
    }

    Serial.println();
    Serial.print("Dang ket noi Wi-Fi: ");
    Serial.println(WIFI_SSID);

    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    const unsigned long startTime = millis();
    const unsigned long timeoutMs = 15000UL;

    while (WiFi.status() != WL_CONNECTED)
    {
        if (millis() - startTime >= timeoutMs)
        {
            Serial.println();
            Serial.println("Ket noi Wi-Fi that bai!");
            return false;
        }

        delay(500);
        Serial.print(".");
    }

    Serial.println();
    Serial.println("Wi-Fi connected!");

    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());

    Serial.print("RSSI: ");
    Serial.print(WiFi.RSSI());
    Serial.println(" dBm");

    return true;
}

// Tạo Client ID 

String createMqttClientId()
{
    const uint64_t chipId = ESP.getEfuseMac();

    String clientId = "esp32-dht22-";
    clientId += String(
        static_cast<uint32_t>(chipId >> 32),
        HEX
    );
    clientId += String(
        static_cast<uint32_t>(chipId),
        HEX
    );

    return clientId;
}

// Kết nối ThingsBoard 

bool connectThingsBoard()
{
    if (mqttClient.connected())
    {
        return true;
    }

    if (WiFi.status() != WL_CONNECTED)
    {
        return false;
    }

    Serial.println("Dang ket noi ThingsBoard...");

    const String clientId = createMqttClientId();

    const bool connected = mqttClient.connect(
        clientId.c_str(),
        THINGSBOARD_TOKEN,
        ""
    );

    if (connected)
    {
        Serial.println("ThingsBoard connected!");
        return true;
    }

    Serial.print("ThingsBoard connection failed. MQTT state: ");
    Serial.println(mqttClient.state());

    return false;
}


// Đọc cảm biến và gửi dữ liệu

void sendTelemetry()
{
    float humidity = dht.readHumidity();
    float temperature = dht.readTemperature();

    if (isnan(temperature) || isnan(humidity))
    {
        Serial.println("Khong doc duoc du lieu tu DHT22!");
        return;
    }

    temperature += TEMPERATURE_OFFSET;
    humidity += HUMIDITY_OFFSET;

    const bool temperatureAlarm =
        temperature > TEMPERATURE_HIGH_LIMIT;

    const bool humidityAlarm =
        humidity > HUMIDITY_HIGH_LIMIT;

    char payload[256];

    const int payloadLength = snprintf(
        payload,
        sizeof(payload),
        "{"
        "\"temperature\":%.2f,"
        "\"humidity\":%.2f,"
        "\"temperatureAlarm\":%s,"
        "\"humidityAlarm\":%s,"
        "\"rssi\":%ld"
        "}",
        temperature,
        humidity,
        temperatureAlarm ? "true" : "false",
        humidityAlarm ? "true" : "false",
        static_cast<long>(WiFi.RSSI())
    );

    if (payloadLength <= 0 ||
        payloadLength >= static_cast<int>(sizeof(payload)))
    {
        Serial.println("Loi tao JSON telemetry!");
        return;
    }

    Serial.println();
    Serial.println("Du lieu chuan bi gui:");
    Serial.println(payload);

    const bool success = mqttClient.publish(
        TELEMETRY_TOPIC,
        payload
    );

    if (success)
    {
        Serial.println("Gui telemetry thanh cong!");
    }
    else
    {
        Serial.println("Gui telemetry that bai!");
    }
}

// setup
void setup()
{
    Serial.begin(115200);
    delay(1000);

    Serial.println();
    Serial.println("==============================");
    Serial.println("ESP32 DHT22 ThingsBoard");
    Serial.println("==============================");

    dht.begin();

    mqttClient.setServer(
        THINGSBOARD_SERVER,
        THINGSBOARD_PORT
    );

    mqttClient.setKeepAlive(60);

    connectWiFi();
}
// loop
void loop()
{
    if (WiFi.status() != WL_CONNECTED)
    {
        if (!connectWiFi())
        {
            delay(3000);
            return;
        }
    }

    if (!mqttClient.connected())
    {
        if (!connectThingsBoard())
        {
            delay(3000);
            return;
        }
    }

    mqttClient.loop();

    const unsigned long currentTime = millis();

    if (currentTime - previousSendTime >= SEND_INTERVAL_MS)
    {
        previousSendTime = currentTime;
        sendTelemetry();
    }

    delay(10);
}