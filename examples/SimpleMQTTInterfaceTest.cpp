#include <iostream>
#include <string>

// Define DISABLE_MQTT to force the use of our mock implementation
#define DISABLE_MQTT 1

// Define HAVE_PAHO_MQTT to enable the mock implementation
#define HAVE_PAHO_MQTT 1

#include "../include/iot/MQTTInterface.h"

using namespace AIMusicHardware;

int main() {
    std::cout << "=== Simple MQTT Interface Test ===" << std::endl;
    std::cout << "This demonstrates our MQTTInterface implementation." << std::endl;
    std::cout << std::endl;

    try {
        // Create MQTT Interface
        std::cout << "Creating MQTT Interface..." << std::endl;
        MQTTInterface mqttInterface;

        // Configure connection options
        std::cout << "Setting connection options..." << std::endl;
        mqttInterface.setConnectionOptions(60, true, true);
        mqttInterface.setDefaultQoS(0);
        mqttInterface.setLastWill("AIMusicHardware/status", "offline", 1, true);

        // Set up message callback with a simple lambda
        std::cout << "Setting up message callback..." << std::endl;
        mqttInterface.setMessageCallback([](const std::string& topic, const std::string& payload) {
            std::cout << "Message received: " << topic << " -> " << payload << std::endl;
        });

        // Connect to broker
        std::cout << "Connecting to broker..." << std::endl;
        bool connected = mqttInterface.connect("localhost", 1883, "SimpleMQTTInterfaceTest");

        if (connected) {
            std::cout << "Successfully connected to broker" << std::endl;

            // Check if connected
            if (mqttInterface.isConnected()) {
                std::cout << "isConnected() reports connection is active" << std::endl;
            }

            // Publish a message
            std::cout << "Publishing a test message..." << std::endl;
            mqttInterface.publish("test/topic", "Hello from MQTTInterface!");

            // Subscribe to a topic
            std::cout << "Subscribing to test topic..." << std::endl;
            mqttInterface.subscribe("test/topic");

            // Update once
            std::cout << "Updating..." << std::endl;
            mqttInterface.update();

            // Disconnect
            std::cout << "Disconnecting from broker..." << std::endl;
            mqttInterface.disconnect();

            // Verify disconnection
            if (!mqttInterface.isConnected()) {
                std::cout << "Successfully disconnected from broker" << std::endl;
            }
        } else {
            std::cout << "Failed to connect to broker" << std::endl;
        }

        std::cout << "Test completed successfully!" << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}