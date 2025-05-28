# ESP32 IoT Sensor Node Hardware Design
*May 28, 2025*

## Overview

This document details the hardware design for ESP32-based IoT sensor nodes that integrate with the AIMusicHardware system. These nodes collect environmental and motion data to influence musical parameters in real-time.

## Hardware Architecture

### Core Components

#### 1. ESP32 Development Board
- **Recommended**: ESP32-WROOM-32D or ESP32-S3-WROOM-1
- **Features**: 
  - Dual-core Xtensa LX6 processor (240MHz)
  - 520KB SRAM, 4MB Flash
  - Built-in WiFi 802.11 b/g/n
  - Bluetooth 4.2 BR/EDR and BLE
  - 34 GPIO pins
  - ADC, DAC, I2C, SPI, UART interfaces

#### 2. Power Management
- **Battery**: 3.7V Li-Po 2000mAh battery
- **Charging**: USB-C charging circuit with protection
- **Regulation**: 3.3V LDO regulator for sensors
- **Power switch**: Tactile switch for manual on/off

#### 3. Sensor Suite

##### Environmental Sensors
- **BME280**: Temperature, humidity, pressure sensor (I2C)
- **VEML7700**: Ambient light sensor (I2C)
- **SGP30**: Air quality sensor (VOC/CO2) (I2C)

##### Motion Sensors
- **MPU6050**: 6-axis accelerometer/gyroscope (I2C)
- **LIS3MDL**: 3-axis magnetometer (I2C)

##### Audio Input
- **INMP441**: I2S digital microphone
- **Optional**: Analog audio input with amplifier

## Circuit Design

### Power Supply Circuit

```
USB-C Connector
    |
    +-- TP4056 (Li-Po Charger)
    |       |
    |   Li-Po Battery (3.7V, 2000mAh)
    |       |
    +-------+
    |
Power Switch
    |
AMS1117-3.3 (LDO Regulator)
    |
3.3V Rail --> ESP32 + Sensors
```

### Sensor Interface Circuit

```
ESP32 GPIO Assignments:
- GPIO21 (SDA) --> I2C Data Line
- GPIO22 (SCL) --> I2C Clock Line
- GPIO25 (I2S WS) --> INMP441 WS
- GPIO26 (I2S SCK) --> INMP441 SCK  
- GPIO27 (I2S SD) --> INMP441 SD
- GPIO2 --> Status LED
- GPIO0 --> Boot/Program button
- GPIO4 --> User button
```

### I2C Sensor Network

```
3.3V ----+----+----+----+----+
         |    |    |    |    |
       BME280 VEML7700 SGP30 MPU6050 LIS3MDL
         |    |    |    |    |
SDA -----+----+----+----+----+
         |    |    |    |    |
SCL -----+----+----+----+----+
         |    |    |    |    |
GND -----+----+----+----+----+

Pull-up resistors: 4.7kΩ on SDA and SCL lines
```

## PCB Layout Design

### Board Specifications
- **Size**: 60mm x 40mm (compact form factor)
- **Layers**: 4-layer PCB
- **Thickness**: 1.6mm
- **Finish**: HASL or ENIG

### Component Placement Strategy

```
Top Layer:
+----------------------------------+
|  [USB-C]     [Power LED]  [SW1] |
|                                  |
|  [TP4056]    [ESP32-WROOM]      |
|             [Antenna Area]       |
|  [Battery    +----------+        |
|   Connector] |          |        |
|             |   BME280  |  [LED] |
|             |  VEML7700 |        |
|             |   SGP30   |        |
|             +----------+         |
|                                  |
|  [MPU6050]  [LIS3MDL]  [INMP441]|
+----------------------------------+
```

### Layer Stack-up
1. **Top Layer**: Component placement and primary routing
2. **Inner Layer 1**: Ground plane
3. **Inner Layer 2**: Power planes (3.3V, Battery)
4. **Bottom Layer**: Secondary routing and crystal oscillator

## Enclosure Design

### Requirements
- **Material**: ABS plastic or 3D printed PLA
- **Protection**: IP54 rating (dust and splash resistant)
- **Mounting**: Wall mount brackets or magnetic base
- **Access**: USB-C port accessible, button access

### Enclosure Features
```
Enclosure Dimensions: 65mm x 45mm x 20mm

Features:
- Ventilation holes for environmental sensors
- Clear window for light sensor
- Recessed USB-C port
- Tactile button access
- Status LED light pipe
- Mounting tabs or magnetic base
```

