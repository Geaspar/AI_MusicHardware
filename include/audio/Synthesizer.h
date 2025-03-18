#pragma once

#include <vector>
#include <array>
#include <memory>
#include <string>
#include <unordered_map>
#include "../sequencer/Sequencer.h" // Include for Envelope struct

// Include the new architecture
#include "../synthesis/framework/processor.h"
#include "../synthesis/voice/voice_manager.h"
#include "../synthesis/wavetable/wavetable.h"
#include "../synthesis/modulators/envelope.h"
#include "../synthesis/modulators/modulation_matrix.h"

namespace AIMusicHardware {

// Keep for backward compatibility
enum class OscillatorType {
    Sine,
    Square,
    Saw,
    Triangle,
    Noise
};

/**
 * Enhanced Synthesizer class using the new architecture.
 * This is a top-level wrapper that coordinates all components.
 */
class Synthesizer : public Processor {
public:
    Synthesizer(int sampleRate = 44100);
    ~Synthesizer() override;
    
    bool initialize();
    
    // Basic note control
    void noteOn(int midiNote, float velocity);
    void noteOn(int midiNote, float velocity, const AIMusicHardware::Envelope& env);
    void noteOff(int midiNote);
    void allNotesOff();
    
    // Legacy oscillator type for backward compatibility
    void setOscillatorType(OscillatorType type);
    
    // Wavetable control
    void setWavetable(std::shared_ptr<Wavetable> wavetable);
    void createDefaultWavetable();
    
    // Voice management
    void setVoiceCount(int count);
    int getVoiceCount() const;
    
    // Modulation system
    ModulationMatrix* getModulationMatrix() { return &modulationMatrix_; }
    
    // Processor implementation
    void process(float* buffer, int numFrames) override;
    void reset() override;
    void setSampleRate(int sampleRate) override;
    std::string getName() const override { return "Synthesizer"; }
    
    // Add LFO/Envelope modulation sources
    void createModulationSources();
    
    // Add and remove effects
    void addEffect(std::unique_ptr<Processor> effect);
    void removeEffect(size_t index);
    Processor* getEffect(size_t index);
    size_t getNumEffects() const;
    
private:
    // Convert legacy oscillator type to wavetable frame position
    float oscTypeToFramePosition(OscillatorType type);
    
    // Convert legacy envelope to new envelope parameters
    void legacyEnvelopeToNew(const AIMusicHardware::Envelope& legacyEnv, 
                             AIMusicHardware::ModEnvelope* newEnv);
    
    // Components
    std::unique_ptr<VoiceManager> voiceManager_;
    std::shared_ptr<Wavetable> currentWavetable_;
    ProcessorRouter effectChain_;
    ModulationMatrix modulationMatrix_;
    
    // Legacy compatibility
    OscillatorType currentOscType_;
};

} // namespace AIMusicHardware