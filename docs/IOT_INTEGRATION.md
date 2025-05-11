# IoT Integration Guide

This guide explains how to use the IoT integration features in the AIMusicHardware project. The integration allows connecting to IoT sensors, controllers, and other devices to create interactive musical environments.

## Overview

The IoT integration system consists of several components:

1. **MQTT Interface**: Handles communication with MQTT brokers
2. **IoT Device Manager**: Manages device discovery and registration
3. **IoT Event Adapter**: Maps IoT messages to events and parameters
4. **IoT Configuration Manager**: Manages device configurations and topic mappings

## Installation

To use the IoT integration features, you need to install the Paho MQTT C and C++ libraries. We provide a script to install these libraries locally within the project:

```bash
# Make the script executable
chmod +x tools/install_mqtt_libs.sh

# Run the installation script
./tools/install_mqtt_libs.sh
```

This script will:
1. Clone the Paho MQTT C and C++ libraries from GitHub
2. Build and install them locally in the vendor directory
3. Configure them to work with the project

After installation, rebuild the project:

```bash
./build.sh
```

## Testing the IoT Integration

We provide two test applications to verify that the IoT integration is working:

1. **TestMQTTIntegration**: Basic test of MQTT connectivity and message handling
2. **IoTConfigManagerDemo**: Full demonstration of the IoT Configuration Manager with interactive CLI

To run the basic test:

```bash
./build/bin/TestMQTTIntegration [broker_host] [broker_port] [client_id]
```

To run the full demo:

```bash
./build/bin/IoTConfigManagerDemo [broker_host] [broker_port]
```

## Components

### MQTTInterface

The `MQTTInterface` class provides a C++ wrapper around the Paho MQTT library with these features:

- Connect to MQTT brokers with configurable options
- Subscribe to topics with wildcard support
- Publish messages with QoS and retention options
- Set up callback handlers for incoming messages
- Configure Last Will and Testament messages

Example usage:

```cpp
// Create MQTT interface
MQTTInterface mqttInterface;

// Set connection options
mqttInterface.setConnectionOptions(60, true, true);
mqttInterface.setDefaultQoS(1);
mqttInterface.setLastWill("device/status", "offline", 1, true);

// Connect to broker
mqttInterface.connect("localhost", 1883, "MyClientID");

// Subscribe to topics
mqttInterface.subscribe("sensors/#");

// Set up message callback
mqttInterface.setMessageCallback([](const std::string& topic, const std::string& payload) {
    std::cout << "Received message on " << topic << ": " << payload << std::endl;
});

// Publish a message
mqttInterface.publish("device/status", "online", 1, true);
```

### IoTDevice

The `IoTDevice` class represents an IoT device with its capabilities and metadata:

- Device identity (ID, name, type)
- Device metadata (model, manufacturer, firmware)
- Communication topics
- Device capabilities
- Connection status

Example:

```cpp
// Create a device
IoTDevice device("temp_sensor", "Temperature Sensor", IoTDevice::Type::SENSOR);
device.setModel("DHT22");
device.setManufacturer("AIMusicHardware");
device.setFirmwareVersion("1.0.0");

// Add topics
device.addTopic("temp_sensor/temperature");
device.addTopic("temp_sensor/humidity");
device.addTopic("temp_sensor/status");

// Add capabilities
device.addCapability("temperature", "celsius");
device.addCapability("humidity", "percentage");

// Convert to JSON for storage or transmission
nlohmann::json deviceJson = device.toJson();
```

### IoTEventAdapter

The `IoTEventAdapter` connects IoT messages to the event and parameter system:

- Map IoT topics to events
- Map IoT topics to parameters
- Convert message payloads to appropriate data types
- Apply mapping functions to sensor values

Example:

```cpp
// Create event adapter
IoTEventAdapter adapter(&mqttInterface, &eventBus);

// Map temperature topic to parameter
FloatParameter tempParam("temperature", 0.0f, 100.0f, 20.0f);
adapter.mapTopicToParameter("temp_sensor/temperature", &tempParam);

// Map motion topic to event
adapter.mapTopicToEvent("motion_sensor/motion", "motion_detected");

// Set up converter for temperature readings
adapter.registerSensorType("temp_sensor/temperature", 
                         IoTParameterConverter::SensorType::TEMPERATURE,
                         0.0f, 40.0f, true);

// Start processing messages
adapter.start();
```

### IoTConfigManager

The `IoTConfigManager` manages device discovery and configuration:

- Discover new devices via discovery topics
- Keep track of device status (connected/disconnected)
- Save and load device configurations
- Apply topic mappings based on device capabilities

Example:

```cpp
// Create config manager
IoTConfigManager configManager(&mqttInterface, &eventAdapter);

// Set discovery topics
configManager.setDiscoveryTopics({"discovery/#", "homeassistant/+/+/config"});

// Start discovery
configManager.startDiscovery();

// Set up discovery callback
configManager.setDeviceDiscoveryCallback([](const IoTDevice& device) {
    std::cout << "New device discovered: " << device.getName() << std::endl;
});

// Find devices with specific capabilities
auto temperatureSensors = configManager.findDevicesByCapability("temperature");
```

