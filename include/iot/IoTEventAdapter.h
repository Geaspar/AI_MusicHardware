#pragma once

#include "IoTInterface.h"
#include "IoTParameterTypes.h"
#include "../ui/parameters/Parameter.h"
#include "../ui/parameters/ParameterGroup.h"
#include "../events/EventBus.h"
#include "../events/Event.h"
#include <map>
#include <string>
#include <functional>
#include <vector>
#include <memory>
#include <any>

namespace AIMusicHardware {

/**
 * @brief Bridge between IoT messages and the Event/Parameter system
 * 
 * This class provides the connection between IoT messages and our adaptive music
 * systems. It allows mapping of MQTT topics to events and parameters, with
 * conversion functions for different data formats.
 */
class IoTEventAdapter {
public:
    /**
     * @brief Constructor
     * 
     * @param iotInterface Pointer to the IoT interface
     * @param eventBus Pointer to the event bus (optional)
     */
    IoTEventAdapter(IoTInterface* iotInterface, EventBus* eventBus = nullptr);
    
    /**
     * @brief Destructor
     */
    ~IoTEventAdapter();
    
    /**
     * @brief Map an IoT topic to an event type
     * 
     * When a message matching this topic is received, an event of the specified
     * type will be dispatched with the payload data.
     * 
     * @param topic IoT topic (may include wildcards)
     * @param eventType Type of event to dispatch
     */
    void mapTopicToEvent(const std::string& topic, const std::string& eventType);
    
    /**
     * @brief Map an IoT topic to a parameter
     * 
     * When a message matching this topic is received, the parameter value
     * will be updated based on the payload data.
     * 
     * @param topic IoT topic (may include wildcards)
     * @param parameter Pointer to the parameter to update
     */
    void mapTopicToParameter(const std::string& topic, Parameter* parameter);
    
    /**
     * @brief Set a message converter for a topic-to-event mapping
     * 
     * This converter transforms the string payload into the appropriate
     * data type for the event.
     * 
     * @param topic IoT topic
     * @param converter Function that converts string payload to an std::any value
     */
    void setMessageConverter(const std::string& topic, 
                           std::function<std::any(const std::string&)> converter);
    
    /**
     * @brief Set a parameter converter for a topic-to-parameter mapping
     * 
     * This converter transforms the string payload into the appropriate
     * value type for the parameter.
     * 
     * @param topic IoT topic
     * @param converter Function that converts string payload to float
     */
    void setParameterConverter(const std::string& topic,
                             std::function<float(const std::string&)> converter);
    
    /**
     * @brief Register a sensor type for a topic
     * 
     * This provides a standard converter for common sensor types.
     * 
     * @param topic IoT topic
     * @param sensorType Type of sensor
     * @param minValue Minimum expected sensor value
     * @param maxValue Maximum expected sensor value
     * @param normalized Whether to normalize to 0-1 range
     */
    void registerSensorType(const std::string& topic, 
                          IoTParameterConverter::SensorType sensorType,
                          float minValue = 0.0f,
                          float maxValue = 1.0f,
                          bool normalized = true);
    
    /**
     * @brief Register a mapping mode for parameter conversion
     * 
     * This applies a mapping function to the sensor value.
     * 
     * @param topic IoT topic
     * @param mappingMode Mapping mode to apply
     * @param threshold Threshold value for threshold mode
     * @param exponent Exponent for exponential mode
     */
    void setMappingMode(const std::string& topic,
                      IoTParameterMappings::MappingMode mappingMode,
                      float threshold = 0.5f,
                      float exponent = 2.0f);
    
    /**
     * @brief Start processing IoT messages
     */
    void start();
    
    /**
     * @brief Stop processing IoT messages
     */
    void stop();
    
    /**
     * @brief Check if adapter is running
     * 
     * @return true if running, false otherwise
     */
    bool isRunning() const { return isRunning_; }
    
    /**
     * @brief Set the event bus
     * 
     * @param eventBus Pointer to the event bus
     */
    void setEventBus(EventBus* eventBus) { eventBus_ = eventBus; }
    
    /**
     * @brief Get the event bus
     * 
     * @return EventBus* Pointer to the event bus
     */
    EventBus* getEventBus() const { return eventBus_; }
    
    /**
     * @brief Get the IoT interface
     * 
     * @return IoTInterface* Pointer to the IoT interface
     */
    IoTInterface* getIoTInterface() const { return iotInterface_; }
    
private:
    IoTInterface* iotInterface_;
    EventBus* eventBus_;
    bool isRunning_ = false;
    
    // Topic mappings
    struct TopicEventMapping {
        std::string topic;
        std::string eventType;
        std::function<std::any(const std::string&)> converter;
    };
    
    struct TopicParameterMapping {
        std::string topic;
        Parameter* parameter;
        std::function<float(const std::string&)> converter;
    };
    
    std::vector<TopicEventMapping> eventMappings_;
    std::vector<TopicParameterMapping> parameterMappings_;
    
    // Topic pattern matching
    bool matchTopicPattern(const std::string& topic, const std::string& pattern) const;
    
    // Handle incoming messages
    void onIoTMessage(const std::string& topic, const std::string& payload);
    
    // Helper to find mappings for a topic
    TopicEventMapping* findEventMapping(const std::string& topic);
    TopicParameterMapping* findParameterMapping(const std::string& topic);
};

} // namespace AIMusicHardware