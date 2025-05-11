# IoT Integration for AIMusicHardware

This document outlines the implementation approach for integrating IoT (Internet of Things) capabilities with the AIMusicHardware adaptive music engine. This integration enables the synthesizer to respond to real-world inputs through connected sensors and devices, creating an environment-aware music system.

## Overview

The IoT integration allows the AIMusicHardware synth to:

1. Receive data from various IoT sensors and devices
2. Use sensor data as input for the Event System to trigger musical changes
3. Map continuous sensor data to Parameters for real-time control
4. Enable state changes based on environmental conditions
5. Create complex mappings between physical phenomena and musical elements

This implementation builds upon our existing adaptive music systems:
- State-Based Music System
- Vertical Remix System
- Horizontal Re-sequencing
- Parameter System
- Event System
- RTPC (Real-Time Parameter Control)

## IoT Technologies

### Recommended Technologies

1. **MQTT Protocol**
   - Lightweight publish/subscribe messaging protocol
   - Low bandwidth requirements
   - Widely supported in IoT ecosystems
   - Simple to implement and scale
   - Topic-based message routing

2. **ESP32 Microcontroller**
   - Low-cost, high-performance microcontroller
   - Built-in WiFi and Bluetooth
   - Large community and extensive library support
   - Ideal for creating sensor nodes
   - Can interface with many sensor types

3. **Node-RED (Optional)**
   - Flow-based programming for IoT
   - Visual wiring of devices, APIs, and services
   - Large library of pre-built nodes
   - Useful for prototyping and data routing

4. **Home Assistant (Optional)**
   - Open-source home automation platform
   - Can act as a central IoT hub
   - Extensive device support
   - Automation capabilities

5. **WebSockets**
   - Real-time bidirectional communication
   - Useful for web interfaces
   - Allows browser-based visualization and control

## Implementation Architecture

### 1. IoT Interface Layer

The core interface for IoT communication:

```cpp
// IoTInterface.h
class IoTInterface {
public:
    virtual ~IoTInterface() = default;
    
    // Connect to IoT network/broker
    virtual bool connect(const std::string& host, int port, const std::string& clientId) = 0;
    
    // Disconnect from IoT network/broker
    virtual void disconnect() = 0;
    
    // Check connection status
    virtual bool isConnected() const = 0;
    
    // Process incoming messages
    virtual void update() = 0;
    
    // Subscribe to a topic to receive messages
    virtual bool subscribe(const std::string& topic) = 0;
    
    // Unsubscribe from a topic
    virtual bool unsubscribe(const std::string& topic) = 0;
    
    // Publish a message to a topic
    virtual bool publish(const std::string& topic, const std::string& payload) = 0;
    
    // Register message handler
    using MessageCallback = std::function<void(const std::string& topic, 
                                             const std::string& payload)>;
    virtual void setMessageCallback(MessageCallback callback) = 0;
};
```

### 2. MQTT Implementation

Implementation of the IoT interface using MQTT:

```cpp
// MQTTInterface.h
class MQTTInterface : public IoTInterface {
public:
    MQTTInterface();
    ~MQTTInterface() override;
    
    // IoTInterface implementation
    bool connect(const std::string& host, int port, const std::string& clientId) override;
    void disconnect() override;
    bool isConnected() const override;
    void update() override;
    bool subscribe(const std::string& topic) override;
    bool unsubscribe(const std::string& topic) override;
    bool publish(const std::string& topic, const std::string& payload) override;
    void setMessageCallback(MessageCallback callback) override;
    
private:
    // MQTT client implementation (using Mosquitto or Paho)
    std::unique_ptr<mqtt::async_client> client_;
    MessageCallback messageCallback_;
    
    // Internal message handler
    void onMessage(const std::string& topic, const mqtt::const_message_ptr& msg);
    
    // Connection status tracking
    bool isConnected_ = false;
    std::string host_;
    int port_;
    std::string clientId_;
    
    // Subscription tracking
    std::set<std::string> subscriptions_;
};
```

### 3. IoT Event Adapter

Bridge between IoT messages and our Event System:

```cpp
// IoTEventAdapter.h
class IoTEventAdapter {
public:
    IoTEventAdapter(IoTInterface* iotInterface, EventBus* eventBus);
    ~IoTEventAdapter();
    
    // Map IoT topics to event types
    void mapTopicToEvent(const std::string& topic, const std::string& eventType);
    
    // Map topic payload value to parameter
    void mapTopicToParameter(const std::string& topic, Parameter* parameter);
    
    // Add payload conversion function
    void setMessageConverter(const std::string& topic, 
                           std::function<std::any(const std::string&)> converter);
    
    // Add parameter value conversion
    void setParameterConverter(const std::string& topic,
                             std::function<float(const std::string&)> converter);
    
    // Start/stop processing
    void start();
    void stop();
    
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
};
```

### 4. IoT Configuration Manager

Manage device discovery and configuration:

```cpp
// IoTConfigManager.h
class IoTConfigManager {
public:
    IoTConfigManager();
    ~IoTConfigManager();
    
    // Load/save configuration
    bool loadConfig(const std::string& filename);
    bool saveConfig(const std::string& filename);
    
    // Device discovery
    void startDiscovery();
    void stopDiscovery();
    
    // Get discovered devices
    std::vector<IoTDevice> getDiscoveredDevices() const;
    
    // Device management
    void addDevice(const IoTDevice& device);
    void removeDevice(const std::string& deviceId);
    
    // Get device info
    IoTDevice* getDevice(const std::string& deviceId);
    
private:
    std::vector<IoTDevice> devices_;
    bool isDiscovering_ = false;
    std::unique_ptr<IoTInterface> discoveryInterface_;
    
    // Discovery implementation
    void onDeviceDiscovered(const IoTDevice& device);
    void parseDiscoveryMessage(const std::string& topic, const std::string& payload);
};

// IoTDevice structure
struct IoTDevice {
    std::string id;
    std::string name;
    std::string type;
    std::vector<std::string> topics;
    std::map<std::string, std::string> capabilities;
};
```

### 5. IoT Modulation Source

Enable sensor data to be used as modulation source:

```cpp
// IoTModulationSource.h
class IoTModulationSource : public ModulationSource {
public:
    IoTModulationSource(const SourceId& id, const std::string& name, 
                       const std::string& topic);
    ~IoTModulationSource() override;
    
    // Configure topic and format
    void setTopic(const std::string& topic);
    std::string getTopic() const { return topic_; }
    
    // Set value format conversion
    void setValueConverter(std::function<float(const std::string&)> converter);
    
    // Set value range
    void setRange(float min, float max);
    float getMinValue() const { return minValue_; }
    float getMaxValue() const { return maxValue_; }
    
    // Set smoothing
    void setSmoothing(float timeInSeconds);
    float getSmoothing() const { return smoothingTime_; }
    
    // ModulationSource interface
    float getValue() const override;
    void update(double deltaTime) override;
    void reset() override;
    nlohmann::json toJson() const override;
    
    // IoT message handler
    void onMessage(const std::string& topic, const std::string& payload);
    
private:
    std::string topic_;
    std::function<float(const std::string&)> converter_;
    float minValue_ = 0.0f;
    float maxValue_ = 1.0f;
    float currentValue_ = 0.0f;
    float targetValue_ = 0.0f;
    
    // Smoothing
    float smoothingTime_ = 0.0f;
};
```

## Integration with Existing Systems

### State-Based Music System Integration

```cpp
// In StateMachine.cpp
void StateMachine::registerIoTIntegration(IoTEventAdapter* iotAdapter) {
    // Map IoT events to state changes
    EventBus::getInstance().addEventListener("iot_state_change", [this](const Event& event) {
        if (auto* stateEvent = dynamic_cast<const StateChangeEvent*>(&event)) {
            this->changeState(stateEvent->getTargetState());
        } else if (event.hasPayload()) {
            // Try to get state name from payload
            try {
                std::string stateName = event.getPayload<std::string>();
                this->changeState(stateName);
            } catch (...) {
                // Ignore conversion errors
            }
        }
    });
    
    // Create state-specific IoT publishers
    for (const auto& [id, state] : states_) {
        // When entering a state, publish to IoT
        state->setEnterCallback([this, id](MusicState* state) {
            // Publish state change to IoT
            if (iotInterface_) {
                iotInterface_->publish("music/state", id);
            }
        });
    }
}
```

### Event System Integration

