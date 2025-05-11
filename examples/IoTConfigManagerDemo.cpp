#include <iostream>
#include <iomanip>
#include <string>
#include <thread>
#include <chrono>
#include <csignal>
#include <atomic>
#include <sstream>
#include <map>
#include <filesystem>

#include "../include/iot/IoTInterface.h"
#include "../include/iot/MQTTInterface.h"
#include "../include/iot/IoTEventAdapter.h"
#include "../include/iot/IoTConfigManager.h"
#include "../include/iot/IoTDevice.h"
#include "../include/events/EventBus.h"
#include "../include/events/EventListener.h"

using namespace AIMusicHardware;
namespace fs = std::filesystem;

// Global flag for termination
std::atomic<bool> running{true};

// Signal handler
void signalHandler(int signal) {
    std::cout << "Caught signal " << signal << ", shutting down..." << std::endl;
    running = false;
}

// Helper to format time_t
std::string formatTime(time_t time) {
    char buffer[64];
    struct tm* timeinfo = std::localtime(&time);
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", timeinfo);
    return std::string(buffer);
}

// Helper to format device type
std::string formatDeviceType(IoTDevice::Type type) {
    switch (type) {
        case IoTDevice::Type::SENSOR:
            return "Sensor";
        case IoTDevice::Type::ACTUATOR:
            return "Actuator";
        case IoTDevice::Type::CONTROLLER:
            return "Controller";
        case IoTDevice::Type::DISPLAY:
            return "Display";
        case IoTDevice::Type::HUB:
            return "Hub";
        case IoTDevice::Type::UNKNOWN:
        default:
            return "Unknown";
    }
}

// Helper to print device information
void printDevice(const IoTDevice& device) {
    std::cout << "Device: " << device.getName() << " (" << device.getId() << ")" << std::endl;
    std::cout << "  Type: " << formatDeviceType(device.getType()) << std::endl;
    std::cout << "  Model: " << device.getModel() << std::endl;
    std::cout << "  Manufacturer: " << device.getManufacturer() << std::endl;
    std::cout << "  Firmware: " << device.getFirmwareVersion() << std::endl;
    std::cout << "  Status: " << (device.isConnected() ? "Connected" : "Disconnected") << std::endl;
    std::cout << "  Last Seen: " << formatTime(device.getLastSeen()) << std::endl;
    
    // Print topics
    std::cout << "  Topics:" << std::endl;
    for (const auto& topic : device.getTopics()) {
        std::cout << "    - " << topic << std::endl;
    }
    
    // Print capabilities
    std::cout << "  Capabilities:" << std::endl;
    for (const auto& [name, value] : device.getCapabilities()) {
        std::cout << "    - " << name << ": " << value << std::endl;
    }
    
    std::cout << std::endl;
}

// Event listener for IoT events
class IoTEventMonitor : public EventListener {
public:
    IoTEventMonitor() {
        // Register for all events
        EventBus::getInstance().addEventListener("temperature_update", this);
        EventBus::getInstance().addEventListener("humidity_update", this);
        EventBus::getInstance().addEventListener("light_update", this);
        EventBus::getInstance().addEventListener("motion_detected", this);
        EventBus::getInstance().addEventListener("button_press", this);
        EventBus::getInstance().addEventListener("control_change", this);
        EventBus::getInstance().addEventListener("state_update", this);
        EventBus::getInstance().addEventListener("sensor_update", this);
        EventBus::getInstance().addEventListener("actuator_update", this);
        EventBus::getInstance().addEventListener("controller_input", this);
        EventBus::getInstance().addEventListener("iot_message", this);
    }
    
    ~IoTEventMonitor() {
        // Unregister from all events
        EventBus::getInstance().removeEventListener("temperature_update", this);
        EventBus::getInstance().removeEventListener("humidity_update", this);
        EventBus::getInstance().removeEventListener("light_update", this);
        EventBus::getInstance().removeEventListener("motion_detected", this);
        EventBus::getInstance().removeEventListener("button_press", this);
        EventBus::getInstance().removeEventListener("control_change", this);
        EventBus::getInstance().removeEventListener("state_update", this);
        EventBus::getInstance().removeEventListener("sensor_update", this);
        EventBus::getInstance().removeEventListener("actuator_update", this);
        EventBus::getInstance().removeEventListener("controller_input", this);
        EventBus::getInstance().removeEventListener("iot_message", this);
    }
    
