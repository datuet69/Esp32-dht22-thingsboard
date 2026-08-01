#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>
#include <esp_sleep.h>
#include "secrets.h"
#define DHT_PIN 4
#define DHT_TYPE DHT22

DHT dht(DHT_PIN, DHT_TYPE);

constexpr float TEMPERATURE_OFFSET = -2.0F;
constexpr float HUMIDITY_OFFSET = 0.0F;

constexpr float TEMPERATURE_HIGH_LIMIT = 35.0F;
constexpr float HUMIDITY_HIGH_LIMIT = 85.0F;

constexpr char THINGSBOARD_SERVER[] =
    "eu.thingsboard.cloud";

constexpr uint16_t THINGSBOARD_PORT = 1883;

constexpr char TELEMETRY_TOPIC[] =
    "v1/devices/me/telemetry";

// Deep Sleep
constexpr uint64_t MICROSECONDS_PER_SECOND = 1000000ULL;

// Để 30 giây khi thử nghiệm
constexpr uint64_t SLEEP_TIME_SECONDS = 30ULL;

// Biến này giữ được qua Deep Sleep
RTC_DATA_ATTR unsigned int wakeCount = 0;

// Thời gian timeout

constexpr unsigned long WIFI_TIMEOUT_MS = 15000UL;
constexpr unsigned long MQTT_TIMEOUT_MS = 10000UL;

WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

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

bool connectWiFi()
{
    Serial.println();
    Serial.print("Dang ket noi Wi-Fi: ");
    Serial.println(WIFI_SSID);

    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    const unsigned long startTime = millis();

    while (WiFi.status() != WL_CONNECTED)
    {
        if (millis() - startTime >= WIFI_TIMEOUT_MS)
        {
            Serial.println();
            Serial.println("Ket noi Wi-Fi that bai: Het thoi gian cho.");
            return false;
        }

        Serial.print(".");
        delay(500);
    }

    Serial.println();
    Serial.println("Wi-Fi connected!");

    Serial.print("IP: ");
    Serial.println(WiFi.localIP());

    Serial.print("RSSI: ");
    Serial.print(WiFi.RSSI());
    Serial.println(" dBm");

    return true;
}

bool connectThingsBoard()
{
    mqttClient.setServer(
        THINGSBOARD_SERVER,
        THINGSBOARD_PORT
    );

    const String clientId = createMqttClientId();

    Serial.println("Dang ket noi ThingsBoard...");

    const unsigned long startTime = millis();

    while (!mqttClient.connected())
    {
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

        Serial.print("MQTT state: ");
        Serial.println(mqttClient.state());

        if (millis() - startTime >= MQTT_TIMEOUT_MS)
        {
            Serial.println(
                "Ket noi ThingsBoard that bai: Het thoi gian cho."
            );
            return false;
        }

        delay(1000);
    }

    return true;
}


bool sendTelemetry(
    const float temperature,
    const float humidity
)
{
    const bool temperatureAlarm =
        temperature > TEMPERATURE_HIGH_LIMIT;

    const bool humidityAlarm =
        humidity > HUMIDITY_HIGH_LIMIT;

    const long rssi = WiFi.RSSI();

    char payload[300];

    const int payloadLength = snprintf(
        payload,
        sizeof(payload),
        "{"
        "\"temperature\":%.2f,"
        "\"humidity\":%.2f,"
        "\"temperatureAlarm\":%s,"
        "\"humidityAlarm\":%s,"
        "\"rssi\":%ld,"
        "\"wakeCount\":%u"
        "}",
        temperature,
        humidity,
        temperatureAlarm ? "true" : "false",
        humidityAlarm ? "true" : "false",
        rssi,
        wakeCount
    );

    if (
        payloadLength <= 0 ||
        payloadLength >= static_cast<int>(sizeof(payload))
    )
    {
        Serial.println("Loi tao du lieu JSON.");
        return false;
    }

    Serial.println();
    Serial.println("Du lieu gui len ThingsBoard:");
    Serial.println(payload);

    const bool success = mqttClient.publish(
        TELEMETRY_TOPIC,
        payload
    );

    mqttClient.loop();

    delay(500);

    if (success)
    {
        Serial.println("Gui telemetry thanh cong!");
    }
    else
    {
        Serial.println("Gui telemetry that bai!");
    }

    return success;
}

void disconnectConnections()
{
    if (mqttClient.connected())
    {
        mqttClient.disconnect();
    }

    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);

    delay(100);
}

void enterDeepSleep()
{
    Serial.println();
    Serial.print("ESP32 se ngu trong ");
    Serial.print(SLEEP_TIME_SECONDS);
    Serial.println(" giay.");

    disconnectConnections();

    esp_sleep_enable_timer_wakeup(
        SLEEP_TIME_SECONDS * MICROSECONDS_PER_SECOND
    );

    Serial.println("Bat dau Deep Sleep...");
    Serial.flush();

    esp_deep_sleep_start();
}


void setup()
{
    Serial.begin(115200);

    // Dùng 3 giây trong giai đoạn thử nghiệm
    delay(3000);

    wakeCount++;

    Serial.println();
    Serial.println("=======================================");
    Serial.println("ESP32 DHT22 THINGSBOARD DEEP SLEEP");
    Serial.println("=======================================");

    Serial.print("Lan khoi dong/thuc day: ");
    Serial.println(wakeCount);

    // ---------------------------------------------
    // Bước 1: Khởi động và đọc DHT22
    // ---------------------------------------------
    dht.begin();

    // Chờ cảm biến ổn định
    delay(2000);

    float humidity = dht.readHumidity();
    float temperature = dht.readTemperature();

    if (isnan(temperature) || isnan(humidity))
    {
        Serial.println("Loi: Khong doc duoc du lieu DHT22.");

        // Không để ESP32 chạy mãi khi cảm biến lỗi
        enterDeepSleep();
    }

    temperature += TEMPERATURE_OFFSET;
    humidity += HUMIDITY_OFFSET;

    Serial.println();
    Serial.println("Du lieu cam bien:");

    Serial.print("Nhiet do: ");
    Serial.print(temperature, 2);
    Serial.println(" do C");

    Serial.print("Do am: ");
    Serial.print(humidity, 2);
    Serial.println(" %");



    if (!connectWiFi())
    {
        enterDeepSleep();
    }


    if (!connectThingsBoard())
    {
        enterDeepSleep();
    }

   
    sendTelemetry(
        temperature,
        humidity
    );


    enterDeepSleep();
}

void loop()
{
    
}