```cpp
// In IoTEventAdapter.cpp
void IoTEventAdapter::onIoTMessage(const std::string& topic, const std::string& payload) {
    // Check for event mappings
    for (const auto& mapping : eventMappings_) {
        if (topic == mapping.topic || matchTopicPattern(topic, mapping.topic)) {
            // Convert payload if needed
            std::any eventData = payload;
            if (mapping.converter) {
                eventData = mapping.converter(payload);
            }
            
            // Create and dispatch event
            auto event = std::make_unique<Event>(mapping.eventType);
            event->setPayload(eventData);
            eventBus_->dispatchEvent(*event);
        }
    }
    
    // Check for parameter mappings
    for (const auto& mapping : parameterMappings_) {
        if (topic == mapping.topic || matchTopicPattern(topic, mapping.topic)) {
            // Convert payload to parameter value
            float value = 0.0f;
            if (mapping.converter) {
                value = mapping.converter(payload);
            } else {
                // Default conversion (try to parse as float)
                try {
                    value = std::stof(payload);
                } catch (...) {
                    // Ignore conversion errors
                }
            }
            
            // Set parameter value
            if (auto* floatParam = dynamic_cast<FloatParameter*>(mapping.parameter)) {
                floatParam->setValue(value);
            } else if (auto* intParam = dynamic_cast<IntParameter*>(mapping.parameter)) {
                intParam->setValue(static_cast<int>(std::round(value)));
            }
        }
    }
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
        std::regex rePattern(pattern);
        // Replace + with regex for any non-slash sequence
        std::string regexPattern = std::regex_replace(pattern, std::regex("\\+"), "([^/]+)");
        std::regex re(regexPattern);
        return std::regex_match(topic, re);
    }
    
    // Multi-level wildcard #
    if (pattern.find('#') != std::string::npos) {
        // # must be the last character and preceded by /
        if (pattern.back() == '#') {
            std::string prefix = pattern.substr(0, pattern.length() - 2); // Remove /# at end
            return topic.compare(0, prefix.length(), prefix) == 0;
        }
    }
    
    return false;
}
```

### Parameter System Integration

```cpp
// In ParameterManager.h
class ParameterManager {
public:
    // ... existing code ...
    
    // IoT integration
    void registerIoTAdapter(IoTEventAdapter* adapter);
    
    // Create IoT-connected parameters
    template<typename T, typename... Args>
    T* createIoTParameter(const std::string& topic, Args&&... args);
    
private:
    // ... existing code ...
    
    IoTEventAdapter* iotAdapter_ = nullptr;
};

// In ParameterManager.cpp
template<typename T, typename... Args>
T* ParameterManager::createIoTParameter(const std::string& topic, Args&&... args) {
    // Create normal parameter
    T* param = createParameter<T>(std::forward<Args>(args)...);
    
    // Register with IoT adapter
    if (iotAdapter_ && param) {
        iotAdapter_->mapTopicToParameter(topic, param);
    }
    
    return param;
}
```

### RTPC System Integration

```cpp
// In ModulationMatrix.h
class ModulationMatrix {
public:
    // ... existing code ...
    
    // Create IoT modulation source
    IoTModulationSource* createIoTModulationSource(
        const std::string& id, 
        const std::string& name,
        const std::string& topic);
        
    // Register with IoT adapter
    void registerIoTAdapter(IoTEventAdapter* adapter);
    
private:
    // ... existing code ...
    
    IoTEventAdapter* iotAdapter_ = nullptr;
    std::vector<IoTModulationSource*> iotSources_;
};

// In ModulationMatrix.cpp
IoTModulationSource* ModulationMatrix::createIoTModulationSource(
    const std::string& id, 
    const std::string& name,
    const std::string& topic) {
    
    auto source = std::make_unique<IoTModulationSource>(id, name, topic);
    IoTModulationSource* result = source.get();
    
    // Add to sources collection
    sources_[id] = std::move(source);
    iotSources_.push_back(result);
    
    // Register with IoT adapter if available
    if (iotAdapter_) {
        iotAdapter_->setMessageCallback(topic, [result](const std::string& t, const std::string& p) {
            result->onMessage(t, p);
        });
    }
    
    return result;
}
```

## Implementation Details

### MQTT Interface Implementation

