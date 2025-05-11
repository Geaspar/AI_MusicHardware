#include "../../include/effects/FormantFilter.h"
#include "../../include/effects/EffectUtils.h"
#include <cmath>
#include <algorithm>

namespace AIMusicHardware {

//=============================================================================
// FormantBand Implementation
//=============================================================================

FormantFilterModel::FormantBand::FormantBand()
    : sampleRate_(44100.0f), frequency_(1000.0f), bandwidth_(100.0f), gain_(1.0f) {
    
    // Initialize filter state
    for (int ch = 0; ch < 2; ++ch) {
        x1_[ch] = x2_[ch] = y1_[ch] = y2_[ch] = 0.0f;
    }
    
    updateCoefficients();
}

void FormantFilterModel::FormantBand::init(float sampleRate, float frequency, float bandwidth, float gain) {
    sampleRate_ = sampleRate;
    frequency_ = frequency;
    bandwidth_ = bandwidth;
    gain_ = gain;
    
    updateCoefficients();
}

float FormantFilterModel::FormantBand::process(float input, int channel) {
    // Check channel range
    channel = std::min(1, std::max(0, channel));
    
    // Process input through biquad filter
    float output = b0_ * input + b1_ * x1_[channel] + b2_ * x2_[channel] - a1_ * y1_[channel] - a2_ * y2_[channel];
    
    // Update filter state
    x2_[channel] = x1_[channel];
    x1_[channel] = input;
    y2_[channel] = y1_[channel];
    y1_[channel] = output;
    
    return output * gain_;
}

void FormantFilterModel::FormantBand::updateCoefficients() {
    // Bandpass filter with Q derived from bandwidth
    float omega = 2.0f * PI * frequency_ / sampleRate_;
    float sinOmega = std::sin(omega);
    float cosOmega = std::cos(omega);
    
    // Calculate Q from bandwidth
    float Q = frequency_ / bandwidth_;
    float alpha = sinOmega / (2.0f * Q);
    
    // Calculate biquad coefficients for bandpass
    b0_ = alpha;
    b1_ = 0.0f;
    b2_ = -alpha;
    a0_ = 1.0f + alpha;
    a1_ = -2.0f * cosOmega;
    a2_ = 1.0f - alpha;
    
    // Normalize coefficients
    b0_ /= a0_;
    b1_ /= a0_;
    b2_ /= a0_;
    a1_ /= a0_;
    a2_ /= a0_;
}

//=============================================================================
// FormantFilterModel Implementation
//=============================================================================

FormantFilterModel::FormantFilterModel(int sampleRate)
    : FilterModel(sampleRate), currentVowel_(Vowel::A), morphPosition_(0.0f), gender_(0.5f) {
    
    // Initialize default parameters
    parameters_["vowel"] = 0.0f;        // Vowel selection (0-4)
    parameters_["morph"] = 0.0f;        // Morph position (0-1)
    parameters_["gender"] = 0.5f;       // Gender factor (0-1)
    parameters_["resonance"] = 0.7f;    // Resonance factor (0-1)
    
    // Initialize the formant tables
    initFormantTables();
    
    // Set initial vowel
    setVowel(Vowel::A);
    
    // Initialize formant bands
    updateCoefficients();
}

FormantFilterModel::~FormantFilterModel() {
}

void FormantFilterModel::setSampleRate(int sampleRate) {
    FilterModel::setSampleRate(sampleRate);
    updateCoefficients();
}

std::string FormantFilterModel::getTypeName() const {
    return "Formant Filter";
}

void FormantFilterModel::initFormantTables() {
    // Initialize formant tables for each vowel
    // These values are typical for an adult male voice
    // Frequencies in Hz, bandwidth in Hz, gain is relative (0-1)
    
    // 'A' (ah as in father)
    formantTable_[static_cast<int>(Vowel::A)][0] = {800.0f, 80.0f, 1.0f};
    formantTable_[static_cast<int>(Vowel::A)][1] = {1150.0f, 90.0f, 0.5f};
    formantTable_[static_cast<int>(Vowel::A)][2] = {2900.0f, 120.0f, 0.3f};
    
    // 'E' (eh as in bed)
    formantTable_[static_cast<int>(Vowel::E)][0] = {600.0f, 60.0f, 1.0f};
    formantTable_[static_cast<int>(Vowel::E)][1] = {1700.0f, 90.0f, 0.5f};
    formantTable_[static_cast<int>(Vowel::E)][2] = {2600.0f, 100.0f, 0.3f};
    
    // 'I' (ee as in see)
    formantTable_[static_cast<int>(Vowel::I)][0] = {250.0f, 60.0f, 1.0f};
    formantTable_[static_cast<int>(Vowel::I)][1] = {1900.0f, 90.0f, 0.5f};
    formantTable_[static_cast<int>(Vowel::I)][2] = {2800.0f, 100.0f, 0.3f};
    
    // 'O' (oh as in go)
    formantTable_[static_cast<int>(Vowel::O)][0] = {400.0f, 40.0f, 1.0f};
    formantTable_[static_cast<int>(Vowel::O)][1] = {800.0f, 80.0f, 0.5f};
    formantTable_[static_cast<int>(Vowel::O)][2] = {2600.0f, 100.0f, 0.3f};
    
    // 'U' (oo as in boot)
    formantTable_[static_cast<int>(Vowel::U)][0] = {350.0f, 40.0f, 1.0f};
    formantTable_[static_cast<int>(Vowel::U)][1] = {600.0f, 80.0f, 0.5f};
    formantTable_[static_cast<int>(Vowel::U)][2] = {2700.0f, 100.0f, 0.3f};
}

void FormantFilterModel::setVowel(Vowel vowel) {
    currentVowel_ = vowel;
    
    // Copy the formant data for the selected vowel
    for (int i = 0; i < kNumFormants; ++i) {
        currentFormants_[i] = formantTable_[static_cast<int>(vowel)][i];
    }
}

void FormantFilterModel::setVowelMorph(float position) {
    // Constrain position to the valid range (0-4)
    position = std::clamp(position, 0.0f, 4.0f);
    morphPosition_ = position;
    
    // Determine which vowels we're morphing between
    int vowel1 = static_cast<int>(position);
    int vowel2 = (vowel1 + 1) % 5;  // Wrap around to 0
    
    // Calculate blend factor (0.0-1.0)
    float blend = position - vowel1;
    
    // Linear interpolation between vowel formants
    for (int i = 0; i < kNumFormants; ++i) {
        FormantData& f1 = formantTable_[vowel1][i];
        FormantData& f2 = formantTable_[vowel2][i];
        
        currentFormants_[i].frequency = f1.frequency * (1.0f - blend) + f2.frequency * blend;
        currentFormants_[i].bandwidth = f1.bandwidth * (1.0f - blend) + f2.bandwidth * blend;
        currentFormants_[i].gain = f1.gain * (1.0f - blend) + f2.gain * blend;
    }
}

void FormantFilterModel::updateCoefficients() {
    // Apply gender factor to formant frequencies
    // Female formants are typically higher in frequency
    float genderFactor = 1.0f + gender_ * 0.5f;  // 1.0 for male, 1.5 for female
    
    // Apply resonance factor to bandwidths and gains
    float resonance = parameters_["resonance"];
    float bandwidthFactor = 1.0f - resonance * 0.7f;  // Higher resonance = narrower bandwidth
    float gainFactor = 1.0f + resonance * 1.0f;       // Higher resonance = higher gain
    
    // Configure each formant band
    for (int i = 0; i < kNumFormants; ++i) {
        float freq = currentFormants_[i].frequency * genderFactor;
        float bandwidth = currentFormants_[i].bandwidth * bandwidthFactor;
        float gain = currentFormants_[i].gain * gainFactor;
        
        formantBands_[i].init(sampleRate_, freq, bandwidth, gain);
    }
}

void FormantFilterModel::process(float* buffer, int numFrames, int channels) {
    // Check if parameters have changed
    float vowel = parameters_["vowel"];
    float morph = parameters_["morph"];
    float gender = parameters_["gender"];
    float resonance = parameters_["resonance"];
    
    static float lastVowel = -1.0f;
    static float lastMorph = -1.0f;
    static float lastGender = -1.0f;
    static float lastResonance = -1.0f;
    
    bool updateNeeded = false;
    
    // Check if vowel selection has changed
    if (lastVowel != vowel && morph < 0.01f) {
        int vowelIndex = static_cast<int>(vowel * 4.9f); // Scale 0-1 to 0-4 (with safety margin)
        setVowel(static_cast<Vowel>(vowelIndex));
        updateNeeded = true;
        lastVowel = vowel;
    }
    
    // Check if morph has changed
    if (lastMorph != morph) {
        if (morph > 0.01f) {
            // Morphing mode
            setVowelMorph(vowel * 4.0f); // Scale 0-1 to 0-4 for position
        }
        updateNeeded = true;
        lastMorph = morph;
    }
    
    // Check if gender factor has changed
    if (lastGender != gender) {
        gender_ = gender;
        updateNeeded = true;
        lastGender = gender;
    }
    
    // Check if resonance has changed
    if (lastResonance != resonance) {
        updateNeeded = true;
        lastResonance = resonance;
    }
    
    // Update coefficients if needed
    if (updateNeeded) {
        updateCoefficients();
    }
    
    // Process audio through formant bands
    for (int i = 0; i < numFrames * channels; i += channels) {
        for (int ch = 0; ch < std::min(channels, 2); ++ch) {
            float input = buffer[i + ch];
            float output = 0.0f;
            
            // Process through each formant band and sum
            for (int band = 0; band < kNumFormants; ++band) {
                output += formantBands_[band].process(input, ch);
            }
            
            // Scale the output (prevent clipping)
            output *= 0.33f;  // Divide by roughly the number of bands
            
            // Write to buffer
            buffer[i + ch] = output;
        }
    }
}

void FormantFilterModel::setParameter(const std::string& name, float value) {
    bool recalculate = false;
    
    if (name == "vowel") {
        parameters_["vowel"] = std::clamp(value, 0.0f, 1.0f);
        recalculate = true;
    }
    else if (name == "morph") {
        parameters_["morph"] = std::clamp(value, 0.0f, 1.0f);
        recalculate = true;
    }
    else if (name == "gender") {
        parameters_["gender"] = std::clamp(value, 0.0f, 1.0f);
        recalculate = true;
    }
    else if (name == "resonance") {
        parameters_["resonance"] = std::clamp(value, 0.0f, 1.0f);
        recalculate = true;
    }
    else {
        // For any other parameters, use base implementation
        FilterModel::setParameter(name, value);
    }
    
    if (recalculate) {
        updateCoefficients();
    }
}

} // namespace AIMusicHardware