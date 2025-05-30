# IoT Technical Reference

## Architecture Overview

The IoT integration system follows a layered architecture designed for flexibility, performance, and maintainability:

```
Application Layer
├── IoTConfigManager (Configuration & Management)
├── IoTEventAdapter (Event Integration)
└── IoTParameterTypes (Type System)

Protocol Layer
├── MQTTInterface (MQTT Implementation)
├── IoTInterface (Abstract Base)
└── mqtt_include.h (Conditional Headers)

Integration Layer
├── ParameterManager (Parameter Binding)
├── EventBus (Event System)
└── StateBasedMusicSystem (Adaptive Music)
```

## Core Classes

### IoTInterface (Abstract Base)

```cpp
class IoTInterface {
public:
    virtual ~IoTInterface() = default;
    
    // Connection management
    virtual bool connect(const std::string& host, int port, const std::string& clientId) = 0;
    virtual void disconnect() = 0;
    virtual bool isConnected() const = 0;
    virtual void update() = 0;
    
    // Topic management
    virtual bool subscribe(const std::string& topic) = 0;
    virtual bool unsubscribe(const std::string& topic) = 0;
    
    // Message publishing
    virtual bool publish(const std::string& topic, const std::string& payload) = 0;
    virtual bool publish(const std::string& topic, const std::string& payload, 
                         int qos, bool retain) = 0;
    
    // Callback management
    using MessageCallback = std::function<void(const std::string& topic, 
                                            const std::string& payload)>;
    virtual void setMessageCallback(MessageCallback callback) = 0;
    virtual void setTopicCallback(const std::string& topic, MessageCallback callback) = 0;
    virtual void removeTopicCallback(const std::string& topic) = 0;
};
```

### MQTTInterface Implementation

```cpp
class MQTTInterface : public IoTInterface {
private:
    // Connection state
    std::unique_ptr<mqtt::async_client> client_;
    std::unique_ptr<mqtt::callback> callback_;
    std::string clientId_;
    std::string serverUri_;
    bool connected_ = false;
    
    // Threading
    std::mutex connectionMutex_;
    std::mutex callbackMutex_;
    std::thread updateThread_;
    std::atomic<bool> shouldStop_{false};
    
    // Callbacks
    MessageCallback globalCallback_;
    std::map<std::string, MessageCallback> topicCallbacks_;
    std::map<std::string, std::regex> topicPatterns_;
    
public:
    // Connection options
    struct ConnectionOptions {
        std::string username;
        std::string password;
        int keepAlive = 60;
        bool cleanSession = true;
        int connectTimeout = 30;
        
        // SSL/TLS options
        bool useSSL = false;
        std::string caCertPath;
        std::string clientCertPath;
        std::string clientKeyPath;
        
        // Last Will and Testament
        std::string willTopic;
        std::string willMessage;
        int willQoS = 0;
        bool willRetain = false;
    };
    
    bool connect(const std::string& host, int port, const std::string& clientId,
                 const ConnectionOptions& options = {});
};
```

### IoTConfigManager