```cpp
bool MQTTInterface::connect(const std::string& host, int port, const std::string& clientId) {
    host_ = host;
    port_ = port;
    clientId_ = clientId;
    
    try {
        // Create Paho MQTT client
        std::string serverURI = "tcp://" + host + ":" + std::to_string(port);
        client_ = std::make_unique<mqtt::async_client>(serverURI, clientId);
        
        // Set callbacks
        client_->set_callback(*this);
        
        // Connection options
        mqtt::connect_options connOpts;
        connOpts.set_keep_alive_interval(20);
        connOpts.set_clean_session(true);
        
        // Connect to broker
        client_->connect(connOpts)->wait();
        isConnected_ = true;
        
        // Resubscribe to topics
        for (const auto& topic : subscriptions_) {
            client_->subscribe(topic, 0)->wait();
        }
        
        return true;
    }
    catch (const mqtt::exception& exc) {
        std::cerr << "MQTT connection failed: " << exc.what() << std::endl;
        isConnected_ = false;
        return false;
    }
}

void MQTTInterface::update() {
    // Process MQTT client events
    // Note: Paho MQTT client uses callbacks, so this may be a no-op
    // depending on implementation
    
    // Check connection status and reconnect if needed
    if (!isConnected_ && !host_.empty()) {
        connect(host_, port_, clientId_);
    }
}

bool MQTTInterface::subscribe(const std::string& topic) {
    if (!isConnected_) return false;
    
    try {
        client_->subscribe(topic, 0)->wait();
        subscriptions_.insert(topic);
        return true;
    }
    catch (const mqtt::exception& exc) {
        std::cerr << "MQTT subscribe failed: " << exc.what() << std::endl;
        return false;
    }
}

bool MQTTInterface::publish(const std::string& topic, const std::string& payload) {
    if (!isConnected_) return false;
    
    try {
        mqtt::message_ptr pubmsg = mqtt::make_message(topic, payload);
        client_->publish(pubmsg)->wait();
        return true;
    }
    catch (const mqtt::exception& exc) {
        std::cerr << "MQTT publish failed: " << exc.what() << std::endl;
        return false;
    }
}

// MQTT callback override
void MQTTInterface::message_arrived(mqtt::const_message_ptr msg) {
    // Forward to our callback
    if (messageCallback_) {
        messageCallback_(msg->get_topic(), msg->get_payload_str());
    }
}
```

### IoT Modulation Source Implementation

```cpp
void IoTModulationSource::onMessage(const std::string& topic, const std::string& payload) {
    if (topic != topic_ || !enabled_) return;
    
    // Convert payload to float value
    float value = 0.0f;
    
    if (converter_) {
        value = converter_(payload);
    } else {
        // Default conversion (try to parse as float)
        try {
            value = std::stof(payload);
        } catch (...) {
            return; // Ignore conversion errors
        }
    }
    
    // Normalize to 0-1 range if needed
    if (minValue_ != 0.0f || maxValue_ != 1.0f) {
        value = (value - minValue_) / (maxValue_ - minValue_);
        value = std::clamp(value, 0.0f, 1.0f);
    }
    
    // Store value (smoothing applied in update)
    targetValue_ = value;
}

void IoTModulationSource::update(double deltaTime) {
    if (!enabled_) return;
    
    // Apply smoothing if enabled
    if (smoothingTime_ > 0.0f) {
        float smoothingFactor = static_cast<float>(deltaTime) / smoothingTime_;
        if (smoothingFactor > 1.0f) smoothingFactor = 1.0f;
        
        currentValue_ += smoothingFactor * (targetValue_ - currentValue_);
    } else {
        currentValue_ = targetValue_;
    }
}

float IoTModulationSource::getValue() const {
    if (!enabled_) return 0.0f;
    return currentValue_;
}
```

## Example Use Cases

### 1. Weather-Responsive Music System

