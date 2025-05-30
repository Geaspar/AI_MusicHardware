#include <iostream>
#include <thread>
#include <chrono>
#include "../include/midi/MidiCCLearning.h"

using namespace AIMusicHardware;

class TestParameterReceiver {
private:
    std::map<std::string, float> parameters_;
    
public:
    void setParameter(const std::string& paramId, float value) {
        parameters_[paramId] = value;
        std::cout << "Parameter Updated: " << paramId << " = " << value << std::endl;
    }
    
    float getParameter(const std::string& paramId) const {
        auto it = parameters_.find(paramId);
        return it != parameters_.end() ? it->second : 0.0f;
    }
    
    void printAllParameters() const {
        std::cout << "\n=== Current Parameters ===" << std::endl;
        for (const auto& param : parameters_) {
            std::cout << param.first << ": " << param.second << std::endl;
        }
        std::cout << "========================\n" << std::endl;
    }
};

int main() {
    std::cout << "=== MIDI CC Learning System Test ===" << std::endl;
    
    // Create test receiver
    TestParameterReceiver receiver;
    
    // Get CC Learning manager
    auto& manager = MidiCCLearningManager::getInstance();
    manager.initialize();
    
    auto& learning = manager.getLearning();
    
    // Set up callbacks
    learning.setParameterChangeCallback([&receiver](const std::string& paramId, float value) {
        receiver.setParameter(paramId, value);
    });
    
    learning.setLearningStateCallback([](MidiCCLearning::LearningState state, const std::string& message) {
        std::cout << "Learning State: " << message << std::endl;
    });
    
    learning.setMappingCreatedCallback([](const MidiCCLearning::CCMapping& mapping) {
        std::cout << "New Mapping: CC" << mapping.ccNumber 
                  << " (ch " << mapping.channel << ") -> " << mapping.parameterId << std::endl;
    });
    
    std::cout << "\n=== Test 1: Manual Parameter Learning ===" << std::endl;
    
    // Test 1: Learn a specific parameter
    std::cout << "Starting learning for 'filter_cutoff'..." << std::endl;
    learning.startLearning("filter_cutoff", std::chrono::milliseconds{3000});
    
    // Simulate CC input after a short delay
    std::this_thread::sleep_for(std::chrono::milliseconds{500});
    std::cout << "Simulating CC74 movement..." << std::endl;
    learning.processMidiCC(0, 74, 64, "Test Controller");  // CC74 value 64 (middle)
    
    // Wait a bit
    std::this_thread::sleep_for(std::chrono::milliseconds{1000});
    
    std::cout << "\n=== Test 2: Auto Learning Mode ===" << std::endl;
    
    // Test 2: Auto-learning mode
    std::cout << "Starting auto-learning for 5 seconds..." << std::endl;
    learning.startAutoLearning(std::chrono::milliseconds{5000});
    
    // Simulate multiple CC movements
    std::this_thread::sleep_for(std::chrono::milliseconds{500});
    std::cout << "Simulating multiple CC movements..." << std::endl;
    
    learning.processMidiCC(0, 1, 127, "Test Controller");   // Modwheel
    learning.processMidiCC(0, 1, 100, "Test Controller");
    learning.processMidiCC(0, 1, 80, "Test Controller");
    
    learning.processMidiCC(0, 7, 64, "Test Controller");    // Volume
    learning.processMidiCC(0, 7, 90, "Test Controller");
    learning.processMidiCC(0, 7, 50, "Test Controller");
    
    learning.processMidiCC(0, 10, 32, "Test Controller");   // Pan
    learning.processMidiCC(0, 10, 96, "Test Controller");
    
    // Wait for auto-learning to complete
    std::this_thread::sleep_for(std::chrono::milliseconds{5500});
    
    std::cout << "\n=== Test 3: Manual Mapping Creation ===" << std::endl;
    
    // Test 3: Create manual mappings
    MidiCCLearning::CCMapping resonanceMapping;
    resonanceMapping.channel = 0;
    resonanceMapping.ccNumber = 71; // Filter resonance
    resonanceMapping.parameterId = "filter_resonance";
    resonanceMapping.minValue = 0.0f;
    resonanceMapping.maxValue = 1.0f;
    resonanceMapping.curveType = MidiCCLearning::CCMapping::CurveType::SShape;
    resonanceMapping.deviceName = "Manual Mapping";
    
    learning.createMapping(resonanceMapping);
    
    MidiCCLearning::CCMapping volumeMapping;
    volumeMapping.channel = 0;
    volumeMapping.ccNumber = 7; // Master volume
    volumeMapping.parameterId = "master_volume";
    volumeMapping.minValue = 0.0f;
    volumeMapping.maxValue = 1.0f;
    volumeMapping.curveType = MidiCCLearning::CCMapping::CurveType::Logarithmic;
    volumeMapping.deviceName = "Manual Mapping";
    
    learning.createMapping(volumeMapping);
    
    std::cout << "\n=== Test 4: Testing Created Mappings ===" << std::endl;
    
    // Test the mappings by sending CC data
    std::cout << "Testing filter resonance (CC71)..." << std::endl;
    learning.processMidiCC(0, 71, 0, "Test");     // Min value
    learning.processMidiCC(0, 71, 127, "Test");   // Max value
    learning.processMidiCC(0, 71, 64, "Test");    // Middle value
    
    std::cout << "Testing master volume (CC7)..." << std::endl;
    learning.processMidiCC(0, 7, 0, "Test");      // Min value
    learning.processMidiCC(0, 7, 127, "Test");    // Max value
    learning.processMidiCC(0, 7, 96, "Test");     // 75% value
    
    // Test existing filter cutoff mapping (from learning)
    std::cout << "Testing filter cutoff (CC74)..." << std::endl;
    learning.processMidiCC(0, 74, 32, "Test");    // 25% value
    learning.processMidiCC(0, 74, 96, "Test");    // 75% value
    
    receiver.printAllParameters();
    
    std::cout << "\n=== Test 5: Mapping Information ===" << std::endl;
    
    auto allMappings = learning.getAllMappings();
    std::cout << "Total mappings: " << allMappings.size() << std::endl;
    
    for (const auto& mapping : allMappings) {
        std::cout << "Mapping: CC" << mapping.ccNumber 
                  << " (ch " << mapping.channel << ") -> " << mapping.parameterId
                  << " [" << mapping.minValue << " to " << mapping.maxValue << "]"
                  << " Device: " << mapping.deviceName << std::endl;
    }
    
    auto stats = learning.getStatistics();
    std::cout << "\n=== Learning Statistics ===" << std::endl;
    std::cout << "Total mappings: " << stats.totalMappings << std::endl;
    std::cout << "Active mappings: " << stats.activeMappings << std::endl;
    std::cout << "Messages processed: " << stats.messagesProcessed << std::endl;
    std::cout << "Learning sessions completed: " << stats.learningSessionsCompleted << std::endl;
    
    std::cout << "\nCC Usage Count:" << std::endl;
    for (const auto& usage : stats.ccUsageCount) {
        std::cout << "  CC" << usage.first << ": " << usage.second << " times" << std::endl;
    }
    
    std::cout << "\n=== Test 6: Persistence ===" << std::endl;
    
    // Test saving and loading
    std::string testFile = "test_cc_mappings.json";
    
    std::cout << "Saving mappings to " << testFile << "..." << std::endl;
    if (learning.saveMappings(testFile)) {
        std::cout << "Mappings saved successfully!" << std::endl;
        
        // Clear mappings
        learning.clearAllMappings();
        std::cout << "Cleared all mappings. Current count: " << learning.getAllMappings().size() << std::endl;
        
        // Load them back
        std::cout << "Loading mappings from " << testFile << "..." << std::endl;
        if (learning.loadMappings(testFile)) {
            std::cout << "Mappings loaded successfully! Count: " << learning.getAllMappings().size() << std::endl;
        } else {
            std::cout << "Failed to load mappings!" << std::endl;
        }
    } else {
        std::cout << "Failed to save mappings!" << std::endl;
    }
    
    std::cout << "\n=== MIDI CC Learning Test Complete ===" << std::endl;
    
    // Shutdown manager
    manager.shutdown();
    
    return 0;
}