#include "../../include/audio/Synthesizer.h"
#include "../../include/sequencer/Sequencer.h"
#include <cmath>
#include <algorithm>
#include <random>
#include <iostream>

namespace AIMusicHardware {

// Helper LFO source for modulation
class LfoSource : public ModulationSource {
public:
    enum class WaveShape {
        Sine,
        Triangle,
        Saw,
        Square,
        Random
    };
    
    LfoSource(const std::string& name, int sampleRate = 44100)
        : ModulationSource(name),
          sampleRate_(sampleRate),
          frequency_(1.0f),  // 1 Hz default
          phase_(0.0f),
          shape_(WaveShape::Sine),
          value_(0.0f) {
    }
    
    float getValue() const override {
        return value_;
    }
    
    void update() override {
        // Update phase
        phase_ += frequency_ / sampleRate_;
        if (phase_ >= 1.0f) {
            phase_ -= 1.0f;
        }
        
        // Generate value based on wave shape
        switch (shape_) {
            case WaveShape::Sine:
                value_ = std::sin(phase_ * 2.0f * 3.14159265359f);
                break;
                
            case WaveShape::Triangle:
                value_ = (phase_ < 0.5f) ? 
                    (4.0f * phase_ - 1.0f) : 
                    (3.0f - 4.0f * phase_);
                break;
                
            case WaveShape::Saw:
                value_ = 2.0f * phase_ - 1.0f;
                break;
                
            case WaveShape::Square:
                value_ = (phase_ < 0.5f) ? 1.0f : -1.0f;
                break;
                
            case WaveShape::Random:
                if (phase_ < prevPhase_) {
                    // Generate new random value when phase wraps
                    std::random_device rd;
                    std::mt19937 gen(rd());
                    std::uniform_real_distribution<float> dist(-1.0f, 1.0f);
                    value_ = dist(gen);
                }
                break;
        }
        
        prevPhase_ = phase_;
    }
    
    void setFrequency(float freq) {
        frequency_ = std::clamp(freq, 0.01f, 20.0f); // Limit to reasonable range
    }
    
    void setShape(WaveShape shape) {
        shape_ = shape;
    }
    
