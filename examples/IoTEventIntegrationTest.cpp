#include <iostream>
#include <iomanip>
#include <string>
#include <thread>
#include <chrono>
#include <csignal>
#include <atomic>
#include <unordered_map>

#include "../include/iot/IoTInterface.h"
#include "../include/iot/MQTTInterface.h"
#include "../include/iot/IoTEventAdapter.h"
#include "../include/events/EventBus.h"
#include "../include/events/EventListener.h"

using namespace AIMusicHardware;

// Global flag for termination
std::atomic<bool> running{true};

// Signal handler
void signalHandler(int signal) {
    std::cout << "Caught signal " << signal << ", shutting down..." << std::endl;
    running = false;
}

// Mock state machine for testing state changes
class MockStateMachine : public EventListener {
public:
    MockStateMachine() {
        // Register for state change events
        EventBus::getInstance().addEventListener("state_change", this);
    }
    
    ~MockStateMachine() {
        // Unregister from events
        EventBus::getInstance().removeEventListener("state_change", this);
    }
    
    void onEvent(const Event& event) override {
        if (event.getId() == "state_change") {
            try {
                if (auto* stateEvent = dynamic_cast<const StateChangeEvent*>(&event)) {
                    std::string newState = stateEvent->getTargetState();
                    std::cout << "State Machine: Changing state to " << newState << std::endl;
                    currentState_ = newState;
                }
            } catch (const std::exception& e) {
                std::cerr << "Error handling state change event: " << e.what() << std::endl;
            }
        }
    }
    
    std::string getCurrentState() const {
        return currentState_;
    }
    
private:
    std::string currentState_ = "idle";
};

// Mock sequencer for testing pattern control
class MockSequencer : public EventListener {
public:
    MockSequencer() {
        // Register for pattern control events
        EventBus::getInstance().addEventListener("pattern_control", this);
    }
    
    ~MockSequencer() {
        // Unregister from events
        EventBus::getInstance().removeEventListener("pattern_control", this);
    }
    
    void onEvent(const Event& event) override {
        if (event.getId() == "pattern_control") {
            try {
                if (auto* patternEvent = dynamic_cast<const PatternEvent*>(&event)) {
                    std::string patternId = patternEvent->getPatternId();
                    PatternEvent::Action action = patternEvent->getAction();
                    
                    switch (action) {
                        case PatternEvent::Action::START:
                            std::cout << "Sequencer: Starting pattern " << patternId << std::endl;
                            activePatterns_[patternId] = true;
                            break;
                            
                        case PatternEvent::Action::STOP:
                            std::cout << "Sequencer: Stopping pattern " << patternId << std::endl;
                            activePatterns_[patternId] = false;
                            break;
                            
                        case PatternEvent::Action::PAUSE:
                            std::cout << "Sequencer: Pausing pattern " << patternId << std::endl;
                            break;
                            
                        case PatternEvent::Action::RESUME:
                            std::cout << "Sequencer: Resuming pattern " << patternId << std::endl;
                            activePatterns_[patternId] = true;
                            break;
                            
                        case PatternEvent::Action::RESTART:
                            std::cout << "Sequencer: Restarting pattern " << patternId << std::endl;
                            activePatterns_[patternId] = true;
                            break;
                    }
                }
            } catch (const std::exception& e) {
                std::cerr << "Error handling pattern event: " << e.what() << std::endl;
            }
        }
    }
    
    bool isPatternActive(const std::string& patternId) const {
        auto it = activePatterns_.find(patternId);
        return it != activePatterns_.end() && it->second;
    }
    
private:
    std::unordered_map<std::string, bool> activePatterns_;
};

// Generic IoT message listener for logging
class IoTMessageLogger : public EventListener {
public:
    IoTMessageLogger() {
        // Register for IoT message events
        EventBus::getInstance().addEventListener("iot_message", this);
    }
    
    ~IoTMessageLogger() {
        // Unregister from events
        EventBus::getInstance().removeEventListener("iot_message", this);
    }
    
