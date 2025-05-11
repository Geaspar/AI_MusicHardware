# MQTT Interface Implementation

This document provides details about the implementation of the MQTT interface in the AIMusicHardware project, including the mock implementation for environments without the Paho MQTT libraries.

## Overview

The MQTT interface provides communication with MQTT brokers, enabling the AIMusicHardware system to interact with IoT devices and sensors. The implementation includes:

1. A complete implementation using the Paho MQTT C++ client libraries
2. A fallback mock implementation when the Paho libraries are not available
3. Conditional compilation to select the appropriate implementation

## Installation Requirements

To use the full MQTT functionality, the Paho MQTT C and C++ libraries must be installed. We provide an installation script at `./tools/install_mqtt_libs.sh`.

## Fallback Mock Implementation

The fallback implementation is provided through the `mqtt_include.h` file. This file contains mock implementations of the key MQTT classes when the real libraries are not present.

### Key Features of the Mock Implementation

- Provides the same interface as the real Paho MQTT client
- Outputs informative messages to the console
- Operates entirely in-memory without actual network connections
- Allows code that depends on MQTT to compile and run without the real libraries

### Class Structure of the Mock Implementation

1. **mqtt::message**: Represents an MQTT message with topic, payload, QoS, and retention flag
2. **mqtt::token**: Represents an operation token for tracking asynchronous operations
3. **mqtt::delivery_token**: Specialization of token for message delivery tracking
4. **mqtt::callback**: Interface for handling message arrival and connection events
5. **mqtt::connect_options**: Configuration options for MQTT connections
6. **mqtt::async_client**: The main client class for MQTT communication

## Conditional Compilation

The implementation uses preprocessor directives to select either the real Paho MQTT implementation or the mock implementation:

```cpp
#if defined(HAVE_PAHO_MQTT) && !defined(DISABLE_MQTT)
// Use real Paho MQTT library
#include <mqtt/async_client.h>
#include <mqtt/topic.h>
#include <mqtt/message.h>
#include <mqtt/connect_options.h>
#else
// Use mock implementation
namespace mqtt {
    // Mock classes defined here
}
#endif
```

## MQTTInterface Class

The `MQTTInterface` class implements the `IoTInterface` and provides MQTT-specific functionality:

```cpp
namespace AIMusicHardware {

class MQTTInterface : public IoTInterface {
public:
    MQTTInterface();
    ~MQTTInterface();
    
    // IoTInterface implementation
    bool connect(const std::string& host, int port, const std::string& clientId) override;
    void disconnect() override;
    bool isConnected() const override;
    void update() override;
    bool subscribe(const std::string& topic) override;
    bool unsubscribe(const std::string& topic) override;
    bool publish(const std::string& topic, const std::string& payload) override;
    bool publish(const std::string& topic, const std::string& payload, int qos, bool retain) override;
    void setMessageCallback(MessageCallback callback) override;
    
    // MQTT-specific features
    void setConnectionOptions(int keepAliveInterval, bool cleanSession, bool automaticReconnect);
    void setLastWill(const std::string& topic, const std::string& payload, int qos, bool retained);
    void setDefaultQoS(int qos);
    
private:
    // MQTT client implementation
    std::unique_ptr<mqtt::async_client> client_;
    std::unique_ptr<mqtt::connect_options> connectOptions_;
    
    // Connection details
    std::string host_;
    int port_;
    std::string clientId_;
    bool isConnected_ = false;
    
    // Message handling
    MessageCallback globalMessageCallback_;
    std::map<std::string, MessageCallback> topicCallbacks_;
    
    // Connection options
    int keepAliveInterval_ = 60;  // seconds
    bool cleanSession_ = true;
    bool automaticReconnect_ = true;
    
    // Last Will and Testament
    bool hasLastWill_ = false;
    std::string lastWillTopic_;
    std::string lastWillPayload_;
    int lastWillQoS_ = 0;
    bool lastWillRetained_ = false;
    
    // Default QoS
    int defaultQoS_ = 0;
    
    // Subscription tracking
    std::set<std::string> subscriptions_;
    
    // Thread safety
    mutable std::mutex mutex_;
    
    // Utility methods
    std::string getServerURI() const;
    void onMessage(const mqtt::const_message_ptr& msg);
    bool matchTopicPattern(const std::string& topic, const std::string& pattern) const;
};

} // namespace AIMusicHardware
```

## Usage Examples

### Basic Usage

