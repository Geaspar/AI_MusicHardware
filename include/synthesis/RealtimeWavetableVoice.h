#pragma once

#include "synthesis/voice/voice_manager.h"
#include "synthesis/FrequencyDomainWavetable.h"
#include "audio/FourierTransform.h"

namespace AIMusicHardware {

class RealtimeWavetableVoice : public Voice {
public:
    RealtimeWavetableVoice(std::shared_ptr<FrequencyDomainWavetable> wavetable, double sampleRate);

    void process(float* outputBuffer, int numSamples);

    void noteOn(int noteNumber, float velocity);
    void noteOff();

    bool isPlaying() const { return is_playing_; }

private:
    std::shared_ptr<FrequencyDomainWavetable> wavetable_;
    std::unique_ptr<FourierTransform> fft_;
    double sample_rate_;
    float frequency_;
    float amplitude_;
    bool is_playing_ = false;
    int current_frame_ = 0;
    double phase_ = 0.0;
};

class RealtimeWavetableVoiceManager : public VoiceManager {
public:
    RealtimeWavetableVoiceManager(int sampleRate = 44100, int maxVoices = 16);
    ~RealtimeWavetableVoiceManager();

    void setWavetable(std::shared_ptr<FrequencyDomainWavetable> wavetable);

protected:
    std::unique_ptr<Voice> createVoice() override;

private:
    std::shared_ptr<FrequencyDomainWavetable> currentWavetable_;
};

} // namespace AIMusicHardware
