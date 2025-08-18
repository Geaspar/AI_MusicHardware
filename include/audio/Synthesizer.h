#pragma once

#include <vector>
#include <array>
#include <memory>
#include <string>
#include <unordered_map>
#include <map>
#include "../sequencer/Sequencer.h" // Include for Envelope struct

// Include the new architecture
#include "../synthesis/framework/processor.h"
#include "../synthesis/wavetable/wavetable.h"
#include "../synthesis/modulators/envelope.h"
#include "../synthesis/modulators/modulation_matrix.h"
#include "../synthesis/voice/band_limited_voice.h"
#include "../synthesis/wavetable/hybrid_wavetable.h"
#include "../synthesis/wavetable/hybrid_wavetable_ops.h"
#include "../synthesis/wavetable/hybrid_wavetable_cache.h"

namespace AIMusicHardware {

// Forward declaration
class VoiceManager;
class RealtimeWavetableVoiceManager;

// Keep for backward compatibility
enum class OscillatorType {
    Sine,
    Square,
    Saw,
    Triangle,
    Noise
};

enum class VoiceManagerType {
    Standard,
    BandLimited,
    RealTime
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
    virtual void noteOn(int midiNote, float velocity, int channel = 0);
    virtual void noteOn(int midiNote, float velocity, const AIMusicHardware::Envelope& env, int channel = 0);
    virtual void noteOff(int midiNote, int channel = 0);
    virtual void allNotesOff(int channel = -1);  // -1 means all channels
    
    // MIDI-specific control methods
    void sustainOn(int channel = 0);
    void sustainOff(int channel = 0);
    void setPitchBend(float value, int channel = 0);  // value range: -1.0 to 1.0
    void setAftertouch(int note, float pressure, int channel = 0);
    void setChannelPressure(float pressure, int channel = 0);
    void resetAllControllers();
    
    // Parameter system
    void setParameter(const std::string& paramId, float value);
    float getParameter(const std::string& paramId) const;

    // Parameter methods for preset management
    std::map<std::string, float> getAllParameters() const;
    void setAllParameters(const std::map<std::string, float>& parameters);
    
    // Legacy oscillator type for backward compatibility
    void setOscillatorType(OscillatorType type);
    
    // Wavetable control
    void setWavetable(std::shared_ptr<Wavetable> wavetable);
    void createDefaultWavetable();
    
    // Voice management
    void setVoiceCount(int count);
    int getVoiceCount() const;
    VoiceManager* getVoiceManager() { return voiceManager_.get(); }
    void setVoiceManagerType(VoiceManagerType type);
    VoiceManagerType getVoiceManagerType() const { return voiceManagerType_; }
    
    // Modulation system
    ModulationMatrix* getModulationMatrix() { return &modulationMatrix_; }
    // Convenience: list destination names
    std::vector<std::string> getModDestinationNames() const { return modulationMatrix_.listDestinationNames(); }
    
    // Connect modulation source to destination
    void connectModulation(const std::string& sourceName, const std::string& destName, float amount);
    void disconnectModulation(const std::string& sourceName, const std::string& destName);
    
    // LFO control
    void setLFORate(int lfoIndex, float rate);
    void setLFOShape(int lfoIndex, int shape);
    void setLFODepth(int lfoIndex, float depth);
    
    // Global pitch modulation amounts (applied to all voices)
    void setGlobalPitchModulationAmount(const std::string& source, float semitones);
    
    // MIDI-driven continuous sources
    void setModWheel(float value); // 0.0 - 1.0
    
    // Processor implementation
    void process(float* buffer, int numFrames) override;
    void reset() override;
    void setSampleRate(int sampleRate) override;
    std::string getName() const override { return "Synthesizer"; }
    // Begin a short pop-free ramp after preset apply
    void startPresetApplyRamp(float seconds) {
        int n = std::max(0, static_cast<int>(seconds * static_cast<float>(getSampleRate())));
        paramApplyRampTotalSamples_ = n;
        paramApplyRampRemainingSamples_ = n;
    }