    void onEvent(const Event& event) override {
        // Print event information
        std::cout << "Event: " << event.getId() << std::endl;
        
        // Extract payload if available
        if (event.hasPayload()) {
            try {
                std::string payload = event.getPayload<std::string>();
                std::cout << "  Payload: " << payload << std::endl;
            } catch (const std::exception& e) {
                std::cout << "  Payload: <non-string payload>" << std::endl;
            }
        }
        
        std::cout << "  Time: " << formatTime(std::time(nullptr)) << std::endl;
        std::cout << std::endl;
    }
};

// Helper to create a test device
IoTDevice createTestDevice(const std::string& id, const std::string& name, IoTDevice::Type type) {
    IoTDevice device(id, name, type);
    
    // Set metadata
    device.setModel("Test Model");
    device.setManufacturer("AIMusicHardware");
    device.setFirmwareVersion("1.0.0");
    device.setConnected(true);
    device.updateLastSeen();
    
    // Add topics based on type
    switch (type) {
        case IoTDevice::Type::SENSOR:
            device.addTopic(id + "/temperature");
            device.addTopic(id + "/humidity");
            device.addTopic(id + "/light");
            device.addTopic(id + "/motion");
            device.addTopic(id + "/status");
            
            // Add capabilities
            device.addCapability("temperature", "celsius");
            device.addCapability("humidity", "percentage");
            device.addCapability("light", "lux");
            device.addCapability("motion", "binary");
            break;
            
        case IoTDevice::Type::ACTUATOR:
            device.addTopic(id + "/set");
            device.addTopic(id + "/state");
            device.addTopic(id + "/status");
            
            // Add capabilities
            device.addCapability("switch", "binary");
            device.addCapability("brightness", "percentage");
            break;
            
        case IoTDevice::Type::CONTROLLER:
            device.addTopic(id + "/button");
            device.addTopic(id + "/slider");
            device.addTopic(id + "/status");
            
            // Add capabilities
            device.addCapability("button", "momentary");
            device.addCapability("slider", "continuous");
            break;
            
        default:
            device.addTopic(id + "/data");
            device.addTopic(id + "/status");
            break;
    }
    
    return device;
}

// Helper to simulate IoT messages
void simulateDeviceMessages(IoTInterface* iotInterface, const IoTDevice& device) {
    if (!iotInterface) return;
    
    // Simulate messages based on device type
    switch (device.getType()) {
        case IoTDevice::Type::SENSOR: {
            // Send temperature reading
            float temperature = 20.0f + (rand() % 100) / 10.0f;  // 20.0 - 30.0
            iotInterface->publish(device.getId() + "/temperature", std::to_string(temperature));
            
            // Send humidity reading
            float humidity = 40.0f + (rand() % 400) / 10.0f;  // 40.0 - 80.0
            iotInterface->publish(device.getId() + "/humidity", std::to_string(humidity));
            
            // Send light reading
            float light = 100.0f + (rand() % 900);  // 100 - 1000
            iotInterface->publish(device.getId() + "/light", std::to_string(light));
            
            // Send motion reading (randomly)
            if (rand() % 10 == 0) {  // 10% chance
                iotInterface->publish(device.getId() + "/motion", "1");
            } else {
                iotInterface->publish(device.getId() + "/motion", "0");
            }
            break;
        }
            
        case IoTDevice::Type::ACTUATOR: {
            // Send state updates
            bool state = (rand() % 2 == 0);
            iotInterface->publish(device.getId() + "/state", state ? "on" : "off");
            break;
        }
            
        case IoTDevice::Type::CONTROLLER: {
            // Send button press (randomly)
            if (rand() % 5 == 0) {  // 20% chance
                iotInterface->publish(device.getId() + "/button", "pressed");
            }
            
            // Send slider value
            float slider = (rand() % 100) / 100.0f;  // 0.0 - 1.0
            iotInterface->publish(device.getId() + "/slider", std::to_string(slider));
            break;
        }
            
        default:
            // Send generic data
            iotInterface->publish(device.getId() + "/data", "value=" + std::to_string(rand() % 100));
            break;
    }
    
    // Send status update
    iotInterface->publish(device.getId() + "/status", "online");
}

// Helper for discovery of IoT devices
void simulateDeviceDiscovery(IoTInterface* iotInterface, const IoTDevice& device) {
    if (!iotInterface) return;
    
    // Convert device to JSON
    nlohmann::json json = device.toJson();
    std::string payload = json.dump();
    
    // Publish to discovery topic
    iotInterface->publish("discovery/devices", payload);
}

