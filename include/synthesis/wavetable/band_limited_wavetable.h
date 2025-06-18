#pragma once

#include <memory>
#include <vector>
#include <complex>
#include <cmath>

namespace AIMusicHardware {

/**
 * Band-limited wavetable implementation inspired by Vital's approach.
 * This creates multiple versions of each waveform with different numbers
 * of harmonics to prevent aliasing at different frequencies.
 */
class BandLimitedWavetable {
public:
    static constexpr int kWaveformSize = 2048;  // Size of each waveform
    static constexpr int kNumBands = 11;        // Number of frequency bands (like Vital)
    static constexpr int kNumHarmonics = kWaveformSize / 2;
    static constexpr float kDefaultSampleRate = 44100.0f;
    
    // Waveform types
    enum class WaveType {
        Sine,
        Saw,
        Square,
        Triangle,
        Count
    };
    
    /**
     * Single band-limited waveform for a specific frequency range
     */
    struct WaveformBand {
        std::vector<float> samples;  // Time-domain samples
        float minFrequency;          // Minimum frequency for this band (Hz)
        float maxFrequency;          // Maximum frequency for this band (Hz)
        int maxHarmonic;             // Maximum harmonic included in this band
    };
    
    BandLimitedWavetable(float sampleRate = kDefaultSampleRate);
    ~BandLimitedWavetable();
    
    /**
     * Initialize wavetable with a specific waveform type
     */
    void initWaveform(WaveType type);
    
    /**
     * Get a sample from the wavetable
     * @param phase Phase position (0.0 - 1.0)
     * @param frequency Current oscillator frequency in Hz
     * @return Interpolated sample value
     */
    float getSample(float phase, float frequency) const;
    
    /**
     * Set sample rate (rebuilds wavetables if changed)
     */
    void setSampleRate(float sampleRate);
    
    /**
     * Get the appropriate band index for a given frequency
     */
    int getBandIndex(float frequency) const;
    
    /**
     * Enable/disable interpolation between bands
     */
    void setBandInterpolation(bool enable) { bandInterpolation_ = enable; }
    
private:
    /**
     * Generate band-limited waveforms using additive synthesis
     */
    void generateBandLimitedWaveforms(WaveType type);
    
    /**
     * Calculate the maximum harmonic that won't alias at a given frequency
     */
    int calculateMaxHarmonic(float frequency) const;
    
    /**
     * Generate a single band-limited waveform
     */
    void generateWaveformBand(WaveformBand& band, WaveType type, int maxHarmonic);
    
    /**
     * Additive synthesis functions for each waveform type
     */
    void generateSineBand(WaveformBand& band, int maxHarmonic);
    void generateSawBand(WaveformBand& band, int maxHarmonic);
    void generateSquareBand(WaveformBand& band, int maxHarmonic);
    void generateTriangleBand(WaveformBand& band, int maxHarmonic);
    
    /**
     * Apply FFT-based band limiting (alternative approach)
     */
    void applyFFTBandLimit(std::vector<float>& samples, int maxHarmonic);
    
    /**
     * Linear interpolation between samples
     */
    float interpolateSample(const std::vector<float>& samples, float phase) const;
    
    // Member variables
    std::vector<WaveformBand> bands_;
    float sampleRate_;
    WaveType currentType_;
    bool bandInterpolation_;
    
    // FFT workspace (if using FFT approach)
    std::vector<std::complex<float>> fftBuffer_;
};

/**
 * Oversampling processor for additional anti-aliasing
 */
class OversamplingProcessor {
public:
    enum class Factor {
        x1 = 1,
        x2 = 2,
        x4 = 4,
        x8 = 8
    };
    
    OversamplingProcessor(Factor factor = Factor::x2);
    ~OversamplingProcessor();
    
    /**
     * Process a block of samples with oversampling
     * @param input Input samples at base sample rate
     * @param output Output samples at base sample rate
     * @param numSamples Number of samples to process
     * @param generator Function that generates oversampled samples
     */
    void process(const float* input, float* output, int numSamples,
                 std::function<float()> generator);
    
    /**
     * Get the oversampling factor
     */
    int getFactor() const { return static_cast<int>(factor_); }
    
private:
    /**
     * Upsample input (insert zeros and filter)
     */
    void upsample(const float* input, float* output, int numSamples);
    
    /**
     * Downsample output (filter and decimate)
     */
    void downsample(const float* input, float* output, int numSamples);
    
    /**
     * Design anti-aliasing filter coefficients
     */
    void designFilter();
    
    Factor factor_;
    std::vector<float> upsampleBuffer_;
    std::vector<float> downsampleBuffer_;
    std::vector<float> filterCoeffs_;
    std::vector<float> filterState_;
};

} // namespace AIMusicHardware