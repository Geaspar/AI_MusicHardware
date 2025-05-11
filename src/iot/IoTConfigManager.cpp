#include "../../include/iot/IoTConfigManager.h"
#include <iostream>
#include <fstream>
#include <regex>
#include <algorithm>
#include <filesystem>

namespace fs = std::filesystem;

namespace AIMusicHardware {

IoTConfigManager::IoTConfigManager(IoTInterface* iotInterface, IoTEventAdapter* eventAdapter)
    : iotInterface_(iotInterface),
      eventAdapter_(eventAdapter) {
    // Set up default discovery topics
    setupDefaultDiscoveryTopics();
}

IoTConfigManager::~IoTConfigManager() {
    // Stop discovery if active
    if (isDiscovering_) {
        stopDiscovery();
    }
}

void IoTConfigManager::connectToInterface(IoTInterface* iotInterface) {
    // Lock for thread safety
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Stop discovery if active
    bool wasDiscovering = isDiscovering_;
    if (wasDiscovering) {
        stopDiscovery();
    }
    
    // Update interface
    iotInterface_ = iotInterface;
    
    // Restart discovery if it was active
    if (wasDiscovering && iotInterface_) {
        startDiscovery();
    }
}

void IoTConfigManager::connectToEventAdapter(IoTEventAdapter* eventAdapter) {
    // Lock for thread safety
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Update adapter
    eventAdapter_ = eventAdapter;
    
    // Re-apply mappings if we have an adapter
    if (eventAdapter_) {
        applyTopicMappings();
    }
}

bool IoTConfigManager::loadConfig(const std::string& filename) {
    // Lock for thread safety
    std::lock_guard<std::mutex> lock(mutex_);
    
    try {
        // Use provided filename or default
        std::string configFile = filename.empty() ? getDefaultConfigFilePath() : filename;
        
        // Check if file exists
        if (!fs::exists(configFile)) {
            std::cerr << "Configuration file does not exist: " << configFile << std::endl;
            return false;
        }
        
        // Open and read file
        std::ifstream file(configFile);
        if (!file.is_open()) {
            std::cerr << "Failed to open configuration file: " << configFile << std::endl;
            return false;
        }
        
        // Parse JSON
        nlohmann::json json;
        file >> json;
        file.close();
        
        // Process configuration
        return parseConfigJson(json);
    } catch (const std::exception& e) {
        std::cerr << "Error loading configuration: " << e.what() << std::endl;
        return false;
    }
}

bool IoTConfigManager::saveConfig(const std::string& filename) {
    // Lock for thread safety
    std::lock_guard<std::mutex> lock(mutex_);
    
    try {
        // Ensure config directory exists
        ensureConfigDirectory();
        
        // Use provided filename or default
        std::string configFile = filename.empty() ? getDefaultConfigFilePath() : filename;
        
        // Create JSON representation
        nlohmann::json json = createConfigJson();
        
        // Open and write file
        std::ofstream file(configFile);
        if (!file.is_open()) {
            std::cerr << "Failed to open configuration file for writing: " << configFile << std::endl;
            return false;
        }
        
        // Write pretty-printed JSON
        file << json.dump(4);
        file.close();
        
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error saving configuration: " << e.what() << std::endl;
        return false;
    }
}

void IoTConfigManager::startDiscovery() {
    // Lock for thread safety
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Check if already discovering
    if (isDiscovering_ || !iotInterface_) {
        return;
    }
    
    // Subscribe to discovery topics
    for (const auto& topic : discoveryTopics_) {
        iotInterface_->subscribe(topic);
    }
    
    // Register message callback
    iotInterface_->setMessageCallback([this](const std::string& topic, const std::string& payload) {
        this->processIncomingMessage(topic, payload);
    });
    
    isDiscovering_ = true;
}

void IoTConfigManager::stopDiscovery() {
    // Lock for thread safety
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Check if not discovering or no interface
    if (!isDiscovering_ || !iotInterface_) {
        return;
    }
    
    // Unsubscribe from discovery topics
    for (const auto& topic : discoveryTopics_) {
        iotInterface_->unsubscribe(topic);
    }
    
    isDiscovering_ = false;
}

std::vector<IoTDevice> IoTConfigManager::getDiscoveredDevices() const {
    // Lock for thread safety
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Collect devices
    std::vector<IoTDevice> result;
    result.reserve(devices_.size());
    
    for (const auto& [id, device] : devices_) {
        result.push_back(device);
    }
    
    return result;
}

bool IoTConfigManager::addDevice(const IoTDevice& device) {
    // Check if device is valid
    if (!device.isValid()) {
        return false;
    }
    
    // Lock for thread safety
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Add or update device
    std::string deviceId = device.getId();
    bool isNew = (devices_.find(deviceId) == devices_.end());
    devices_[deviceId] = device;
    
    // Call discovery callback if it's a new device
    if (isNew && deviceDiscoveryCallback_) {
        deviceDiscoveryCallback_(device);
    }
    
    // Apply topic mappings for this device
    if (eventAdapter_) {
        applyTopicMappingsForDevice(deviceId);
    }
    
    return true;
}

bool IoTConfigManager::removeDevice(const std::string& deviceId) {
    // Lock for thread safety
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Look up device
    auto it = devices_.find(deviceId);
    if (it == devices_.end()) {
        return false;
    }
    
    // Remove device
    devices_.erase(it);
    return true;
}

IoTDevice* IoTConfigManager::getDevice(const std::string& deviceId) {
    // Lock for thread safety
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Look up device
    auto it = devices_.find(deviceId);
    if (it == devices_.end()) {
        return nullptr;
    }
    
    return &it->second;
}

const IoTDevice* IoTConfigManager::getDevice(const std::string& deviceId) const {
    // Lock for thread safety
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Look up device
    auto it = devices_.find(deviceId);
    if (it == devices_.end()) {
        return nullptr;
    }
    
    return &it->second;
}

std::vector<IoTDevice*> IoTConfigManager::findDevicesByType(IoTDevice::Type type) {
    // Lock for thread safety
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Collect matching devices
    std::vector<IoTDevice*> result;
    
    for (auto& [id, device] : devices_) {
        if (device.getType() == type) {
            result.push_back(&device);
        }
    }
    
    return result;
}

std::vector<IoTDevice*> IoTConfigManager::findDevicesByCapability(const std::string& capability) {
    // Lock for thread safety
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Collect matching devices
    std::vector<IoTDevice*> result;
    
    for (auto& [id, device] : devices_) {
        if (device.hasCapability(capability)) {
            result.push_back(&device);
        }
    }
    
    return result;
}

bool IoTConfigManager::updateDeviceStatus(const std::string& deviceId, bool connected) {
    // Lock for thread safety
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Look up device
    auto it = devices_.find(deviceId);
    if (it == devices_.end()) {
        return false;
    }
    
    // Check if status actually changed
    bool previousStatus = it->second.isConnected();
    if (previousStatus == connected) {
        // Just update the last seen timestamp
        it->second.updateLastSeen();
        return true;
    }
    
    // Update status
    it->second.setConnected(connected);
    it->second.updateLastSeen();
    
    // Call status callback
    if (deviceStatusCallback_) {
        deviceStatusCallback_(it->second, connected);
    }
    
    return true;
}

void IoTConfigManager::setConfigDirectory(const std::string& directory) {
    // Lock for thread safety
    std::lock_guard<std::mutex> lock(mutex_);
    
    configDirectory_ = directory;
}

void IoTConfigManager::setDiscoveryTopics(const std::vector<std::string>& topics) {
    // Lock for thread safety
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Check if discovery is active
    bool wasDiscovering = isDiscovering_;
    if (wasDiscovering) {
        stopDiscovery();
    }
    
    // Update topics
    discoveryTopics_ = topics;
    
    // Restart discovery if it was active
    if (wasDiscovering) {
        startDiscovery();
    }
}

void IoTConfigManager::processIncomingMessage(const std::string& topic, const std::string& payload) {
    // Check if this is a discovery topic
    if (isDiscoveryTopic(topic)) {
        parseDiscoveryMessage(topic, payload);
        return;
    }
    
    // Check for device status/heartbeat topics
    // Status topics are usually in format: status/<device_id> or <device_id>/status
    std::regex statusRegex1("status/([^/]+)");
    std::regex statusRegex2("([^/]+)/status");
    
    std::smatch match;
    if (std::regex_match(topic, match, statusRegex1) || std::regex_match(topic, match, statusRegex2)) {
        if (match.size() > 1) {
            std::string deviceId = match[1].str();
            
            // Parse payload for status (usually "online"/"offline" or "1"/"0")
            bool connected = false;
            if (payload == "online" || payload == "1" || payload == "true" || payload == "connected") {
                connected = true;
            }
            
            // Update device status
            updateDeviceStatus(deviceId, connected);
        }
    }
}

int IoTConfigManager::applyTopicMappings() {
    // Lock for thread safety
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Check if we have an event adapter
    if (!eventAdapter_) {
        return 0;
    }
    
    int mappingCount = 0;
    
    // Apply mappings for each device
    for (const auto& [id, device] : devices_) {
        mappingCount += applyTopicMappingsForDevice(id);
    }
    
    return mappingCount;
}

int IoTConfigManager::applyTopicMappingsForDevice(const std::string& deviceId) {
    // No need to lock here - called from methods that already lock
    
    // Check if we have an event adapter
    if (!eventAdapter_) {
        return 0;
    }
    
    // Get device
    auto it = devices_.find(deviceId);
    if (it == devices_.end()) {
        return 0;
    }
    
    const IoTDevice& device = it->second;
    int mappingCount = 0;
    
    // Apply mappings based on device type
    switch (device.getType()) {
        case IoTDevice::Type::SENSOR:
            mapSensorCapabilities(device);
            break;
            
        case IoTDevice::Type::ACTUATOR:
            mapActuatorCapabilities(device);
            break;
            
        case IoTDevice::Type::CONTROLLER:
            mapControllerCapabilities(device);
            break;
            
        default:
            // For unknown types, use default mappings
            createDefaultMappings(device);
            break;
    }
    
    return mappingCount;
}

void IoTConfigManager::clear() {
    // Lock for thread safety
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Clear all devices
    devices_.clear();
}

// Private helper methods
void IoTConfigManager::mapSensorCapabilities(const IoTDevice& device) {
    if (!eventAdapter_) return;
    
    // Map sensor capabilities to appropriate parameters
    for (const auto& topic : device.getTopics()) {
        // Check for common sensor types in topic name
        
        // Temperature
        if (topic.find("temperature") != std::string::npos || 
            topic.find("temp") != std::string::npos) {
            
            eventAdapter_->mapTopicToEvent(topic, "temperature_update");
            
            // If we had a parameter system connected, we could also do:
            // eventAdapter_->registerSensorType(topic, IoTParameterConverter::SensorType::TEMPERATURE);
        }
        
        // Humidity
        else if (topic.find("humidity") != std::string::npos) {
            eventAdapter_->mapTopicToEvent(topic, "humidity_update");
            // eventAdapter_->registerSensorType(topic, IoTParameterConverter::SensorType::HUMIDITY);
        }
        
        // Light
        else if (topic.find("light") != std::string::npos || 
                 topic.find("lux") != std::string::npos) {
            
            eventAdapter_->mapTopicToEvent(topic, "light_update");
            // eventAdapter_->registerSensorType(topic, IoTParameterConverter::SensorType::LIGHT);
        }
        
        // Motion
        else if (topic.find("motion") != std::string::npos || 
                 topic.find("presence") != std::string::npos || 
                 topic.find("occupancy") != std::string::npos) {
            
            eventAdapter_->mapTopicToEvent(topic, "motion_detected");
            // eventAdapter_->registerSensorType(topic, IoTParameterConverter::SensorType::MOTION);
        }
        
        // Other sensors - map as generic events
        else {
            eventAdapter_->mapTopicToEvent(topic, "sensor_update");
        }
    }
}

void IoTConfigManager::mapActuatorCapabilities(const IoTDevice& device) {
    if (!eventAdapter_) return;
    
    // For actuators, we focus more on response to events than generating events
    
    // Look for device controllers through topics
    for (const auto& topic : device.getTopics()) {
        // Standard command topics (usually for sending commands)
        if (topic.find("/set") != std::string::npos || 
            topic.find("/cmd") != std::string::npos ||
            topic.find("/command") != std::string::npos) {
            
            // These are usually for sending commands, not receiving events
            continue;
        }
        
        // State topics (usually for receiving state updates)
        if (topic.find("/state") != std::string::npos) {
            eventAdapter_->mapTopicToEvent(topic, "state_update");
        }
        
        // Generic topics - map as actuator events
        else {
            eventAdapter_->mapTopicToEvent(topic, "actuator_update");
        }
    }
}

void IoTConfigManager::mapControllerCapabilities(const IoTDevice& device) {
    if (!eventAdapter_) return;
    
    // For controllers, map buttons, sliders, etc., to events
    
    for (const auto& topic : device.getTopics()) {
        // Button topics
        if (topic.find("button") != std::string::npos || 
            topic.find("switch") != std::string::npos) {
            
            eventAdapter_->mapTopicToEvent(topic, "button_press");
        }
        
        // Slider/knob topics
        else if (topic.find("slider") != std::string::npos || 
                 topic.find("knob") != std::string::npos || 
                 topic.find("dial") != std::string::npos) {
            
            eventAdapter_->mapTopicToEvent(topic, "control_change");
        }
        
        // Generic topics - map as controller events
        else {
            eventAdapter_->mapTopicToEvent(topic, "controller_input");
        }
    }
}

void IoTConfigManager::createDefaultMappings(const IoTDevice& device) {
    if (!eventAdapter_) return;
    
    // For unknown device types, create generic mappings
    
    for (const auto& topic : device.getTopics()) {
        // Skip status topics
        if (topic.find("status") != std::string::npos) {
            continue;
        }
        
        // Map everything else as generic events
        eventAdapter_->mapTopicToEvent(topic, "iot_message");
    }
}

void IoTConfigManager::parseDiscoveryMessage(const std::string& topic, const std::string& payload) {
    try {
        // Parse payload as JSON
        nlohmann::json json = nlohmann::json::parse(payload);
        
        // Check if it has the required fields for a device announcement
        if (!json.contains("id") || !json["id"].is_string()) {
            return;
        }
        
        // Create device from JSON
        IoTDevice device = IoTDevice::fromJson(json);
        
        // Device is automatically connected when discovered
        device.setConnected(true);
        device.updateLastSeen();
        
        // Add device to registry
        addDevice(device);
        
    } catch (const std::exception& e) {
        std::cerr << "Error parsing discovery message: " << e.what() << std::endl;
    }
}

bool IoTConfigManager::isDiscoveryTopic(const std::string& topic) const {
    for (const auto& discoveryTopic : discoveryTopics_) {
        // Check for exact match
        if (topic == discoveryTopic) {
            return true;
        }
        
        // Handle wildcards
        if (discoveryTopic.back() == '#') {
            // Multi-level wildcard
            std::string prefix = discoveryTopic.substr(0, discoveryTopic.size() - 1);
            if (topic.compare(0, prefix.size(), prefix) == 0) {
                return true;
            }
        }
        
        // Could add support for single-level wildcard (+) if needed
    }
    
    return false;
}

void IoTConfigManager::setupDefaultDiscoveryTopics() {
    discoveryTopics_ = {
        "discovery/devices",
        "devices/discovery",
        "homeassistant/+/+/config",  // Home Assistant discovery format
        "homie/#"                    // Homie convention format
    };
}

nlohmann::json IoTConfigManager::createConfigJson() const {
    nlohmann::json json;
    
    // Add configuration metadata
    json["version"] = "1.0";
    json["timestamp"] = std::time(nullptr);
    
    // Add discovery topics
    json["discovery_topics"] = discoveryTopics_;
    
    // Add devices
    json["devices"] = nlohmann::json::array();
    for (const auto& [id, device] : devices_) {
        json["devices"].push_back(device.toJson());
    }
    
    return json;
}

bool IoTConfigManager::parseConfigJson(const nlohmann::json& json) {
    try {
        // Clear existing configuration
        clear();
        
        // Parse discovery topics
        if (json.contains("discovery_topics") && json["discovery_topics"].is_array()) {
            std::vector<std::string> topics;
            for (const auto& topic : json["discovery_topics"]) {
                if (topic.is_string()) {
                    topics.push_back(topic.get<std::string>());
                }
            }
            
            if (!topics.empty()) {
                discoveryTopics_ = topics;
            }
        }
        
        // Parse devices
        if (json.contains("devices") && json["devices"].is_array()) {
            for (const auto& deviceJson : json["devices"]) {
                IoTDevice device = IoTDevice::fromJson(deviceJson);
                if (device.isValid()) {
                    devices_[device.getId()] = device;
                }
            }
        }
        
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error parsing configuration JSON: " << e.what() << std::endl;
        return false;
    }
}

std::string IoTConfigManager::getDefaultConfigFilePath() const {
    return configDirectory_ + "/devices.json";
}

void IoTConfigManager::ensureConfigDirectory() const {
    try {
        if (!fs::exists(configDirectory_)) {
            fs::create_directories(configDirectory_);
        }
    } catch (const std::exception& e) {
        std::cerr << "Error creating config directory: " << e.what() << std::endl;
    }
}

} // namespace AIMusicHardware