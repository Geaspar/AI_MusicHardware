#pragma once

#include "AdvancedFilter.h"
#include <array>

namespace AIMusicHardware {

/**
 * @brief Formant Filter model
 * 
 * Implements a vowel formant filter that can simulate vocal vowel sounds
 * by implementing parallel band-pass filters at specific formant frequencies.
 */
class FormantFilterModel : public FilterModel {
public:
    enum class Vowel {
        A,    // 'ah' as in 'father'
        E,    // 'eh' as in 'bed'
        I,    // 'ee' as in 'see'
        O,    // 'oh' as in 'go'
        U     // 'oo' as in 'boot'
    };
    
    static constexpr int kNumFormants = 3;  // Number of formant bands
    
    FormantFilterModel(int sampleRate = 44100);
    ~FormantFilterModel() override;
    
    void process(float* buffer, int numFrames, int channels) override;
    void setParameter(const std::string& name, float value) override;
    void setSampleRate(int sampleRate) override;
    
    std::string getTypeName() const override;
    
private:
    // Updates internal calculations
    void updateCoefficients();
    
    // Set current vowel
    void setVowel(Vowel vowel);
    
    // Interpolate between vowels
    void setVowelMorph(float position);
    
    // Initialize formant tables
    void initFormantTables();
    
    // Biquad filter implementation for a single formant
    class FormantBand {
    public:
        FormantBand();
        
        // Initialize the band
        void init(float sampleRate, float frequency, float bandwidth, float gain);
        
        // Process a single sample
        float process(float input, int channel);
        
    private:
        // Calculate coefficients
        void updateCoefficients();
        
        // Biquad parameters
        float sampleRate_;
        float frequency_;
        float bandwidth_;
        float gain_;
        
        // Biquad coefficients
        float a0_, a1_, a2_, b0_, b1_, b2_;
        
        // Filter state (per channel)
        float x1_[2], x2_[2];
        float y1_[2], y2_[2];
    };
    
    // Current vowel formant data
    struct FormantData {
        float frequency;
        float bandwidth;
        float gain;
    };
    
    // Table of formant frequencies, bandwidths, and gains for each vowel
    std::array<std::array<FormantData, kNumFormants>, 5> formantTable_;
    
    // Active formant bands (one per formant)
    std::array<FormantBand, kNumFormants> formantBands_;
    
    // Currently active vowel
    Vowel currentVowel_;
    
    // Interpolation position between vowels (0.0-4.0)
    float morphPosition_;
    
    // Current formant data
    std::array<FormantData, kNumFormants> currentFormants_;
    
    // Gender factor (0 = masculine, 1 = feminine)
    float gender_;
};

} // namespace AIMusicHardware