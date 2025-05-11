#pragma once

#include "IoTInterface.h"
#include "IoTDevice.h"
#include "IoTEventAdapter.h"
#include <string>
#include <vector>
#include <map>
#include <set>
#include <mutex>
#include <functional>
#include <memory>
#include <filesystem>

namespace AIMusicHardware {

/**
 * @brief Manages IoT device discovery and configuration
 * 
 * The IoTConfigManager handles discovering and tracking IoT devices,
 * managing their configurations, and providing a unified interface for
 * device management. It supports loading and saving configurations, 
 * automatic device discovery, and topic mapping.
 */
class IoTConfigManager {
public:
    /**
     * @brief Constructor
     * 
     * @param iotInterface Pointer to IoT interface for communication
     * @param eventAdapter Pointer to event adapter for mapping events (optional)
     */
    IoTConfigManager(IoTInterface* iotInterface = nullptr, 
                    IoTEventAdapter* eventAdapter = nullptr);
    
    /**
     * @brief Destructor
     */
    ~IoTConfigManager();
    
    /**
     * @brief Connect to IoT interface
     * 
     * @param iotInterface Pointer to IoT interface
     */
    void connectToInterface(IoTInterface* iotInterface);
    
    /**
     * @brief Connect to event adapter
     * 
     * @param eventAdapter Pointer to event adapter
     */
    void connectToEventAdapter(IoTEventAdapter* eventAdapter);
    
    /**
     * @brief Load configuration from file
     * 
     * @param filename Path to configuration file
     * @return true if configuration loaded successfully
     */
    bool loadConfig(const std::string& filename);
    
    /**
     * @brief Save configuration to file
     * 
     * @param filename Path to configuration file
     * @return true if configuration saved successfully
     */
    bool saveConfig(const std::string& filename);
    
    /**
     * @brief Start device discovery
     * 
     * Begin listening for device announcements on discovery topics.
     * Newly discovered devices will be added to the device registry.
     */
    void startDiscovery();
    
    /**
     * @brief Stop device discovery
     */
    void stopDiscovery();
    
    /**
     * @brief Check if discovery is active
     * 
     * @return true if discovery is active
     */
    bool isDiscovering() const { return isDiscovering_; }
    
    /**
     * @brief Get all discovered devices
     * 
     * @return std::vector<IoTDevice> List of devices
     */
    std::vector<IoTDevice> getDiscoveredDevices() const;
    
    /**
     * @brief Add device to registry
     * 
     * @param device Device to add
     * @return true if device was added
     */
    bool addDevice(const IoTDevice& device);
    
    /**
     * @brief Remove device from registry
     * 
     * @param deviceId ID of device to remove
     * @return true if device was found and removed
     */
    bool removeDevice(const std::string& deviceId);
    
    /**
     * @brief Get device by ID
     * 
     * @param deviceId Device ID to find
     * @return IoTDevice* Pointer to device (nullptr if not found)
     */
    IoTDevice* getDevice(const std::string& deviceId);
    
    /**
     * @brief Get const device by ID
     * 
     * @param deviceId Device ID to find
     * @return const IoTDevice* Pointer to device (nullptr if not found)
     */
    const IoTDevice* getDevice(const std::string& deviceId) const;
    
    /**
     * @brief Find devices by type
     * 
     * @param type Device type to search for
     * @return std::vector<IoTDevice*> List of matching devices
     */
    std::vector<IoTDevice*> findDevicesByType(IoTDevice::Type type);
    
    /**
     * @brief Find devices by capability
     * 
     * @param capability Capability name to search for
     * @return std::vector<IoTDevice*> List of matching devices
     */
    std::vector<IoTDevice*> findDevicesByCapability(const std::string& capability);
    
    /**
     * @brief Set device discovery callback
     * 
     * This callback is called when a new device is discovered
     * 
     * @param callback Function to call when a device is discovered
     */
    using DeviceDiscoveryCallback = std::function<void(const IoTDevice&)>;
    void setDeviceDiscoveryCallback(DeviceDiscoveryCallback callback) {
        deviceDiscoveryCallback_ = callback;
    }
    
    /**
     * @brief Set device status change callback
     * 
     * This callback is called when a device's status changes
     * 
     * @param callback Function to call when a device's status changes
     */
    using DeviceStatusCallback = std::function<void(const IoTDevice&, bool connected)>;
    void setDeviceStatusCallback(DeviceStatusCallback callback) {
        deviceStatusCallback_ = callback;
    }
    
    /**
     * @brief Update device status
     * 
     * @param deviceId Device ID to update
     * @param connected New connection status
     * @return true if device was found and updated
     */
    bool updateDeviceStatus(const std::string& deviceId, bool connected);
    
    /**
     * @brief Set configuration directory
     * 
     * @param directory Directory for configuration files
     */
    void setConfigDirectory(const std::string& directory);
    
    /**
     * @brief Get configuration directory
     * 
     * @return std::string Configuration directory
     */
    std::string getConfigDirectory() const { return configDirectory_; }
    
    /**
     * @brief Set default discovery topics
     * 
     * @param topics List of discovery topics
     */
    void setDiscoveryTopics(const std::vector<std::string>& topics);
    
    /**
     * @brief Get discovery topics
     * 
     * @return const std::vector<std::string>& Discovery topics
     */
    const std::vector<std::string>& getDiscoveryTopics() const { return discoveryTopics_; }
    
    /**
     * @brief Process incoming message
     * 
     * This is called internally by the IoT interface when a message is received.
     * 
     * @param topic Message topic
     * @param payload Message payload
     */
    void processIncomingMessage(const std::string& topic, const std::string& payload);
    
    /**
     * @brief Apply topic mappings for all devices
     * 
     * Maps device topics to events and parameters based on capabilities.
     * 
     * @return int Number of mappings created
     */
    int applyTopicMappings();
    
    /**
     * @brief Apply topic mappings for a specific device
     * 
     * @param deviceId Device ID to map
     * @return int Number of mappings created
     */
    int applyTopicMappingsForDevice(const std::string& deviceId);
    
    /**
     * @brief Clear all devices and mappings
     */
    void clear();
    
private:
    IoTInterface* iotInterface_ = nullptr;
    IoTEventAdapter* eventAdapter_ = nullptr;
    
    std::string configDirectory_ = "./config/iot";
    std::vector<std::string> discoveryTopics_;
    std::map<std::string, IoTDevice> devices_;
    
    bool isDiscovering_ = false;
    DeviceDiscoveryCallback deviceDiscoveryCallback_;
    DeviceStatusCallback deviceStatusCallback_;
    
    // For thread safety
    mutable std::mutex mutex_;
    
    // Mapping helpers
    void mapSensorCapabilities(const IoTDevice& device);
    void mapActuatorCapabilities(const IoTDevice& device);
    void mapControllerCapabilities(const IoTDevice& device);
    void createDefaultMappings(const IoTDevice& device);
    
    // Discovery helpers
    void parseDiscoveryMessage(const std::string& topic, const std::string& payload);
    bool isDiscoveryTopic(const std::string& topic) const;
    void setupDefaultDiscoveryTopics();
    
    // Configuration helpers
    nlohmann::json createConfigJson() const;
    bool parseConfigJson(const nlohmann::json& json);
    std::string getDefaultConfigFilePath() const;
    void ensureConfigDirectory() const;
};

} // namespace AIMusicHardware