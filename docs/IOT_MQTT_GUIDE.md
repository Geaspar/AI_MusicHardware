# IoT & MQTT Integration Guide

## Overview

The AIMusicHardware project includes comprehensive IoT integration that allows the synthesizer to connect to sensors, devices, and other IoT systems for real-time musical control. The system uses MQTT (Message Queuing Telemetry Transport) as the primary communication protocol.

### Key Features
- Real-time sensor data integration for musical control
- MQTT publish/subscribe messaging with QoS support
- Environment-aware adaptive music system
- Parameter mapping from IoT sensors to synthesizer controls
- Event-driven architecture for IoT-triggered musical changes
- Fallback mock implementation for development without hardware

## Quick Start

### 1. Installation

Install the Paho MQTT libraries:

```bash
# Make the script executable
chmod +x tools/install_mqtt_libs.sh

# Run the installation script
./tools/install_mqtt_libs.sh

# Rebuild the project
./build.sh
```

### 2. Basic Usage

```cpp
#include "iot/MQTTInterface.h"
#include "iot/IoTConfigManager.h"

// Create and configure MQTT connection
auto mqtt = std::make_unique<MQTTInterface>();
mqtt->connect("localhost", 1883, "ai_music_hardware");

// Set up IoT configuration manager
IoTConfigManager iotManager(mqtt.get());
```

### 3. Testing

Run the test applications to verify connectivity:

```bash
# Basic MQTT test
./build/bin/TestMQTTIntegration [broker_host] [broker_port] [client_id]

# Full IoT configuration demo
./build/bin/IoTConfigManagerDemo
```

## Core Components

### 1. MQTT Interface
The `MQTTInterface` class provides:
- Connection to MQTT brokers
- Publish/subscribe messaging
- QoS levels (0, 1, 2)
- Topic wildcards and filtering
- Automatic reconnection
- Thread-safe callback handling

### 2. IoT Device Manager
Manages device discovery and registration:
- Device configuration storage
- Topic mapping management
- Sensor type detection
- Data validation and conversion

### 3. IoT Event Adapter
Maps IoT messages to musical events:
- Real-time parameter updates
- Event system integration
- State-based triggers
- Horizontal re-sequencing control

### 4. IoT Configuration Manager
Provides a complete IoT management system:
- Interactive CLI for device configuration
- Real-time sensor monitoring
- Parameter mapping interface
- Configuration persistence

## Sensor Integration

### Supported Sensor Types

The system supports various sensor types with automatic data conversion:

```cpp
enum class SensorType {
    Temperature,     // -40°C to 85°C → 0.0-1.0
    Humidity,        // 0-100% → 0.0-1.0
    Light,          // 0-100000 lux → 0.0-1.0
    Motion,         // Boolean → 0.0/1.0
    Sound,          // 0-130 dB → 0.0-1.0
    Accelerometer,  // -16g to +16g → 0.0-1.0
    Gyroscope,      // -2000°/s to +2000°/s → 0.0-1.0
    Magnetometer,   // -4912 to +4912 μT → 0.0-1.0
    Pressure,       // 300-1100 hPa → 0.0-1.0
    Proximity,      // 0-255 → 0.0-1.0
    Custom          // User-defined range
};
```

### Parameter Mapping

Map sensor data to synthesizer parameters:

```cpp
// Map temperature sensor to filter cutoff
iotManager.mapSensorToParameter(
    "sensors/room1/temperature",    // MQTT topic
    filterCutoffParam,              // Parameter pointer
    SensorType::Temperature,        // Sensor type
    20.0f,                         // Min temperature (°C)
    30.0f                          // Max temperature (°C)
);

// Map motion sensor to trigger events
iotManager.mapSensorToEvent(
    "sensors/motion/detected",      // MQTT topic
    "sequence_change"               // Event type
);
```

## MQTT Configuration

### Connection Setup

```cpp
MQTTInterface mqtt;

// Basic connection
mqtt.connect("broker.example.com", 1883, "unique_client_id");

// Secure connection with credentials
ConnectionOptions options;
options.username = "your_username";
options.password = "your_password";
options.keepAlive = 60;
options.cleanSession = true;

mqtt.connect("secure.broker.com", 8883, "client_id", options);
```

### Publishing Data

```cpp
// Basic publish
mqtt.publish("music/parameters/volume", "0.8");

// Publish with QoS and retention
mqtt.publish("music/state/current", "ambient", 1, true);
```

