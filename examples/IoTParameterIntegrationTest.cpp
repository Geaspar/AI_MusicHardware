#include <iostream>
#include <iomanip>
#include <string>
#include <thread>
#include <chrono>
#include <csignal>
#include <atomic>

#include "../include/ui/parameters/ParameterManager.h"
#include "../include/iot/IoTInterface.h"
#include "../include/iot/MQTTInterface.h"
#include "../include/iot/IoTParameterTypes.h"

using namespace AIMusicHardware;

// Global flag for termination
std::atomic<bool> running{true};

// Signal handler
void signalHandler(int signal) {
    std::cout << "Caught signal " << signal << ", shutting down..." << std::endl;
    running = false;
}

// Helper to print parameter info
void printParameterInfo(Parameter* param) {
    if (!param) {
        std::cout << "  [NULL PARAMETER]" << std::endl;
        return;
    }
    
    std::cout << "  " << param->getName() << " (" << param->getId() << "): ";
    
    switch (param->getType()) {
        case Parameter::Type::FLOAT:
            std::cout << std::fixed << std::setprecision(2) 
                      << static_cast<FloatParameter*>(param)->getValue();
            break;
            
        case Parameter::Type::INT:
            std::cout << static_cast<IntParameter*>(param)->getValue();
            break;
            
        case Parameter::Type::BOOL:
            std::cout << (static_cast<BoolParameter*>(param)->getValue() ? "true" : "false");
            break;
            
        case Parameter::Type::ENUM:
            std::cout << static_cast<EnumParameter*>(param)->getCurrentValueName() 
                      << " (" << static_cast<EnumParameter*>(param)->getValue() << ")";
            break;
            
        case Parameter::Type::TRIGGER:
            std::cout << "[Trigger]";
            break;
            
        default:
            std::cout << "[Unknown Type]";
            break;
    }
    
    std::cout << std::endl;
}

// Print all parameters in a group
void printGroup(ParameterGroup* group, int depth = 0) {
    if (!group) return;
    
    // Indentation
    std::string indent(depth * 2, ' ');
    
    // Print group info
    std::cout << indent << "Group: " << group->getName() 
              << " (" << group->getId() << ")" << std::endl;
    
    // Print parameters
    for (const auto& [id, param] : group->getParameters()) {
        std::cout << indent << "  ";
        printParameterInfo(param.get());
    }
    
    // Print nested groups
    for (const auto& [id, nestedGroup] : group->getGroups()) {
        printGroup(nestedGroup.get(), depth + 1);
    }
}