// Main CLI loop
void cliMainLoop(IoTConfigManager& configManager, IoTInterface* iotInterface) {
    std::cout << "\nCommands:\n";
    std::cout << "  list                  - List all devices\n";
    std::cout << "  add <type>            - Add test device (sensor, actuator, controller)\n";
    std::cout << "  remove <id>           - Remove device\n";
    std::cout << "  simulate <id>         - Simulate messages from device\n";
    std::cout << "  discover <id>         - Simulate device discovery\n";
    std::cout << "  save <filename>       - Save configuration\n";
    std::cout << "  load <filename>       - Load configuration\n";
    std::cout << "  start                 - Start discovery\n";
    std::cout << "  stop                  - Stop discovery\n";
    std::cout << "  topic <topic> <msg>   - Publish message to topic\n";
    std::cout << "  quit                  - Exit program\n";
    std::cout << std::endl;
    
    std::string command;
    std::map<std::string, int> deviceCounters = {
        {"sensor", 0},
        {"actuator", 0},
        {"controller", 0}
    };
    
    while (running) {
        std::cout << "> ";
        std::getline(std::cin, command);
        
        if (command == "quit" || command == "exit") {
            running = false;
            break;
        }
        
        std::istringstream iss(command);
        std::string cmd;
        iss >> cmd;
        
        if (cmd == "list") {
            // List all devices
            auto devices = configManager.getDiscoveredDevices();
            std::cout << "Discovered devices (" << devices.size() << "):" << std::endl;
            for (const auto& device : devices) {
                printDevice(device);
            }
        }
        else if (cmd == "add") {
            // Add test device
            std::string type;
            iss >> type;
            
            IoTDevice::Type deviceType = IoTDevice::Type::UNKNOWN;
            if (type == "sensor") deviceType = IoTDevice::Type::SENSOR;
            else if (type == "actuator") deviceType = IoTDevice::Type::ACTUATOR;
            else if (type == "controller") deviceType = IoTDevice::Type::CONTROLLER;
            else {
                std::cout << "Unknown device type. Use sensor, actuator, or controller." << std::endl;
                continue;
            }
            
            // Create device
            deviceCounters[type]++;
            std::string id = type + "_" + std::to_string(deviceCounters[type]);
            std::string name = "Test " + type + " " + std::to_string(deviceCounters[type]);
            
            IoTDevice device = createTestDevice(id, name, deviceType);
            configManager.addDevice(device);
            
            std::cout << "Added device: " << id << std::endl;
        }
        else if (cmd == "remove") {
            // Remove device
            std::string id;
            iss >> id;
            
            if (configManager.removeDevice(id)) {
                std::cout << "Removed device: " << id << std::endl;
            } else {
                std::cout << "Device not found: " << id << std::endl;
            }
        }
        else if (cmd == "simulate") {
            // Simulate device messages
            std::string id;
            iss >> id;
            
            const IoTDevice* device = configManager.getDevice(id);
            if (device) {
                simulateDeviceMessages(iotInterface, *device);
                std::cout << "Simulated messages from device: " << id << std::endl;
            } else {
                std::cout << "Device not found: " << id << std::endl;
            }
        }
        else if (cmd == "discover") {
            // Simulate device discovery
            std::string id;
            iss >> id;
            
            const IoTDevice* device = configManager.getDevice(id);
            if (device) {
                simulateDeviceDiscovery(iotInterface, *device);
                std::cout << "Simulated discovery of device: " << id << std::endl;
            } else {
                std::cout << "Device not found: " << id << std::endl;
            }
        }
        else if (cmd == "save") {
            // Save configuration
            std::string filename;
            iss >> filename;
            
            if (configManager.saveConfig(filename)) {
                std::cout << "Configuration saved to: " << filename << std::endl;
            } else {
                std::cout << "Failed to save configuration" << std::endl;
            }
        }
        else if (cmd == "load") {
            // Load configuration
            std::string filename;
            iss >> filename;
            
            if (configManager.loadConfig(filename)) {
                std::cout << "Configuration loaded from: " << filename << std::endl;
            } else {
                std::cout << "Failed to load configuration" << std::endl;
            }
        }
        else if (cmd == "start") {
            // Start discovery
            configManager.startDiscovery();
            std::cout << "Discovery started" << std::endl;
        }
        else if (cmd == "stop") {
            // Stop discovery
            configManager.stopDiscovery();
            std::cout << "Discovery stopped" << std::endl;
        }
        else if (cmd == "topic") {
            // Publish message to topic
            std::string topic, message;
            iss >> topic;
            
            // Get rest of line as message
            std::getline(iss, message);
            if (!message.empty() && message[0] == ' ') {
                message = message.substr(1);  // Remove leading space
            }
            
            if (iotInterface) {
                iotInterface->publish(topic, message);
                std::cout << "Published to " << topic << ": " << message << std::endl;
            } else {
                std::cout << "MQTT interface not available" << std::endl;
            }
        }
        else if (cmd == "help") {
            // Show help
            std::cout << "\nCommands:\n";
            std::cout << "  list                  - List all devices\n";
            std::cout << "  add <type>            - Add test device (sensor, actuator, controller)\n";
            std::cout << "  remove <id>           - Remove device\n";
            std::cout << "  simulate <id>         - Simulate messages from device\n";
            std::cout << "  discover <id>         - Simulate device discovery\n";
            std::cout << "  save <filename>       - Save configuration\n";
            std::cout << "  load <filename>       - Load configuration\n";
            std::cout << "  start                 - Start discovery\n";
            std::cout << "  stop                  - Stop discovery\n";
            std::cout << "  topic <topic> <msg>   - Publish message to topic\n";
            std::cout << "  quit                  - Exit program\n";
        }
        else {
            std::cout << "Unknown command. Type 'help' for available commands." << std::endl;
        }
    }
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
    
    std::cout << "IoT Configuration Manager Demo" << std::endl;
    std::cout << "===============================" << std::endl;
    std::cout << "Connecting to MQTT broker at " << brokerHost << ":" << brokerPort << std::endl;
    
    try {
        // Create temp directory for configuration
        std::string configDir = "./iot_config_test";
        fs::create_directories(configDir);
        
        // Initialize EventBus
        EventBus& eventBus = EventBus::getInstance();
        
        // Create IoT event monitor
        IoTEventMonitor monitor;
        
        // Create MQTT interface
        std::unique_ptr<MQTTInterface> mqtt = std::make_unique<MQTTInterface>();
        
        // Set connection options
        mqtt->setConnectionOptions(60, true, true);
        
        // Try to connect to broker
        bool connected = false;
        try {
            connected = mqtt->connect(brokerHost, brokerPort, "AIMusicHardwareConfigDemo");
        } catch (const std::exception& e) {
            std::cerr << "MQTT connection error: " << e.what() << std::endl;
        }
        
        if (connected) {
            std::cout << "Connected to MQTT broker" << std::endl;
        } else {
            std::cout << "Failed to connect to MQTT broker. Continuing in offline mode." << std::endl;
        }
        
        // Create IoT event adapter
        IoTEventAdapter adapter(mqtt.get(), &eventBus);
        
        // Create configuration manager
        IoTConfigManager configManager(mqtt.get(), &adapter);
        configManager.setConfigDirectory(configDir);
        
        // Set up device discovery callback
        configManager.setDeviceDiscoveryCallback([](const IoTDevice& device) {
            std::cout << "New device discovered: " << device.getId() << std::endl;
            printDevice(device);
        });
        
        // Set up device status callback
        configManager.setDeviceStatusCallback([](const IoTDevice& device, bool connected) {
            std::cout << "Device status changed: " << device.getId() 
                      << " is now " << (connected ? "connected" : "disconnected") << std::endl;
        });
        
        // Start adapter
        adapter.start();
        
        // Start device discovery
        configManager.startDiscovery();
        
        // Show test setup info
        std::cout << "\nIoT Configuration Manager Demo" << std::endl;
        std::cout << "This demo allows you to:" << std::endl;
        std::cout << "- Add virtual IoT devices" << std::endl;
        std::cout << "- Simulate device messages" << std::endl;
        std::cout << "- Test device discovery" << std::endl;
        std::cout << "- Save and load device configurations" << std::endl;
        std::cout << std::endl;
        
        // Run the CLI
        cliMainLoop(configManager, mqtt.get());
        
        // Clean up
        std::cout << "Shutting down..." << std::endl;
        
        // Stop discovery
        configManager.stopDiscovery();
        
        // Stop adapter
        adapter.stop();
        
        // Disconnect
        if (connected) {
            mqtt->disconnect();
        }
        
        std::cout << "Done" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}