    void onEvent(const Event& event) override {
        if (event.getId() == "iot_message") {
            try {
                if (auto* iotEvent = dynamic_cast<const IoTEvent*>(&event)) {
                    std::cout << "IoT Message: " << iotEvent->getTopic() 
                              << " = " << iotEvent->getPayload() << std::endl;
                }
            } catch (const std::exception& e) {
                std::cerr << "Error handling IoT event: " << e.what() << std::endl;
            }
        }
    }
};

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
    
    std::cout << "IoT Event Integration Test" << std::endl;
    std::cout << "==========================" << std::endl;
    std::cout << "Connecting to MQTT broker at " << brokerHost << ":" << brokerPort << std::endl;
    
    try {
        // Initialize EventBus
        EventBus& eventBus = EventBus::getInstance();
        
        // Create mock components
        MockStateMachine stateMachine;
        MockSequencer sequencer;
        IoTMessageLogger logger;
        
        // Create MQTT interface
        auto mqtt = std::make_unique<MQTTInterface>();
        
        // Set connection options
        mqtt->setConnectionOptions(60, true, true);
        
        // Connect to broker
        bool connected = mqtt->connect(brokerHost, brokerPort, "AIMusicHardwareEventTest");
        
        if (!connected) {
            std::cerr << "Failed to connect to MQTT broker" << std::endl;
            return 1;
        }
        
        std::cout << "Connected to MQTT broker" << std::endl;
        
        // Create IoT event adapter
        IoTEventAdapter adapter(mqtt.get(), &eventBus);
        
        // Set up event mappings
        adapter.mapTopicToEvent("music/state", "state_change");
        adapter.mapTopicToEvent("music/pattern", "pattern_control");
        
        // Start adapter
        adapter.start();
        
        // Subscribe to all topics
        mqtt->subscribe("music/#");
        
        // Register for generic parameter events to see all events
        eventBus.addEventListener("parameter_change", [](const Event& event) {
            try {
                if (auto* paramEvent = dynamic_cast<const ParameterEvent*>(&event)) {
                    std::cout << "Parameter Event: " << paramEvent->getParameterId() 
                              << " = " << paramEvent->getValue() << std::endl;
                }
            } catch (const std::exception& e) {
                std::cerr << "Error handling parameter event: " << e.what() << std::endl;
            }
        });
        
        // Publish test messages
        std::cout << "\nSending test messages..." << std::endl;
        
        // State change messages
        mqtt->publish("music/state", "combat");
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        // Pattern control messages
        mqtt->publish("music/pattern", "battle_loop:start");
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        mqtt->publish("music/pattern", "ambient_background:start");
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        // Parameter change messages
        mqtt->publish("music/parameter/intensity", "0.75");
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        // Wait for events to be processed
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        
        // Print current state
        std::cout << "\nCurrent state: " << stateMachine.getCurrentState() << std::endl;
        std::cout << "Active patterns: " 
                  << "battle_loop=" << (sequencer.isPatternActive("battle_loop") ? "active" : "inactive")
                  << ", ambient_background=" << (sequencer.isPatternActive("ambient_background") ? "active" : "inactive")
                  << std::endl;
        
        // Main loop
        std::cout << "\nEntering main loop. Press Ctrl+C to exit." << std::endl;
        std::cout << "You can publish messages to music/# topics from another MQTT client" << std::endl;
        std::cout << " - music/state <state_name> : Change state" << std::endl;
        std::cout << " - music/pattern <pattern_id>:start|stop|pause|resume|restart : Control pattern" << std::endl;
        std::cout << " - music/parameter/<param_id> <value> : Change parameter value" << std::endl;
        
        int counter = 0;
        while (running) {
            // Process MQTT messages
            mqtt->update();
            
            // Update event bus (process scheduled events)
            eventBus.update(0.01); // 10ms
            
            // Print state every ~3 seconds
            if (++counter % 300 == 0) {
                std::cout << "\nCurrent state: " << stateMachine.getCurrentState() << std::endl;
                std::cout << "Active patterns: " 
                          << "battle_loop=" << (sequencer.isPatternActive("battle_loop") ? "active" : "inactive")
                          << ", ambient_background=" << (sequencer.isPatternActive("ambient_background") ? "active" : "inactive")
                          << std::endl;
                
                // If we're in combat mode, simulate a scheduled state change to explore
                if (stateMachine.getCurrentState() == "combat" && counter % 1200 == 0) {
                    std::cout << "Scheduling state change to 'explore' in 3 seconds..." << std::endl;
                    
                    // Create state change event
                    StateChangeEvent stateEvent("explore");
                    
                    // Schedule for 3 seconds in the future
                    eventBus.scheduleEvent(stateEvent, 3.0);
                }
            }
            
            // Sleep for 10ms
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        
        std::cout << "Disconnecting from MQTT broker..." << std::endl;
        
        // Stop adapter
        adapter.stop();
        
        // Disconnect
        mqtt->disconnect();
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}