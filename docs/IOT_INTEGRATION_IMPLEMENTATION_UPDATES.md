# IoT Integration Updates - Paho MQTT Implementation Details

Based on the reference implementation of Paho MQTT, this document provides specific implementation details to enhance our IoT integration for the AIMusicHardware project.

## Paho MQTT Client Implementation

The reference implementation demonstrates effective use of the Eclipse Paho MQTT C client library. We will incorporate these proven approaches into our implementation:

### MQTT Client Setup

```cpp
// MQTTInterface.cpp implementation details based on reference code
bool MQTTInterface::connect(const std::string& host, int port, const std::string& clientId) {
    // Store connection details for potential reconnection
    host_ = host;
    port_ = port;
    clientId_ = clientId;
    
    try {
        // Create connection string
        std::string serverURI = "tcp://" + host + ":" + std::to_string(port);
        
        // Create MQTT client instance
        client_ = std::make_unique<mqtt::async_client>(serverURI, clientId);
        
        // Set up callbacks
        client_->set_callback(*this);
        
        // Connection options
        mqtt::connect_options connOpts;
        connOpts.set_keep_alive_interval(20);
        connOpts.set_clean_session(true);
        
        // Set authentication if provided
        if (!username_.empty() && !password_.empty()) {
            connOpts.set_user_name(username_);
            connOpts.set_password(password_);
        }
        
        // Set Last Will and Testament (LWT) message if configured
        if (lwt_topic_ != "") {
            mqtt::will_options willOptions;
            willOptions.set_topic(lwt_topic_);
            willOptions.set_payload(lwt_message_);
            willOptions.set_qos(lwt_qos_);
            willOptions.set_retained(lwt_retained_);
            connOpts.set_will(willOptions);
        }
        
        // Connect to broker
        mqtt::token_ptr conntok = client_->connect(connOpts);
        conntok->wait();
        isConnected_ = true;
        
        // Resubscribe to topics if reconnecting
        for (const auto& topic : subscriptions_) {
            client_->subscribe(topic, qos_)->wait();
        }
        
        return true;
    }
    catch (const mqtt::exception& exc) {
        std::cerr << "MQTT connection failed: " << exc.what() << std::endl;
        isConnected_ = false;
        return false;
    }
}
```

### MQTT Message Publishing

```cpp
bool MQTTInterface::publish(const std::string& topic, const std::string& payload, int qos, bool retained) {
    if (!isConnected_) {
        if (!autoReconnect()) return false;
    }
    
    try {
        mqtt::message_ptr pubmsg = mqtt::make_message(topic, payload);
        pubmsg->set_qos(qos);
        pubmsg->set_retained(retained);
        
        mqtt::delivery_token_ptr pubtok = client_->publish(pubmsg);
        pubtok->wait_for(publishTimeout_);
        
        return pubtok->is_complete();
    }
    catch (const mqtt::exception& exc) {
        std::cerr << "MQTT publish failed: " << exc.what() << std::endl;
        return false;
    }
}
```

### MQTT Message Subscription

```cpp
void MQTTInterface::messageArrived(mqtt::const_message_ptr msg) {
    if (messageCallback_) {
        messageCallback_(msg->get_topic(), msg->get_payload_str());
    }
}

bool MQTTInterface::subscribe(const std::string& topic, int qos) {
    if (!isConnected_) {
        if (!autoReconnect()) return false;
    }
    
    try {
        mqtt::token_ptr subtok = client_->subscribe(topic, qos);
        subtok->wait();
        subscriptions_[topic] = qos;
        return true;
    }
    catch (const mqtt::exception& exc) {
        std::cerr << "MQTT subscribe failed: " << exc.what() << std::endl;
        return false;
    }
}
```

## JSON Implementation

Based on the reference `JsonParse.cpp`, we'll implement JSON handling for IoT messages:

```cpp
// JSON Utilities for IoT messages
class JsonUtils {
public:
    static nlohmann::json parseJson(const std::string& jsonStr) {
        try {
            return nlohmann::json::parse(jsonStr);
        }
        catch (nlohmann::json::parse_error& e) {
            std::cerr << "JSON parse error: " << e.what() << std::endl;
            return nlohmann::json();
        }
    }
    
    // Create JSON for sensor data
    static std::string createSensorJson(const std::map<std::string, float>& sensorValues) {
        nlohmann::json j;
        for (const auto& [key, value] : sensorValues) {
            j[key] = value;
        }
        return j.dump();
    }
    
    // Extract float value from JSON
    static float extractFloatValue(const nlohmann::json& json, const std::string& key) {
        if (json.contains(key) && json[key].is_number()) {
            return json[key].get<float>();
        }
        return 0.0f;
    }
    
    // Extract string value from JSON
    static std::string extractStringValue(const nlohmann::json& json, const std::string& key) {
        if (json.contains(key) && json[key].is_string()) {
            return json[key].get<std::string>();
        }
        return "";
    }
};
```

## Last Will and Testament Support

Based on the reference implementation's LWT handling, we'll add robust LWT support:

```cpp
// Add to MQTTInterface.h
class MQTTInterface : public IoTInterface {
public:
    // ... existing code ...
    
    // Last Will and Testament configuration
    void setLastWill(const std::string& topic, const std::string& message, 
                   int qos = 1, bool retained = false);
    
private:
    // ... existing code ...
    
    // LWT properties
    std::string lwt_topic_;
    std::string lwt_message_;
    int lwt_qos_ = 1;
    bool lwt_retained_ = false;
};

// Implementation in MQTTInterface.cpp
void MQTTInterface::setLastWill(const std::string& topic, const std::string& message, 
                             int qos, bool retained) {
    lwt_topic_ = topic;
    lwt_message_ = message;
    lwt_qos_ = qos;
    lwt_retained_ = retained;
    
    // If already connected, need to disconnect and reconnect for LWT to take effect
    if (isConnected_) {
        disconnect();
        connect(host_, port_, clientId_);
    }
}
```

## Connection Reliability Enhancements

Based on the reference implementation, we'll add enhanced connection management:

```cpp
// Add to MQTTInterface.h
class MQTTInterface : public IoTInterface {
public:
    // ... existing code ...
    
    // Configure automatic reconnection
    void setAutoReconnect(bool autoReconnect, int maxRetries = 5, int retryInterval = 5000);
    
private:
    // ... existing code ...
    
    // Reconnection properties
    bool autoReconnect_ = true;
    int maxRetries_ = 5;
    int retryInterval_ = 5000;
    int connectRetries_ = 0;
    
    bool autoReconnect();
};

// Implementation in MQTTInterface.cpp
bool MQTTInterface::autoReconnect() {
    if (!autoReconnect_ || connectRetries_ >= maxRetries_) return false;
    
    connectRetries_++;
    std::cout << "Attempting to reconnect to MQTT broker (attempt " 
              << connectRetries_ << "/" << maxRetries_ << ")" << std::endl;
    
    bool success = connect(host_, port_, clientId_);
    
    if (success) {
        connectRetries_ = 0;
        return true;
    }
    
    // Wait before next reconnection attempt
    std::this_thread::sleep_for(std::chrono::milliseconds(retryInterval_));
    return false;
}

void MQTTInterface::setAutoReconnect(bool autoReconnect, int maxRetries, int retryInterval) {
    autoReconnect_ = autoReconnect;
    maxRetries_ = maxRetries;
    retryInterval_ = retryInterval;
}
```

## Quality of Service Configuration

The reference implementation uses different QoS levels, which we'll support through a configuration API:

```cpp
// Add to IoTInterface.h
class IoTInterface {
public:
    // ... existing code ...
    
    // QoS configuration constants
    static const int QOS_AT_MOST_ONCE = 0;
    static const int QOS_AT_LEAST_ONCE = 1;
    static const int QOS_EXACTLY_ONCE = 2;
    
    // Set default QoS for publications
    virtual void setDefaultQoS(int qos) = 0;
    
    // Publish with specific QoS
    virtual bool publish(const std::string& topic, const std::string& payload, 
                       int qos, bool retained = false) = 0;
};

// Implementation in MQTTInterface.cpp
void MQTTInterface::setDefaultQoS(int qos) {
    defaultQoS_ = qos;
}

bool MQTTInterface::publish(const std::string& topic, const std::string& payload) {
    return publish(topic, payload, defaultQoS_, false);
}
```

## MQTT Error Handling

Based on the error handling in the reference implementation:

```cpp
// Add to MQTTInterface.h
class MQTTInterface : public IoTInterface {
public:
    // ... existing code ...
    
    // Get last error message
    std::string getLastError() const { return lastErrorMessage_; }
    
private:
    // ... existing code ...
    
    std::string lastErrorMessage_;
    
    // Error handling methods
    void setError(const std::string& message);
};

// Implementation in MQTTInterface.cpp
void MQTTInterface::setError(const std::string& message) {
    lastErrorMessage_ = message;
    
    // Log the error
    std::cerr << "MQTT Error: " << message << std::endl;
    
    // Optionally notify error listeners
    if (errorCallback_) {
        errorCallback_(message);
    }
}
```

## Topic Pattern Matching Implementation

The reference implementation subscribes to wildcard topics (e.g., "ee513/+"), which we'll implement in our code:

```cpp
// Improved topic pattern matching
bool IoTEventAdapter::matchTopicPattern(const std::string& topic, const std::string& pattern) const {
    // Helper function to split topic into segments
    auto splitTopic = [](const std::string& topic) {
        std::vector<std::string> segments;
        std::string segment;
        std::istringstream stream(topic);
        while (std::getline(stream, segment, '/')) {
            segments.push_back(segment);
        }
        return segments;
    };
    
    // Exact match
    if (pattern == topic) {
        return true;
    }
    
    // Multi-level wildcard #
    if (pattern.find('#') != std::string::npos) {
        // # must be the last character and preceded by /
        if (pattern.back() == '#') {
            if (pattern.length() == 1) {
                // Pattern is just "#" - matches everything
                return true;
            }
            if (pattern[pattern.length() - 2] == '/') {
                // Check if topic starts with the prefix before "/#"
                std::string prefix = pattern.substr(0, pattern.length() - 2);
                return topic == prefix || 
                       (topic.length() > prefix.length() && 
                        topic.substr(0, prefix.length()) == prefix && 
                        topic[prefix.length()] == '/');
            }
        }
    }
    
    // Single-level wildcard +
    if (pattern.find('+') != std::string::npos) {
        std::vector<std::string> patternSegments = splitTopic(pattern);
        std::vector<std::string> topicSegments = splitTopic(topic);
        
        // Different lengths (except for trailing # in pattern) means no match
        if (patternSegments.size() != topicSegments.size()) {
            return false;
        }
        
        // Compare each segment
        for (size_t i = 0; i < patternSegments.size(); i++) {
            if (patternSegments[i] != "+" && patternSegments[i] != topicSegments[i]) {
                return false;
            }
        }
        
        return true;
    }
    
    return false;
}
```

## Enhanced Sensor Data Example

Based on the sensor handling in the reference implementation:

```cpp
// Example usage with sensor data
class SensorPublisher {
public:
    SensorPublisher(IoTInterface* iotInterface) 
        : iotInterface_(iotInterface) {}
    
    void publishSensorData(const std::string& topic, 
                         const std::map<std::string, float>& sensorValues) {
        // Create JSON payload
        std::string payload = JsonUtils::createSensorJson(sensorValues);
        
        // Publish with QoS 1 (at least once)
        iotInterface_->publish(topic, payload, IoTInterface::QOS_AT_LEAST_ONCE);
    }
    
    // Specialized publish method for accelerometer data
    void publishAccelerometerData(const std::string& topic, 
                               float pitch, float roll, float yaw = 0.0f) {
        std::map<std::string, float> values = {
            {"pitch", pitch},
            {"roll", roll},
            {"yaw", yaw}
        };
        publishSensorData(topic, values);
    }
    
private:
    IoTInterface* iotInterface_;
};
```