```cpp
class IoTConfigManager {
public:
    struct DeviceConfig {
        std::string deviceId;
        std::string deviceType;
        std::string name;
        std::map<std::string, std::string> properties;
        std::vector<std::string> topics;
        bool enabled = true;
        
        // Sensor-specific configuration
        IoTParameterConverter::SensorType sensorType;
        float minValue = 0.0f;
        float maxValue = 1.0f;
        std::string unit;
        
        // Update configuration
        float updateRate = 1.0f;  // Hz
        bool useFiltering = true;
        float filterAlpha = 0.1f;  // Low-pass filter coefficient
    };
    
    struct ParameterMapping {
        std::string topic;
        std::string parameterId;
        IoTParameterConverter::SensorType sensorType;
        float minValue, maxValue;
        IoTParameterMappings::MappingMode mappingMode;
        float threshold = 0.0f;
        float exponent = 1.0f;
        bool enabled = true;
    };
    
    struct EventMapping {
        std::string topic;
        std::string eventType;
        std::string condition;  // "=", ">", "<", ">=", "<=", "!="
        std::string value;
        bool enabled = true;
    };
    
private:
    IoTInterface* iotInterface_;
    std::map<std::string, DeviceConfig> devices_;
    std::map<std::string, ParameterMapping> parameterMappings_;
    std::map<std::string, EventMapping> eventMappings_;
    
    // Data filtering and validation
    std::map<std::string, float> lastValues_;
    std::map<std::string, std::chrono::steady_clock::time_point> lastUpdates_;
    
    // Statistics
    struct TopicStats {
        uint64_t messageCount = 0;
        uint64_t errorCount = 0;
        std::chrono::steady_clock::time_point lastMessage;
        float averageValue = 0.0f;
        float minValue = std::numeric_limits<float>::max();
        float maxValue = std::numeric_limits<float>::lowest();
    };
    std::map<std::string, TopicStats> stats_;
};
```

### IoTEventAdapter

```cpp
class IoTEventAdapter {
public:
    struct TopicEventMapping {
        std::string topic;
        std::string eventType;
        std::regex topicPattern;
        
        // Conditional logic
        enum ConditionType { Always, Equals, GreaterThan, LessThan, Range };
        ConditionType condition = Always;
        std::string expectedValue;
        float numericThreshold = 0.0f;
        float rangeMin = 0.0f, rangeMax = 1.0f;
    };
    
    struct TopicParameterMapping {
        std::string topic;
        Parameter* parameter;
        IoTParameterConverter::SensorType sensorType;
        float minValue, maxValue;
        
        // Filtering and smoothing
        bool useSmoothing = false;
        float smoothingFactor = 0.1f;
        float lastValue = 0.0f;
        
        // Rate limiting
        float maxUpdateRate = 60.0f;  // Hz
        std::chrono::steady_clock::time_point lastUpdate;
    };
    
private:
    IoTInterface* iotInterface_;
    EventBus* eventBus_;
    
    std::vector<TopicEventMapping> eventMappings_;
    std::vector<TopicParameterMapping> parameterMappings_;
    std::map<std::string, IoTParameterConverter::SensorType> sensorTypes_;
    
    // Threading
    std::atomic<bool> isRunning_{false};
    std::thread processingThread_;
    
    // Message queue for thread-safe processing
    struct QueuedMessage {
        std::string topic;
        std::string payload;
        std::chrono::steady_clock::time_point timestamp;
    };
    
    static constexpr size_t QUEUE_SIZE = 1024;
    std::array<QueuedMessage, QUEUE_SIZE> messageQueue_;
    std::atomic<size_t> queueHead_{0};
    std::atomic<size_t> queueTail_{0};
};
```

## Data Conversion System

### IoTParameterConverter

```cpp
class IoTParameterConverter {
public:
    enum class SensorType {
        Temperature,    // Celsius
        Humidity,       // Percentage
        Light,          // Lux
        Motion,         // Boolean
        Sound,          // Decibels
        Accelerometer,  // G-force
        Gyroscope,      // Degrees/second
        Magnetometer,   // Microtesla
        Pressure,       // Hectopascals
        Proximity,      // Distance (0-255)
        Custom          // User-defined
    };
    
    struct ConversionSpec {
        float inputMin, inputMax;
        float outputMin, outputMax;
        enum CurveType { Linear, Exponential, Logarithmic, Custom } curve = Linear;
        std::function<float(float)> customCurve;
    };
    
    static float convertSensorValue(float rawValue, SensorType sensorType,
                                   float targetMin = 0.0f, float targetMax = 1.0f);
    
    static ConversionSpec getDefaultConversion(SensorType sensorType);
    
    // Standard sensor ranges
    static constexpr float TEMP_MIN = -40.0f, TEMP_MAX = 85.0f;
    static constexpr float HUMIDITY_MIN = 0.0f, HUMIDITY_MAX = 100.0f;
    static constexpr float LIGHT_MIN = 0.0f, LIGHT_MAX = 100000.0f;
    static constexpr float SOUND_MIN = 0.0f, SOUND_MAX = 130.0f;
    static constexpr float ACCEL_MIN = -16.0f, ACCEL_MAX = 16.0f;
    static constexpr float GYRO_MIN = -2000.0f, GYRO_MAX = 2000.0f;
    static constexpr float MAG_MIN = -4912.0f, MAG_MAX = 4912.0f;
    static constexpr float PRESSURE_MIN = 300.0f, PRESSURE_MAX = 1100.0f;
    static constexpr float PROXIMITY_MIN = 0.0f, PROXIMITY_MAX = 255.0f;
};
```

