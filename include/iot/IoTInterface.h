#pragma once

#include <string>
#include <functional>
#include <map>
#include <set>
#include <any>

namespace AIMusicHardware {

/**
 * @brief Base interface for IoT communication
 * 
 * This abstract class defines the interface for IoT communication protocols.
 * It provides methods for connecting to IoT networks/brokers, subscribing to topics,
 * publishing messages, and handling incoming messages.
 */
class IoTInterface {
public:
    virtual ~IoTInterface() = default;
    
    /**
     * @brief Connect to IoT network/broker
     * 
     * @param host Hostname or IP address of the broker
     * @param port Port number
     * @param clientId Unique client identifier
     * @return true if connection successful, false otherwise
     */
    virtual bool connect(const std::string& host, int port, const std::string& clientId) = 0;
    
    /**
     * @brief Disconnect from IoT network/broker
     */
    virtual void disconnect() = 0;
    
    /**
     * @brief Check connection status
     * 
     * @return true if connected, false otherwise
     */
    virtual bool isConnected() const = 0;
    
    /**
     * @brief Process incoming messages and connection status
     * 
     * This method should be called regularly to process incoming messages
     * and maintain the connection.
     */
    virtual void update() = 0;
    
    /**
     * @brief Subscribe to a topic to receive messages
     * 
     * @param topic The topic to subscribe to (may include wildcards)
     * @return true if subscription successful, false otherwise
     */
    virtual bool subscribe(const std::string& topic) = 0;
    
    /**
     * @brief Unsubscribe from a topic
     * 
     * @param topic The topic to unsubscribe from
     * @return true if unsubscription successful, false otherwise
     */
    virtual bool unsubscribe(const std::string& topic) = 0;
    
    /**
     * @brief Publish a message to a topic
     * 
     * @param topic The topic to publish to
     * @param payload The message payload
     * @return true if publish successful, false otherwise
     */
    virtual bool publish(const std::string& topic, const std::string& payload) = 0;
    
    /**
     * @brief Publish a message with quality of service and retention option
     * 
     * @param topic The topic to publish to
     * @param payload The message payload
     * @param qos Quality of Service level (0, 1, or 2)
     * @param retain Whether to retain the message on the broker
     * @return true if publish successful, false otherwise
     */
    virtual bool publish(const std::string& topic, const std::string& payload, 
                         int qos, bool retain) = 0;
    
    /**
     * @brief Register message handler
     * 
     * @param callback Function to call when a message is received
     */
    using MessageCallback = std::function<void(const std::string& topic, 
                                            const std::string& payload)>;
    virtual void setMessageCallback(MessageCallback callback) = 0;
    
    /**
     * @brief Register a callback for a specific topic
     * 
     * @param topic Topic to receive messages for
     * @param callback Function to call when a message matching this topic is received
     */
    virtual void setTopicCallback(const std::string& topic, MessageCallback callback) = 0;
    
    /**
     * @brief Remove a topic-specific callback
     * 
     * @param topic Topic to remove callback for
     */
    virtual void removeTopicCallback(const std::string& topic) = 0;
};

} // namespace AIMusicHardware