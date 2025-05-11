#include "../../include/iot/IoTDevice.h"
#include <algorithm>
#include <ctime>

namespace AIMusicHardware {

IoTDevice::IoTDevice(const std::string& id, const std::string& name, Type type)
    : id_(id),
      name_(name.empty() ? id : name),
      type_(type) {
    // Initialize lastSeen to current time
    updateLastSeen();
}

void IoTDevice::addTopic(const std::string& topic) {
    // Check if topic already exists
    if (std::find(topics_.begin(), topics_.end(), topic) == topics_.end()) {
        topics_.push_back(topic);
    }
}

bool IoTDevice::removeTopic(const std::string& topic) {
    auto it = std::find(topics_.begin(), topics_.end(), topic);
    if (it != topics_.end()) {
        topics_.erase(it);
        return true;
    }
    return false;
}

void IoTDevice::addCapability(const std::string& name, const std::string& value) {
    capabilities_[name] = value;
}

bool IoTDevice::removeCapability(const std::string& name) {
    auto it = capabilities_.find(name);
    if (it != capabilities_.end()) {
        capabilities_.erase(it);
        return true;
    }
    return false;
}

bool IoTDevice::hasCapability(const std::string& name) const {
    return capabilities_.find(name) != capabilities_.end();
}

std::optional<std::string> IoTDevice::getCapabilityValue(const std::string& name) const {
    auto it = capabilities_.find(name);
    if (it != capabilities_.end()) {
        return it->second;
    }
    return std::nullopt;
}

void IoTDevice::updateLastSeen(time_t timestamp) {
    if (timestamp == 0) {
        // Use current time if not specified
        lastSeen_ = std::time(nullptr);
    } else {
        lastSeen_ = timestamp;
    }
}

nlohmann::json IoTDevice::toJson() const {
    nlohmann::json json;
    
    // Basic properties
    json["id"] = id_;
    json["name"] = name_;
    json["type"] = typeToString(type_);
    
    // Additional metadata
    if (!model_.empty()) json["model"] = model_;
    if (!manufacturer_.empty()) json["manufacturer"] = manufacturer_;
    if (!firmwareVersion_.empty()) json["firmware_version"] = firmwareVersion_;
    
    // Topics
    json["topics"] = topics_;
    
    // Capabilities
    json["capabilities"] = capabilities_;
    
    // Status
    json["connected"] = isConnected_;
    json["last_seen"] = lastSeen_;
    
    return json;
}

IoTDevice IoTDevice::fromJson(const nlohmann::json& json) {
    IoTDevice device;
    
    // Parse basic properties
    if (json.contains("id") && json["id"].is_string()) {
        device.id_ = json["id"].get<std::string>();
    }
    
    if (json.contains("name") && json["name"].is_string()) {
        device.name_ = json["name"].get<std::string>();
    }
    
    if (json.contains("type") && json["type"].is_string()) {
        device.type_ = stringToType(json["type"].get<std::string>());
    }
    
    // Parse additional metadata
    if (json.contains("model") && json["model"].is_string()) {
        device.model_ = json["model"].get<std::string>();
    }
    
    if (json.contains("manufacturer") && json["manufacturer"].is_string()) {
        device.manufacturer_ = json["manufacturer"].get<std::string>();
    }
    
    if (json.contains("firmware_version") && json["firmware_version"].is_string()) {
        device.firmwareVersion_ = json["firmware_version"].get<std::string>();
    }
    
    // Parse topics
    if (json.contains("topics") && json["topics"].is_array()) {
        for (const auto& topic : json["topics"]) {
            if (topic.is_string()) {
                device.topics_.push_back(topic.get<std::string>());
            }
        }
    }
    
    // Parse capabilities
    if (json.contains("capabilities") && json["capabilities"].is_object()) {
        for (auto it = json["capabilities"].begin(); it != json["capabilities"].end(); ++it) {
            if (it.value().is_string()) {
                device.capabilities_[it.key()] = it.value().get<std::string>();
            }
        }
    }
    
    // Parse status
    if (json.contains("connected") && json["connected"].is_boolean()) {
        device.isConnected_ = json["connected"].get<bool>();
    }
    
    if (json.contains("last_seen") && json["last_seen"].is_number()) {
        device.lastSeen_ = json["last_seen"].get<time_t>();
    }
    
    return device;
}

bool IoTDevice::isValid() const {
    // A valid device must have a non-empty ID
    return !id_.empty();
}

std::string IoTDevice::typeToString(Type type) {
    switch (type) {
        case Type::SENSOR:
            return "sensor";
        case Type::ACTUATOR:
            return "actuator";
        case Type::CONTROLLER:
            return "controller";
        case Type::DISPLAY:
            return "display";
        case Type::HUB:
            return "hub";
        case Type::UNKNOWN:
        default:
            return "unknown";
    }
}

IoTDevice::Type IoTDevice::stringToType(const std::string& typeStr) {
    if (typeStr == "sensor") return Type::SENSOR;
    if (typeStr == "actuator") return Type::ACTUATOR;
    if (typeStr == "controller") return Type::CONTROLLER;
    if (typeStr == "display") return Type::DISPLAY;
    if (typeStr == "hub") return Type::HUB;
    return Type::UNKNOWN;
}

} // namespace AIMusicHardware