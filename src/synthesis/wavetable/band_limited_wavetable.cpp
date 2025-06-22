#include "../../../include/synthesis/wavetable/band_limited_wavetable.h"
#include <algorithm>
#include <cstring>
#include <iostream>

namespace AIMusicHardware {

constexpr float PI = 3.14159265358979323846f;
constexpr float TWO_PI = 2.0f * PI;

BandLimitedWavetable::BandLimitedWavetable(float sampleRate)
    : sampleRate_(sampleRate),
      currentType_(WaveType::Sine),
      bandInterpolation_(true) {
    
    // Initialize bands with frequency ranges similar to Vital
    bands_.resize(kNumBands);
    
    // Set up frequency ranges for each band
    // Each band covers roughly one octave, with more bands at higher frequencies
    float baseFreq = 20.0f;  // Start at 20 Hz
    
    for (int i = 0; i < kNumBands; ++i) {
        bands_[i].samples.resize(kWaveformSize);
        bands_[i].minFrequency = baseFreq;
        bands_[i].maxFrequency = baseFreq * 2.0f;
        
        // Calculate max harmonic for this band's highest frequency
        bands_[i].maxHarmonic = calculateMaxHarmonic(bands_[i].maxFrequency);
        
        baseFreq *= 2.0f;  // Each band is one octave higher
    }
    
    // Initialize FFT buffer
    fftBuffer_.resize(kWaveformSize);
}

BandLimitedWavetable::~BandLimitedWavetable() {
}

void BandLimitedWavetable::initWaveform(WaveType type) {
    currentType_ = type;
    generateBandLimitedWaveforms(type);
}

float BandLimitedWavetable::getSample(float phase, float frequency) const {
    if (bands_.empty()) {
        return 0.0f;
    }
    
    // Ensure phase is in range [0, 1)
    phase = phase - std::floor(phase);
    
    // Find the appropriate band for this frequency
    int bandIndex = getBandIndex(frequency);
    
    
    if (!bandInterpolation_ || bandIndex == 0 || bandIndex >= kNumBands - 1) {
        // No interpolation between bands
        return interpolateSample(bands_[bandIndex].samples, phase);
    } else {
        // Interpolate between adjacent bands for smooth transitions
        const auto& band1 = bands_[bandIndex];
        const auto& band2 = bands_[bandIndex + 1];
        
        // Calculate interpolation factor
        float t = (frequency - band1.minFrequency) / 
                  (band1.maxFrequency - band1.minFrequency);
        t = std::clamp(t, 0.0f, 1.0f);
        
        // Get samples from both bands
        float sample1 = interpolateSample(band1.samples, phase);
        float sample2 = interpolateSample(band2.samples, phase);
        
        // Linear interpolation between bands
        return sample1 * (1.0f - t) + sample2 * t;
    }
}

void BandLimitedWavetable::setSampleRate(float sampleRate) {
    if (std::abs(sampleRate_ - sampleRate) > 0.01f) {
        sampleRate_ = sampleRate;
        // Recalculate max harmonics for each band
        for (int i = 0; i < kNumBands; ++i) {
            bands_[i].maxHarmonic = calculateMaxHarmonic(bands_[i].maxFrequency);
        }
        // Regenerate waveforms with new band limits
        generateBandLimitedWaveforms(currentType_);
    }
}

int BandLimitedWavetable::getBandIndex(float frequency) const {
    // Binary search for the appropriate band
    int low = 0;
    int high = kNumBands - 1;
    
    while (low < high) {
        int mid = (low + high) / 2;
        if (frequency > bands_[mid].maxFrequency) {
            low = mid + 1;
        } else {
            high = mid;
        }
    }
    
    return std::clamp(low, 0, kNumBands - 1);
}

void BandLimitedWavetable::generateBandLimitedWaveforms(WaveType type) {
    for (auto& band : bands_) {
        generateWaveformBand(band, type, band.maxHarmonic);
    }
}

int BandLimitedWavetable::calculateMaxHarmonic(float frequency) const {
    // Nyquist frequency
    float nyquist = sampleRate_ * 0.5f;
    
    // Maximum harmonic that won't alias
    int maxHarmonic = static_cast<int>(nyquist / frequency);
    
    // Leave some headroom to avoid frequencies too close to Nyquist
    maxHarmonic = static_cast<int>(maxHarmonic * 0.95f);
    
    // Clamp to reasonable range
    return std::clamp(maxHarmonic, 1, kNumHarmonics);
}

void BandLimitedWavetable::generateWaveformBand(WaveformBand& band, WaveType type, int maxHarmonic) {
    switch (type) {
        case WaveType::Sine:
            generateSineBand(band, maxHarmonic);
            break;
        case WaveType::Saw:
            generateSawBand(band, maxHarmonic);
            break;
        case WaveType::Square:
            generateSquareBand(band, maxHarmonic);
            break;
        case WaveType::Triangle:
            generateTriangleBand(band, maxHarmonic);
            break;
        default:
            // Clear to silence
            std::fill(band.samples.begin(), band.samples.end(), 0.0f);
            break;
    }
}

void BandLimitedWavetable::generateSineBand(WaveformBand& band, int maxHarmonic) {
    // Sine wave has only the fundamental
    for (int i = 0; i < kWaveformSize; ++i) {
        float phase = static_cast<float>(i) / kWaveformSize;
        band.samples[i] = std::sin(TWO_PI * phase);
    }
    
}

void BandLimitedWavetable::generateSawBand(WaveformBand& band, int maxHarmonic) {
    // Sawtooth wave: sum of harmonics with amplitude 1/n
    std::fill(band.samples.begin(), band.samples.end(), 0.0f);
    
    for (int harmonic = 1; harmonic <= maxHarmonic; ++harmonic) {
        float amplitude = 1.0f / harmonic;
        
        for (int i = 0; i < kWaveformSize; ++i) {
            float phase = static_cast<float>(i) / kWaveformSize;
            band.samples[i] += amplitude * std::sin(TWO_PI * harmonic * phase);
        }
    }
    
    // Normalize
    float maxVal = 0.0f;
    for (float sample : band.samples) {
        maxVal = std::max(maxVal, std::abs(sample));
    }
    
    if (maxVal > 0.0f) {
        float scale = 1.0f / maxVal;
        for (float& sample : band.samples) {
            sample *= scale;
        }
    }
}

void BandLimitedWavetable::generateSquareBand(WaveformBand& band, int maxHarmonic) {
    // Square wave: sum of odd harmonics with amplitude 1/n
    std::fill(band.samples.begin(), band.samples.end(), 0.0f);
    
    for (int harmonic = 1; harmonic <= maxHarmonic; harmonic += 2) {
        float amplitude = 1.0f / harmonic;
        
        for (int i = 0; i < kWaveformSize; ++i) {
            float phase = static_cast<float>(i) / kWaveformSize;
            band.samples[i] += amplitude * std::sin(TWO_PI * harmonic * phase);
        }
    }
    
    // Normalize
    float maxVal = 0.0f;
    for (float sample : band.samples) {
        maxVal = std::max(maxVal, std::abs(sample));
    }
    
    if (maxVal > 0.0f) {
        float scale = 1.0f / maxVal;
        for (float& sample : band.samples) {
            sample *= scale;
        }
    }
}

void BandLimitedWavetable::generateTriangleBand(WaveformBand& band, int maxHarmonic) {
    // Triangle wave: sum of odd harmonics with amplitude 1/n²
    std::fill(band.samples.begin(), band.samples.end(), 0.0f);
    
    for (int harmonic = 1; harmonic <= maxHarmonic; harmonic += 2) {
        float amplitude = 1.0f / (harmonic * harmonic);
        // Alternate sign for each odd harmonic
        if (((harmonic - 1) / 2) % 2 == 1) {
            amplitude = -amplitude;
        }
        
        for (int i = 0; i < kWaveformSize; ++i) {
            float phase = static_cast<float>(i) / kWaveformSize;
            band.samples[i] += amplitude * std::sin(TWO_PI * harmonic * phase);
        }
    }
    
    // Normalize
    float maxVal = 0.0f;
    for (float sample : band.samples) {
        maxVal = std::max(maxVal, std::abs(sample));
    }
    
    if (maxVal > 0.0f) {
        float scale = 1.0f / maxVal;
        for (float& sample : band.samples) {
            sample *= scale;
        }
    }
}

float BandLimitedWavetable::interpolateSample(const std::vector<float>& samples, float phase) const {
    // Convert phase to sample index
    float indexFloat = phase * samples.size();
    int index1 = static_cast<int>(indexFloat) % samples.size();
    int index2 = (index1 + 1) % samples.size();
    
    // Fractional part for interpolation
    float frac = indexFloat - std::floor(indexFloat);
    
    // Linear interpolation
    return samples[index1] * (1.0f - frac) + samples[index2] * frac;
}

// OversamplingProcessor implementation

OversamplingProcessor::OversamplingProcessor(Factor factor)
    : factor_(factor) {
    designFilter();
}

OversamplingProcessor::~OversamplingProcessor() {
}

void OversamplingProcessor::process(const float* input, float* output, int numSamples,
                                    std::function<float()> generator) {
    int oversampleFactor = static_cast<int>(factor_);
    
    if (oversampleFactor == 1) {
        // No oversampling, just generate samples directly
        for (int i = 0; i < numSamples; ++i) {
            output[i] = generator();
        }
        return;
    }
    
    // Ensure buffers are large enough
    int oversampledSize = numSamples * oversampleFactor;
    if (upsampleBuffer_.size() < oversampledSize) {
        upsampleBuffer_.resize(oversampledSize);
        downsampleBuffer_.resize(oversampledSize);
    }
    
    // Generate oversampled data
    for (int i = 0; i < oversampledSize; ++i) {
        upsampleBuffer_[i] = generator();
    }
    
    // Apply anti-aliasing filter and downsample
    downsample(upsampleBuffer_.data(), output, numSamples);
}

void OversamplingProcessor::downsample(const float* input, float* output, int numSamples) {
    int factor = static_cast<int>(factor_);
    
    // Simple averaging downsampler (can be improved with proper filter)
    for (int i = 0; i < numSamples; ++i) {
        float sum = 0.0f;
        for (int j = 0; j < factor; ++j) {
            sum += input[i * factor + j];
        }
        output[i] = sum / factor;
    }
}

void OversamplingProcessor::designFilter() {
    // For now, using simple averaging
    // TODO: Implement proper Butterworth or Chebyshev filter
}

} // namespace AIMusicHardware