/**
 * ESP32 IoT Sensor Node Firmware
 * AIMusicHardware Project
 * 
 * This firmware collects environmental and motion sensor data
 * and transmits it via MQTT to the main music system.
 */

#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <Adafruit_BME280.h>
#include <Adafruit_VEML7700.h>
#include <MPU6050.h>
#include <driver/i2s.h>
#include <esp_sleep.h>
#include <esp_wifi.h>
#include "config.h"

// Hardware configuration
#define I2C_SDA 21
#define I2C_SCL 22
#define I2S_WS 25
#define I2S_SCK 26
#define I2S_SD 27
#define STATUS_LED 2
#define USER_BUTTON 4

// Sleep configuration
#define SLEEP_DURATION_MINUTES 1
#define SLEEP_DURATION_US (SLEEP_DURATION_MINUTES * 60 * 1000000ULL)

// Sensor objects
Adafruit_BME280 bme;
Adafruit_VEML7700 veml;
MPU6050 mpu;

// WiFi and MQTT
WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

// Device configuration
struct DeviceConfig {
    String deviceId;
    String location;
    String wifiSSID;
    String wifiPassword;
    String mqttBroker;
    int mqttPort;
    String mqttUser;
    String mqttPassword;
} config;

// Sensor data structure
struct SensorData {
    // Environmental
    float temperature;
    float humidity;
    float pressure;
    float lightLevel;
    uint16_t tvoc;
    uint16_t co2;
    
    // Motion
    float accelX, accelY, accelZ;
    float gyroX, gyroY, gyroZ;
    float magX, magY, magZ;
    
    // Audio
    float audioLevelDB;
    float peakFrequency;
    
    // System
    float batteryVoltage;
    int wifiRSSI;
    unsigned long uptime;
    
    // Timestamp
    unsigned long timestamp;
};

void setup() {
    Serial.begin(115200);
    Serial.println("ESP32 Sensor Node Starting...");
    
    // Initialize hardware
    initializeGPIO();
    initializeI2C();
    initializeSensors();
    
    // Load configuration
    loadConfiguration();
    
    // Connect to WiFi
    connectWiFi();
    
    // Connect to MQTT
    connectMQTT();
    
    // Send startup message
    sendStatusMessage("online");
    
    Serial.println("Setup complete. Starting sensor loop...");
}

void loop() {
    // Collect sensor data
    SensorData data = collectSensorData();
    
    // Publish data via MQTT
    publishSensorData(data);
    
    // Handle MQTT messages
    mqttClient.loop();
    
    // Check battery level
    checkBatteryLevel();
    
    // Enter sleep mode to save power
    enterSleepMode();
}

void initializeGPIO() {
    pinMode(STATUS_LED, OUTPUT);
    pinMode(USER_BUTTON, INPUT_PULLUP);
    
    // Flash LED to indicate startup
    for (int i = 0; i < 3; i++) {
        digitalWrite(STATUS_LED, HIGH);
        delay(200);
        digitalWrite(STATUS_LED, LOW);
        delay(200);
    }
}

void initializeI2C() {
    Wire.begin(I2C_SDA, I2C_SCL);
    Wire.setClock(100000); // 100kHz for reliability
    
    Serial.println("I2C initialized");
}

void initializeSensors() {
    // Initialize BME280 (Temperature, Humidity, Pressure)
    if (!bme.begin(0x76)) {
        Serial.println("Could not find BME280 sensor!");
    } else {
        Serial.println("BME280 sensor initialized");
    }
    
    // Initialize VEML7700 (Light sensor)
    if (!veml.begin()) {
        Serial.println("Could not find VEML7700 sensor!");
    } else {
        Serial.println("VEML7700 sensor initialized");
        veml.setGain(VEML7700_GAIN_1);
        veml.setIntegrationTime(VEML7700_IT_800MS);
    }
    
    // Initialize MPU6050 (Accelerometer/Gyroscope)
    mpu.initialize();
    if (mpu.testConnection()) {
        Serial.println("MPU6050 sensor initialized");
    } else {
        Serial.println("Could not find MPU6050 sensor!");
    }
    
    // Initialize I2S for audio input
    initializeI2S();
}

void initializeI2S() {
    i2s_config_t i2s_config = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
        .sample_rate = 44100,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT,
        .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
        .communication_format = I2S_COMM_FORMAT_I2S,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = 4,
        .dma_buf_len = 1024,
        .use_apll = false,
        .tx_desc_auto_clear = false,
        .fixed_mclk = 0
    };
    
    i2s_pin_config_t pin_config = {
        .bck_io_num = I2S_SCK,
        .ws_io_num = I2S_WS,
        .data_out_num = I2S_PIN_NO_CHANGE,
        .data_in_num = I2S_SD
    };
    
    i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);
    i2s_set_pin(I2S_NUM_0, &pin_config);
    
    Serial.println("I2S audio input initialized");
}

