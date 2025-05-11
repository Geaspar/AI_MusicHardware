#include <iostream>
#include <string>
#include <chrono>
#include <thread>
#include <csignal>

// Include the MQTT interface
#include "../include/iot/MQTTInterface.h"

using namespace AIMusicHardware;
using namespace std;

// Signal handling for clean shutdown
volatile sig_atomic_t exitFlag = 0;

void signalHandler(int signum) {
    cout << "Interrupt signal (" << signum << ") received. Exiting..." << endl;
    exitFlag = 1;
}

// Message callback function
void onMessageReceived(const string& topic, const string& payload) {
    cout << "Message received:" << endl;
    cout << "  Topic: " << topic << endl;
    cout << "  Payload: " << payload << endl;
}

int main(int argc, char* argv[]) {
    // Register signal handler
    signal(SIGINT, signalHandler);
    
    cout << "Real MQTT Test Application" << endl;
    cout << "==========================" << endl;
    
    // Default MQTT settings
    string broker = "localhost";
    int port = 1883;
    string clientId = "AIMusicHardware_Test";
    
    // Allow command-line override
    if (argc > 1) broker = argv[1];
    if (argc > 2) port = atoi(argv[2]);
    if (argc > 3) clientId = argv[3];
    
    // Create MQTT interface
    MQTTInterface mqtt;
    
    // Configure connection options
    mqtt.setConnectionOptions(60, true, true);
    
    // Set last will message
    mqtt.setLastWill("AIMusicHardware/status", 
                    "{\"status\":\"disconnected\",\"client\":\"" + clientId + "\"}", 
                    1, true);
    
    // Set QoS level to 1 (at least once delivery)
    mqtt.setDefaultQoS(1);
    
    // Register global message callback
    mqtt.setMessageCallback(onMessageReceived);
    
    // Try to connect
    cout << "Connecting to MQTT broker at " << broker << ":" << port << "..." << endl;
    
    bool connected = mqtt.connect(broker, port, clientId);
    if (!connected) {
        cerr << "Failed to connect to MQTT broker" << endl;
        
        // Check if we're running with the mock implementation
        cout << "\nChecking implementation type:" << endl;
#if defined(HAVE_PAHO_MQTT) && !defined(DISABLE_MQTT)
        cout << "Using real Paho MQTT implementation" << endl;
        cout << "Please make sure the MQTT broker is running and accessible." << endl;
        cout << "You can install Mosquitto with: " << endl;
        cout << "  brew install mosquitto (on macOS)" << endl;
        cout << "  apt-get install mosquitto (on Debian/Ubuntu)" << endl;
        cout << "  dnf install mosquitto (on Fedora)" << endl;
        cout << "And start it with: mosquitto -v" << endl;
#else
        cout << "Using mock MQTT implementation" << endl;
        cout << "To use the real implementation, install the Paho MQTT libraries:" << endl;
        cout << "  ./tools/install_mqtt_libs.sh" << endl;
        cout << "Then rebuild the project with:" << endl;
        cout << "  cd build && cmake .. && make" << endl;
#endif
        return 1;
    }
    
    cout << "Connected successfully" << endl;
    
    // Subscribe to test topics
    mqtt.subscribe("AIMusicHardware/test/#");
    mqtt.subscribe("AIMusicHardware/+/status");
    
    // Register topic-specific callback
    mqtt.setTopicCallback("AIMusicHardware/test/echo", [](const string& topic, const string& payload) {
        cout << "Echo received on " << topic << ": " << payload << endl;
        // Would normally respond here
    });
    
    // Publish online status
    mqtt.publish("AIMusicHardware/status", 
                "{\"status\":\"online\",\"client\":\"" + clientId + "\"}",
                1, true);
    
    // For testing purposes, we'll just do a few cycles then exit
    cout << "\nIn test mode, we'll just do a brief test rather than run indefinitely." << endl;
    cout << "Demonstrating message publishing..." << endl;
    
    for (int i = 0; i < 3; i++) {
        try {
            // Call update to process incoming messages and maintain connection
            mqtt.update();
            
            string payload = "{\"counter\":" + to_string(i) + 
                            ",\"timestamp\":\"" + 
                            to_string(chrono::system_clock::now().time_since_epoch().count()) + 
                            "\"}";
            
            cout << "Publishing message to AIMusicHardware/test/counter: " << payload << endl;
            mqtt.publish("AIMusicHardware/test/counter", payload);
            
            // Sleep for a fraction of a second to make output readable
            this_thread::sleep_for(chrono::milliseconds(500));
            
        } catch (const exception& e) {
            cerr << "Error in loop: " << e.what() << endl;
            break;
        }
    }
    
    cout << "\nTest completed successfully!" << endl;
    cout << "\nNote: This is using the MQTT mock implementation." << endl;
    cout << "For full MQTT functionality, please install the Paho MQTT libraries with:" << endl;
    cout << "./tools/install_mqtt_libs.sh" << endl;
    
    // Publish offline status before disconnecting
    cout << "\nDisconnecting..." << endl;
    mqtt.publish("AIMusicHardware/status", 
                "{\"status\":\"offline\",\"client\":\"" + clientId + "\"}",
                1, true);
    
    // Disconnect
    mqtt.disconnect();
    
    cout << "Disconnected. Exiting." << endl;
    return 0;
}