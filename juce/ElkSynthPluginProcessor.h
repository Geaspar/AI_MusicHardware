#pragma once

#include <atomic>
#include <algorithm>
#include <functional>
#include <memory>
#include <juce_audio_processors/juce_audio_processors.h>

namespace AIMusicHardware {
class Synthesizer;
class Sequencer;
class HostSync;
class ClockSource;
class EffectProcessor;
} // namespace AIMusicHardware

enum class SensorTarget {
    FilterCutoff = 0,
    MasterVolume = 1,
    PitchBend    = 2
};

enum class SensorMode {
    Manual = 0,
    External = 1
};

// JUCE AudioProcessor wrapper around the existing AIMusicHardware engine.
class ElkSynthProcessor : public juce::AudioProcessor {
public:
    ElkSynthProcessor();
    ~ElkSynthProcessor() override;

    // JUCE AudioProcessor mandatory overrides
    const juce::String getName() const override { return "AIMH Elk Synth"; }
    void prepareToPlay(double newSampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }

    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock&) override {}
    void setStateInformation(const void*, int) override {}

    // Called from the JUCE editor (keyboard + controls).
    void handleExternalNoteOn(int midiNote, float velocity);
    void handleExternalNoteOff(int midiNote);

    void setOscillatorTypeFromUI(int index);
    void setMasterVolumeFromUI(float value);
    void setEnvelopeAttackFromUI(float value);
    void setEnvelopeDecayFromUI(float value);
    void setEnvelopeSustainFromUI(float value);
    void setEnvelopeReleaseFromUI(float value);
    void setFilterCutoffFromUI(float value);
    void setFilterResonanceFromUI(float value);
    void setDelayMixFromUI(float value);
    void setReverbMixFromUI(float value);
    void setLfo1RateFromUI(float hz);
    void setLfo1FilterDepthFromUI(float amount);
    void setExternalLightNormalized(float value);       // external (poller/file) source
    void setExternalDistanceNormalized(float value);    // external (poller/file) source
    void setManualLightNormalized(float value);         // UI slider source
    void setManualDistanceNormalized(float value);      // UI slider source
    void setLightTargetFromUI(int targetIndex);
    void setDistanceTargetFromUI(int targetIndex);
    void setSensorModeFromUI(int modeIndex);
    void startSensorPoller(std::function<float()> reader, int intervalMs = 20);
    void startSensorPoller(std::function<float()> lightReader,
                           std::function<float()> distanceReader,
                           int intervalMs = 20);
    void stopSensorPoller();

private:
    class SensorPoller : public juce::Thread {
    public:
        using Reader = std::function<float()>;
        SensorPoller(Reader lightReader, Reader distanceReader, ElkSynthProcessor& owner, int intervalMs)
            : juce::Thread("SensorPoller"),
              lightReader_(std::move(lightReader)),
              distanceReader_(std::move(distanceReader)),
              owner_(owner),
              intervalMs_(std::max(5, intervalMs)) {}

        void run() override {
            while (!threadShouldExit()) {
                if (lightReader_) {
                    owner_.setExternalLightNormalized(lightReader_());
                }
                if (distanceReader_) {
                    owner_.setExternalDistanceNormalized(distanceReader_());
                }
                wait(static_cast<int>(intervalMs_));
            }
        }

    private:
        Reader lightReader_;
        Reader distanceReader_;
        ElkSynthProcessor& owner_;
        int intervalMs_;
    };

    double sampleRate_ = 44100.0;
    std::unique_ptr<AIMusicHardware::Synthesizer> synth_;
    std::unique_ptr<AIMusicHardware::Sequencer>   seq_;
    std::unique_ptr<AIMusicHardware::HostSync>    hostSync_;
    std::unique_ptr<AIMusicHardware::ClockSource> clockSource_;
    std::unique_ptr<AIMusicHardware::EffectProcessor> fxProcessor_;
    int delayEffectIndex_ = -1;
    int reverbEffectIndex_ = -1;
    std::atomic<float> manualLightValueNorm_{1.0f};
    std::atomic<float> manualDistanceValueNorm_{0.5f};
    std::atomic<float> externalLightValueNorm_{1.0f};
    std::atomic<float> externalDistanceValueNorm_{0.5f};
    std::atomic<int> lightTarget_{static_cast<int>(SensorTarget::FilterCutoff)};
    std::atomic<int> distanceTarget_{static_cast<int>(SensorTarget::PitchBend)};
    std::atomic<int> sensorMode_{static_cast<int>(SensorMode::Manual)};
    float lastLightApplied_ = -1.0f; // audio-thread only
    float lastDistanceApplied_ = -1.0f; // audio-thread only
    int lastLightTargetApplied_ = -1; // audio-thread only
    int lastDistanceTargetApplied_ = -1; // audio-thread only
    std::unique_ptr<SensorPoller> sensorPoller_;
};
