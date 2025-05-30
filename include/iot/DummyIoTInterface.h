#pragma once

#include "IoTInterface.h"
#include <iostream>

namespace AIMusicHardware {

/**
 * @brief Dummy IoT interface for testing without actual IoT connectivity
 */
class DummyIoTInterface : public IoTInterface {
public:
    DummyIoTInterface() = default;
    
    bool connect(const std::string& host, int port, const std::string& clientId) override { 
        std::cout << "DummyIoT: Connected to " << host << ":" << port << " as " << clientId << std::endl;
        connected_ = true;
        return true; 
    }
    
    void disconnect() override { 
        connected_ = false;
    }
    
    bool isConnected() const override { 
        return connected_; 
    }
    
    void update() override {
        // Nothing to update for dummy interface
    }
    
    bool subscribe(const std::string& topic) override {
        subscribedTopics_.insert(topic);
        return true;
    }
    
    bool unsubscribe(const std::string& topic) override {
        subscribedTopics_.erase(topic);
        return true;
    }
    
    bool publish(const std::string& topic, const std::string& payload) override {
        // Just log for dummy implementation
        std::cout << "DummyIoT: Published to " << topic << ": " << payload << std::endl;
        return true;
    }
    
    bool publish(const std::string& topic, const std::string& payload, 
                 int qos, bool retain) override {
        // Just log for dummy implementation
        std::cout << "DummyIoT: Published to " << topic << " (QoS=" << qos 
                  << ", retain=" << retain << "): " << payload << std::endl;
        return true;
    }
    
    void setMessageCallback(MessageCallback callback) override {
        messageCallback_ = callback;
    }
    
    void setTopicCallback(const std::string& topic, MessageCallback callback) override {
        topicCallbacks_[topic] = callback;
    }
    
    void removeTopicCallback(const std::string& topic) override {
        topicCallbacks_.erase(topic);
    }
    
private:
    bool connected_ = false;
    std::set<std::string> subscribedTopics_;
    MessageCallback messageCallback_;
    std::map<std::string, MessageCallback> topicCallbacks_;
};

} // namespace AIMusicHardware