```cpp
#include "MQTTInterface.h"

// Create MQTT interface
AIMusicHardware::MQTTInterface mqtt;

// Set connection options
mqtt.setConnectionOptions(60, true, true);
mqtt.setDefaultQoS(1);
mqtt.setLastWill("device/status", "offline", 1, true);

// Connect to broker
if (mqtt.connect("localhost", 1883, "AIMusicDevice")) {
    // Publish online status
    mqtt.publish("device/status", "online", 1, true);
    
    // Subscribe to control topic
    mqtt.subscribe("device/control");
    
    // Set up message callback
    mqtt.setMessageCallback([](const std::string& topic, const std::string& payload) {
        std::cout << "Received: " << topic << " -> " << payload << std::endl;
        
        // Process control commands
        if (topic == "device/control") {
            if (payload == "start") {
                // Start operation
            } else if (payload == "stop") {
                // Stop operation
            }
        }
    });
    
    // Main loop
    while (true) {
        // Process MQTT messages
        mqtt.update();
        
        // Do other work
        // ...
        
        // Sleep briefly
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    // Disconnect when done
    mqtt.disconnect();
}
```

### Using Topic-Specific Callbacks

```cpp
// Set up topic-specific callback
mqtt.setTopicCallback("sensors/temperature", [](const std::string& topic, const std::string& payload) {
    float temperature = std::stof(payload);
    std::cout << "Temperature: " << temperature << "°C" << std::endl;
    
    // Update parameter
    temperatureParameter->setValue(temperature);
});

// Set up another topic-specific callback
mqtt.setTopicCallback("sensors/humidity", [](const std::string& topic, const std::string& payload) {
    float humidity = std::stof(payload);
    std::cout << "Humidity: " << humidity << "%" << std::endl;
    
    // Update parameter
    humidityParameter->setValue(humidity);
});
```

## Working with the Mock Implementation

The mock implementation is used automatically when the Paho MQTT libraries are not available or when `DISABLE_MQTT` is defined.

To explicitly use the mock implementation in a test file:

```cpp
// Define DISABLE_MQTT to force the use of our mock implementation
#define DISABLE_MQTT 1

// Define HAVE_PAHO_MQTT to enable the mock implementation
#define HAVE_PAHO_MQTT 1

#include "MQTTInterface.h"

// Create and use the interface as normal
AIMusicHardware::MQTTInterface mqtt;
mqtt.connect("localhost", 1883, "TestClient");
```

## Testing the MQTT Implementation

Several test applications are provided to verify the MQTT functionality:

1. **SimpleMQTTTest**: Tests the basic MQTT client functionality using the mock implementation
2. **SimpleMQTTInterfaceTest**: Tests the MQTTInterface class with the mock implementation
3. **TestMQTTIntegration**: Tests integration with the IoT system, including device discovery and event mapping

To run the tests:

```bash
# Build and run the basic test
cd build
cmake ..
make SimpleMQTTTest
./bin/SimpleMQTTTest

# Build and run the interface test
make SimpleMQTTInterfaceTest
./bin/SimpleMQTTInterfaceTest

# Build and run the integration test
make TestMQTTIntegration
./bin/TestMQTTIntegration
```

## Troubleshooting

If you encounter issues with the MQTT implementation:

1. **Check Library Availability**: Verify if the Paho MQTT libraries are installed by running `pkg-config --modversion paho-mqtt3a paho-mqttpp3`

2. **Check for Compilation Errors**: Look for errors related to the MQTT include files or linking errors

3. **Test with Mock Implementation**: Try using the mock implementation by defining `DISABLE_MQTT` to isolate issues

4. **Check Console Output**: The mock implementation outputs detailed logs to the console, which can help in debugging

5. **Install Libraries**: If needed, install the libraries using the provided script: `./tools/install_mqtt_libs.sh`

## Implementation Details

### Message Handling Process

1. The `MQTTInterface` subscribes to topics and sets up callbacks
2. When a message arrives, the Paho client invokes the `messageArrived` method
3. This method forwards the message to any registered topic-specific callbacks
4. If no topic-specific callback exists, the global callback is invoked
5. Callbacks are executed with the topic and payload

### Thread Safety

The implementation uses mutex locks to ensure thread safety:

```cpp
std::lock_guard<std::mutex> lock(mutex_);
```

This is important because MQTT callbacks may come from different threads than the main application thread.

### Reconnection Handling

The implementation includes automatic reconnection capabilities:

```cpp
void MQTTInterface::update() {
    // Check if we need to reconnect
    if (!isConnected() && !host_.empty()) {
        reconnect();
    }
}

void MQTTInterface::reconnect() {
    // Only attempt reconnection if we have connection details
    if (host_.empty() || clientId_.empty()) {
        return;
    }
    
    try {
        // Attempt to reconnect
        connect(host_, port_, clientId_);
    }
    catch (const std::exception& e) {
        std::cerr << "Reconnection attempt failed: " << e.what() << std::endl;
    }
}
```

## Conclusion

The MQTT interface implementation provides a robust foundation for IoT communication in the AIMusicHardware project. The dual implementation approach (real and mock) ensures that the code can run in any environment, with or without the Paho MQTT libraries installed.