void loadConfiguration() {
    // In a real implementation, this would load from EEPROM or file system
    config.deviceId = "sensor_node_" + String(ESP.getEfuseMac(), HEX);
    config.location = "studio_room";
    config.wifiSSID = WIFI_SSID;
    config.wifiPassword = WIFI_PASSWORD;
    config.mqttBroker = MQTT_BROKER;
    config.mqttPort = MQTT_PORT;
    config.mqttUser = MQTT_USER;
    config.mqttPassword = MQTT_PASSWORD;
    
    Serial.println("Configuration loaded:");
    Serial.println("Device ID: " + config.deviceId);
    Serial.println("Location: " + config.location);
}

void connectWiFi() {
    WiFi.begin(config.wifiSSID.c_str(), config.wifiPassword.c_str());
    
    Serial.print("Connecting to WiFi");
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
        delay(500);
        Serial.print(".");
        attempts++;
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println();
        Serial.println("WiFi connected!");
        Serial.print("IP address: ");
        Serial.println(WiFi.localIP());
        Serial.print("RSSI: ");
        Serial.println(WiFi.RSSI());
    } else {
        Serial.println();
        Serial.println("WiFi connection failed!");
    }
}

void connectMQTT() {
    mqttClient.setServer(config.mqttBroker.c_str(), config.mqttPort);
    mqttClient.setCallback(mqttCallback);
    
    Serial.print("Connecting to MQTT broker");
    int attempts = 0;
    while (!mqttClient.connected() && attempts < 10) {
        Serial.print(".");
        
        String clientId = config.deviceId + "_" + String(random(0xffff), HEX);
        
        if (mqttClient.connect(clientId.c_str(), config.mqttUser.c_str(), config.mqttPassword.c_str())) {
            Serial.println();
            Serial.println("MQTT connected!");
            
            // Subscribe to control topics
            String controlTopic = "AIMusicHardware/control/" + config.deviceId;
            mqttClient.subscribe(controlTopic.c_str());
            
        } else {
            Serial.print("failed, rc=");
            Serial.print(mqttClient.state());
            delay(2000);
        }
        attempts++;
    }
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
    String message = "";
    for (unsigned int i = 0; i < length; i++) {
        message += (char)payload[i];
    }
    
    Serial.println("MQTT message received: " + String(topic) + " = " + message);
    
    // Parse control messages
    if (String(topic).endsWith("/control/" + config.deviceId)) {
        handleControlMessage(message);
    }
}

void handleControlMessage(String message) {
    DynamicJsonDocument doc(512);
    deserializeJson(doc, message);
    
    if (doc.containsKey("command")) {
        String command = doc["command"];
        
        if (command == "reboot") {
            Serial.println("Reboot command received");
            ESP.restart();
        } else if (command == "sleep") {
            int duration = doc["duration"] | 60; // Default 60 seconds
            Serial.println("Sleep command received for " + String(duration) + " seconds");
            esp_sleep_enable_timer_wakeup(duration * 1000000ULL);
            esp_deep_sleep_start();
        } else if (command == "calibrate") {
            Serial.println("Calibrate command received");
            calibrateSensors();
        }
    }
}

SensorData collectSensorData() {
    SensorData data;
    
    // Environmental sensors
    data.temperature = bme.readTemperature();
    data.humidity = bme.readHumidity();
    data.pressure = bme.readPressure() / 100.0F; // Convert to hPa
    data.lightLevel = veml.readLux();
    
    // Motion sensors
    int16_t ax, ay, az, gx, gy, gz;
    mpu.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);
    
    data.accelX = ax / 16384.0; // Convert to g
    data.accelY = ay / 16384.0;
    data.accelZ = az / 16384.0;
    data.gyroX = gx / 131.0; // Convert to degrees/sec
    data.gyroY = gy / 131.0;
    data.gyroZ = gz / 131.0;
    
    // Audio level measurement
    data.audioLevelDB = measureAudioLevel();
    data.peakFrequency = 440.0; // Placeholder - would need FFT for real implementation
    
    // System information
    data.batteryVoltage = analogRead(A0) * 3.3 / 4095.0 * 2; // Voltage divider
    data.wifiRSSI = WiFi.RSSI();
    data.uptime = millis();
    data.timestamp = WiFi.getTime();
    
    return data;
}