### IoTParameterMappings

```cpp
class IoTParameterMappings {
public:
    enum class MappingMode {
        Direct,         // 1:1 mapping
        Inverted,       // 1 - x mapping
        Threshold,      // Step function at threshold
        Exponential,    // x^exponent mapping
        Logarithmic,    // log(x) mapping
        Sine,           // sin(x * pi/2) mapping
        Custom          // User-defined function
    };
    
    struct MappingFunction {
        MappingMode mode;
        float threshold = 0.5f;
        float exponent = 2.0f;
        std::function<float(float)> customFunction;
        
        float apply(float input) const;
    };
    
    // Predefined mapping functions
    static MappingFunction createLinear();
    static MappingFunction createInverted();
    static MappingFunction createThreshold(float threshold);
    static MappingFunction createExponential(float exponent);
    static MappingFunction createLogarithmic();
    static MappingFunction createSine();
};
```

## MQTT Implementation Details

### Conditional Compilation

The system uses conditional compilation to gracefully handle missing MQTT libraries:

```cpp
// mqtt_include.h
#ifdef HAVE_PAHO_MQTT && !defined(USE_MOCK_MQTT)
    #include <mqtt/async_client.h>
    #include <mqtt/callback.h>
    #include <mqtt/connect_options.h>
    #include <mqtt/message.h>
    #include <mqtt/topic.h>
    
    using MQTTClient = mqtt::async_client;
    using MQTTMessage = mqtt::message;
    using MQTTConnectOptions = mqtt::connect_options;
    
#else
    // Mock implementation
    class MockMQTTClient {
    public:
        bool connect(const std::string& uri) { return true; }
        void disconnect() {}
        bool is_connected() const { return true; }
        void publish(const std::string& topic, const std::string& payload) {}
        void subscribe(const std::string& topic) {}
    };
    
    using MQTTClient = MockMQTTClient;
#endif
```

### Thread Safety

The MQTT implementation is fully thread-safe:

```cpp
class MQTTInterface : public IoTInterface {
private:
    // Thread-safe message handling
    void messageArrived(mqtt::const_message_ptr msg) {
        std::string topic = msg->get_topic();
        std::string payload = msg->to_string();
        
        // Queue message for processing in main thread
        QueuedMessage queuedMsg{topic, payload, std::chrono::steady_clock::now()};
        
        size_t tail = queueTail_.load(std::memory_order_relaxed);
        size_t nextTail = (tail + 1) % QUEUE_SIZE;
        
        if (nextTail != queueHead_.load(std::memory_order_acquire)) {
            messageQueue_[tail] = std::move(queuedMsg);
            queueTail_.store(nextTail, std::memory_order_release);
        }
    }
    
    // Process queued messages
    void processMessages() {
        size_t head = queueHead_.load(std::memory_order_relaxed);
        size_t tail = queueTail_.load(std::memory_order_acquire);
        
        while (head != tail) {
            const auto& msg = messageQueue_[head];
            
            // Process message callbacks
            if (globalCallback_) {
                globalCallback_(msg.topic, msg.payload);
            }
            
            // Check topic-specific callbacks
            for (const auto& [pattern, callback] : topicCallbacks_) {
                if (std::regex_match(msg.topic, topicPatterns_[pattern])) {
                    callback(msg.topic, msg.payload);
                }
            }
            
            head = (head + 1) % QUEUE_SIZE;
        }
        
        queueHead_.store(head, std::memory_order_release);
    }
};
```

