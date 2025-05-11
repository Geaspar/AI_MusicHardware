#include <iostream>
#include <chrono>
#include <thread>
#include <string>
#include <csignal>
#include <atomic>

#include "../include/iot/IoTInterface.h"
#include "../include/iot/MQTTInterface.h"
#include "../include/iot/IoTEventAdapter.h"
#include "../include/iot/IoTParameterTypes.h"

std::atomic<bool> running{true};

void signalHandler(int signal) {
    std::cout << "Caught signal " << signal << ", shutting down..." << std::endl;
    running = false;
}

int main(int argc, char** argv) {
    // Set up signal handling
    std::signal(SIGINT, signalHandler);
    
    // Parse command line arguments
    std::string brokerHost = "localhost";
    int brokerPort = 1883;
    
    if (argc > 1) {
        brokerHost = argv[1];
    }
    
    if (argc > 2) {
        brokerPort = std::stoi(argv[2]);
    }
    
    std::cout << "IoT Parameter Demo" << std::endl;
    std::cout << "====================" << std::endl;
    std::cout << "Connecting to MQTT broker at " << brokerHost << ":" << brokerPort << std::endl;
    
    try {
        // Create MQTT interface
        AIMusicHardware::MQTTInterface mqtt;
        
        // Set connection options
        mqtt.setConnectionOptions(60, true, true);
        
        // Connect to broker
        bool connected = mqtt.connect(brokerHost, brokerPort, "AIMusicHardwareDemo");
        
        if (!connected) {
            std::cerr << "Failed to connect to MQTT broker" << std::endl;
            return 1;
        }
        
        std::cout << "Connected to MQTT broker" << std::endl;
        
        // Create event adapter
        AIMusicHardware::IoTEventAdapter adapter(&mqtt);
        
        // Subscribe to test topics
        mqtt.subscribe("test/#");
        
        // Register message handlers 
        mqtt.setMessageCallback([](const std::string& topic, const std::string& payload) {
            std::cout << "Received message on topic '" << topic << "': " << payload << std::endl;
        });
        
        // Start adapter
        adapter.start();
        
        // Set up parameter converters for different sensor types
        // Temperature sensor
        adapter.registerSensorType("test/temperature", 
            AIMusicHardware::IoTParameterConverter::SensorType::TEMPERATURE,
            -20.0f, 40.0f);
        
        // Humidity sensor
        adapter.registerSensorType("test/humidity",
            AIMusicHardware::IoTParameterConverter::SensorType::HUMIDITY,
            0.0f, 100.0f);
        
        // Light sensor with logarithmic mapping
        adapter.registerSensorType("test/light",
            AIMusicHardware::IoTParameterConverter::SensorType::LIGHT,
            0.0f, 10000.0f);
        adapter.setMappingMode("test/light",
            AIMusicHardware::IoTParameterMappings::MappingMode::LOGARITHMIC);
        
        // Motion sensor with threshold mapping
        adapter.registerSensorType("test/motion",
            AIMusicHardware::IoTParameterConverter::SensorType::MOTION);
        adapter.setMappingMode("test/motion",
            AIMusicHardware::IoTParameterMappings::MappingMode::THRESHOLD,
            0.5f);
        
        // Publish test messages
        std::cout << "Publishing test messages..." << std::endl;
        
        mqtt.publish("test/info", "IoT Parameter Demo started", 0, true);
        mqtt.publish("test/temperature", "23.5");
        mqtt.publish("test/humidity", "45.8");
        mqtt.publish("test/light", "850");
        mqtt.publish("test/motion", "1");
        
        std::cout << "Test messages published" << std::endl;
        std::cout << "Press Ctrl+C to exit" << std::endl;
        
        // Main loop
        int counter = 0;
        while (running) {
            // Process MQTT messages
            mqtt.update();
            
            // Publish counter every second
            mqtt.publish("test/counter", std::to_string(counter++));
            
            // Sleep for a second
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
        
        std::cout << "Disconnecting from MQTT broker..." << std::endl;
        mqtt.publish("test/info", "IoT Parameter Demo stopping", 0, true);
        
        // Stop adapter
        adapter.stop();
        
        // Disconnect
        mqtt.disconnect();
        
        std::cout << "Disconnected" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}