    void setSampleRate(int sampleRate) {
        sampleRate_ = sampleRate;
    }
    
private:
    int sampleRate_;
    float frequency_;
    float phase_;
    float prevPhase_ = 0.0f;
    WaveShape shape_;
    float value_;
};

// Synthesizer implementation
Synthesizer::Synthesizer(int sampleRate)
    : Processor(sampleRate),
      currentOscType_(OscillatorType::Sine) {
      
    // Create VoiceManager
    voiceManager_ = std::make_unique<VoiceManager>(sampleRate);
    
    // Create default wavetable
    createDefaultWavetable();
    
    // Create modulation sources
    createModulationSources();
}

Synthesizer::~Synthesizer() {
}

void Synthesizer::createDefaultWavetable() {
    currentWavetable_ = std::make_shared<Wavetable>();
    currentWavetable_->initBasicWaveforms();
    
    if (voiceManager_) {
        voiceManager_->setWavetable(currentWavetable_);
    }
}

bool Synthesizer::initialize() {
    try {
        // Nothing to do here now, constructor handles initialization
        return true;
    } catch (const std::exception& e) {
        // Handle any exceptions during initialization
        return false;
    }
}

void Synthesizer::createModulationSources() {
    // Create LFO sources
    auto lfo1 = std::make_unique<LfoSource>("LFO1", sampleRate_);
    auto lfo2 = std::make_unique<LfoSource>("LFO2", sampleRate_);
    
    // Set different default shapes
    lfo1->setFrequency(1.0f);  // 1 Hz
    lfo2->setFrequency(0.5f);  // 0.5 Hz
    
    // Add to modulation matrix
    modulationMatrix_.addSource(std::move(lfo1));
    modulationMatrix_.addSource(std::move(lfo2));
}

void Synthesizer::setSampleRate(int sampleRate) {
    // Call base class method
    Processor::setSampleRate(sampleRate);

    // Update components
    if (voiceManager_) {
        voiceManager_->setSampleRate(sampleRate);
    }

    effectChain_.setSampleRate(sampleRate);
    
    // Update LFOs
    if (auto lfo1 = dynamic_cast<LfoSource*>(modulationMatrix_.getSource("LFO1"))) {
        lfo1->setSampleRate(sampleRate);
    }
    
    if (auto lfo2 = dynamic_cast<LfoSource*>(modulationMatrix_.getSource("LFO2"))) {
        lfo2->setSampleRate(sampleRate);
    }
}

void Synthesizer::noteOn(int midiNote, float velocity, int channel) {
    if (voiceManager_) {
        voiceManager_->noteOn(midiNote, velocity, channel);
    }
}

void Synthesizer::noteOn(int midiNote, float velocity, const AIMusicHardware::Envelope& legacyEnv, int channel) {
    if (voiceManager_) {
        // Legacy support - first use standard noteOn
        voiceManager_->noteOn(midiNote, velocity, channel);
        
        // Then find and update the envelope for this note
        // This is not implemented here since we don't have direct access to voice envelopes
        // through VoiceManager. In a real implementation, we'd need to extend VoiceManager
        // to support this or handle envelope mapping differently.
    }
}

void Synthesizer::noteOff(int midiNote, int channel) {
    if (voiceManager_) {
        voiceManager_->noteOff(midiNote, channel);
    }
}

void Synthesizer::allNotesOff(int channel) {
    if (voiceManager_) {
        voiceManager_->allNotesOff(channel);
    }
}

void Synthesizer::sustainOn(int channel) {
    if (voiceManager_) {
        voiceManager_->sustainOn(channel);
        std::cout << "Sustain pedal on for channel " << channel << std::endl;
    }
}

void Synthesizer::sustainOff(int channel) {
    if (voiceManager_) {
        voiceManager_->sustainOff(channel);
        std::cout << "Sustain pedal off for channel " << channel << std::endl;
    }
}

void Synthesizer::setPitchBend(float value, int channel) {
    if (voiceManager_) {
        voiceManager_->setPitchBend(value, channel);
        std::cout << "Pitch bend value " << value << " for channel " << channel << std::endl;
    }
}

void Synthesizer::setAftertouch(int note, float pressure, int channel) {
    if (voiceManager_) {
        voiceManager_->setAftertouch(note, pressure, channel);
        std::cout << "Aftertouch for note " << note << " with pressure " << pressure 
                  << " on channel " << channel << std::endl;
    }
}

void Synthesizer::setChannelPressure(float pressure, int channel) {
    if (voiceManager_) {
        voiceManager_->setChannelPressure(pressure, channel);
        std::cout << "Channel pressure " << pressure << " for channel " << channel << std::endl;
    }
}

void Synthesizer::resetAllControllers() {
    if (voiceManager_) {
        voiceManager_->resetAllControllers();
        std::cout << "Resetting all controllers" << std::endl;
    }
}

void Synthesizer::setParameter(const std::string& paramId, float value) {
    // We'll need a parameter system for this in the future
    // For now, just handle a few basic parameters

    if (paramId == "oscillator_frame") {
        if (voiceManager_) {
            // Update oscillator frame position in all voices
            for (int i = 0; i < voiceManager_->getMaxVoices(); ++i) {
                if (auto* voice = voiceManager_->getVoice(i)) {
                    if (auto* osc = voice->getOscillator()) {
                        osc->setFramePosition(value);
                    }
                }
            }
            std::cout << "Setting oscillator frame to " << value << std::endl;
        }
    }
    else if (paramId == "oscillator_type") {
        // Convert 0-4 float value to oscillator type
        int typeIndex = static_cast<int>(value);
        if (typeIndex >= 0 && typeIndex <= 4) {
            setOscillatorType(static_cast<OscillatorType>(typeIndex));
        }
    }
    else if (paramId == "filter_cutoff") {
        // For future implementation - will need to add filter to VoiceManager
        std::cout << "Setting filter cutoff to " << value << std::endl;
    }
    else if (paramId == "filter_resonance") {
        // For future implementation - will need to add filter to VoiceManager
        std::cout << "Setting filter resonance to " << value << std::endl;
    }
    else if (paramId == "master_volume") {
        // For future implementation - adjust master volume
        std::cout << "Setting master volume to " << value << std::endl;
    }
    else if (paramId == "envelope_attack") {
        // Update all voices' attack time
        if (voiceManager_) {
            for (int i = 0; i < voiceManager_->getMaxVoices(); ++i) {
                if (auto* voice = voiceManager_->getVoice(i)) {
                    if (auto* envelope = voice->getEnvelope()) {
                        envelope->setAttack(value);
                    }
                }
            }
        }
        std::cout << "Setting envelope attack to " << value << " seconds" << std::endl;
    }
    else if (paramId == "envelope_decay") {
        // Update all voices' decay time
        if (voiceManager_) {
            for (int i = 0; i < voiceManager_->getMaxVoices(); ++i) {
                if (auto* voice = voiceManager_->getVoice(i)) {
                    if (auto* envelope = voice->getEnvelope()) {
                        envelope->setDecay(value);
                    }
                }
            }
        }
        std::cout << "Setting envelope decay to " << value << " seconds" << std::endl;
    }
    else if (paramId == "envelope_sustain") {
        // Update all voices' sustain level
        if (voiceManager_) {
            for (int i = 0; i < voiceManager_->getMaxVoices(); ++i) {
                if (auto* voice = voiceManager_->getVoice(i)) {
                    if (auto* envelope = voice->getEnvelope()) {
                        envelope->setSustain(value);
                    }
                }
            }
        }
        std::cout << "Setting envelope sustain to " << value << std::endl;
    }
    else if (paramId == "envelope_release") {
        // Update all voices' release time
        if (voiceManager_) {
            for (int i = 0; i < voiceManager_->getMaxVoices(); ++i) {
                if (auto* voice = voiceManager_->getVoice(i)) {
                    if (auto* envelope = voice->getEnvelope()) {
                        envelope->setRelease(value);
                    }
                }
            }
        }
        std::cout << "Setting envelope release to " << value << " seconds" << std::endl;
    }
    else if (paramId == "voice_count") {
        // Set number of voices
        int count = static_cast<int>(value);
        if (count > 0) {
            setVoiceCount(count);
        }
    }
    else if (paramId.find("lfo") == 0) {
        // Handle LFO parameters (lfo1_rate, lfo1_shape, etc.)
        std::cout << "Setting " << paramId << " to " << value << std::endl;

        // Parse LFO index and parameter name
        size_t underscorePos = paramId.find('_');
        if (underscorePos != std::string::npos && underscorePos < paramId.size() - 1) {
            std::string lfoName = paramId.substr(0, underscorePos); // "lfo1", "lfo2", etc.
            std::string paramName = paramId.substr(underscorePos + 1); // "rate", "shape", etc.

            if (auto* source = modulationMatrix_.getSource(lfoName)) {
                if (auto* lfo = dynamic_cast<LfoSource*>(source)) {
                    if (paramName == "rate") {
                        lfo->setFrequency(value);
                    }
                    else if (paramName == "shape") {
                        // Convert 0-4 float value to LFO shape
                        int shapeIndex = static_cast<int>(value);
                        if (shapeIndex >= 0 && shapeIndex <= 4) {
                            lfo->setShape(static_cast<LfoSource::WaveShape>(shapeIndex));
                        }
                    }
                }
            }
        }
    }
    else {
        std::cout << "Unknown parameter: " << paramId << std::endl;
    }
}

float Synthesizer::getParameter(const std::string& paramId) const {
    // Getting actual parameters

    if (paramId == "oscillator_frame") {
        // In a real implementation, we'd get this from the first voice or from a stored value
        // For now, return based on current oscillator type
        return oscTypeToFramePosition(currentOscType_);
    }
    else if (paramId == "oscillator_type") {
        return static_cast<float>(currentOscType_);
    }
    else if (paramId == "filter_cutoff") {
        // For future implementation - will need to add filter to VoiceManager
        return 1.0f; // Default value
    }
    else if (paramId == "filter_resonance") {
        // For future implementation - will need to add filter to VoiceManager
        return 0.5f; // Default value
    }
    else if (paramId == "master_volume") {
        // For future implementation - get master volume
        return 0.7f; // Default value
    }
    else if (paramId == "envelope_attack") {
        return 0.01f; // Default 10ms attack
    }
    else if (paramId == "envelope_decay") {
        return 0.1f; // Default 100ms decay
    }
    else if (paramId == "envelope_sustain") {
        return 0.7f; // Default 70% sustain
    }
    else if (paramId == "envelope_release") {
        return 0.5f; // Default 500ms release
    }
    else if (paramId == "voice_count") {
        return static_cast<float>(getVoiceCount());
    }
    else if (paramId.find("lfo") == 0) {
        // Handle LFO parameters (lfo1_rate, lfo1_shape, etc.)

        // Parse LFO index and parameter name
        size_t underscorePos = paramId.find('_');
        if (underscorePos != std::string::npos && underscorePos < paramId.size() - 1) {
            std::string lfoName = paramId.substr(0, underscorePos); // "lfo1", "lfo2", etc.
            std::string paramName = paramId.substr(underscorePos + 1); // "rate", "shape", etc.

            if (auto* source = modulationMatrix_.getSource(lfoName)) {
                if (auto* lfo = dynamic_cast<LfoSource*>(source)) {
                    if (paramName == "rate") {
                        // LFO frequency is private, we would need a getter
                        return 1.0f; // Default value
                    }
                    else if (paramName == "shape") {
                        // LFO shape is private, we would need a getter
                        return 0.0f; // Default value (sine)
                    }
                }
            }
        }
    }

    // Unknown parameter
    return 0.0f;
}

std::map<std::string, float> Synthesizer::getAllParameters() const {
    std::map<std::string, float> parameters;

    // Store core synthesizer parameters
    parameters["oscillator_type"] = static_cast<float>(currentOscType_);
    parameters["oscillator_frame"] = oscTypeToFramePosition(currentOscType_);
    parameters["voice_count"] = static_cast<float>(getVoiceCount());
    parameters["master_volume"] = 0.7f; // Default value for now

    // Future parameters to add when implemented:
    parameters["filter_cutoff"] = 1.0f;
    parameters["filter_resonance"] = 0.5f;

    // LFO Parameters
    parameters["lfo1_rate"] = 1.0f;
    parameters["lfo1_shape"] = 0.0f; // Sine
    parameters["lfo2_rate"] = 0.5f;
    parameters["lfo2_shape"] = 0.0f; // Sine

    // Add modulation connections when implemented

    return parameters;
}

void Synthesizer::setAllParameters(const std::map<std::string, float>& parameters) {
    // Apply all parameters at once
    for (const auto& [paramId, value] : parameters) {
        setParameter(paramId, value);
    }
}

void Synthesizer::setOscillatorType(OscillatorType type) {
    currentOscType_ = type;

    // Convert oscillator type to wavetable frame position
    float framePos = oscTypeToFramePosition(type);

    // Update frame position in all active voices
    if (voiceManager_) {
        // Iterate through all voices and set their oscillator's frame position
        for (int i = 0; i < voiceManager_->getMaxVoices(); ++i) {
            if (auto* voice = voiceManager_->getVoice(i)) {
                if (auto* osc = voice->getOscillator()) {
                    osc->setFramePosition(framePos);
                }
            }
        }
    }

    std::cout << "Oscillator type changed to " << static_cast<int>(type)
              << " (frame position: " << framePos << ")" << std::endl;
}

float Synthesizer::oscTypeToFramePosition(OscillatorType type) const {
    // Map oscillator type to frame position (0-1)
    switch (type) {
        case OscillatorType::Sine:
            return 0.0f;
        case OscillatorType::Saw:
            return 0.25f;
        case OscillatorType::Square:
            return 0.5f;
        case OscillatorType::Triangle:
            return 0.75f;
        case OscillatorType::Noise:
            return 1.0f;
        default:
            return 0.0f;
    }
}

void Synthesizer::setWavetable(std::shared_ptr<Wavetable> wavetable) {
    if (wavetable) {
        currentWavetable_ = wavetable;
        
        if (voiceManager_) {
            voiceManager_->setWavetable(wavetable);
        }
    }
}

void Synthesizer::setVoiceCount(int count) {
    if (voiceManager_) {
        voiceManager_->setMaxVoices(count);
    }
}

int Synthesizer::getVoiceCount() const {
    return voiceManager_ ? voiceManager_->getMaxVoices() : 0;
}

void Synthesizer::process(float* buffer, int numFrames) {
    if (!enabled_) {
        return;
    }
    
    // Clear buffer
    std::fill(buffer, buffer + numFrames * 2, 0.0f);
    
    // Update modulation matrix
    modulationMatrix_.update();
    
    // Process voices through voice manager
    if (voiceManager_) {
        voiceManager_->process(buffer, numFrames);
    }
    
    // Process effects chain
    effectChain_.process(buffer, numFrames);
    
    // Final limiter to prevent clipping
    const float masterVolume = 0.7f;
    for (int i = 0; i < numFrames * 2; ++i) {
        buffer[i] *= masterVolume;
        buffer[i] = std::clamp(buffer[i], -1.0f, 1.0f);
    }
}

void Synthesizer::reset() {
    Processor::reset();
    
    // Reset all components
    if (voiceManager_) {
        voiceManager_->allNotesOff();
    }
    
    effectChain_.reset();
}

void Synthesizer::legacyEnvelopeToNew(const AIMusicHardware::Envelope& legacyEnv, 
                                     AIMusicHardware::ModEnvelope* newEnv) {
    if (!newEnv) {
        return;
    }
    
    // Map legacy envelope parameters to new envelope
    newEnv->setAttack(legacyEnv.attack);
    newEnv->setDecay(legacyEnv.decay);
    newEnv->setSustain(legacyEnv.sustain);
    newEnv->setRelease(legacyEnv.release);
}

void Synthesizer::addEffect(std::unique_ptr<Processor> effect) {
    if (effect) {
        effectChain_.addProcessor(std::move(effect));
    }
}

void Synthesizer::removeEffect(size_t index) {
    effectChain_.removeProcessor(index);
}

Processor* Synthesizer::getEffect(size_t index) {
    return effectChain_.getProcessor(index);
}

size_t Synthesizer::getNumEffects() const {
    return effectChain_.getNumProcessors();
}

} // namespace AIMusicHardware