```cpp
// Setup MQTT connection
auto iotInterface = std::make_unique<MQTTInterface>();
iotInterface->connect("mqtt.weather-service.com", 1883, "music-synth");

// Create IoT-Event adapter
IoTEventAdapter iotAdapter(iotInterface.get(), &EventBus::getInstance());

// Create weather parameters
auto* temperatureParam = paramManager.createParameter<FloatParameter>(
    "environment/temperature", "Temperature", 20.0f);
temperatureParam->setRange(-20.0f, 40.0f);

auto* humidityParam = paramManager.createParameter<FloatParameter>(
    "environment/humidity", "Humidity", 50.0f);
humidityParam->setRange(0.0f, 100.0f);

// Map weather topics to parameters
iotAdapter.mapTopicToParameter("weather/local/temperature", temperatureParam);
iotAdapter.mapTopicToParameter("weather/local/humidity", humidityParam);

// Create state mapping based on weather condition
iotAdapter.mapTopicToEvent("weather/local/condition", "weatherChange");

// In StateMachine setup
stateMachine.addEventListener("weatherChange", [&](const Event& event) {
    std::string condition = event.getPayload<std::string>();
    
    if (condition == "sunny") {
        stateMachine.changeState("bright");
    } else if (condition == "rainy") {
        stateMachine.changeState("melancholic");
    } else if (condition == "storm") {
        stateMachine.changeState("intense");
    }
});

// Create temperature-based sound modulation
auto* lfoDepthParam = paramManager.getParameterByPath("lfo/depth");
auto* reverbWetParam = paramManager.getParameterByPath("reverb/wet");

// Higher temperature = more LFO, less reverb
auto tempMapping = rtProcessor.createMapping("temp_to_lfo", "Temperature -> LFO Depth");
tempMapping->setSourceParameter(temperatureParam);
tempMapping->setTargetParameter(lfoDepthParam);
tempMapping->setSourceRange(0.0f, 30.0f);
tempMapping->setTargetRange(0.1f, 0.8f);

// Higher humidity = more reverb
auto humidityMapping = rtProcessor.createMapping("humidity_to_reverb", "Humidity -> Reverb");
humidityMapping->setSourceParameter(humidityParam);
humidityMapping->setTargetParameter(reverbWetParam);
humidityMapping->setSourceRange(40.0f, 90.0f);
humidityMapping->setTargetRange(0.2f, 0.8f);

// Start processing IoT messages
iotAdapter.start();
```

### 2. Home Automation Integration

```cpp
// Connect to home automation system
iotInterface->connect("home-assistant.local", 1883, "music-synth");

// Subscribe to relevant topics
iotInterface->subscribe("home/livingroom/#");

// Map motion detection to event trigger
iotAdapter.mapTopicToEvent("home/livingroom/motion", "roomActivity");

// Map light level to ambient intensity parameter
auto* ambientParam = paramManager.createParameter<FloatParameter>(
    "music/ambient/intensity", "Ambient Intensity", 0.5f);
ambientParam->setRange(0.0f, 1.0f);

iotAdapter.mapTopicToParameter("home/livingroom/light_level", ambientParam);
iotAdapter.setParameterConverter("home/livingroom/light_level", 
    [](const std::string& payload) {
        // Convert light level (lux) to normalized parameter (0-1)
        float lux = std::stof(payload);
        return std::clamp(lux / 1000.0f, 0.0f, 1.0f);
    });

// Create room activity event handler
EventBus::getInstance().addEventListener("roomActivity", [&](const Event& event) {
    // When motion detected, transition to more active state
    std::string payload = event.getPayload<std::string>();
    if (payload == "detected") {
        stateMachine.changeState("active");
        
        // Schedule return to ambient state after 5 minutes of no motion
        EventBus::getInstance().scheduleEvent(StateChangeEvent("ambient"), 300.0);
    }
});

// Create time-of-day based state changes
iotAdapter.mapTopicToEvent("home/time/period", "timePeriod");
EventBus::getInstance().addEventListener("timePeriod", [&](const Event& event) {
    std::string period = event.getPayload<std::string>();
    
    if (period == "morning") {
        stateMachine.changeState("morning");
    } else if (period == "day") {
        stateMachine.changeState("day");
    } else if (period == "evening") {
        stateMachine.changeState("evening");
    } else if (period == "night") {
        stateMachine.changeState("night");
    }
});
```

### 3. Sensor-Based Musical Control

