#include "../include/synthesis/voice/MpeAwareVoiceManager.h"
#include "../include/midi/MpeConfiguration.h"
#include "../include/midi/MpeChannelAllocator.h"
#include "../include/audio/AudioEngine.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <functional>
#include <vector>

using namespace AIMusicHardware;

// Audio processor for testing
class TestAudioProcessor {
public:
    TestAudioProcessor(MpeAwareVoiceManager& voiceManager)
        : voiceManager_(voiceManager) {}

    void process(float* buffer, int numFrames) {
        // Clear the buffer
        for (int i = 0; i < numFrames * 2; ++i) {
            buffer[i] = 0.0f;
        }

        // Process the voice manager
        voiceManager_.process(buffer, numFrames);
    }

private:
    MpeAwareVoiceManager& voiceManager_;
};

// Print MPE settings
void printMpeSettings(const MpeConfiguration& mpeConfig) {
    const auto& lowerZone = mpeConfig.getLowerZone();
    const auto& upperZone = mpeConfig.getUpperZone();
    
    std::cout << "MPE Configuration:\n";
    std::cout << "Lower Zone: " << (lowerZone.active ? "Active" : "Inactive") << "\n";
    if (lowerZone.active) {
        std::cout << "  Master Channel: " << lowerZone.masterChannel << "\n";
        std::cout << "  Member Channels: " << lowerZone.startMemberChannel 
                  << " to " << lowerZone.endMemberChannel << "\n";
        std::cout << "  Pitch Bend Range: " << lowerZone.pitchBendRange << " semitones\n";
    }
    
    std::cout << "Upper Zone: " << (upperZone.active ? "Active" : "Inactive") << "\n";
    if (upperZone.active) {
        std::cout << "  Master Channel: " << upperZone.masterChannel << "\n";
        std::cout << "  Member Channels: " << upperZone.startMemberChannel 
                  << " to " << upperZone.endMemberChannel << "\n";
        std::cout << "  Pitch Bend Range: " << upperZone.pitchBendRange << " semitones\n";
    }
    std::cout << std::endl;
}

// Test pattern to demonstrate MPE expression
void playMpeExpressionDemo(MpeAwareVoiceManager& voiceManager, MpeChannelAllocator& allocator) {
    std::cout << "Playing MPE expression demo...\n";
    
    // Play a chord with increasing values of timbre across notes
    const std::vector<int> chordNotes = {60, 64, 67, 72};
    std::vector<int> channels;
    
    // Start with all notes at default expression values
    std::cout << "Playing chord with default expression values...\n";
    for (size_t i = 0; i < chordNotes.size(); ++i) {
        int note = chordNotes[i];
        int channel = allocator.allocateChannel(note, 100, true);
        
        if (channel >= 0) {
            channels.push_back(channel);
            voiceManager.noteOnWithExpression(note, 0.8f, channel, 0.0f, 0.5f, 0.0f);
            std::cout << "Note " << note << " on channel " << channel << "\n";
        }
    }
    
    // Wait a moment
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    
    // Modify timbre (brighten each note progressively)
    std::cout << "Applying timbre modulation (brightness)...\n";
    for (size_t i = 0; i < channels.size(); ++i) {
        float timbre = 0.5f + (float)(i+1) * 0.1f;  // Increasing brightness
        voiceManager.updateNoteTimbre(channels[i], timbre);
        std::cout << "Channel " << channels[i] << " timbre set to " << timbre << "\n";
        std::this_thread::sleep_for(std::chrono::milliseconds(300));
    }
    
    // Wait a moment
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    
    // Pitch bend sweep on all notes
    std::cout << "Applying pitch bend sweep to all notes...\n";
    const int steps = 20;
    for (int i = 0; i < steps; ++i) {
        float bendAmount = -0.5f + (float)i / (float)(steps-1);  // -0.5 to 0.5
        for (int channel : channels) {
            voiceManager.updateNotePitchBend(channel, bendAmount);
        }
        std::cout << "Pitch bend: " << bendAmount << "\n";
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    // Wait a moment
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    
    // Apply pressure to each note one by one
    std::cout << "Applying pressure to each note in sequence...\n";
    for (size_t i = 0; i < channels.size(); ++i) {
        // Gradually increase pressure
        for (int j = 0; j < 10; ++j) {
            float pressure = (float)j / 9.0f;
            voiceManager.updateNotePressure(channels[i], pressure);
            std::cout << "Channel " << channels[i] << " pressure set to " << pressure << "\n";
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        
        // Gradually decrease pressure
        for (int j = 9; j >= 0; --j) {
            float pressure = (float)j / 9.0f;
            voiceManager.updateNotePressure(channels[i], pressure);
            std::cout << "Channel " << channels[i] << " pressure set to " << pressure << "\n";
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }
    
    // Wait a moment
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    
    // Release all notes
    std::cout << "Releasing all notes...\n";
    for (size_t i = 0; i < chordNotes.size(); ++i) {
        if (i < channels.size()) {
            voiceManager.noteOff(chordNotes[i], channels[i]);
            allocator.releaseChannel(channels[i]);
        }
    }
}

int main() {
    std::cout << "MPE Voice Manager Test\n";
    std::cout << "---------------------------\n";
    
    // Setup MPE configuration
    MpeConfiguration mpeConfig;
    mpeConfig.setLowerZone(true, 7);  // Enable lower zone with 7 member channels
    
    // Print MPE config
    printMpeSettings(mpeConfig);
    
    // Create MPE channel allocator
    MpeChannelAllocator allocator(mpeConfig);
    
    // Create MPE-aware voice manager
    MpeAwareVoiceManager voiceManager(44100, 16, mpeConfig);
    
    // Initialize audio engine
    AudioEngine audioEngine;
    TestAudioProcessor audioProcessor(voiceManager);

    // Set up the audio callback
    audioEngine.setAudioCallback([&audioProcessor](float* buffer, int numFrames) {
        audioProcessor.process(buffer, numFrames);
    });

    // Start audio (initialize audio engine)
    if (!audioEngine.initialize()) {
        std::cerr << "Failed to initialize audio engine!\n";
        return 1;
    }

    std::cout << "Audio engine started. Playing MPE demo...\n";

    // Run the MPE expression demo
    playMpeExpressionDemo(voiceManager, allocator);

    // Let final notes release fully
    std::this_thread::sleep_for(std::chrono::milliseconds(2000));

    // Stop audio
    audioEngine.shutdown();
    std::cout << "Audio engine stopped.\n";
    
    return 0;
}