### Quality of Service (QoS)

MQTT QoS levels are fully supported:

```cpp
enum class QoSLevel : int {
    AtMostOnce = 0,     // Fire and forget
    AtLeastOnce = 1,    // Acknowledged delivery
    ExactlyOnce = 2     // Guaranteed delivery
};

bool MQTTInterface::publish(const std::string& topic, const std::string& payload,
                           QoSLevel qos, bool retain) {
    if (!isConnected()) return false;
    
    auto msg = mqtt::make_message(topic, payload);
    msg->set_qos(static_cast<int>(qos));
    msg->set_retained(retain);
    
    try {
        auto tok = client_->publish(msg);
        if (qos > QoSLevel::AtMostOnce) {
            tok->wait();  // Wait for acknowledgment
        }
        return true;
    } catch (const mqtt::exception& exc) {
        std::cerr << "MQTT publish failed: " << exc.what() << std::endl;
        return false;
    }
}
```

## Performance Considerations

### Message Rate Limiting

```cpp
class RateLimiter {
private:
    std::chrono::steady_clock::time_point lastUpdate_;
    float maxRate_;  // Messages per second
    
public:
    RateLimiter(float maxRate) : maxRate_(maxRate) {}
    
    bool canUpdate() {
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration<float>(now - lastUpdate_).count();
        
        if (elapsed >= (1.0f / maxRate_)) {
            lastUpdate_ = now;
            return true;
        }
        return false;
    }
};
```

### Memory Management

The system uses object pooling for frequent allocations:

```cpp
template<typename T, size_t PoolSize = 256>
class ObjectPool {
private:
    std::array<T, PoolSize> pool_;
    std::array<bool, PoolSize> used_;
    size_t nextIndex_ = 0;
    
public:
    T* acquire() {
        for (size_t i = 0; i < PoolSize; ++i) {
            size_t index = (nextIndex_ + i) % PoolSize;
            if (!used_[index]) {
                used_[index] = true;
                nextIndex_ = (index + 1) % PoolSize;
                return &pool_[index];
            }
        }
        return nullptr;  // Pool exhausted
    }
    
    void release(T* obj) {
        size_t index = obj - &pool_[0];
        if (index < PoolSize) {
            used_[index] = false;
        }
    }
};
```

## Testing Framework

### Mock Implementation

```cpp
class MockMQTTInterface : public IoTInterface {
private:
    struct MockMessage {
        std::string topic;
        std::string payload;
        std::chrono::steady_clock::time_point timestamp;
    };
    
    std::vector<MockMessage> publishedMessages_;
    std::set<std::string> subscribedTopics_;
    MessageCallback messageCallback_;
    std::map<std::string, MessageCallback> topicCallbacks_;
    
public:
    // Simulate message reception
    void simulateMessage(const std::string& topic, const std::string& payload) {
        if (subscribedTopics_.count(topic) || 
            std::any_of(subscribedTopics_.begin(), subscribedTopics_.end(),
                       [&topic](const std::string& pattern) {
                           return topicMatches(topic, pattern);
                       })) {
            
            if (messageCallback_) {
                messageCallback_(topic, payload);
            }
            
            for (const auto& [pattern, callback] : topicCallbacks_) {
                if (topicMatches(topic, pattern)) {
                    callback(topic, payload);
                }
            }
        }
    }
    
    // Access for testing
    const std::vector<MockMessage>& getPublishedMessages() const {
        return publishedMessages_;
    }
    
    void clearPublishedMessages() {
        publishedMessages_.clear();
    }
};
```

This technical reference provides the complete implementation details for the IoT/MQTT system, enabling developers to understand, extend, and maintain the codebase effectively.