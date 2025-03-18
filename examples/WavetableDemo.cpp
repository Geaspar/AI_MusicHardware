#include "../include/audio/AudioEngine.h"
#include "../include/audio/Synthesizer.h"
#include "../include/effects/AllEffects.h"
#include "../include/synthesis/effects/effect_processor.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <cmath>

using namespace AIMusicHardware;

// Custom effect processor that inherits from the new architecture
class SimpleReverbEffect : public Processor {
public:
    SimpleReverbEffect(int sampleRate = 44100)
        : Processor(sampleRate),
          decay_(0.8f),
          mix_(0.5f) {
        // Initialize delay lines
        delayLines_.resize(8);
        for (int i = 0; i < 8; i++) {
            // Different delay times for a rich sound
            int delayLength = sampleRate * (0.05f + i * 0.01f); // 50ms - 120ms
            delayLines_[i].resize(delayLength, 0.0f);
        }
        
        // Initialize positions
        positions_.resize(8, 0);
    }
    
    void process(float* buffer, int numFrames) override {
        if (!enabled_) {
            return;
        }
        
        for (int i = 0; i < numFrames; i++) {
            // Get input samples for left and right channels
            float inputL = buffer[i * 2];
            float inputR = buffer[i * 2 + 1];
            
            // Process through delay lines
            float reverbL = 0.0f;
            float reverbR = 0.0f;
            
            for (int j = 0; j < 4; j++) {
                // Process left channel through first 4 delay lines
                reverbL += processDelayLine(j, inputL);
                
                // Process right channel through next 4 delay lines
                reverbR += processDelayLine(j + 4, inputR);
            }
            
            // Mix dry and wet signals
            buffer[i * 2] = inputL * (1.0f - mix_) + reverbL * mix_;
            buffer[i * 2 + 1] = inputR * (1.0f - mix_) + reverbR * mix_;
        }
    }
    
    void setDecay(float decay) {
        decay_ = std::clamp(decay, 0.0f, 0.99f);
    }
    
    void setMix(float mix) {
        mix_ = std::clamp(mix, 0.0f, 1.0f);
    }
    
    std::string getName() const override {
        return "SimpleReverb";
    }
    
private:
    float processDelayLine(int index, float input) {
        // Get current position in the delay line
        int position = positions_[index];
        
        // Get the delayed sample
        float delayed = delayLines_[index][position];
        
        // Calculate new sample (input + feedback)
        float newSample = input + delayed * decay_;
        
        // Store the new sample
        delayLines_[index][position] = newSample;
        
        // Increment position
        positions_[index] = (position + 1) % delayLines_[index].size();
        
        return delayed;
    }
    
    std::vector<std::vector<float>> delayLines_;
    std::vector<int> positions_;
    float decay_;
    float mix_;
};

// Example melody to play
struct Note {
    int midiNote;
    float duration;    // in seconds
    float velocity;
};

const std::vector<Note> melody = {
    {60, 0.5, 0.8},   // C4
    {64, 0.5, 0.7},   // E4
    {67, 0.5, 0.7},   // G4
    {72, 1.0, 0.8},   // C5
    {67, 0.5, 0.7},   // G4
    {64, 0.5, 0.7},   // E4
    {60, 1.0, 0.8},   // C4
};

int main() {
    // Create audio engine and synthesizer
    AudioEngine audioEngine;
    Synthesizer synth(44100);
    
    // Initialize
    synth.initialize();
    
    // Configure voice count
    synth.setVoiceCount(8);
    
    // Add effects
    auto reverb = std::make_unique<SimpleReverbEffect>(44100);
    reverb->setDecay(0.8f);
    reverb->setMix(0.3f);
    synth.addEffect(std::move(reverb));
    
    // Create a test wavetable with morphing between shapes
    auto wavetable = std::make_shared<Wavetable>();
    wavetable->initBasicWaveforms();
    synth.setWavetable(wavetable);
    
    // Set up modulation
    // Get modulation matrix
    ModulationMatrix* modMatrix = synth.getModulationMatrix();
    
    // Create destination for wavetable position
    // This is just for demonstration. In a real implementation, we'd need 
    // additional code to expose wavetable position as a destination parameter.
    
    // Start audio processing
    if (!audioEngine.start([&](float* buffer, int numFrames) {
        synth.process(buffer, numFrames);
        return true;
    })) {
        std::cerr << "Failed to start audio engine\n";
        return 1;
    }
    
    std::cout << "Wavetable Synthesizer Demo\n";
    std::cout << "Playing melody...\n";
    
    // Play the melody
    for (const auto& note : melody) {
        // Note on
        synth.noteOn(note.midiNote, note.velocity);
        
        // Wait for note duration
        std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<int>(note.duration * 1000)));
        
        // Note off
        synth.noteOff(note.midiNote);
        
        // Small pause between notes
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    
    // Allow reverb tail to fade out
    std::cout << "Melody finished, waiting for reverb tail...\n";
    std::this_thread::sleep_for(std::chrono::seconds(2));
    
    // Stop audio processing
    audioEngine.stop();
    
    std::cout << "Demo completed\n";
    
    return 0;
}