// Based on the Paho C code example from www.eclipse.org/paho/
#include "ADXL345.h"

#include <iostream>
#include <sstream>
#include <fstream>
#include <string.h>
#include "MQTTClient.h"
#define  CPU_TEMP "/sys/class/thermal/thermal_zone0/temp"
using namespace std;

//Please replace the following address with the address of your server
#define ADDRESS    "tcp://192.168.0.238:1883"
#define CLIENTID   "rpi1"
#define AUTHMETHOD "warfieldg"
#define AUTHTOKEN  "password1234"
#define TOPIC      "ee513/CPUTemp"
#define TOPIC_2      "ee513/Accel"
#define QOS        1
#define QOS_2        2
#define TIMEOUT    10000L

float getCPUTemperature() {        // get the CPU temperature
   int cpuTemp;                    // store as an int
   fstream fs;
   fs.open(CPU_TEMP, fstream::in); // read from the file
   fs >> cpuTemp;
   fs.close();
   return (((float)cpuTemp)/1000);
}

string getCPULoad() {
    ifstream loadavgFile("/proc/loadavg");
    string load;
    getline(loadavgFile, load);  // Directly read the first line
    loadavgFile.close();
    return load;  // Return the entire line containing all load averages
}



int main(int argc, char* argv[]) {
   char str_payload[100];          // Set your max message size here
   char str_payload_2[100];
   MQTTClient client;
   MQTTClient_connectOptions opts = MQTTClient_connectOptions_initializer;
   MQTTClient_message pubmsg = MQTTClient_message_initializer;
   MQTTClient_message pubmsg_2 = MQTTClient_message_initializer;

   MQTTClient_willOptions will_opts = MQTTClient_willOptions_initializer; //initialising will opts
   const char* lastWillTopic = "ee513/LastWill";
   const char* lastWillMessage = "Last Will MSG!: Disconnected unexpectedly";

   // Setting Last Will parameters
   will_opts.topicName = lastWillTopic;
   will_opts.message = lastWillMessage;
   will_opts.qos = 1;  // QoS for last will message
   will_opts.retained = 0;  // Not retained

   MQTTClient_deliveryToken token;
   MQTTClient_create(&client, ADDRESS, CLIENTID, MQTTCLIENT_PERSISTENCE_NONE, NULL);
   opts.keepAliveInterval = 20;
   opts.cleansession = 1;
   opts.username = AUTHMETHOD;
   opts.password = AUTHTOKEN;
   opts.will = &will_opts;

   int rc;
   if ((rc = MQTTClient_connect(client, &opts)) != MQTTCLIENT_SUCCESS) {
      cout << "Failed to connect, return code " << rc << endl;
      return -1;
   }
   exploringRPi::ADXL345 sensor(1,0x53); // Code from Application.cpp
   sensor.setResolution(exploringRPi::ADXL345::NORMAL);
   sensor.setRange(exploringRPi::ADXL345::PLUSMINUS_4_G);
   sensor.readSensorState();
   float pitch = sensor.getPitch();
   float roll = sensor.getRoll();

   sprintf(str_payload_2, "{\"Pitch\": %f, \"Roll\": %f}", pitch, roll);
   sprintf(str_payload, "{\"d\":{\"CPUTemp\": %f, \"CPULoad\": \"%s\"}}", getCPUTemperature(), getCPULoad().c_str());

   pubmsg.payload = str_payload;
   pubmsg.payloadlen = strlen(str_payload);
   pubmsg.qos = QOS_2;
   pubmsg.retained = 0;

   pubmsg_2.payload = str_payload_2;
   pubmsg_2.payloadlen = strlen(str_payload_2);
   pubmsg_2.qos = QOS_2;
   pubmsg_2.retained = 0;


//   MQTTClient_publishMessage(client, TOPIC, &pubmsg, &token);
   MQTTClient_publishMessage(client, TOPIC_2, &pubmsg_2, &token);
   cout << "Waiting for up to " << (int)(TIMEOUT/1000) <<
        " seconds for publication of " << str_payload <<
        " \non topic " << TOPIC << " for ClientID: " << CLIENTID << endl;
   rc = MQTTClient_waitForCompletion(client, token, TIMEOUT);
   cout << "Message with token " << (int)token << " delivered." << endl;
   MQTTClient_disconnect(client, 10000);
   MQTTClient_destroy(&client);
   return rc;
}