    // Temporary: runtime toggle for hybrid wavetable (for testing)
    void setHybridWavetableEnabled(bool enabled);
    bool isHybridWavetableEnabled() const { return hybridWavetableEnabled_; }
    // Settings: timbre mode (0=Linear, 1=MinPhase tentative)
    void setHybridTimbreMinPhase(bool minPhase) { hybridTimbreMinPhase_ = minPhase; }
    bool isHybridTimbreMinPhase() const { return hybridTimbreMinPhase_; }
    // Telemetry helpers
    uint64_t hybridCacheHits() const { return spectralCache_ ? spectralCache_->hits() : 0; }
    uint64_t hybridCacheMisses() const { return spectralCache_ ? spectralCache_->misses() : 0; }
    size_t hybridQueueSize() const { return spectralWorker_ ? spectralWorker_->queueSize() : 0; }
    size_t hybridInFlight() const { return spectralWorker_ ? spectralWorker_->inFlightCount() : 0; }
    bool hybridMinPhase() const { return hybridTimbreMinPhase_; }
    // Expose spectral services for Settings controls
    std::shared_ptr<SpectralWavetableCache> getSpectralCache() { return spectralCache_; }
    SpectralRenderWorker* getSpectralWorker() { return spectralWorker_.get(); }
    
    // Add LFO/Envelope modulation sources
    void createModulationSources();
    
    // Add and remove effects
    void addEffect(std::unique_ptr<Processor> effect);
    void removeEffect(size_t index);
    Processor* getEffect(size_t index);
    size_t getNumEffects() const;
    
    // Set external effect processor for filter control
    void setExternalEffectProcessor(class EffectProcessor* effectProcessor) {
        externalEffectProcessor_ = effectProcessor;
    }
    
    // Quality settings for band-limited oscillators
    void setOversamplingEnabled(bool enable);
    void setOversamplingFactor(OversamplingProcessor::Factor factor);
    
private:
    // Convert legacy oscillator type to wavetable frame position
    float oscTypeToFramePosition(OscillatorType type) const;
    
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
    
    // Modulation state
    std::unordered_map<std::string, float> baseParameterValues_;
    
    // Global pitch modulation amounts
    std::unordered_map<std::string, float> globalPitchModAmounts_;
    
    // LFO pointers for direct access (optimization)
    class LfoSource* lfo1_ = nullptr;
    class LfoSource* lfo2_ = nullptr;
    
    // External effect processor for filter control
    class EffectProcessor* externalEffectProcessor_ = nullptr;
    
    // Voice manager state
    VoiceManagerType voiceManagerType_ = VoiceManagerType::Standard;
    bool oversamplingEnabled_ = false;
    OversamplingProcessor::Factor oversamplingFactor_ = OversamplingProcessor::Factor::x1;

    // Hybrid spectral wavetable (Phase 1 scaffolding)
    std::shared_ptr<SpectralWavetableCache> spectralCache_;
    std::unique_ptr<SpectralRenderWorker> spectralWorker_;
    bool hybridWavetableEnabled_ = false; // gate usage
    bool hybridTimbreMinPhase_ = false;   // UI setting (future DSP)

    // Continuous values for MIDI-driven modulation sources
    float modWheelValue_ = 0.0f;     // 0..1
    float aftertouchValue_ = 0.0f;   // 0..1 (channel pressure or max poly AT)
    float velocityValue_ = 0.0f;     // 0..1 (last note-on velocity)

    // Preset-level output trim smoothing
    float presetTrimTargetDb_ = 0.0f;       // desired trim in dB
    float presetTrimLinear_ = 1.0f;         // smoothed linear gain applied pre-master
    float presetTrimSmoothingAlpha_ = 0.1f; // per-call smoothing factor (0..1)

    // Master volume smoothing (pop-free loads)
    float masterVolumeSmoothed_ = 1.0f;     // linear gain smoothed each call
    float masterVolumeSmoothingAlpha_ = 0.12f;

    // Short ramp after preset apply to avoid clicks
    int paramApplyRampRemainingSamples_ = 0;
    int paramApplyRampTotalSamples_ = 0;
};

} // namespace AIMusicHardware