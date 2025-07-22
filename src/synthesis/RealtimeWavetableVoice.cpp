#include "../../include/synthesis/RealtimeWavetableVoice.h"
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace AIMusicHardware {

RealtimeWavetableVoice::RealtimeWavetableVoice(std::shared_ptr<FrequencyDomainWavetable> wavetable, double sampleRate)
    : Voice(sampleRate), wavetable_(wavetable), sample_rate_(sampleRate), frequency_(0.0f), amplitude_(0.0f) {
    fft_ = std::make_unique<FourierTransform>();
}

void RealtimeWavetableVoice::process(float* outputBuffer, int numSamples) {
    if (!is_playing_) {
        for (int i = 0; i < numSamples; ++i) {
            outputBuffer[i] = 0.0f;
        }
        return;
    }

    const float nyquist = sample_rate_ / 2.0f;
    const auto& harmonic_data = wavetable_->getHarmonicData(current_frame_);

    std::vector<std::complex<float>> spectrum(harmonic_data.size());
    for (size_t i = 0; i < harmonic_data.size(); ++i) {
        float harmonic_frequency = frequency_ * (i + 1);
        if (harmonic_frequency < nyquist) {
            spectrum[i] = harmonic_data[i];
        } else {
            spectrum[i] = {0.0f, 0.0f};
        }
    }

    std::vector<std::complex<float>> time_domain_data(2048);
    fft_->performIFFT(spectrum);

    // This is a simplified version. A real implementation would need to handle
    // the phase correctly and interpolate between samples.
    for (int i = 0; i < numSamples; ++i) {
        double phase_increment = frequency_ / sample_rate_;
        phase_ += phase_increment;
        if (phase_ >= 1.0) {
            phase_ -= 1.0;
        }
        int sample_index = static_cast<int>(phase_ * 2048);
        outputBuffer[i] = spectrum[sample_index].real() * amplitude_;
    }
}

void RealtimeWavetableVoice::noteOn(int noteNumber, float velocity) {
    frequency_ = 440.0 * std::pow(2.0, (noteNumber - 69.0) / 12.0);
    amplitude_ = velocity;
    is_playing_ = true;
    phase_ = 0.0;
}

void RealtimeWavetableVoice::noteOff() {
    is_playing_ = false;
}

// --- RealtimeWavetableVoiceManager Implementation ---

RealtimeWavetableVoiceManager::RealtimeWavetableVoiceManager(int sampleRate, int maxVoices)
    : VoiceManager(sampleRate, maxVoices) {}

RealtimeWavetableVoiceManager::~RealtimeWavetableVoiceManager() {}

void RealtimeWavetableVoiceManager::setWavetable(std::shared_ptr<FrequencyDomainWavetable> wavetable) {
    currentWavetable_ = wavetable;
}

std::unique_ptr<Voice> RealtimeWavetableVoiceManager::createVoice() {
    return std::make_unique<RealtimeWavetableVoice>(currentWavetable_, sampleRate_);
}

} // namespace AIMusicHardware