```cpp
// Connect to local sensor network
iotInterface->connect("192.168.1.100", 1883, "music-synth");

// Subscribe to sensor topics
iotInterface->subscribe("sensors/#");

// Create RTPC mapping for continuous control
auto* lfoFreqParam = modMatrix.getModulationSource("main_lfo")->getFrequencyParameter();

// Map accelerometer data to LFO frequency
iotAdapter.mapTopicToParameter("sensors/accel/magnitude", lfoFreqParam);
iotAdapter.setParameterConverter("sensors/accel/magnitude", 
    [](const std::string& payload) {
        // Convert acceleration magnitude to LFO frequency (0.1 - 10 Hz)
        float accel = std::stof(payload);
        return 0.1f + (std::min(accel, 20.0f) / 20.0f) * 9.9f;
    });

// Create pressure sensor to filter cutoff mapping
auto* cutoffParam = paramManager.getParameterByPath("filter/cutoff");

iotAdapter.mapTopicToParameter("sensors/pressure", cutoffParam);
iotAdapter.setParameterConverter("sensors/pressure", 
    [](const std::string& payload) {
        // Map pressure (0-100) to filter cutoff (100-10000 Hz)
        float pressure = std::stof(payload);
        float normalized = std::clamp(pressure / 100.0f, 0.0f, 1.0f);
        return 100.0f + normalized * 9900.0f;
    });

// Create an IoT modulation source for proximity sensor
auto proximitySrc = modMatrix.createIoTModulationSource(
    "proximity_mod", "Proximity Modulation", "sensors/proximity");
proximitySrc->setRange(0.0f, 150.0f);    // 0-150cm range
proximitySrc->setSmoothing(0.1f);        // 100ms smoothing

// Connect proximity to filter resonance
auto* resonanceParam = paramManager.getParameterByPath("filter/resonance");
auto proxMapping = rtProcessor.createMapping("prox_to_res", "Proximity -> Resonance");
proxMapping->setTargetParameter(resonanceParam);
proxMapping->setTargetRange(0.1f, 0.9f);
proxMapping->setCurve(std::make_unique<ExponentialCurve>(3.0f));  // Exponential curve

// Connect proximity source to mapping
modMatrix.connectSourceToMapping("proximity_mod", "prox_to_res", 1.0f);
```

### 4. Interactive Installation

```cpp
// Connect to exhibition space IoT network
iotInterface->connect("exhibition.local", 1883, "music-installation");

// Subscribe to all exhibition topics
iotInterface->subscribe("exhibition/#");

// Create visitor counter
auto* visitorCountParam = paramManager.createParameter<IntParameter>(
    "installation/visitorCount", "Visitor Count", 0);
visitorCountParam->setRange(0, 100);

// Map visitor counter to parameter
iotAdapter.mapTopicToParameter("exhibition/visitors/count", visitorCountParam);

// Map visitor density to parameter
auto* densityParam = paramManager.createParameter<FloatParameter>(
    "installation/density", "Visitor Density", 0.0f);
densityParam->setRange(0.0f, 1.0f);

iotAdapter.mapTopicToParameter("exhibition/visitors/density", densityParam);

// Create zone activity events
iotAdapter.mapTopicToEvent("exhibition/zone1/activity", "zone1Activity");
iotAdapter.mapTopicToEvent("exhibition/zone2/activity", "zone2Activity");
iotAdapter.mapTopicToEvent("exhibition/zone3/activity", "zone3Activity");

// Create interactive sound elements
EventBus::getInstance().addEventListener("zone1Activity", [&](const Event& event) {
    float activity = std::stof(event.getPayload<std::string>());
    
    // Trigger different musical elements based on activity level
    if (activity > 0.8f) {
        // High activity - play intense segment
        EventBus::getInstance().dispatchEvent(PatternEvent("intense_pattern", PatternEvent::Action::START));
    } else if (activity > 0.4f) {
        // Medium activity - play moderate segment
        EventBus::getInstance().dispatchEvent(PatternEvent("moderate_pattern", PatternEvent::Action::START));
    } else if (activity < 0.1f && activity > 0.0f) {
        // Low activity - play ambient segment
        EventBus::getInstance().dispatchEvent(PatternEvent("ambient_pattern", PatternEvent::Action::START));
    }
});

// Create mapping from visitor density to musical intensity
auto densityMapping = rtProcessor.createMapping("density_to_intensity", "Visitor Density -> Music Intensity");
densityMapping->setSourceParameter(densityParam);

// Use density to control vertical mixing
auto* layerManager = verticalRemixSystem.getLayerManager();
densityMapping->setTargetParameter(paramManager.getParameterByPath("mix/intensity"));

// Connect to display system to show visualization
iotInterface->subscribe("exhibition/display/command");
iotAdapter.mapTopicToEvent("exhibition/display/command", "displayCommand");

// Publish current music state back to IoT
stateMachine.addEventListener("stateChanged", [&](const Event& event) {
    std::string newState = event.getPayload<std::string>();
    iotInterface->publish("music/currentState", newState);
});
```

