#include "../../include/iot/MQTTInterface.h"

#include <iostream>
#include <regex>
#include <stdexcept>
#include <thread>
#include <chrono>

namespace AIMusicHardware {

#if defined(HAVE_PAHO_MQTT) && !defined(DISABLE_MQTT)
// MQTT callback handler implementation for real Paho
class MQTTCallbackHandler : public virtual mqtt::callback {
public:
    MQTTCallbackHandler(MQTTInterface& interface) : interface_(interface) {}

    void message_arrived(mqtt::const_message_ptr msg) override {
        interface_.onMessage(msg);
    }

    void connection_lost(const std::string& cause) override {
        std::cerr << "MQTT connection lost: " << cause << std::endl;
    }

private:
    MQTTInterface& interface_;
};
#endif

// Implementation of MQTTInterface
MQTTInterface::MQTTInterface() :
    port_(1883),
    defaultQoS_(0)
{
#if defined(HAVE_PAHO_MQTT) && !defined(DISABLE_MQTT)
    connectOptions_ = std::make_unique<mqtt::connect_options>();
#else
    connectOptions_ = std::make_unique<mqtt::connect_options>();
    std::cerr << "Note: Using MQTT mock implementation." << std::endl;
#endif
}

MQTTInterface::~MQTTInterface() {
    disconnect();
}

bool MQTTInterface::connect(const std::string& host, int port, const std::string& clientId) {
    std::lock_guard<std::mutex> lock(mutex_);

    // Save connection parameters
    host_ = host;
    port_ = port;
    clientId_ = clientId;

    try {
        // Create MQTT client
        std::string serverURI = getServerURI();
        client_ = std::make_unique<mqtt::async_client>(serverURI, clientId);

#if defined(HAVE_PAHO_MQTT) && !defined(DISABLE_MQTT)
        // Set callback handler for real MQTT
        auto callback = std::make_shared<MQTTCallbackHandler>(*this);
        client_->set_callback(*callback);
#else
        // Set callback handler for mock MQTT
        // The mock implementation doesn't need a separate callback handler
#endif

        // Configure connection options
        connectOptions_->set_keep_alive_interval(keepAliveInterval_);
        connectOptions_->set_clean_session(cleanSession_);
        connectOptions_->set_automatic_reconnect(automaticReconnect_);

        // Set Last Will and Testament if configured
        if (hasLastWill_) {
            mqtt::message_ptr willMsg = mqtt::make_message(
                lastWillTopic_, lastWillPayload_, lastWillQoS_, lastWillRetained_);
            connectOptions_->set_will_message(willMsg);
        }

        // Connect to broker
        mqtt::token_ptr conntok = client_->connect(*connectOptions_);
        conntok->wait();

        // Subscribe to previously subscribed topics
        for (const auto& topic : subscriptions_) {
            client_->subscribe(topic, defaultQoS_)->wait();
        }

        isConnected_ = true;
        return true;
    }
    catch (const mqtt::exception& exc) {
        std::cerr << "MQTT connection failed: " << exc.what() << std::endl;
        isConnected_ = false;
        return false;
    }
}

void MQTTInterface::disconnect() {
    std::lock_guard<std::mutex> lock(mutex_);

    if (client_ && client_->is_connected()) {
        try {
            client_->disconnect()->wait();
        }
        catch (const mqtt::exception& exc) {
            std::cerr << "MQTT disconnect error: " << exc.what() << std::endl;
        }
    }

    isConnected_ = false;
}

bool MQTTInterface::isConnected() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return client_ && client_->is_connected();
}

void MQTTInterface::update() {
    // Check if we need to reconnect
    if (!isConnected() && !host_.empty()) {
        reconnect();
    }
    
    // Note: Paho MQTT client is event-driven with callbacks
    // This method primarily handles reconnection and can be
    // extended for additional periodic tasks
}

bool MQTTInterface::subscribe(const std::string& topic) {
    std::lock_guard<std::mutex> lock(mutex_);

    // Add to subscription list even if not connected
    subscriptions_.insert(topic);

    if (!isConnected()) {
        return false;
    }

    try {
        client_->subscribe(topic, defaultQoS_)->wait();
        return true;
    }
    catch (const mqtt::exception& exc) {
        std::cerr << "MQTT subscribe failed: " << exc.what() << std::endl;
        return false;
    }
}

bool MQTTInterface::unsubscribe(const std::string& topic) {
    std::lock_guard<std::mutex> lock(mutex_);

    // Remove from subscription list
    subscriptions_.erase(topic);

    if (!isConnected()) {
        return false;
    }

    try {
        client_->unsubscribe(topic)->wait();
        return true;
    }
    catch (const mqtt::exception& exc) {
        std::cerr << "MQTT unsubscribe failed: " << exc.what() << std::endl;
        return false;
    }
}

