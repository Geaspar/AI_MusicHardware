#pragma once

#include "IoTInterface.h"
#include "mqtt_include.h"
#include <memory>
#include <string>
#include <map>
#include <set>
#include <mutex>

namespace AIMusicHardware {

// Forward declaration for friend class
#if defined(HAVE_PAHO_MQTT)
class MQTTCallbackHandler;
#endif

/**
 * @brief MQTT implementation of the IoTInterface
 * 
 * This class provides an implementation of the IoTInterface using the Paho MQTT library.
 * It handles MQTT-specific details such as quality of service, message retention,
 * connection options, and topic wildcards.
 */
class MQTTInterface : public IoTInterface {
#if defined(HAVE_PAHO_MQTT)
    friend class MQTTCallbackHandler;
#endif
public:
    /**
     * @brief Constructor
     */
    MQTTInterface();
    
    /**
     * @brief Destructor
     */
    ~MQTTInterface() override;
    
    // IoTInterface implementation
    bool connect(const std::string& host, int port, const std::string& clientId) override;
    void disconnect() override;
    bool isConnected() const override;
    void update() override;
    bool subscribe(const std::string& topic) override;
    bool unsubscribe(const std::string& topic) override;
    bool publish(const std::string& topic, const std::string& payload) override;
    bool publish(const std::string& topic, const std::string& payload, int qos, bool retain) override;
    void setMessageCallback(MessageCallback callback) override;
    void setTopicCallback(const std::string& topic, MessageCallback callback) override;
    void removeTopicCallback(const std::string& topic) override;
    
    /**
     * @brief Set connection options
     * 
     * @param keepAliveInterval Keep alive interval in seconds
     * @param cleanSession Whether to use a clean session
     * @param automaticReconnect Whether to enable automatic reconnection
     */
    void setConnectionOptions(int keepAliveInterval, bool cleanSession, bool automaticReconnect);
    
    /**
     * @brief Set last will and testament message
     * 
     * This message will be published by the broker if the client disconnects unexpectedly.
     * 
     * @param topic Topic for last will message
     * @param payload Message payload
     * @param qos Quality of Service level (0, 1, or 2)
     * @param retained Whether the last will message should be retained
     */
    void setLastWill(const std::string& topic, const std::string& payload, int qos, bool retained);
    
    /**
     * @brief Set default quality of service level
     * 
     * @param qos Quality of Service level (0, 1, or 2)
     */
    void setDefaultQoS(int qos);
    
private:
    // Paho MQTT client
    std::unique_ptr<mqtt::async_client> client_;
    std::unique_ptr<mqtt::connect_options> connectOptions_;
    
    // Connection details
    std::string host_;
    int port_;
    std::string clientId_;
    bool isConnected_ = false;
    
    // Message handling
    MessageCallback globalMessageCallback_;
    std::map<std::string, MessageCallback> topicCallbacks_;
    
    // Connection options
    int keepAliveInterval_ = 60;  // seconds
    bool cleanSession_ = true;
    bool automaticReconnect_ = true;
    
    // Last Will and Testament
    bool hasLastWill_ = false;
    std::string lastWillTopic_;
    std::string lastWillPayload_;
    int lastWillQoS_ = 0;
    bool lastWillRetained_ = false;
    
    // Default QoS
    int defaultQoS_ = 0;
    
    // Subscription tracking
    std::set<std::string> subscriptions_;
    
    // Thread safety
    mutable std::mutex mutex_;
    
    // Internal callback handlers
    void onMessage(const mqtt::const_message_ptr& msg);
    bool matchTopicPattern(const std::string& topic, const std::string& pattern) const;
    
    // Reconnection handling
    void reconnect();
    
    // Create server URI from host and port
    std::string getServerURI() const;
};

} // namespace AIMusicHardware