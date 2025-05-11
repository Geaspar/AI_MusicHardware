#include "../../include/iot/IoTEventAdapter.h"
#include "../../include/ui/ParameterManager.h"
#include <regex>
#include <iostream>

namespace AIMusicHardware {

// IoTEventAdapter implementation
IoTEventAdapter::IoTEventAdapter(IoTInterface* iotInterface, EventBus* eventBus)
    : iotInterface_(iotInterface), eventBus_(eventBus), isRunning_(false) {
    
    if (!iotInterface_) {
        throw std::invalid_argument("IoTInterface cannot be null");
    }
}

IoTEventAdapter::~IoTEventAdapter() {
    stop();
}

void IoTEventAdapter::mapTopicToEvent(const std::string& topic, const std::string& eventType) {
    // Check if mapping already exists
    for (auto& mapping : eventMappings_) {
        if (mapping.topic == topic) {
            // Update existing mapping
            mapping.eventType = eventType;
            return;
        }
    }
    
    // Create new mapping
    TopicEventMapping mapping;
    mapping.topic = topic;
    mapping.eventType = eventType;
    
    eventMappings_.push_back(mapping);
    
    // If adapter is running, subscribe to topic
    if (isRunning_ && iotInterface_) {
        iotInterface_->subscribe(topic);
    }
}

void IoTEventAdapter::mapTopicToParameter(const std::string& topic, Parameter* parameter) {
    if (!parameter) {
        return;
    }
    
    // Check if mapping already exists
    for (auto& mapping : parameterMappings_) {
        if (mapping.topic == topic && mapping.parameter == parameter) {
            return; // Mapping already exists
        }
    }
    
    // Create new mapping
    TopicParameterMapping mapping;
    mapping.topic = topic;
    mapping.parameter = parameter;
    
    parameterMappings_.push_back(mapping);
    
    // If adapter is running, subscribe to topic
    if (isRunning_ && iotInterface_) {
        iotInterface_->subscribe(topic);
    }
}

void IoTEventAdapter::setMessageConverter(const std::string& topic, 
                                         std::function<std::any(const std::string&)> converter) {
    // Find the event mapping
    TopicEventMapping* mapping = findEventMapping(topic);
    
    if (mapping) {
        mapping->converter = converter;
    }
}

void IoTEventAdapter::setParameterConverter(const std::string& topic,
                                           std::function<float(const std::string&)> converter) {
    // Find the parameter mapping
    TopicParameterMapping* mapping = findParameterMapping(topic);
    
    if (mapping) {
        mapping->converter = converter;
    }
}

void IoTEventAdapter::registerSensorType(const std::string& topic, 
                                       IoTParameterConverter::SensorType sensorType,
                                       float minValue, float maxValue,
                                       bool normalized) {
    // Get the appropriate converter
    auto converter = IoTParameterConverter::getConverter(
        sensorType, minValue, maxValue, normalized);
    
    // Set the converter for the topic
    setParameterConverter(topic, converter);
}

void IoTEventAdapter::setMappingMode(const std::string& topic,
                                   IoTParameterMappings::MappingMode mappingMode,
                                   float threshold, float exponent) {
    // Find the parameter mapping
    TopicParameterMapping* mapping = findParameterMapping(topic);
    
    if (mapping) {
        // Get the current converter
        auto currentConverter = mapping->converter;
        
        // Create the mapping function
        auto mappingFunc = IoTParameterMappings::createMapping(
            mappingMode, threshold, exponent);
        
        // Chain the conversions
        if (currentConverter) {
            mapping->converter = IoTParameterMappings::chainConversions(
                currentConverter, mappingFunc);
        } else {
            // If no converter exists, create a default string-to-float converter
            mapping->converter = IoTParameterMappings::chainConversions(
                [](const std::string& payload) {
                    try {
                        return std::stof(payload);
                    } catch (...) {
                        return 0.0f;
                    }
                },
                mappingFunc
            );
        }
    }
}

void IoTEventAdapter::start() {
    if (isRunning_ || !iotInterface_) {
        return;
    }
    
    // Subscribe to all mapped topics
    for (const auto& mapping : eventMappings_) {
        iotInterface_->subscribe(mapping.topic);
    }
    
    for (const auto& mapping : parameterMappings_) {
        iotInterface_->subscribe(mapping.topic);
    }
    
    // Set global message callback
    iotInterface_->setMessageCallback([this](const std::string& topic, const std::string& payload) {
        this->onIoTMessage(topic, payload);
    });
    
    isRunning_ = true;
}

void IoTEventAdapter::stop() {
    if (!isRunning_ || !iotInterface_) {
        return;
    }
    
    // Remove message callback
    iotInterface_->setMessageCallback(nullptr);
    
    isRunning_ = false;
}

bool IoTEventAdapter::matchTopicPattern(const std::string& topic, const std::string& pattern) const {
    // Simple wildcard matching for MQTT topics
    // e.g., "sensors/#" matches "sensors/temp", "sensors/humidity/kitchen", etc.
    
    // Exact match
    if (pattern == topic) {
        return true;
    }
    
    // Single-level wildcard +
    if (pattern.find('+') != std::string::npos) {
        // Replace + with regex for any non-slash sequence
        std::string regexPattern = pattern;
        size_t pos = 0;
        while ((pos = regexPattern.find('+', pos)) != std::string::npos) {
            regexPattern.replace(pos, 1, "([^/]+)");
            pos += 7; // length of "([^/]+)"
        }
        
        std::regex re(regexPattern);
        return std::regex_match(topic, re);
    }
    
    // Multi-level wildcard #
    if (pattern.find('#') != std::string::npos) {
        // # must be the last character and preceded by /
        if (pattern.back() == '#') {
            std::string prefix = pattern.substr(0, pattern.length() - 1);
            
            // If # is the only character, it matches everything
            if (prefix.empty()) {
                return true;
            }
            
            // Check if topic starts with prefix
            return topic.compare(0, prefix.length(), prefix) == 0;
        }
    }
    
    return false;
}

void IoTEventAdapter::onIoTMessage(const std::string& topic, const std::string& payload) {
    // First, create a generic IoT event for any subscribers who want all messages
    IoTEvent iotEvent(topic, payload);
    if (eventBus_) {
        eventBus_->dispatchEvent(iotEvent);
    }

    // Handle specific event mappings
    for (const auto& mapping : eventMappings_) {
        if (topic == mapping.topic || matchTopicPattern(topic, mapping.topic)) {
            // Create appropriate event type based on mapping.eventType
            std::unique_ptr<Event> event;

            if (mapping.eventType == "state_change") {
                // Use payload as state name
                std::string stateName = payload;
                if (mapping.converter) {
                    try {
                        stateName = std::any_cast<std::string>(mapping.converter(payload));
                    } catch (std::exception& e) {
                        std::cerr << "Error converting payload to state name: " << e.what() << std::endl;
                    }
                }
                event = std::make_unique<StateChangeEvent>(stateName);
            }
            else if (mapping.eventType == "pattern_control") {
                // Parse payload for pattern ID and action
                std::string patternId = topic; // Default to topic as pattern ID
                PatternEvent::Action action = PatternEvent::Action::START; // Default action

                // Try to extract pattern ID and action from payload if it's in format "pattern_id:action"
                size_t colonPos = payload.find(':');
                if (colonPos != std::string::npos) {
                    patternId = payload.substr(0, colonPos);
                    std::string actionStr = payload.substr(colonPos + 1);

                    if (actionStr == "start") action = PatternEvent::Action::START;
                    else if (actionStr == "stop") action = PatternEvent::Action::STOP;
                    else if (actionStr == "pause") action = PatternEvent::Action::PAUSE;
                    else if (actionStr == "resume") action = PatternEvent::Action::RESUME;
                    else if (actionStr == "restart") action = PatternEvent::Action::RESTART;
                }

                event = std::make_unique<PatternEvent>(patternId, action);
            }
            else if (mapping.eventType == "parameter_change") {
                // Extract parameter ID and value
                std::string parameterId = topic; // Default to topic as parameter ID
                float value = 0.0f;

                // Try to parse payload as float
                try {
                    value = std::stof(payload);
                } catch (...) {
                    // If not a float, check if it's in format "parameter_id:value"
                    size_t colonPos = payload.find(':');
                    if (colonPos != std::string::npos) {
                        parameterId = payload.substr(0, colonPos);
                        try {
                            value = std::stof(payload.substr(colonPos + 1));
                        } catch (...) {
                            // Unable to parse value
                            std::cerr << "Unable to parse parameter value from payload: " << payload << std::endl;
                            continue;
                        }
                    } else {
                        // Unable to parse value
                        std::cerr << "Unable to parse parameter value from payload: " << payload << std::endl;
                        continue;
                    }
                }

                event = std::make_unique<ParameterEvent>(parameterId, value);
            }
            else {
                // Generic event
                event = std::make_unique<Event>(mapping.eventType);

                // Apply payload conversion if available
                if (mapping.converter) {
                    try {
                        std::any eventData = mapping.converter(payload);
                        event->setPayload(eventData);
                    } catch (std::exception& e) {
                        std::cerr << "Error converting payload: " << e.what() << std::endl;
                        // Set raw payload as string if conversion fails
                        event->setPayload(payload);
                    }
                } else {
                    // Use raw payload as string
                    event->setPayload(payload);
                }
            }

            // Dispatch event if event bus exists
            if (eventBus_ && event) {
                eventBus_->dispatchEvent(*event);
            }
        }
    }
    
    // Handle parameter mappings
    for (const auto& mapping : parameterMappings_) {
        if (topic == mapping.topic || matchTopicPattern(topic, mapping.topic)) {
            // Get parameter
            Parameter* parameter = mapping.parameter;
            if (!parameter) continue;

            // Convert payload to parameter value
            float value = 0.0f;

            if (mapping.converter) {
                // Use converter if available
                try {
                    value = mapping.converter(payload);
                } catch (std::exception& e) {
                    std::cerr << "Error converting payload for parameter: " << e.what() << std::endl;
                    continue;
                }
            } else {
                // Default conversion (try to parse as float)
                try {
                    value = std::stof(payload);
                } catch (...) {
                    continue; // Skip if conversion fails
                }
            }

            // Update parameter value based on its type
            try {
                switch (parameter->getType()) {
                    case Parameter::Type::FLOAT: {
                        auto* floatParam = static_cast<FloatParameter*>(parameter);
                        floatParam->setValue(value);
                        std::cout << "Setting float parameter " << parameter->getName()
                                  << " to " << value << std::endl;
                        break;
                    }

                    case Parameter::Type::INT: {
                        auto* intParam = static_cast<IntParameter*>(parameter);
                        intParam->setValue(static_cast<int>(std::round(value)));
                        std::cout << "Setting int parameter " << parameter->getName()
                                  << " to " << static_cast<int>(std::round(value)) << std::endl;
                        break;
                    }

                    case Parameter::Type::BOOL: {
                        auto* boolParam = static_cast<BoolParameter*>(parameter);
                        boolParam->setValue(value >= 0.5f);
                        std::cout << "Setting bool parameter " << parameter->getName()
                                  << " to " << (value >= 0.5f ? "true" : "false") << std::endl;
                        break;
                    }

                    case Parameter::Type::ENUM: {
                        auto* enumParam = static_cast<EnumParameter*>(parameter);
                        // For enum parameters, we can either:
                        // 1. Set by normalized value (0-1 maps to first-last enum value)
                        enumParam->setFromNormalizedValue(value);
                        std::cout << "Setting enum parameter " << parameter->getName()
                                  << " to " << enumParam->getCurrentValueName() << std::endl;
                        break;
                    }

                    case Parameter::Type::TRIGGER: {
                        auto* triggerParam = static_cast<TriggerParameter*>(parameter);
                        if (value >= 0.5f) {
                            triggerParam->trigger();
                            std::cout << "Triggering parameter " << parameter->getName() << std::endl;
                        }
                        break;
                    }

                    default:
                        std::cerr << "Unknown parameter type for " << parameter->getName() << std::endl;
                        break;
                }
            } catch (const std::exception& e) {
                std::cerr << "Error setting parameter value: " << e.what() << std::endl;
            }
        }
    }
}

IoTEventAdapter::TopicEventMapping* IoTEventAdapter::findEventMapping(const std::string& topic) {
    for (auto& mapping : eventMappings_) {
        if (mapping.topic == topic) {
            return &mapping;
        }
    }
    return nullptr;
}

IoTEventAdapter::TopicParameterMapping* IoTEventAdapter::findParameterMapping(const std::string& topic) {
    for (auto& mapping : parameterMappings_) {
        if (mapping.topic == topic) {
            return &mapping;
        }
    }
    return nullptr;
}

} // namespace AIMusicHardware