## Mapping IoT Data to Synthesis Parameters

One of the most powerful features is mapping IoT sensor data to synthesis parameters. This creates interactive sound environments that respond to real-world conditions.

Example mappings:

1. **Temperature to Filter Cutoff**:
   - Higher temperatures open the filter
   - Lower temperatures close the filter

2. **Light Sensor to Reverb Amount**:
   - Darker environments have more reverb
   - Brighter environments have less reverb

3. **Motion Sensor to Sequencer Tempo**:
   - More motion increases tempo
   - Less motion decreases tempo

4. **Humidity to Oscillator Detune**:
   - Higher humidity increases detune for a thicker sound
   - Lower humidity decreases detune for a cleaner sound

To implement these mappings:

```cpp
// Map temperature to filter cutoff
FloatParameter cutoffParam("filter_cutoff", 100.0f, 8000.0f, 1000.0f);
adapter.mapTopicToParameter("temp_sensor/temperature", &cutoffParam);
adapter.registerSensorType("temp_sensor/temperature", 
                         IoTParameterConverter::SensorType::TEMPERATURE,
                         0.0f, 40.0f, true);

// Apply exponential mapping for better musical response
adapter.setMappingMode("temp_sensor/temperature", 
                      IoTParameterMappings::MappingMode::EXPONENTIAL, 
                      0.0f, 2.0f);
```

## Advanced Features

### Custom Parameter Converters

You can create custom converters for special sensor data formats:

```cpp
// Custom converter for a special temperature format
auto customConverter = [](const std::string& payload) {
    // Example: Format is "T:25.5C"
    std::regex tempRegex("T:([0-9.]+)C");
    std::smatch match;
    if (std::regex_search(payload, match, tempRegex) && match.size() > 1) {
        return std::stof(match[1].str());
    }
    return 20.0f; // Default room temperature
};

adapter.setParameterConverter("custom_temp_sensor/temperature", customConverter);
```

### Device Discovery

The system supports auto-discovery of IoT devices following common standards:

1. **Home Assistant Discovery**: 
   - Topic format: `homeassistant/<component>/<node_id>/<object_id>/config`
   - JSON payload contains device configuration

2. **Homie Convention**: 
   - Topic format follows the Homie specification
   - Self-describing device structure

3. **Custom Discovery**:
   - Topic format: `discovery/devices`
   - JSON payload following our device JSON format

To customize discovery topics:

```cpp
configManager.setDiscoveryTopics({
    "discovery/#",                  // Custom discovery
    "homeassistant/+/+/config",     // Home Assistant
    "homie/#"                       // Homie convention
});
```

## Use Cases

1. **Responsive Stage Environment**:
   - Light sensors control visual effects
   - Motion sensors trigger sequence changes
   - Temperature/humidity affect timbre

2. **Smart Home Integration**:
   - Connect to existing smart home devices
   - Generate ambient music based on home state
   - Use music as notification for events

3. **Interactive Installations**:
   - Public spaces with sensor-driven music
   - Collaborative musical environments
   - Data sonification for environmental monitoring

4. **Recording Studio Integration**:
   - Room acoustics analysis and compensation
   - MIDI remote control via IoT
   - Multi-room synchronized playback

## Troubleshooting

### MQTT Connection Issues

If you can't connect to the MQTT broker:

1. Check that the broker is running and accessible
2. Verify port settings (default is 1883 for unencrypted, 8883 for SSL)
3. Check firewall settings
4. Try a different client ID (some brokers restrict ID formats)

### Missing MQTT Libraries

If you get compilation errors about missing MQTT headers:

1. Run the installation script: `./tools/install_mqtt_libs.sh`
2. Check that the libraries were installed in `vendor/paho.mqtt.c` and `vendor/paho.mqtt.cpp`
3. Ensure CMake can find the libraries by checking CMake output

### Device Discovery Problems

If devices aren't being discovered:

1. Check that the device is publishing to the correct discovery topics
2. Verify the JSON format matches what is expected
3. Enable debug output to see what messages are being received
4. Try manually adding the device with `configManager.addDevice(device)`

## Future Enhancements

1. **Security Features**:
   - TLS/SSL support for encrypted connections
   - Authentication with username/password or certificates
   - Access control for device operations

2. **Cloud Integration**:
   - Connect to cloud IoT platforms
   - Remote access to devices
   - Data logging and analysis

3. **Improved Discovery**:
   - Support for more discovery protocols
   - Better auto-configuration of devices
   - Plug-and-play experience

4. **Advanced Mapping**:
   - Multi-sensor fusion for parameter control
   - AI-assisted parameter mapping suggestions
   - Dynamic adaptation to sensor behavior