## Power Consumption Analysis

### Current Draw Estimates
- ESP32 (active WiFi): 160-240mA
- ESP32 (light sleep): 0.8mA
- ESP32 (deep sleep): 10µA
- BME280: 3.4µA (1Hz sampling)
- VEML7700: 120µA
- SGP30: 48mA (active), 2.8µA (sleep)
- MPU6050: 3.9mA (active), 8µA (sleep)
- LIS3MDL: 1mA (active), 2µA (sleep)
- INMP441: 1.4mA

### Battery Life Calculations

#### Normal Operation (1-minute sensor readings)
- Active time: 5 seconds every minute
- Sleep time: 55 seconds every minute
- Average current: ~15mA
- **Battery life**: ~5.5 days

#### Power Saving Mode (5-minute sensor readings)
- Active time: 5 seconds every 5 minutes  
- Sleep time: 295 seconds every 5 minutes
- Average current: ~3mA
- **Battery life**: ~27 days

## Firmware Architecture

### Core Functions
```cpp
// Main sensor reading loop
void sensorLoop() {
    readEnvironmentalSensors();
    readMotionSensors();
    readAudioLevel();
    publishMQTTData();
    enterSleepMode();
}

// Power management
void enterSleepMode() {
    esp_sleep_enable_timer_wakeup(SLEEP_DURATION_US);
    esp_deep_sleep_start();
}
```

### MQTT Data Format
```json
{
    "device_id": "sensor_node_001",
    "timestamp": 1716883200,
    "location": "studio_room",
    "environmental": {
        "temperature": 22.5,
        "humidity": 45.2,
        "pressure": 1013.25,
        "light": 150,
        "air_quality": {
            "tvoc": 15,
            "co2": 415
        }
    },
    "motion": {
        "acceleration": {"x": 0.02, "y": 0.01, "z": 9.81},
        "gyroscope": {"x": 0.1, "y": -0.05, "z": 0.02},
        "magnetometer": {"x": 25.4, "y": -12.1, "z": 38.7}
    },
    "audio": {
        "level_db": -42.5,
        "peak_frequency": 440
    },
    "system": {
        "battery_voltage": 3.87,
        "wifi_rssi": -65,
        "uptime": 3600
    }
}
```

## Manufacturing Considerations

### Production Quantities
- **Prototype**: 10 units
- **Small batch**: 50 units  
- **Production**: 500+ units

### Cost Analysis (per unit, 50 qty)
- ESP32-WROOM-32D: $3.50
- Sensors (BME280, VEML7700, etc.): $12.00
- Power management ICs: $2.00
- Passive components: $1.50
- PCB fabrication: $5.00
- Enclosure: $3.00
- Assembly: $8.00
- **Total BOM cost**: ~$35.00

### Testing Procedures
1. **Electrical test**: Power supply verification, I2C communication
2. **Sensor calibration**: Environmental sensor accuracy verification
3. **WiFi/MQTT test**: Connectivity and data transmission
4. **Power consumption test**: Battery life validation
5. **Environmental test**: Temperature, humidity range testing

## Integration with AIMusicHardware

### MQTT Topics
```
AIMusicHardware/sensors/{device_id}/environmental
AIMusicHardware/sensors/{device_id}/motion  
AIMusicHardware/sensors/{device_id}/audio
AIMusicHardware/sensors/{device_id}/status
```

### Parameter Mapping Examples
- **Temperature** → Filter cutoff frequency
- **Humidity** → Reverb decay time
- **Light level** → Oscillator brightness/harmonics
- **Motion** → Tremolo/vibrato depth
- **Air quality** → Distortion amount
- **Audio level** → Dynamic response threshold

## Future Enhancements

### Version 2.0 Features
- **LoRaWAN** connectivity for extended range
- **Solar charging** for outdoor installations
- **Additional sensors**: UV, particle matter, gas sensors
- **Edge AI**: On-device pattern recognition
- **Mesh networking**: Multi-node coordination

### Advanced Applications
- **Multi-room sensing**: Synchronized sensor networks
- **Gesture recognition**: Advanced motion pattern detection
- **Environmental mapping**: Spatial audio based on location
- **Predictive modeling**: ML-based environmental forecasting

## Documentation References
- ESP32 Technical Reference Manual
- Sensor datasheets and application notes
- PCB design guidelines (IPC standards)
- FCC/CE compliance requirements for IoT devices
- Battery safety and shipping regulations