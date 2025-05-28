/**
 * ESP32 Sensor Node Configuration
 * AIMusicHardware Project
 */

#ifndef CONFIG_H
#define CONFIG_H

// WiFi Configuration
#define WIFI_SSID "AIMusicHardware_Network"
#define WIFI_PASSWORD "YourWiFiPassword"

// MQTT Configuration
#define MQTT_BROKER "192.168.1.100"  // IP of your MQTT broker
#define MQTT_PORT 1883
#define MQTT_USER "aimusicuser"
#define MQTT_PASSWORD "aimusicpass"

// Sensor Configuration
#define BME280_ADDRESS 0x76
#define VEML7700_ADDRESS 0x10
#define MPU6050_ADDRESS 0x68

// Power Management
#define LOW_BATTERY_THRESHOLD 3.3  // Volts
#define CRITICAL_BATTERY_THRESHOLD 3.0  // Volts

// Sleep Configuration
#define NORMAL_SLEEP_MINUTES 1
#define POWER_SAVE_SLEEP_MINUTES 5
#define CRITICAL_SLEEP_MINUTES 15

// Audio Configuration
#define AUDIO_SAMPLE_RATE 44100
#define AUDIO_BITS_PER_SAMPLE 32
#define AUDIO_BUFFER_SIZE 1024

// LED Patterns
#define LED_STARTUP_BLINKS 3
#define LED_NORMAL_BLINK_MS 50
#define LED_ERROR_BLINK_MS 200

// Sensor Calibration
#define CALIBRATION_SAMPLES 100
#define TEMPERATURE_OFFSET 0.0  // Celsius
#define HUMIDITY_OFFSET 0.0     // %RH
#define PRESSURE_OFFSET 0.0     // hPa

// Device Information
#define FIRMWARE_VERSION "1.0.0"
#define HARDWARE_VERSION "1.0"
#define MANUFACTURER "AIMusicHardware"

#endif // CONFIG_H