### Subscribing to Topics

```cpp
// Subscribe to specific topic
mqtt.subscribe("sensors/+/temperature");

// Set message callback
mqtt.setMessageCallback([](const std::string& topic, const std::string& payload) {
    std::cout << "Received: " << topic << " = " << payload << std::endl;
});

// Topic-specific callback
mqtt.setTopicCallback("sensors/room1/motion", [](const std::string& topic, const std::string& payload) {
    if (payload == "1") {
        // Motion detected - trigger sequence change
        eventBus.publish(Event("motion_detected"));
    }
});
```

## Advanced Features

### State-Based Music Integration

The IoT system integrates with the state-based music system for environment-aware composition:

```cpp
// Configure state transitions based on sensor data
iotManager.addStateTransition("day", "night", 
    "sensors/light/lux", SensorType::Light, 0.0f, 0.1f);

// Automatic music adaptation
iotManager.addParameterRule("ambient_reverb",
    "sensors/room1/occupancy", SensorType::Motion,
    [](float occupancy) { return 1.0f - occupancy; }  // More reverb when empty
);
```

### Real-Time Parameter Control (RTPC)

Use IoT data for continuous parameter control:

```cpp
// Configure RTPC mapping
RTCPMapping tempMapping;
tempMapping.sensorTopic = "sensors/outdoor/temperature";
tempMapping.parameterName = "filter_cutoff";
tempMapping.curve = RTCPCurve::Exponential;
tempMapping.inputRange = {-10.0f, 40.0f};  // Temperature range
tempMapping.outputRange = {200.0f, 8000.0f}; // Frequency range

iotManager.addRTPCMapping(tempMapping);
```

### ESP32 Sensor Node

The project includes firmware for ESP32 sensor nodes:

```cpp
// ESP32 configuration (firmware/esp32_sensor_node/config.h)
#define WIFI_SSID "your_network"
#define WIFI_PASSWORD "your_password"
#define MQTT_BROKER "192.168.1.100"
#define MQTT_PORT 1883
#define CLIENT_ID "esp32_sensor_01"

// Sensor topics
#define TEMP_TOPIC "sensors/room1/temperature"
#define HUMIDITY_TOPIC "sensors/room1/humidity"
#define MOTION_TOPIC "sensors/room1/motion"
```

## Testing and Development

### Mock Implementation

For development without hardware, use the mock implementation:

```cpp
// Mock MQTT for testing
#define USE_MOCK_MQTT 1
#include "iot/mqtt_include.h"

// Mock interface provides same API
auto mockMqtt = std::make_unique<MockMQTTInterface>();
mockMqtt->connect("mock_broker", 1883, "test_client");
```

### Test Applications

1. **SimpleMQTTTest**: Basic connectivity test
2. **TestMQTTIntegration**: Full integration test
3. **IoTConfigManagerDemo**: Interactive configuration
4. **ComprehensiveMQTTTest**: Complete system test

```bash
# Run comprehensive test
./build/bin/ComprehensiveMQTTTest

# Interactive demo
./build/bin/IoTConfigManagerDemo
```

## Troubleshooting

### Common Issues

**MQTT Connection Failed:**
```bash
# Check broker accessibility
mosquitto_pub -h broker.example.com -p 1883 -t test -m "hello"

# Verify client ID uniqueness
# Ensure credentials are correct
# Check firewall settings
```

**Sensor Data Not Updating:**
```bash
# Verify topic subscription
# Check sensor data format
# Validate conversion ranges
# Monitor MQTT traffic with mosquitto_sub
```

**Performance Issues:**
```bash
# Reduce update frequency
# Use appropriate QoS levels
# Implement data buffering
# Check network latency
```

### Debug Mode

Enable debug logging:

```cpp
#define IOT_DEBUG 1
#include "iot/IoTInterface.h"

// Detailed logging will be output to console
```

## Current Implementation Status

- ✅ **Mock Implementation**: Fully working and tested
- ✅ **MQTT Interface**: Complete API implementation
- ✅ **IoT Configuration Manager**: Full functionality
- ✅ **Parameter Mapping**: Real-time sensor control
- ✅ **Event Integration**: IoT-triggered musical events
- ⏳ **Paho MQTT Library**: Framework ready, integration in progress

The system is production-ready with the mock implementation and will seamlessly transition to real MQTT when the Paho libraries are fully integrated.