## Practical Example: Integration with an ESP32 Sensor Node

The reference implementation includes an ADXL345 accelerometer. Here's how we might integrate similar sensor data:

```cpp
// Example: Receiving and using accelerometer data from ESP32
void setupAccelerometerControl(IoTEventAdapter* iotAdapter, ModulationMatrix* modMatrix) {
    // Create pitch and roll parameters
    auto* pitchParam = paramManager.createParameter<FloatParameter>(
        "sensors/pitch", "Sensor Pitch", 0.0f);
    pitchParam->setRange(-90.0f, 90.0f);
    
    auto* rollParam = paramManager.createParameter<FloatParameter>(
        "sensors/roll", "Sensor Roll", 0.0f);
    rollParam->setRange(-90.0f, 90.0f);
    
    // Map topics to parameters
    iotAdapter->mapTopicToParameter("sensors/accel/json", pitchParam);
    iotAdapter->setParameterConverter("sensors/accel/json", 
        [](const std::string& payload) {
            auto json = JsonUtils::parseJson(payload);
            return JsonUtils::extractFloatValue(json, "pitch");
        });
        
    iotAdapter->mapTopicToParameter("sensors/accel/json", rollParam);
    iotAdapter->setParameterConverter("sensors/accel/json", 
        [](const std::string& payload) {
            auto json = JsonUtils::parseJson(payload);
            return JsonUtils::extractFloatValue(json, "roll");
        });
    
    // Create mappings for musical control
    
    // 1. Map pitch to filter cutoff
    auto pitchMapping = rtProcessor.createMapping("pitch_to_cutoff", "Pitch -> Filter Cutoff");
    pitchMapping->setSourceParameter(pitchParam);
    pitchMapping->setTargetParameter(paramManager.getParameterByPath("filter/cutoff"));
    pitchMapping->setSourceRange(-45.0f, 45.0f);
    pitchMapping->setTargetRange(200.0f, 8000.0f);
    
    // 2. Map roll to pan position
    auto rollMapping = rtProcessor.createMapping("roll_to_pan", "Roll -> Pan Position");
    rollMapping->setSourceParameter(rollParam);
    rollMapping->setTargetParameter(paramManager.getParameterByPath("mixer/pan"));
    rollMapping->setSourceRange(-45.0f, 45.0f);
    rollMapping->setTargetRange(-1.0f, 1.0f);
    
    // 3. Create threshold event triggers
    auto pitchHighTrigger = std::make_unique<EventTrigger>(
        "pitchHigh", EventTrigger::TriggerType::PARAMETER_THRESHOLD);
    pitchHighTrigger->setParameterThreshold(pitchParam, 30.0f, true);
    pitchHighTrigger->setEvent(std::make_unique<Event>("pitchExceedsThreshold"));
    
    auto rollExtremeTrigger = std::make_unique<EventTrigger>(
        "rollExtreme", EventTrigger::TriggerType::PARAMETER_THRESHOLD);
    rollExtremeTrigger->setParameterThreshold(rollParam, 40.0f, true);
    rollExtremeTrigger->setEvent(std::make_unique<StateChangeEvent>("intense"));
}
```

## Conclusion

These updates, based on the reference Paho MQTT implementation, provide detailed code examples to enhance our IoT integration. The implementation showcases:

1. Proper MQTT client setup and connection management
2. Error handling and reconnection strategies
3. Last Will and Testament configuration
4. QoS control for message delivery guarantees
5. JSON message parsing for sensor data
6. Topic pattern matching for flexible subscriptions
7. Integration with sensor hardware like accelerometers

By incorporating these proven patterns from the reference implementation, our IoT integration will be more robust, secure, and maintainable.