bool MQTTInterface::publish(const std::string& topic, const std::string& payload) {
    return publish(topic, payload, defaultQoS_, false);
}

bool MQTTInterface::publish(const std::string& topic, const std::string& payload,
                           int qos, bool retain) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (!isConnected()) {
        return false;
    }

    try {
        mqtt::message_ptr pubmsg = mqtt::make_message(topic, payload, qos, retain);
        client_->publish(pubmsg)->wait();
        return true;
    }
    catch (const mqtt::exception& exc) {
        std::cerr << "MQTT publish failed: " << exc.what() << std::endl;
        return false;
    }
}

void MQTTInterface::setMessageCallback(MessageCallback callback) {
    std::lock_guard<std::mutex> lock(mutex_);
    globalMessageCallback_ = callback;
}

void MQTTInterface::setTopicCallback(const std::string& topic, MessageCallback callback) {
    std::lock_guard<std::mutex> lock(mutex_);
    topicCallbacks_[topic] = callback;
    
    // Ensure we're subscribed to this topic
    if (subscriptions_.find(topic) == subscriptions_.end()) {
        subscribe(topic);
    }
}

void MQTTInterface::removeTopicCallback(const std::string& topic) {
    std::lock_guard<std::mutex> lock(mutex_);
    topicCallbacks_.erase(topic);
}

void MQTTInterface::setConnectionOptions(int keepAliveInterval, bool cleanSession, bool automaticReconnect) {
    std::lock_guard<std::mutex> lock(mutex_);
    keepAliveInterval_ = keepAliveInterval;
    cleanSession_ = cleanSession;
    automaticReconnect_ = automaticReconnect;
    
    // Update connection options if already created
    if (connectOptions_) {
        connectOptions_->set_keep_alive_interval(keepAliveInterval_);
        connectOptions_->set_clean_session(cleanSession_);
        connectOptions_->set_automatic_reconnect(automaticReconnect_);
    }
}

void MQTTInterface::setLastWill(const std::string& topic, const std::string& payload, 
                                int qos, bool retained) {
    std::lock_guard<std::mutex> lock(mutex_);
    hasLastWill_ = true;
    lastWillTopic_ = topic;
    lastWillPayload_ = payload;
    lastWillQoS_ = qos;
    lastWillRetained_ = retained;
    
    // Update will message if connection options already exist
    if (connectOptions_) {
        mqtt::message_ptr willMsg = mqtt::make_message(
            lastWillTopic_, lastWillPayload_, lastWillQoS_, lastWillRetained_);
        connectOptions_->set_will_message(willMsg);
    }
}

void MQTTInterface::setDefaultQoS(int qos) {
    std::lock_guard<std::mutex> lock(mutex_);
    defaultQoS_ = qos;
}

void MQTTInterface::onMessage(const mqtt::const_message_ptr& msg) {
    std::string topic = msg->get_topic();
    std::string payload = msg->get_payload_str();

    // First check for specific topic callbacks
    bool handledByTopicCallback = false;
    {
        std::lock_guard<std::mutex> lock(mutex_);

        // Direct topic match
        auto it = topicCallbacks_.find(topic);
        if (it != topicCallbacks_.end() && it->second) {
            // Make a copy of the callback to avoid holding the lock during execution
            MessageCallback callback = it->second;
            lock.~lock_guard(); // Explicitly release the lock

            callback(topic, payload);
            handledByTopicCallback = true;
        }
        else {
            // Check for pattern matches
            for (const auto& [pattern, callback] : topicCallbacks_) {
                if (matchTopicPattern(topic, pattern) && callback) {
                    // Make a copy of the callback to avoid holding the lock during execution
                    MessageCallback cb = callback;
                    lock.~lock_guard(); // Explicitly release the lock

                    cb(topic, payload);
                    handledByTopicCallback = true;
                    break;
                }
            }
        }
    }

    // If not handled by a topic callback, use the global callback
    if (!handledByTopicCallback) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (globalMessageCallback_) {
            // Make a copy of the callback to avoid holding the lock during execution
            MessageCallback callback = globalMessageCallback_;
            lock.~lock_guard(); // Explicitly release the lock

            callback(topic, payload);
        }
    }
}

bool MQTTInterface::matchTopicPattern(const std::string& topic, const std::string& pattern) const {
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

void MQTTInterface::reconnect() {
    // Only attempt reconnection if we have connection details
    if (host_.empty() || clientId_.empty()) {
        return;
    }
    
    try {
        // Attempt to reconnect (will use connection options with automatic reconnect)
        connect(host_, port_, clientId_);
    }
    catch (const std::exception& e) {
        std::cerr << "Reconnection attempt failed: " << e.what() << std::endl;
    }
}

std::string MQTTInterface::getServerURI() const {
    return "tcp://" + host_ + ":" + std::to_string(port_);
}

} // namespace AIMusicHardware