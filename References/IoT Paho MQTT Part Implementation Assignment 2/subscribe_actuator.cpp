#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "MQTTClient.h"
#include <pigpio.h> // for LED
#include <json-c/json.h> // to parse JSON

#define ADDRESS     "tcp://192.168.0.238:1883"
#define CLIENTID    "rpi2"
#define AUTHMETHOD  "warfieldg"
#define AUTHTOKEN   "password1234"
#define TOPIC       "ee513/+"
#define PAYLOAD     "Hello World!"
#define QOS         1
#define TIMEOUT     10000L
#define LED_PIN     17

volatile MQTTClient_deliveryToken deliveredtoken;

void delivered(void *context, MQTTClient_deliveryToken dt) {
    printf("Message with token value %d delivery confirmed\n", dt);
    deliveredtoken = dt;
}


int msgarrvd(void *context, char *topicName, int topicLen, MQTTClient_message *message) {
    char* payloadptr = (char*) message->payload;
    json_object *parsed_json = json_tokener_parse(payloadptr);
    json_object *roll_obj;

o    printf("Message arrived\n");
    printf("     topic: %s\n", topicName);
    printf("   message: %s\n", payloadptr);

    // Parse the roll value from JSON
    if (json_object_object_get_ex(parsed_json, "roll", &roll_obj)) {
        int roll = json_object_get_int(roll_obj);

        printf("    roll: %d\n", roll);
        // Control GPIO based on the roll value
        if (roll >= -15 && roll <= 15) {
            gpioWrite(LED_PIN, 0);  // LED OFF
            printf("LED OFF\n");
        } else {
            gpioWrite(LED_PIN, 1);  // LED ON
            printf("LED ON\n");
        }
    }

    // Clean up
    json_object_put(parsed_json);  // Free JSON object
    MQTTClient_freeMessage(&message);
    MQTTClient_free(topicName);

    return 1;
}

void connlost(void *context, char *cause) {
    printf("\nConnection lost\n");
    printf("     cause: %s\n", cause);
}

int main(int argc, char* argv[]) {
    gpioInitialise();

    gpioSetMode(LED_PIN,PI_OUTPUT);  // Set LED pin as output

    MQTTClient client;
    MQTTClient_connectOptions opts = MQTTClient_connectOptions_initializer;
    int rc;
    int ch;

    MQTTClient_create(&client, ADDRESS, CLIENTID, MQTTCLIENT_PERSISTENCE_NONE, NULL);
    opts.keepAliveInterval = 20;
    opts.cleansession = 1;
    opts.username = AUTHMETHOD;
    opts.password = AUTHTOKEN;

    MQTTClient_setCallbacks(client, NULL, connlost, msgarrvd, delivered);
    if ((rc = MQTTClient_connect(client, &opts)) != MQTTCLIENT_SUCCESS) {
        printf("Failed to connect, return code %d\n", rc);
        exit(-1);
    }
    printf("Subscribing to topic %s\nfor client %s using QoS%d\n\n"
           "Press Q<Enter> to quit\n\n", TOPIC, CLIENTID, QOS);
    MQTTClient_subscribe(client, TOPIC, QOS);

    do {
        ch = getchar();
    } while(ch!='Q' && ch != 'q');
    MQTTClient_disconnect(client, 10000);
    MQTTClient_destroy(&client);
    gpioTerminate(); // clear gpio resources
    return rc;
}
