#pragma once

#include <string>
#include <vector>
#include <map>
#include <set>
#include <optional>
#include <nlohmann/json.hpp>

namespace AIMusicHardware {

/**
 * @brief Represents an IoT device with its capabilities and metadata
 * 
 * The IoTDevice class encapsulates information about an IoT device,
 * including its identity, capabilities, topic structure, and metadata.
 * It provides methods for serialization/deserialization and capability
 * queries.
 */
class IoTDevice {
public:
    /**
     * @brief Device type categories
     */
    enum class Type {
        SENSOR,            ///< Measurement and monitoring devices
        ACTUATOR,          ///< Control and output devices
        CONTROLLER,        ///< Human interface devices
        DISPLAY,           ///< Visual feedback devices  
        HUB,               ///< Device that connects other devices
        UNKNOWN            ///< Uncategorized devices
    };
    
    /**
     * @brief Constructor
     * 
     * @param id Unique identifier for the device
     * @param name Human-readable name
     * @param type Device type category
     */
    IoTDevice(const std::string& id, 
             const std::string& name = "", 
             Type type = Type::UNKNOWN);
    
    /**
     * @brief Default constructor
     */
    IoTDevice() = default;
    
    /**
     * @brief Get device ID
     * 
     * @return std::string Device ID
     */
    std::string getId() const { return id_; }
    
    /**
     * @brief Set device ID
     * 
     * @param id New device ID
     */
    void setId(const std::string& id) { id_ = id; }
    
    /**
     * @brief Get device name
     * 
     * @return std::string Device name
     */
    std::string getName() const { return name_; }
    
    /**
     * @brief Set device name
     * 
     * @param name New device name
     */
    void setName(const std::string& name) { name_ = name; }
    
    /**
     * @brief Get device type
     * 
     * @return Type Device type
     */
    Type getType() const { return type_; }
    
    /**
     * @brief Set device type
     * 
     * @param type New device type
     */
    void setType(Type type) { type_ = type; }
    
    /**
     * @brief Get device model
     * 
     * @return std::string Device model
     */
    std::string getModel() const { return model_; }
    
    /**
     * @brief Set device model
     * 
     * @param model New device model
     */
    void setModel(const std::string& model) { model_ = model; }
    
    /**
     * @brief Get manufacturer name
     * 
     * @return std::string Manufacturer name
     */
    std::string getManufacturer() const { return manufacturer_; }
    
    /**
     * @brief Set manufacturer name
     * 
     * @param manufacturer New manufacturer name
     */
    void setManufacturer(const std::string& manufacturer) { manufacturer_ = manufacturer; }
    
    /**
     * @brief Get firmware version
     * 
     * @return std::string Firmware version
     */
    std::string getFirmwareVersion() const { return firmwareVersion_; }
    
    /**
     * @brief Set firmware version
     * 
     * @param version New firmware version
     */
    void setFirmwareVersion(const std::string& version) { firmwareVersion_ = version; }
    
    /**
     * @brief Get all device topics
     * 
     * @return const std::vector<std::string>& List of topics
     */
    const std::vector<std::string>& getTopics() const { return topics_; }
    
    /**
     * @brief Add a topic
     * 
     * @param topic Topic string
     */
    void addTopic(const std::string& topic);
    
    /**
     * @brief Remove a topic
     * 
     * @param topic Topic to remove
     * @return true if topic was found and removed
     */
    bool removeTopic(const std::string& topic);
    
    /**
     * @brief Clear all topics
     */
    void clearTopics() { topics_.clear(); }
    
    /**
     * @brief Get all device capabilities
     * 
     * @return const std::map<std::string, std::string>& Map of capabilities
     */
    const std::map<std::string, std::string>& getCapabilities() const { return capabilities_; }
    
    /**
     * @brief Add a capability
     * 
     * @param name Capability name
     * @param value Capability value/description
     */
    void addCapability(const std::string& name, const std::string& value);
    
    /**
     * @brief Remove a capability
     * 
     * @param name Capability to remove
     * @return true if capability was found and removed
     */
    bool removeCapability(const std::string& name);
    
    /**
     * @brief Check if device has a specific capability
     * 
     * @param name Capability name
     * @return true if capability exists
     */
    bool hasCapability(const std::string& name) const;
    
    /**
     * @brief Get capability value
     * 
     * @param name Capability name
     * @return std::optional<std::string> Capability value (empty if not found)
     */
    std::optional<std::string> getCapabilityValue(const std::string& name) const;
    
    /**
     * @brief Clear all capabilities
     */
    void clearCapabilities() { capabilities_.clear(); }
    
    /**
     * @brief Get connection status
     * 
     * @return bool True if device is connected
     */
    bool isConnected() const { return isConnected_; }
    
    /**
     * @brief Set connection status
     * 
     * @param connected New connection status
     */
    void setConnected(bool connected) { isConnected_ = connected; }
    
    /**
     * @brief Get last seen timestamp
     * 
     * @return time_t Last seen time
     */
    time_t getLastSeen() const { return lastSeen_; }
    
    /**
     * @brief Update last seen timestamp
     * 
     * @param timestamp New timestamp (default: current time)
     */
    void updateLastSeen(time_t timestamp = 0);
    
    /**
     * @brief Convert to JSON representation
     * 
     * @return nlohmann::json JSON representation
     */
    nlohmann::json toJson() const;
    
    /**
     * @brief Create from JSON representation
     * 
     * @param json JSON object
     * @return IoTDevice Constructed device
     */
    static IoTDevice fromJson(const nlohmann::json& json);
    
    /**
     * @brief Check if device metadata is valid
     * 
     * @return true if device has valid ID and type
     */
    bool isValid() const;
    
    /**
     * @brief Convert device type to string
     * 
     * @param type Device type
     * @return std::string String representation
     */
    static std::string typeToString(Type type);
    
    /**
     * @brief Convert string to device type
     * 
     * @param typeStr String representation
     * @return Type Device type
     */
    static Type stringToType(const std::string& typeStr);
    
private:
    std::string id_;                              ///< Unique device identifier
    std::string name_;                            ///< Human-readable name
    Type type_ = Type::UNKNOWN;                   ///< Device type
    std::string model_;                           ///< Device model
    std::string manufacturer_;                    ///< Manufacturer name
    std::string firmwareVersion_;                 ///< Firmware version
    std::vector<std::string> topics_;             ///< Device topics
    std::map<std::string, std::string> capabilities_; ///< Device capabilities
    bool isConnected_ = false;                    ///< Connection status
    time_t lastSeen_ = 0;                         ///< Last seen timestamp
};

} // namespace AIMusicHardware