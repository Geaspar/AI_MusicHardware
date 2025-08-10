#pragma once

#include "synthesis/voice/voice.h"
#include "synthesis/voice/voice_manager.h"
#include "synthesis/FrequencyDomainWavetable.h"
#include "audio/FourierTransform.h"
#include <vector>
#include <complex>

namespace AIMusicHardware {

class ModEnvelope;

class RealtimeWavetableVoice : public Voice {
public:
    RealtimeWavetableVoice(std::shared_ptr<FrequencyDomainWavetable> wavetable, double sampleRate);

    void process(float* outputBuffer, int numSamples) override;

    bool isPlaying() const { return state_ != State::Inactive && state_ != State::Finished; }

private:
    std::shared_ptr<FrequencyDomainWavetable> wavetable_;
    std::unique_ptr<FourierTransform> fft_;
    int current_frame_ = 0;
    double phase_ = 0.0;
    std::vector<float> time_domain_wavetable_;
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