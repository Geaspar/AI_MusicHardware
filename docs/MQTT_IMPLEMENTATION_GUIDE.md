# MQTT Implementation Guide

This guide provides a comprehensive explanation of the MQTT implementation in the AIMusicHardware project. Currently, it focuses on the working mock implementation, with the framework in place for future integration with the real Paho MQTT library.

> **Current Implementation Status**:
> - ✅ **Mock Implementation**: Fully working and tested
> - ⏳ **Real Paho MQTT Implementation**: Framework in place but not yet working
>
> When the Paho MQTT libraries are properly integrated:
> 1. Install the Paho MQTT libraries using `./tools/install_mqtt_libs.sh` (needs fixing)
> 2. Edit `include/iot/mqtt_include.h` and remove `&& 0` from the conditional and uncomment the include lines
> 3. Rebuild the project with `./build.sh`
>
> The mock implementation ensures the project builds and functions without requiring external dependencies while we complete the real implementation.

## Table of Contents

1. [Overview](#overview)
2. [Implementation Architecture](#implementation-architecture)
3. [Installation and Setup](#installation-and-setup)
4. [Using the MQTT Interface](#using-the-mqtt-interface)
5. [Mock Implementation Details](#mock-implementation-details)
6. [Real Implementation Details](#real-implementation-details)
7. [Testing](#testing)
8. [Troubleshooting](#troubleshooting)

## Overview

MQTT (Message Queuing Telemetry Transport) is a lightweight publish/subscribe messaging protocol designed for constrained devices and low-bandwidth, high-latency networks. In the AIMusicHardware project, MQTT is used for IoT communication, allowing the system to connect to external devices, services, and control systems.

Key features of our MQTT implementation:

- Publish/subscribe messaging pattern
- QoS (Quality of Service) level support (0, 1, 2)
- Last Will and Testament (LWT) for detecting unexpected disconnections
- Topic wildcards for flexible subscription matching
- Thread-safe callback handling
- Automatic reconnection
- Fallback mock implementation when Paho MQTT libraries aren't available

## Implementation Architecture

The MQTT implementation follows a layered architecture:

1. **IoTInterface (Base Interface)**: Abstract interface defining IoT communication methods
2. **MQTTInterface (Implementation)**: Concrete implementation using the Paho MQTT C++ library
3. **mqtt_include.h (Conditional Implementation)**: Provides real or mock MQTT classes based on library availability

The implementation is designed to gracefully handle the absence of the Paho MQTT libraries, automatically falling back to a mock implementation during compilation.

## Installation and Setup

### Prerequisites

- CMake 3.14 or higher
- C++17 compatible compiler
- (Optional) Mosquitto MQTT broker for testing

### Installing MQTT Libraries

The project includes an installation script to set up the Paho MQTT libraries:

```bash
# From the project root
./tools/install_mqtt_libs.sh
```

This script:
1. Clones the Paho MQTT C and C++ repositories
2. Builds and installs them to the project's `vendor` directory
3. Sets up the necessary include paths and libraries

### Setting Up an MQTT Broker

For testing with a real MQTT broker, you can use Mosquitto:

```bash
# macOS
brew install mosquitto

# Debian/Ubuntu
sudo apt-get install mosquitto

# Fedora
sudo dnf install mosquitto

# Run the broker in verbose mode
mosquitto -v
```

## Using the MQTT Interface

The `MQTTInterface` class provides a simple API for MQTT communication:

```cpp
#include "iot/MQTTInterface.h"

// Create the interface
AIMusicHardware::MQTTInterface mqtt;

// Configure connection options
mqtt.setConnectionOptions(60, true, true);  // keepAlive, cleanSession, autoReconnect

// Set last will message (optional)
mqtt.setLastWill("device/status", "{\"status\":\"offline\"}", 1, true);

// Set quality of service level
mqtt.setDefaultQoS(1);

// Register message callback
mqtt.setMessageCallback([](const std::string& topic, const std::string& payload) {
    std::cout << "Message received on " << topic << ": " << payload << std::endl;
});

// Connect to broker
if (mqtt.connect("localhost", 1883, "ClientID")) {
    // Subscribe to topics
    mqtt.subscribe("command/#");
    mqtt.subscribe("device/+/control");
    
    // Register topic-specific callback
    mqtt.setTopicCallback("command/special", 
        [](const std::string& topic, const std::string& payload) {
            // Handle special commands
        });
    
    // Publish message
    mqtt.publish("device/status", "{\"status\":\"online\"}", 1, true);
    
    // Call update periodically in your main loop
    while (running) {
        mqtt.update();
        // other application logic
    }
    
    // Disconnect when done
    mqtt.disconnect();
}
```

## Mock Implementation Details

The mock implementation is automatically used when the Paho MQTT libraries are unavailable. It provides:

- Implementation of all key MQTT classes (`async_client`, `message`, `token`, etc.)
- Console output to simulate MQTT operations
- Basic message callback functionality
- No actual network communication

The mock implementation is defined in `mqtt_include.h` and is controlled by the `HAVE_PAHO_MQTT` and `DISABLE_MQTT` preprocessor definitions.

## Real Implementation Details

When the Paho MQTT libraries are available, the `MQTTInterface` uses the real implementation, which provides:

- Full MQTT 3.1.1 protocol support
- Persistent and non-persistent sessions
- Multiple QoS levels
- SSL/TLS support (when built with SSL)
- Last Will and Testament
- Automatic reconnection
- Event-driven callbacks

Key components:

- **`MQTTCallbackHandler`**: Handles MQTT callbacks and routes messages to appropriate handlers
- **`onMessage`**: Processes incoming messages and dispatches to registered callbacks
- **`matchTopicPattern`**: Implements MQTT wildcard matching for topics
- **`reconnect`**: Handles automatic reconnection when the connection is lost

## Testing

The project includes several test applications for MQTT functionality:

1. **SimpleMQTTTest**: Tests the basic MQTT mock implementation
   ```
   ./bin/SimpleMQTTTest
   ```

2. **SimpleMQTTInterfaceTest**: Tests the MQTTInterface class with mock implementation
   ```
   ./bin/SimpleMQTTInterfaceTest
   ```

3. **RealMQTTTest**: Tests the MQTTInterface with the real MQTT implementation
   ```
   ./bin/RealMQTTTest [broker] [port] [clientId]
   ```
   
4. **PahoMQTTCTest**: Tests the Paho MQTT C implementation directly
   ```
   ./bin/PahoMQTTCTest
   ```

5. **IoTParameterDemo**: Tests IoT message handling with parameters
   ```
   ./bin/IoTParameterDemo
   ```

6. **TestMQTTIntegration**: Tests MQTT connectivity and message handling
   ```
   ./bin/TestMQTTIntegration
   ```

## Troubleshooting

### Common Issues

1. **Cannot find MQTT libraries**
   - Run `./tools/install_mqtt_libs.sh` to install the libraries
   - Check CMake output for MQTT detection messages
   - Verify that the Paho MQTT C and C++ libraries are properly built

2. **Compilation errors in MQTT code**
   - Check `mqtt_include.h` for proper forward declarations
   - Ensure all necessary MQTT classes are included
   - Verify that the mock implementation is compatible with your compiler

3. **Connection failures**
   - Verify that the MQTT broker is running
   - Check network connectivity to the broker
   - Review firewall settings
   - Check authentication settings if the broker requires authentication

4. **Message callbacks not working**
   - Ensure callbacks are registered before subscribing to topics
   - Check that topics match exactly or follow wildcard patterns
   - Verify that `update()` is called regularly in your main loop

### Debugging Tips

1. Set the `DISABLE_MQTT` preprocessor definition to force the use of the mock implementation
2. Run tests with increasing verbosity
3. Check broker logs for connection and message details
4. Use MQTT client tools like MQTT Explorer to monitor broker activity

For additional support, contact the AIMusicHardware development team.