float measureAudioLevel() {
    const int sampleCount = 1024;
    int32_t samples[sampleCount];
    size_t bytesRead;
    
    // Read audio samples
    i2s_read(I2S_NUM_0, samples, sizeof(samples), &bytesRead, portMAX_DELAY);
    
    // Calculate RMS level
    float sum = 0;
    for (int i = 0; i < sampleCount; i++) {
        float sample = samples[i] / (float)INT32_MAX;
        sum += sample * sample;
    }
    
    float rms = sqrt(sum / sampleCount);
    float dB = 20 * log10(rms + 1e-10); // Add small value to avoid log(0)
    
    return dB;
}

void publishSensorData(const SensorData& data) {
    DynamicJsonDocument doc(1024);
    
    doc["device_id"] = config.deviceId;
    doc["timestamp"] = data.timestamp;
    doc["location"] = config.location;
    
    // Environmental data
    JsonObject env = doc.createNestedObject("environmental");
    env["temperature"] = data.temperature;
    env["humidity"] = data.humidity;
    env["pressure"] = data.pressure;
    env["light"] = data.lightLevel;
    
    // Motion data
    JsonObject motion = doc.createNestedObject("motion");
    JsonObject accel = motion.createNestedObject("acceleration");
    accel["x"] = data.accelX;
    accel["y"] = data.accelY;
    accel["z"] = data.accelZ;
    
    JsonObject gyro = motion.createNestedObject("gyroscope");
    gyro["x"] = data.gyroX;
    gyro["y"] = data.gyroY;
    gyro["z"] = data.gyroZ;
    
    // Audio data
    JsonObject audio = doc.createNestedObject("audio");
    audio["level_db"] = data.audioLevelDB;
    audio["peak_frequency"] = data.peakFrequency;
    
    // System data
    JsonObject system = doc.createNestedObject("system");
    system["battery_voltage"] = data.batteryVoltage;
    system["wifi_rssi"] = data.wifiRSSI;
    system["uptime"] = data.uptime;
    
    // Serialize and publish
    String jsonString;
    serializeJson(doc, jsonString);
    
    String topic = "AIMusicHardware/sensors/" + config.deviceId + "/data";
    
    if (mqttClient.publish(topic.c_str(), jsonString.c_str())) {
        Serial.println("Sensor data published successfully");
        digitalWrite(STATUS_LED, HIGH);
        delay(50);
        digitalWrite(STATUS_LED, LOW);
    } else {
        Serial.println("Failed to publish sensor data");
    }
}

void sendStatusMessage(String status) {
    DynamicJsonDocument doc(256);
    doc["device_id"] = config.deviceId;
    doc["status"] = status;
    doc["timestamp"] = WiFi.getTime();
    doc["firmware_version"] = "1.0.0";
    
    String jsonString;
    serializeJson(doc, jsonString);
    
    String topic = "AIMusicHardware/sensors/" + config.deviceId + "/status";
    mqttClient.publish(topic.c_str(), jsonString.c_str(), true); // Retained message
}

void checkBatteryLevel() {
    float voltage = analogRead(A0) * 3.3 / 4095.0 * 2;
    
    if (voltage < 3.3) { // Low battery threshold
        Serial.println("Low battery warning: " + String(voltage) + "V");
        
        // Send low battery alert
        DynamicJsonDocument doc(256);
        doc["device_id"] = config.deviceId;
        doc["alert"] = "low_battery";
        doc["voltage"] = voltage;
        doc["timestamp"] = WiFi.getTime();
        
        String jsonString;
        serializeJson(doc, jsonString);
        
        String topic = "AIMusicHardware/alerts/" + config.deviceId;
        mqttClient.publish(topic.c_str(), jsonString.c_str());
    }
}

void calibrateSensors() {
    Serial.println("Starting sensor calibration...");
    
    // BME280 doesn't require calibration - it's factory calibrated
    Serial.println("BME280: Factory calibrated");
    
    // VEML7700 auto-gain calibration
    veml.setGain(VEML7700_GAIN_1_8);
    delay(1000);
    float luxReading = veml.readLux();
    if (luxReading > 1000) {
        veml.setGain(VEML7700_GAIN_1_4);
    } else if (luxReading < 10) {
        veml.setGain(VEML7700_GAIN_2);
    }
    Serial.println("VEML7700: Auto-gain calibrated");
    
    // MPU6050 offset calibration
    mpu.CalibrateAccel(6);
    mpu.CalibrateGyro(6);
    Serial.println("MPU6050: Offset calibration complete");
    
    Serial.println("Sensor calibration complete");
}

void enterSleepMode() {
    Serial.println("Entering sleep mode for " + String(SLEEP_DURATION_MINUTES) + " minute(s)");
    
    // Disconnect WiFi to save power
    WiFi.disconnect();
    esp_wifi_stop();
    
    // Configure wake-up timer
    esp_sleep_enable_timer_wakeup(SLEEP_DURATION_US);
    
    // Enter deep sleep
    esp_deep_sleep_start();
}