## Hardware Requirements

### For the AIMusicHardware Synth

1. **Network Connectivity**
   - Ethernet port or WiFi adapter
   - Optional: Bluetooth for local device connectivity

2. **IoT Bridge (if direct networking not available)**
   - Raspberry Pi Zero W ($10-15)
   - ESP32 development board ($5-10)
   - USB-to-UART adapter for serial communication

3. **Software Requirements**
   - MQTT client library (Mosquitto or Paho)
   - JSON parsing library
   - Optional: WebSocket support for browser interfaces

### For Sensor Nodes

1. **Microcontroller Options**
   - ESP32 ($5-10) - WiFi, Bluetooth, many GPIO pins
   - ESP8266 ($3-5) - WiFi, fewer GPIO pins
   - Arduino Nano 33 IoT ($20) - Higher quality, more stable

2. **Common Sensors**
   - BME280 Temperature/Humidity/Pressure ($3-5)
   - PIR Motion Sensor ($2-3)
   - Light Dependent Resistor (LDR) ($0.50-1)
   - MPU6050 Accelerometer/Gyroscope ($2-4)
   - HC-SR04 Ultrasonic Distance Sensor ($1-2)
   - Sound Level Sensor ($2-3)
   - Capacitive Touch Sensors ($1-2)

3. **Power Options**
   - USB power
   - Battery power (18650 with holder)
   - Solar power for outdoor installations

### For Networking

1. **Local Network**
   - WiFi router
   - Optional: Ethernet switch for wired connections
   
2. **MQTT Broker Options**
   - Mosquitto running on Raspberry Pi ($35-40)
   - Cloud-based broker (HiveMQ, CloudMQTT)
   - Home Assistant with integrated MQTT broker

3. **Optional Infrastructure**
   - Raspberry Pi 4 ($35-50) for running Node-RED, Home Assistant
   - Network storage for logging data
   - Internet gateway for remote access

## Firmware for ESP32 Sensor Nodes

Basic ESP32 firmware for IoT sensors:

```cpp
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>

// Network configuration
const char* ssid = "YourWiFiName";
const char* password = "YourWiFiPassword";
const char* mqtt_server = "192.168.1.100";
const int mqtt_port = 1883;
const char* client_id = "esp32_sensor_node1";

// Topic configuration
const char* base_topic = "sensors/node1/";
const char* temp_topic = "sensors/node1/temperature";
const char* humidity_topic = "sensors/node1/humidity";
const char* pressure_topic = "sensors/node1/pressure";

// Hardware configuration
Adafruit_BME280 bme;
WiFiClient espClient;
PubSubClient client(espClient);

// Timing variables
unsigned long last_publish = 0;
const long publish_interval = 5000; // 5 seconds

void setup() {
  Serial.begin(115200);
  
  // Initialize BME280
  if (!bme.begin(0x76)) {
    Serial.println("Could not find BME280 sensor!");
    while (1);
  }
  
  // Connect to WiFi
  setup_wifi();
  
  // Setup MQTT
  client.setServer(mqtt_server, mqtt_port);
  
  // Announce device capabilities
  announce_device();
}

void setup_wifi() {
  delay(10);
  Serial.println("Connecting to WiFi...");
  
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void reconnect_mqtt() {
  while (!client.connected()) {
    Serial.print("Connecting to MQTT broker...");
    if (client.connect(client_id)) {
      Serial.println("connected");
      // Subscribe to control topics
      client.subscribe("control/sensors/node1/#");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" retrying in 5 seconds");
      delay(5000);
    }
  }
}

void announce_device() {
  // Create JSON document with device capabilities
  StaticJsonDocument<512> doc;
  doc["id"] = client_id;
  doc["name"] = "Environmental Sensor Node 1";
  doc["type"] = "sensor";
  
  JsonArray capabilities = doc.createNestedArray("capabilities");
  capabilities.add("temperature");
  capabilities.add("humidity");
  capabilities.add("pressure");
  
  JsonArray topics = doc.createNestedArray("topics");
  topics.add(temp_topic);
  topics.add(humidity_topic);
  topics.add(pressure_topic);
  
  // Serialize and publish
  char buffer[512];
  serializeJson(doc, buffer);
  
  client.publish("discovery/sensors", buffer, true); // Retained message
}

void publish_sensor_data() {
  // Read sensor values
  float temperature = bme.readTemperature();
  float humidity = bme.readHumidity();
  float pressure = bme.readPressure() / 100.0F; // Convert to hPa
  
  // Check if values are valid
  if (isnan(temperature) || isnan(humidity) || isnan(pressure)) {
    Serial.println("Failed to read from BME280 sensor!");
    return;
  }
  
  // Convert to strings
  char temp_str[10];
  char humidity_str[10];
  char pressure_str[10];
  
  sprintf(temp_str, "%.2f", temperature);
  sprintf(humidity_str, "%.2f", humidity);
  sprintf(pressure_str, "%.2f", pressure);
  
  // Publish values
  client.publish(temp_topic, temp_str);
  client.publish(humidity_topic, humidity_str);
  client.publish(pressure_topic, pressure_str);
  
  // Also publish as JSON
  StaticJsonDocument<256> doc;
  doc["temperature"] = temperature;
  doc["humidity"] = humidity;
  doc["pressure"] = pressure;
  doc["timestamp"] = millis();
  
  char buffer[256];
  serializeJson(doc, buffer);
  
  client.publish("sensors/node1/json", buffer);
}

void loop() {
  // Reconnect to MQTT if needed
  if (!client.connected()) {
    reconnect_mqtt();
  }
  
  // Process MQTT messages
  client.loop();
  
  // Publish sensor data at regular intervals
  unsigned long now = millis();
  if (now - last_publish > publish_interval) {
    last_publish = now;
    publish_sensor_data();
  }
}
```

## Implementation Timeline

1. **Phase 1: Core IoT Interface (2-3 days)**
   - Implement `IoTInterface` base class
   - Create MQTT implementation using Mosquitto or Paho libraries
   - Basic connection and message handling
   - Simple test harness

2. **Phase 2: IoT Event Adapter (2 days)**
   - Create adapter to connect IoT messages to Event System
   - Implement topic-to-event mapping
   - Implement topic-to-parameter mapping
   - Add message format conversion
   - Topic pattern matching

3. **Phase 3: Configuration and Discovery (2 days)**
   - Create IoT configuration management
   - Implement device discovery mechanism
   - Create serialization/deserialization for configurations
   - Implement device registration system

4. **Phase 4: Integration with Existing Systems (3-4 days)**
   - Integrate with Event System
   - Integrate with State Machine
   - Integrate with Parameter System
   - Integrate with RTPC System
   - Create IoT modulation source

5. **Phase 5: Sensor Node Development (3-4 days)**
   - Develop firmware for ESP32 sensor nodes
   - Implement sensor data collection
   - Create MQTT publishing code
   - Implement device announcement protocol
   - Test with various sensor types

6. **Phase 6: Testing and Documentation (2-3 days)**
   - End-to-end testing with real sensors
   - Performance optimization
   - Documentation and example scenarios
   - Troubleshooting guide
   - Create demo setups

## Conclusion

The IoT integration creates a powerful bridge between the physical world and our adaptive music system. By connecting real-world sensors and devices to our sophisticated music engine, we can create truly responsive environmental music that reacts to its surroundings in complex and musical ways.

This implementation leverages our existing systems:
- The Event System handles discrete triggers from IoT devices
- The Parameter System manages continuous data streams
- The State Machine responds to environmental context changes
- The RTPC System provides sophisticated mapping of sensor data

The IoT integration extends our synth beyond a standalone instrument into an environmental music system that can respond to its surroundings. This opens up exciting possibilities for:

- Responsive home audio that adapts to daily activities
- Interactive installations and exhibitions
- Environmental soundscapes that evolve with weather and seasons
- Smart space audio systems
- Therapeutic sound environments that respond to biofeedback

The modular architecture allows for extension with new sensor types, protocols, and mapping strategies as the project evolves.