int main(int argc, char** argv) {
    // Set up signal handling
    std::signal(SIGINT, signalHandler);
    
    // Parse command line arguments
    std::string brokerHost = "localhost";
    int brokerPort = 1883;
    
    if (argc > 1) {
        brokerHost = argv[1];
    }
    
    if (argc > 2) {
        brokerPort = std::stoi(argv[2]);
    }
    
    std::cout << "IoT Parameter Integration Test" << std::endl;
    std::cout << "=============================" << std::endl;
    std::cout << "Connecting to MQTT broker at " << brokerHost << ":" << brokerPort << std::endl;
    
    try {
        // Create MQTT interface
        auto mqtt = std::make_unique<MQTTInterface>();
        
        // Set connection options
        mqtt->setConnectionOptions(60, true, true);
        
        // Connect to broker
        bool connected = mqtt->connect(brokerHost, brokerPort, "AIMusicHardwareTest");
        
        if (!connected) {
            std::cerr << "Failed to connect to MQTT broker" << std::endl;
            return 1;
        }
        
        std::cout << "Connected to MQTT broker" << std::endl;
        
        // Get parameter manager instance
        auto& paramManager = EnhancedParameterManager::getInstance();
        
        // Connect IoT interface to parameter manager
        paramManager.connectIoTInterface(mqtt.get());
        
        // Create test parameters
        auto* envGroup = paramManager.getRootGroup()->createGroup("environment", "Environment");
        
        // Temperature parameter (range: -20 to 40°C)
        auto* tempParam = envGroup->createParameter<FloatParameter>(
            "temperature", "Temperature", 20.0f);
        tempParam->setRange(-20.0f, 40.0f);
        
        // Humidity parameter (range: 0-100%)
        auto* humidityParam = envGroup->createParameter<FloatParameter>(
            "humidity", "Humidity", 50.0f);
        humidityParam->setRange(0.0f, 100.0f);
        
        // Light parameter (range: 0-10000 lux)
        auto* lightParam = envGroup->createParameter<FloatParameter>(
            "light", "Light Level", 500.0f);
        lightParam->setRange(0.0f, 10000.0f);
        
        // Motion parameter (boolean)
        auto* motionParam = envGroup->createParameter<BoolParameter>(
            "motion", "Motion Detected", false);
        
        // Weather condition (enum)
        auto* weatherParam = envGroup->createParameter<EnumParameter>(
            "weather", "Weather Condition");
        weatherParam->addValue(0, "Sunny", "Clear sky");
        weatherParam->addValue(1, "Cloudy", "Overcast");
        weatherParam->addValue(2, "Rain", "Precipitation");
        weatherParam->addValue(3, "Snow", "Snowfall");
        weatherParam->addValue(4, "Stormy", "Thunderstorms");
        
        // Create audio parameters that will be affected by environmental parameters
        auto* synthGroup = paramManager.getRootGroup()->createGroup("synth", "Synthesizer");
        
        // Filter cutoff (affected by temperature)
        auto* cutoffParam = synthGroup->createParameter<FloatParameter>(
            "filter_cutoff", "Filter Cutoff", 0.5f);
        cutoffParam->setRange(0.0f, 1.0f);
        
        // Reverb amount (affected by humidity)
        auto* reverbParam = synthGroup->createParameter<FloatParameter>(
            "reverb_amount", "Reverb Amount", 0.3f);
        reverbParam->setRange(0.0f, 1.0f);
        
        // LFO speed (affected by light)
        auto* lfoSpeedParam = synthGroup->createParameter<FloatParameter>(
            "lfo_speed", "LFO Speed", 1.0f);
        lfoSpeedParam->setRange(0.1f, 10.0f);
        
        // Map IoT topics to parameters
        paramManager.mapIoTTopicToParameter(
            "environment/temperature", tempParam,
            IoTParameterConverter::SensorType::TEMPERATURE,
            -20.0f, 40.0f);

        paramManager.mapIoTTopicToParameter(
            "environment/humidity", humidityParam,
            IoTParameterConverter::SensorType::HUMIDITY,
            0.0f, 100.0f);

        paramManager.mapIoTTopicToParameter(
            "environment/light", lightParam,
            IoTParameterConverter::SensorType::LIGHT,
            0.0f, 10000.0f);
        
        paramManager.mapIoTTopicToParameter(
            "environment/motion", motionParam,
            IoTParameterConverter::SensorType::MOTION);
        
        paramManager.mapIoTTopicToParameter(
            "environment/weather", weatherParam,
            IoTParameterConverter::SensorType::CUSTOM);
        
        // Add some parameter change observers
        tempParam->addChangeObserver([cutoffParam](Parameter* param) {
            auto* floatParam = static_cast<FloatParameter*>(param);
            float temp = floatParam->getValue();
            
            // Map temperature to filter cutoff:
            // - Cold temperatures (< 0°C) -> low cutoff (dark sound)
            // - Hot temperatures (> 30°C) -> high cutoff (bright sound)
            float normalizedTemp = (temp + 20.0f) / 60.0f; // -20 to 40 -> 0 to 1
            normalizedTemp = std::clamp(normalizedTemp, 0.0f, 1.0f);
            
            cutoffParam->setValue(normalizedTemp);
            
            std::cout << "Temperature changed to " << temp << "°C -> Filter cutoff: " 
                      << cutoffParam->getValue() << std::endl;
        });
        
        humidityParam->addChangeObserver([reverbParam](Parameter* param) {
            auto* floatParam = static_cast<FloatParameter*>(param);
            float humidity = floatParam->getValue();
            
            // Map humidity to reverb amount:
            // - Dry air (< 30%) -> little reverb
            // - Humid air (> 70%) -> lots of reverb
            float normalizedHumidity = humidity / 100.0f;
            
            reverbParam->setValue(normalizedHumidity);
            
            std::cout << "Humidity changed to " << humidity << "% -> Reverb amount: " 
                      << reverbParam->getValue() << std::endl;
        });
        
        lightParam->addChangeObserver([lfoSpeedParam](Parameter* param) {
            auto* floatParam = static_cast<FloatParameter*>(param);
            float light = floatParam->getValue();
            
            // Map light to LFO speed (logarithmically):
            // - Dark (< 100 lux) -> slow LFO
            // - Bright (> 5000 lux) -> fast LFO
            float normalizedLight = std::log10(std::max(light, 1.0f)) / 4.0f; // log10(10000) is 4
            normalizedLight = std::clamp(normalizedLight, 0.0f, 1.0f);
            
            // Map 0-1 to 0.1-10 Hz
            float lfoSpeed = 0.1f + normalizedLight * 9.9f;
            lfoSpeedParam->setValue(lfoSpeed);
            
            std::cout << "Light changed to " << light << " lux -> LFO speed: " 
                      << lfoSpeedParam->getValue() << " Hz" << std::endl;
        });
        
        // Set mapping modes for some parameters
        paramManager.setIoTMappingMode(
            "environment/light", 
            IoTParameterMappings::MappingMode::LOGARITHMIC);
        
        // Subscribe to all environment topics
        mqtt->subscribe("environment/#");
        
        // Print initial parameter values
        std::cout << "\nInitial parameter values:" << std::endl;
        printGroup(paramManager.getRootGroup());
        
        // Publish initial test messages to show parameter connectivity
        std::cout << "\nPublishing test messages..." << std::endl;
        
        mqtt->publish("environment/temperature", "22.5");
        mqtt->publish("environment/humidity", "65.3");
        mqtt->publish("environment/light", "850");
        mqtt->publish("environment/motion", "0");
        mqtt->publish("environment/weather", "1"); // Cloudy
        
        // Wait a moment for messages to be processed
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        
        // Print updated parameter values
        std::cout << "\nUpdated parameter values after test messages:" << std::endl;
        printGroup(paramManager.getRootGroup());
        
        // Main loop
        std::cout << "\nEntering main loop. Press Ctrl+C to exit." << std::endl;
        std::cout << "You can publish messages to environment/# topics from another MQTT client" << std::endl;
        
        int counter = 0;
        while (running) {
            // Process MQTT messages
            mqtt->update();
            
            // Update automation (parameter smoothing)
            paramManager.updateAutomation(0.01f); // 10ms
            
            // Print parameter values every 60 iterations (every ~0.6 seconds)
            if (++counter % 60 == 0) {
                std::cout << "\nCurrent parameter values:" << std::endl;
                std::cout << "Temperature: " << tempParam->getValue() << "°C" << std::endl;
                std::cout << "Humidity: " << humidityParam->getValue() << "%" << std::endl;
                std::cout << "Light: " << lightParam->getValue() << " lux" << std::endl;
                std::cout << "Motion: " << (motionParam->getValue() ? "Detected" : "None") << std::endl;
                std::cout << "Weather: " << weatherParam->getCurrentValueName() << std::endl;
                std::cout << "---" << std::endl;
                std::cout << "Filter Cutoff: " << cutoffParam->getValue() << std::endl;
                std::cout << "Reverb Amount: " << reverbParam->getValue() << std::endl;
                std::cout << "LFO Speed: " << lfoSpeedParam->getValue() << " Hz" << std::endl;
            }
            
            // Simple demo: vary temperature to simulate day/night cycle
            if (counter % 1000 == 0) { // Every ~10 seconds
                float time = static_cast<float>(counter) / 1000.0f;
                float temperature = 15.0f + 10.0f * std::sin(time * 0.1f); // 5-25°C
                
                // Publish temperature update
                mqtt->publish("environment/temperature", 
                             std::to_string(temperature));
                
                std::cout << "Publishing simulated temperature: " 
                          << temperature << "°C" << std::endl;
            }
            
            // Sleep for 10ms
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        
        std::cout << "Disconnecting from MQTT broker..." << std::endl;
        mqtt->disconnect();
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}