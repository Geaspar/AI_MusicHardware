#include "../../include/synthesis/RealtimeWavetableVoice.h"
#include "../../include/synthesis/modulators/envelope.h"
#include <cmath>
#include <vector>
#include <complex>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace AIMusicHardware {

RealtimeWavetableVoice::RealtimeWavetableVoice(std::shared_ptr<FrequencyDomainWavetable> wavetable, double sampleRate)
    : Voice(sampleRate), wavetable_(wavetable) {
    fft_ = std::make_unique<FourierTransform>();
    // Initialize the time-domain buffer
    time_domain_wavetable_.resize(2048); 
}

void RealtimeWavetableVoice::process(float* outputBuffer, int numSamples) {
    if (state_ == State::Inactive || state_ == State::Finished) {
        for (int i = 0; i < numSamples * 2; ++i) { // Stereo buffer
            outputBuffer[i] = 0.0f;
        }
        return;
    }

    // 1. Update pitch modulation from base class
    float targetPitch = pitchMod_.calculateTotalPitch();
    pitchMod_.updateSmoothedPitch(targetPitch);
    updateOscillatorFrequency();

    // 2. Generate the wavetable for this block (IFFT)
    const float nyquist = getSampleRate() / 2.0f;
    const auto& harmonic_data = wavetable_->getHarmonicData(current_frame_);
    
    std::vector<std::complex<float>> spectrum(2048, {0.0f, 0.0f}); // Fixed size for IFFT
    for (size_t i = 0; i < harmonic_data.size(); ++i) {
        float harmonic_frequency = frequency_ * (i + 1);
        if (harmonic_frequency < nyquist) {
            spectrum[i] = harmonic_data[i];
        } else {
            break; 
        }
    }

    fft_->performIFFT(spectrum);
    for(size_t i = 0; i < spectrum.size(); ++i) {
        time_domain_wavetable_[i] = spectrum[i].real();
    }

    // 3. Process samples using the generated wavetable
    for (int i = 0; i < numSamples; ++i) {
        double phase_increment = frequency_ / getSampleRate();
        phase_ += phase_increment;
        if (phase_ >= 1.0) {
            phase_ -= 1.0;
        }
        
        double read_pos = phase_ * time_domain_wavetable_.size();
        int index1 = static_cast<int>(read_pos);
        int index2 = (index1 + 1) % time_domain_wavetable_.size();
        float frac = read_pos - index1;
        
        float sample1 = time_domain_wavetable_[index1];
        float sample2 = time_domain_wavetable_[index2];
        float sample = sample1 + frac * (sample2 - sample1);

        float envValue = envelope_->generateValue();
        sample *= envValue * velocity_;

        outputBuffer[i * 2] += sample;
        outputBuffer[i * 2 + 1] += sample;
    }
    
    if (state_ == State::Released && !envelope_->isActive()) {
        state_ = State::Finished;
    }
}

// --- RealtimeWavetableVoiceManager Implementation ---

RealtimeWavetableVoiceManager::RealtimeWavetableVoiceManager(int sampleRate, int maxVoices)
    : VoiceManager(sampleRate, maxVoices) {}

RealtimeWavetableVoiceManager::~RealtimeWavetableVoiceManager() {}

void RealtimeWavetableVoiceManager::setWavetable(std::shared_ptr<FrequencyDomainWavetable> wavetable) {
    currentWavetable_ = wavetable;
}

std::unique_ptr<Voice> RealtimeWavetableVoiceManager::createVoice() {
    if (!currentWavetable_) {
        // Handle error: wavetable not set
        return nullptr;
    }
    return std::make_unique<RealtimeWavetableVoice>(currentWavetable_, getSampleRate());
}

} // namespace AIMusicHardware
