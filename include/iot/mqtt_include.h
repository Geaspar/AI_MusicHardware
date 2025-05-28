#pragma once

#include <string>
#include <memory>
#include <iostream>
#include <functional>
#include <exception>

// This header ensures correct inclusion of MQTT libraries
// based on whether the Paho MQTT library is available

#if defined(HAVE_PAHO_MQTT) && !defined(DISABLE_MQTT) && 0
// The "0" above ensures we always use the mock implementation for now
// Remove the "0" and uncomment these includes when Paho MQTT libraries are properly installed
//#include <mqtt/async_client.h>
//#include <mqtt/topic.h>
//#include <mqtt/message.h>
//#include <mqtt/connect_options.h>
#else
// Dummy implementations for use when Paho MQTT is not available
namespace mqtt {
    // Define an exception that mimics the Paho MQTT exception
    class exception : public std::exception {
    public:
        exception(const std::string& message) : message(message) {}
        virtual const char* what() const throw() { return message.c_str(); }
    private:
        std::string message;
    };

    // Forward declarations
    class message;
    class token;
    class delivery_token;
    class callback;
    class connect_options;

    // Smart pointer typedefs
    using message_ptr = std::shared_ptr<message>;
    using const_message_ptr = std::shared_ptr<const message>;
    using token_ptr = std::shared_ptr<token>;
    using delivery_token_ptr = std::shared_ptr<delivery_token>;
    using callback_ptr = std::shared_ptr<callback>;

    // Simple message class
    class message {
    public:
        message() = default;
        message(const std::string& topic, const std::string& payload, int qos = 0, bool retained = false)
            : topic_(topic), payload_(payload), qos_(qos), retained_(retained) {}

        std::string get_topic() const { return topic_; }
        std::string get_payload_str() const { return payload_; }
        int get_qos() const { return qos_; }
        bool is_retained() const { return retained_; }

    private:
        std::string topic_;
        std::string payload_;
        int qos_ = 0;
        bool retained_ = false;
    };

    // Simple token class
    class token {
    public:
        virtual ~token() = default;
        virtual void wait() {}
        virtual bool wait_for(long timeout) { return true; }
    };

    // Delivery token extends token
    class delivery_token : public token {
    public:
        virtual ~delivery_token() = default;
    };

    // Callback interface class
    class callback {
    public:
        virtual ~callback() = default;
        virtual void message_arrived(const_message_ptr msg) {
            std::cout << "MQTT Mock: Message arrived on topic: " << msg->get_topic() << std::endl;
        }
        virtual void connection_lost(const std::string& cause) {
            std::cout << "MQTT Mock: Connection lost: " << cause << std::endl;
        }
        virtual void delivery_complete(delivery_token_ptr token) {
            std::cout << "MQTT Mock: Delivery complete" << std::endl;
        }
    };

    // Connection options class - needs to be defined before async_client
    class connect_options {
    public:
        connect_options() = default;

        void set_keep_alive_interval(int interval) {}
        void set_clean_session(bool clean) {}
        void set_automatic_reconnect(bool reconnect) {}
        void set_will_message(message_ptr will) {}
    };

    // Basic implementation
    class async_client {
    public:
        async_client(const std::string& serverURI, const std::string& clientId)
            : serverURI_(serverURI), clientId_(clientId), connected_(false), callback_(nullptr) {
            std::cout << "MQTT Mock: Creating client for " << serverURI << ", client ID: " << clientId << std::endl;
        }

        bool is_connected() const { return connected_; }

        token_ptr connect() {
            std::cout << "MQTT Mock: Connecting to " << serverURI_ << std::endl;
            connected_ = true;
            return std::make_shared<token>();
        }

        token_ptr connect(const connect_options& options) {
            std::cout << "MQTT Mock: Connecting to " << serverURI_ << " with options" << std::endl;
            connected_ = true;
            return std::make_shared<token>();
        }

        token_ptr disconnect() {
            std::cout << "MQTT Mock: Disconnecting from " << serverURI_ << std::endl;
            connected_ = false;
            return std::make_shared<token>();
        }

        token_ptr publish(const std::string& topic, const void* payload, size_t size, int qos, bool retained) {
            std::cout << "MQTT Mock: Publishing to topic: " << topic << std::endl;
            return std::make_shared<delivery_token>();
        }

        token_ptr publish(const std::string& topic, const std::string& payload, int qos, bool retained) {
            std::cout << "MQTT Mock: Publishing to topic: " << topic << ", payload: " << payload << std::endl;
            return std::make_shared<delivery_token>();
        }

        token_ptr publish(const std::string& topic, const std::string& payload) {
            std::cout << "MQTT Mock: Publishing to topic: " << topic << ", payload: " << payload << std::endl;
            return std::make_shared<delivery_token>();
        }

        token_ptr publish(const message_ptr& message) {
            std::cout << "MQTT Mock: Publishing message to topic: " << message->get_topic() << std::endl;
            return std::make_shared<delivery_token>();
        }

        token_ptr subscribe(const std::string& topic, int qos) {
            std::cout << "MQTT Mock: Subscribing to topic: " << topic << std::endl;
            return std::make_shared<token>();
        }

        token_ptr unsubscribe(const std::string& topic) {
            std::cout << "MQTT Mock: Unsubscribing from topic: " << topic << std::endl;
            return std::make_shared<token>();
        }

        void set_callback(callback& cb) {
            std::cout << "MQTT Mock: Setting callback" << std::endl;
            callback_ = &cb;
        }

        // For compatibility with original
        void set_callback(void* cb) {
            std::cout << "MQTT Mock: Setting callback (void* version)" << std::endl;
        }

    private:
        std::string serverURI_;
        std::string clientId_;
        bool connected_;
        callback* callback_;
    };

    // make_message factory function
    inline message_ptr make_message(const std::string& topic, const std::string& payload, int qos = 0, bool retained = false) {
        return std::make_shared<message>(topic, payload, qos, retained);
    }
} // namespace mqtt
#endif