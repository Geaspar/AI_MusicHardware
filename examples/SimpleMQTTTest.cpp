#include <iostream>
#include <string>

// Define DISABLE_MQTT to force the use of our mock implementation
#define DISABLE_MQTT 1

// Define HAVE_PAHO_MQTT to enable the mock implementation
#define HAVE_PAHO_MQTT 1

#include "../include/iot/mqtt_include.h"

class MyMQTTCallback : public mqtt::callback {
public:
    void message_arrived(mqtt::const_message_ptr msg) override {
        std::cout << "Message received on topic: " << msg->get_topic() << std::endl;
        std::cout << "Payload: " << msg->get_payload_str() << std::endl;
    }

    void connection_lost(const std::string& cause) override {
        std::cout << "Connection lost: " << cause << std::endl;
    }

    void delivery_complete(mqtt::delivery_token_ptr token) override {
        std::cout << "Delivery complete" << std::endl;
    }
};

int main() {
    std::cout << "=== Simple MQTT Test (Mock Mode) ===" << std::endl;
    std::cout << "This demonstrates our mock MQTT implementation." << std::endl;
    std::cout << std::endl;

    try {
        // Create MQTT client
        std::cout << "Creating MQTT client..." << std::endl;
        mqtt::async_client client("tcp://localhost:1883", "SimpleMQTTTestClient");

        // Create callback handler
        std::cout << "Setting up callback handler..." << std::endl;
        MyMQTTCallback callback;
        client.set_callback(callback);

        // Set connection options
        std::cout << "Setting up connection options..." << std::endl;
        mqtt::connect_options options;
        options.set_keep_alive_interval(60);
        options.set_clean_session(true);
        options.set_automatic_reconnect(true);

        // Connect to broker
        std::cout << "Connecting to broker..." << std::endl;
        mqtt::token_ptr token = client.connect(options);
        token->wait();

        // Subscribe to topics
        std::cout << "Subscribing to test topics..." << std::endl;
        client.subscribe("test/topic", 0);

        // Publish a message
        std::cout << "Publishing a test message..." << std::endl;
        client.publish("test/topic", "Hello from SimpleMQTTTest!", 0, false);

        // Create messages with different methods
        std::cout << "Testing message creation..." << std::endl;
        mqtt::message_ptr msg = mqtt::make_message("test/topic2", "This is a message created with make_message", 1, true);
        client.publish(msg);

        // Test if client is connected
        std::cout << "Checking connection status..." << std::endl;
        if (client.is_connected()) {
            std::cout << "Client is confirmed to be connected." << std::endl;
        } else {
            std::cout << "Client reports it is not connected (this should not happen)." << std::endl;
        }

        // Disconnect from broker
        std::cout << "Disconnecting..." << std::endl;
        client.disconnect();

        // Check disconnection
        if (!client.is_connected()) {
            std::cout << "Client is confirmed to be disconnected." << std::endl;
        } else {
            std::cout << "Client reports it is still connected (this should not happen)." << std::endl;
        }

        std::cout << std::endl;
        std::cout << "Test completed successfully!" << std::endl;
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    std::cout << std::endl;
    std::cout << "Note: This is using the MQTT mock implementation." << std::endl;
    std::cout << "For full MQTT functionality, please install the Paho MQTT libraries with:" << std::endl;
    std::cout << "./tools/install_mqtt_libs.sh" << std::endl;

    return 0;
}