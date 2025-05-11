#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <signal.h>
#include <atomic>

#include "../include/iot/mqtt_include.h"
#include "../include/iot/MQTTInterface.h"
#include "../include/iot/IoTConfigManager.h"
#include "../include/iot/IoTDevice.h"
#include "../include/iot/IoTEventAdapter.h"
#include "../include/events/EventBus.h"

using namespace AIMusicHardware;

// Global flag for termination
std::atomic<bool> running{true};

// Signal handler
void signalHandler(int signal) {
    std::cout << "Caught signal " << signal << ", shutting down..." << std::endl;
    running = false;
}

int main(int argc, char** argv) {
    // Set up signal handling
    signal(SIGINT, signalHandler);
    
    // Parse command line arguments
    std::string brokerHost = "localhost";
    int brokerPort = 1883;
    std::string clientId = "AIMusicHardware_TestMQTT";
    
    if (argc > 1) {
        brokerHost = argv[1];
    }
    
    if (argc > 2) {
        brokerPort = std::stoi(argv[2]);
    }
    
    if (argc > 3) {
        clientId = argv[3];
    }
    
    std::cout << "MQTT Integration Test" << std::endl;
    std::cout << "===================" << std::endl;
    std::cout << "Connecting to MQTT broker at " << brokerHost << ":" << brokerPort << std::endl;
    std::cout << "Client ID: " << clientId << std::endl;
    
    try {
        // Initialize EventBus
        EventBus& eventBus = EventBus::getInstance();
        
        // Create Event Listener
        class EventListener : public AIMusicHardware::EventListener {
        public:
            void onEvent(const Event& event) override {
                std::cout << "Event received: " << event.getId() << std::endl;
                if (event.hasPayload()) {
                    try {
                        // Try to get payload as string
                        std::string payload = event.getPayload<std::string>();
                        std::cout << "  Payload: " << payload << std::endl;
                    } catch (const std::exception& e) {
                        std::cout << "  Payload: <non-string payload>" << std::endl;
                    }
                }
            }
        };
        
        EventListener listener;
        eventBus.addEventListener("iot_message", &listener);
        
        // Create MQTT interface
#if defined(HAVE_PAHO_MQTT)
        std::cout << "Using Paho MQTT library" << std::endl;
#else
        std::cout << "WARNING: Paho MQTT library not found, using stub implementation" << std::endl;
#endif

        // Create MQTT Interface
        MQTTInterface mqttInterface;
        
        // Configure connection options
        mqttInterface.setConnectionOptions(60, true, true);
        mqttInterface.setDefaultQoS(0);
        mqttInterface.setLastWill("AIMusicHardware/status", "offline", 1, true);
        
        // Try to connect
        bool connected = mqttInterface.connect(brokerHost, brokerPort, clientId);
        
        if (connected) {
            std::cout << "Successfully connected to MQTT broker" << std::endl;
            
            // Publish online status
            mqttInterface.publish("AIMusicHardware/status", "online", 1, true);
            
            // Create IoT Event Adapter
            IoTEventAdapter eventAdapter(&mqttInterface, &eventBus);
            
            // Create IoT Config Manager
            IoTConfigManager configManager(&mqttInterface, &eventAdapter);
            
            // Set discovery topics
            std::vector<std::string> discoveryTopics = {
                "discovery/#",
                "homeassistant/+/+/config"
            };
            configManager.setDiscoveryTopics(discoveryTopics);
            
            // Start adapter and discovery
            eventAdapter.start();
            configManager.startDiscovery();
            
            // Create a test device
            IoTDevice testDevice("test_device", "Test Device", IoTDevice::Type::SENSOR);
            testDevice.setModel("Test Model");
            testDevice.setManufacturer("AIMusicHardware");
            testDevice.setFirmwareVersion("1.0.0");
            testDevice.addTopic("test_device/temperature");
            testDevice.addTopic("test_device/humidity");
            testDevice.addCapability("temperature", "celsius");
            testDevice.addCapability("humidity", "percentage");
            
            // Add device to config manager
            configManager.addDevice(testDevice);
            
            // Publish device discovery
            nlohmann::json deviceJson = testDevice.toJson();
            mqttInterface.publish("discovery/devices", deviceJson.dump());
            
            // Main loop - keep running and processing messages
            std::cout << "Running MQTT integration test. Press Ctrl+C to exit." << std::endl;
            int counter = 0;
            
            while (running) {
                // Process any MQTT messages
                mqttInterface.update();
                
                // Every 5 seconds, publish some test data
                if (counter % 50 == 0) {
                    float temp = 20.0f + (rand() % 100) / 10.0f;  // 20.0-30.0°C
                    float humidity = 40.0f + (rand() % 400) / 10.0f;  // 40.0-80.0%
                    
                    std::cout << "Publishing test data: temp=" << temp << "°C, humidity=" << humidity << "%" << std::endl;
                    
                    mqttInterface.publish("test_device/temperature", std::to_string(temp));
                    mqttInterface.publish("test_device/humidity", std::to_string(humidity));
                }
                
                counter++;
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
            
            // Clean up
            std::cout << "Shutting down..." << std::endl;
            
            // Stop discovery and adapter
            configManager.stopDiscovery();
            eventAdapter.stop();
            
            // Publish offline status
            mqttInterface.publish("AIMusicHardware/status", "offline", 1, true);
            
            // Disconnect
            mqttInterface.disconnect();
        } else {
            std::cerr << "Failed to connect to MQTT broker" << std::endl;
            return 1;
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    std::cout << "MQTT integration test completed successfully" << std::endl;
    return 0;
}