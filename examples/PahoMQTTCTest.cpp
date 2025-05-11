#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <MQTTClient.h>
#include <iostream>
#include <string>
#include <chrono>
#include <thread>
#include <signal.h>

// Default settings
#define ADDRESS     "tcp://localhost:1883"
#define CLIENTID    "AIMusicHardware_C_Test"
#define QOS         1
#define TIMEOUT     10000L

// Global variables
volatile MQTTClient_deliveryToken deliveredtoken;
volatile sig_atomic_t exitFlag = 0;

// Signal handler for clean shutdown
void signalHandler(int signum) {
    std::cout << "Interrupt signal (" << signum << ") received. Exiting..." << std::endl;
    exitFlag = 1;
}

// Callback functions for MQTT
void delivered(void *context, MQTTClient_deliveryToken dt) {
    printf("Message with token value %d delivery confirmed\n", dt);
    deliveredtoken = dt;
}

int msgarrvd(void *context, char *topicName, int topicLen, MQTTClient_message *message) {
    int i;
    char* payloadptr;
    
    printf("Message arrived\n");
    printf("     topic: %s\n", topicName);
    printf("   message: ");
    
    payloadptr = (char*)message->payload;
    for(i=0; i<message->payloadlen; i++) {
        putchar(*payloadptr++);
    }
    putchar('\n');
    
    MQTTClient_freeMessage(&message);
    MQTTClient_free(topicName);
    return 1;
}

void connlost(void *context, char *cause) {
    printf("\nConnection lost\n");
    printf("     cause: %s\n", cause);
}

int main(int argc, char* argv[]) {
    // Register signal handler
    signal(SIGINT, signalHandler);
    
    // Initialize variables
    MQTTClient client;
    MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
    MQTTClient_willOptions will_opts = MQTTClient_willOptions_initializer;
    int rc;
    
    printf("Paho MQTT C Client Test\n");
    printf("=======================\n");
    
    // Set up MQTT client
    MQTTClient_create(&client, ADDRESS, CLIENTID,
        MQTTCLIENT_PERSISTENCE_NONE, NULL);
    
    // Set connection options
    conn_opts.keepAliveInterval = 20;
    conn_opts.cleansession = 1;
    
    // Set up Last Will and Testament
    const char* lastWillTopic = "AIMusicHardware/status";
    const char* lastWillMessage = "{\"status\":\"disconnected\",\"client\":\"AIMusicHardware_C_Test\"}";
    will_opts.topicName = lastWillTopic;
    will_opts.message = lastWillMessage;
    will_opts.qos = 1;
    will_opts.retained = 1;
    conn_opts.will = &will_opts;
    
    // Set callbacks
    MQTTClient_setCallbacks(client, NULL, connlost, msgarrvd, delivered);
    
    // Connect to broker
    printf("Connecting to MQTT broker at %s...\n", ADDRESS);
    if ((rc = MQTTClient_connect(client, &conn_opts)) != MQTTCLIENT_SUCCESS) {
        printf("Failed to connect, return code %d\n", rc);
        
        // Provide info about Mosquitto broker
        printf("\nPlease make sure the MQTT broker is running and accessible.\n");
        printf("You can install Mosquitto with:\n");
        printf("  brew install mosquitto (on macOS)\n");
        printf("  apt-get install mosquitto (on Debian/Ubuntu)\n");
        printf("  dnf install mosquitto (on Fedora)\n");
        printf("And start it with: mosquitto -v\n");
        
        return 1;
    }
    
    printf("Connected successfully\n");
    
    // Publish online status
    MQTTClient_message pubmsg = MQTTClient_message_initializer;
    MQTTClient_deliveryToken token;
    const char* onlineStatus = "{\"status\":\"online\",\"client\":\"AIMusicHardware_C_Test\"}";
    
    pubmsg.payload = (void*)onlineStatus;
    pubmsg.payloadlen = strlen(onlineStatus);
    pubmsg.qos = QOS;
    pubmsg.retained = 1;
    
    MQTTClient_publishMessage(client, "AIMusicHardware/status", &pubmsg, &token);
    printf("Waiting for publication of status message\n");
    rc = MQTTClient_waitForCompletion(client, token, TIMEOUT);
    printf("Message with token %d delivered\n", token);
    
    // Subscribe to test topics
    printf("\nSubscribing to topics...\n");
    MQTTClient_subscribe(client, "AIMusicHardware/test/#", QOS);
    MQTTClient_subscribe(client, "AIMusicHardware/+/status", QOS);
    
    printf("\nStarting main loop. Press Ctrl+C to exit.\n");
    printf("Publishing a message every 5 seconds...\n");
    
    int counter = 0;
    
    // Main loop
    while (!exitFlag) {
        // Publish a test message every 5 seconds
        if (counter % 5 == 0) {
            // Create payload with counter and timestamp
            char payload[200];
            sprintf(payload, "{\"counter\":%d,\"timestamp\":\"%ld\"}", 
                   counter/5, 
                   (long)std::chrono::system_clock::now().time_since_epoch().count());
            
            printf("Publishing message to AIMusicHardware/test/counter: %s\n", payload);
            
            pubmsg.payload = payload;
            pubmsg.payloadlen = strlen(payload);
            pubmsg.qos = QOS;
            pubmsg.retained = 0;
            
            MQTTClient_publishMessage(client, "AIMusicHardware/test/counter", &pubmsg, &token);
            // For this test we don't wait for completion to avoid blocking
        }
        
        // Sleep for a second
        std::this_thread::sleep_for(std::chrono::seconds(1));
        counter++;
    }
    
    // Publish offline status before disconnecting
    printf("\nDisconnecting...\n");
    const char* offlineStatus = "{\"status\":\"offline\",\"client\":\"AIMusicHardware_C_Test\"}";
    
    pubmsg.payload = (void*)offlineStatus;
    pubmsg.payloadlen = strlen(offlineStatus);
    pubmsg.qos = QOS;
    pubmsg.retained = 1;
    
    MQTTClient_publishMessage(client, "AIMusicHardware/status", &pubmsg, &token);
    rc = MQTTClient_waitForCompletion(client, token, TIMEOUT);
    
    // Unsubscribe and disconnect
    MQTTClient_unsubscribe(client, "AIMusicHardware/test/#");
    MQTTClient_unsubscribe(client, "AIMusicHardware/+/status");
    MQTTClient_disconnect(client, 10000);
    MQTTClient_destroy(&client);
    
    printf("Disconnected. Exiting.\n");
    return 0;
}