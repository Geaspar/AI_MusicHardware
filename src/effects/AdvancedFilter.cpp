#include "../../include/effects/AdvancedFilter.h"
#include "../../include/effects/EffectUtils.h"
#include "../../include/effects/LadderFilter.h"
#include "../../include/effects/CombFilter.h"
#include "../../include/effects/FormantFilter.h"
#include <cmath>
#include <algorithm>

namespace AIMusicHardware {

//=============================================================================
// FilterModel Implementation
//=============================================================================

FilterModel::FilterModel(int sampleRate)
    : sampleRate_(sampleRate) {
}

FilterModel::~FilterModel() {
}

void FilterModel::setParameter(const std::string& name, float value) {
    parameters_[name] = value;
}

float FilterModel::getParameter(const std::string& name) const {
    auto it = parameters_.find(name);
    if (it != parameters_.end()) {
        return it->second;
    }
    return 0.0f;
}

void FilterModel::setSampleRate(int sampleRate) {
    sampleRate_ = sampleRate;
}

//=============================================================================
// BiquadFilterModel Implementation
//=============================================================================

class BiquadFilterModel : public FilterModel {
public:
    enum class Type {
        LowPass,
        HighPass,
        BandPass,
        Notch
    };
    
    BiquadFilterModel(int sampleRate = 44100, Type type = Type::LowPass);
    ~BiquadFilterModel() override;
    
    void process(float* buffer, int numFrames, int channels) override;
    void setParameter(const std::string& name, float value) override;
    void setSampleRate(int sampleRate) override;
    
    std::string getTypeName() const override;
    
private:
    void calculateCoefficients();
    
    Type type_;
    
    // Biquad filter coefficients
    float a0_, a1_, a2_, b0_, b1_, b2_;
    
    // Filter state variables (for stereo processing)
    float x1_[2], x2_[2]; // Previous input samples
    float y1_[2], y2_[2]; // Previous output samples
};

BiquadFilterModel::BiquadFilterModel(int sampleRate, Type type)
    : FilterModel(sampleRate), type_(type) {
    
    // Initialize default parameters
    parameters_["frequency"] = 1000.0f;
    parameters_["resonance"] = 0.707f;  // Butterworth default
    parameters_["gain"] = 0.0f;
    
    // Initialize filter state
    for (int i = 0; i < 2; ++i) {
        x1_[i] = x2_[i] = y1_[i] = y2_[i] = 0.0f;
    }
    
    // Calculate initial coefficients
    calculateCoefficients();
}

BiquadFilterModel::~BiquadFilterModel() {
}

void BiquadFilterModel::setSampleRate(int sampleRate) {
    FilterModel::setSampleRate(sampleRate);
    calculateCoefficients();
}

std::string BiquadFilterModel::getTypeName() const {
    switch (type_) {
        case Type::LowPass: return "Biquad Low Pass";
        case Type::HighPass: return "Biquad High Pass";
        case Type::BandPass: return "Biquad Band Pass";
        case Type::Notch: return "Biquad Notch";
        default: return "Biquad Filter";
    }
}

void BiquadFilterModel::process(float* buffer, int numFrames, int channels) {
    float frequency = parameters_["frequency"];
    float resonance = parameters_["resonance"];
    
    // Check if parameters changed and recalculate if needed
    static float lastFreq = 0.0f;
    static float lastRes = 0.0f;
    
    if (frequency != lastFreq || resonance != lastRes) {
        calculateCoefficients();
        lastFreq = frequency;
        lastRes = resonance;
    }
    
    // Process audio
    for (int i = 0; i < numFrames * channels; i += channels) {
        for (int ch = 0; ch < std::min(channels, 2); ++ch) {
            // Get input sample
            float input = buffer[i + ch];
            
            // Process through biquad filter
            float output = b0_ * input + b1_ * x1_[ch] + b2_ * x2_[ch] - a1_ * y1_[ch] - a2_ * y2_[ch];
            
            // Update filter state
            x2_[ch] = x1_[ch];
            x1_[ch] = input;
            y2_[ch] = y1_[ch];
            y1_[ch] = output;
            
            // Write output
            buffer[i + ch] = output;
        }
    }
}

void BiquadFilterModel::setParameter(const std::string& name, float value) {
    bool recalculate = false;
    
    if (name == "frequency") {
        parameters_["frequency"] = clamp(value, 20.0f, 20000.0f);
        recalculate = true;
    }
    else if (name == "resonance") {
        parameters_["resonance"] = clamp(value, 0.1f, 10.0f);
        recalculate = true;
    }
    else if (name == "gain") {
        parameters_["gain"] = clamp(value, -24.0f, 24.0f);
        recalculate = true;
    }
    else if (name == "type") {
        int typeInt = static_cast<int>(value);
        if (typeInt >= 0 && typeInt <= 3) {
            type_ = static_cast<Type>(typeInt);
            recalculate = true;
        }
    }
    else {
        // For any other parameters, use base implementation
        FilterModel::setParameter(name, value);
    }
    
    if (recalculate) {
        calculateCoefficients();
    }
}

void BiquadFilterModel::calculateCoefficients() {
    float frequency = parameters_["frequency"];
    float resonance = parameters_["resonance"];
    float gain = parameters_["gain"];
    
    // Normalized frequency (0 to π)
    float omega = 2.0f * PI * frequency / sampleRate_;
    float sinOmega = std::sin(omega);
    float cosOmega = std::cos(omega);
    
    // Alpha for bandwidth/resonance
    float alpha = sinOmega / (2.0f * resonance);
    
    // Intermediate coefficient
    float A = std::pow(10.0f, gain / 40.0f); // Convert dB to linear gain
    
    // Calculate biquad filter coefficients based on filter type
    switch (type_) {
        case Type::LowPass:
            b0_ = (1.0f - cosOmega) / 2.0f;
            b1_ = 1.0f - cosOmega;
            b2_ = (1.0f - cosOmega) / 2.0f;
            a0_ = 1.0f + alpha;
            a1_ = -2.0f * cosOmega;
            a2_ = 1.0f - alpha;
            break;
            
        case Type::HighPass:
            b0_ = (1.0f + cosOmega) / 2.0f;
            b1_ = -(1.0f + cosOmega);
            b2_ = (1.0f + cosOmega) / 2.0f;
            a0_ = 1.0f + alpha;
            a1_ = -2.0f * cosOmega;
            a2_ = 1.0f - alpha;
            break;
            
        case Type::BandPass:
            b0_ = alpha;
            b1_ = 0.0f;
            b2_ = -alpha;
            a0_ = 1.0f + alpha;
            a1_ = -2.0f * cosOmega;
            a2_ = 1.0f - alpha;
            break;
            
        case Type::Notch:
            b0_ = 1.0f;
            b1_ = -2.0f * cosOmega;
            b2_ = 1.0f;
            a0_ = 1.0f + alpha;
            a1_ = -2.0f * cosOmega;
            a2_ = 1.0f - alpha;
            break;
    }
    
    // Normalize coefficients
    b0_ /= a0_;
    b1_ /= a0_;
    b2_ /= a0_;
    a1_ /= a0_;
    a2_ /= a0_;
}

//=============================================================================
// AdvancedFilter Implementation
//=============================================================================

AdvancedFilter::AdvancedFilter(int sampleRate, Type type)
    : Effect(sampleRate),
      currentType_(type),
      blendEnabled_(false),
      blendType_(Type::LowPass),
      blendAmount_(0.0f),
      mix_(1.0f) {
    
    // Create the primary filter
    currentFilter_ = createFilterModel(currentType_);
    
    // Initialize blend filter (but not enabled by default)
    blendFilter_ = createFilterModel(blendType_);
}

AdvancedFilter::~AdvancedFilter() {
}

void AdvancedFilter::setSampleRate(int sampleRate) {
    Effect::setSampleRate(sampleRate);
    
    if (currentFilter_) {
        currentFilter_->setSampleRate(sampleRate);
    }
    
    if (blendFilter_) {
        blendFilter_->setSampleRate(sampleRate);
    }
}

std::string AdvancedFilter::getName() const {
    return "AdvancedFilter: " + (currentFilter_ ? currentFilter_->getTypeName() : "Unknown");
}

std::unique_ptr<FilterModel> AdvancedFilter::createFilterModel(Type type) {
    switch (type) {
        // Standard biquad filters
        case Type::LowPass:
            return std::make_unique<BiquadFilterModel>(sampleRate_, BiquadFilterModel::Type::LowPass);
        case Type::HighPass:
            return std::make_unique<BiquadFilterModel>(sampleRate_, BiquadFilterModel::Type::HighPass);
        case Type::BandPass:
            return std::make_unique<BiquadFilterModel>(sampleRate_, BiquadFilterModel::Type::BandPass);
        case Type::Notch:
            return std::make_unique<BiquadFilterModel>(sampleRate_, BiquadFilterModel::Type::Notch);

        // Advanced filter types
        case Type::LadderLowPass:
            return std::make_unique<LadderFilterModel>(sampleRate_, LadderFilterModel::Type::LowPass);
        case Type::LadderHighPass:
            return std::make_unique<LadderFilterModel>(sampleRate_, LadderFilterModel::Type::HighPass);
        case Type::Comb:
            return std::make_unique<CombFilterModel>(sampleRate_, CombFilterModel::Type::FeedBack);
        case Type::Phaser:
            return std::make_unique<CombFilterModel>(sampleRate_, CombFilterModel::Type::FeedForward);
        case Type::Formant:
            return std::make_unique<FormantFilterModel>(sampleRate_);

        default:
            // Default to low pass if unknown
            return std::make_unique<BiquadFilterModel>(sampleRate_, BiquadFilterModel::Type::LowPass);
    }
}

void AdvancedFilter::process(float* buffer, int numFrames) {
    const int channels = 2; // Stereo processing
    
    if (!currentFilter_) {
        return; // No filter to process with
    }
    
    if (!blendEnabled_ || !blendFilter_) {
        // Simple case - just process with the current filter
        
        // First, make a copy for wet/dry mixing if not fully wet
        if (mix_ < 1.0f) {
            // Ensure temp buffer is large enough
            tempBuffer_.resize(numFrames * channels);
            std::copy(buffer, buffer + numFrames * channels, tempBuffer_.begin());
        }
        
        // Process with the current filter
        currentFilter_->process(buffer, numFrames, channels);
        
        // Mix dry/wet if needed
        if (mix_ < 1.0f) {
            for (int i = 0; i < numFrames * channels; ++i) {
                buffer[i] = tempBuffer_[i] * (1.0f - mix_) + buffer[i] * mix_;
            }
        }
    }
    else {
        // Blending case - need to process with both filters and mix
        
        // Ensure temp buffer is large enough
        tempBuffer_.resize(numFrames * channels);
        
        // Copy input for second filter and wet/dry mix
        std::copy(buffer, buffer + numFrames * channels, tempBuffer_.begin());
        
        // Process with current filter (directly in the input buffer)
        currentFilter_->process(buffer, numFrames, channels);
        
        // Process with blend filter (in the temp buffer)
        blendFilter_->process(tempBuffer_.data(), numFrames, channels);
        
        // Blend between the two filters and handle wet/dry mix
        for (int i = 0; i < numFrames * channels; ++i) {
            // First blend between the two filters
            float blendedSample = buffer[i] * (1.0f - blendAmount_) + tempBuffer_[i] * blendAmount_;
            
            // Then apply wet/dry mix using the original input
            if (mix_ < 1.0f) {
                // Keep track of the original input for wet/dry mixing
                float inputSample = i % channels == 0 ? buffer[i] : buffer[i + 1];
                buffer[i] = inputSample * (1.0f - mix_) + blendedSample * mix_;
            }
            else {
                buffer[i] = blendedSample;
            }
        }
    }
}

void AdvancedFilter::setFilterType(Type type) {
    if (type == currentType_) {
        return; // No change needed
    }
    
    currentType_ = type;
    
    // Create new filter of the requested type
    auto newFilter = createFilterModel(type);
    
    // Copy parameters from old filter if possible
    if (currentFilter_) {
        for (const auto& param : {"frequency", "resonance", "gain"}) {
            newFilter->setParameter(param, currentFilter_->getParameter(param));
        }
    }
    
    // Swap in the new filter
    currentFilter_ = std::move(newFilter);
}

void AdvancedFilter::setParameter(const std::string& name, float value) {
    bool paramHandled = false;
    
    // Global filter parameters
    if (name == "mix") {
        mix_ = clamp(value, 0.0f, 1.0f);
        paramHandled = true;
    }
    else if (name == "type") {
        int typeInt = static_cast<int>(value);
        if (typeInt >= 0 && typeInt < static_cast<int>(Type::NumTypes)) {
            setFilterType(static_cast<Type>(typeInt));
        }
        paramHandled = true;
    }
    else if (name == "blend_enabled") {
        blendEnabled_ = value > 0.5f;
        paramHandled = true;
    }
    else if (name == "blend_amount") {
        blendAmount_ = clamp(value, 0.0f, 1.0f);
        paramHandled = true;
    }
    else if (name == "blend_type") {
        int typeInt = static_cast<int>(value);
        if (typeInt >= 0 && typeInt < static_cast<int>(Type::NumTypes)) {
            if (typeInt != static_cast<int>(blendType_)) {
                blendType_ = static_cast<Type>(typeInt);
                blendFilter_ = createFilterModel(blendType_);
            }
        }
        paramHandled = true;
    }
    
    // Forward to the active filter(s)
    if (!paramHandled && currentFilter_) {
        currentFilter_->setParameter(name, value);
        
        // Also update blend filter with the same parameters
        if (blendFilter_) {
            blendFilter_->setParameter(name, value);
        }
    }
}

float AdvancedFilter::getParameter(const std::string& name) const {
    // Global filter parameters
    if (name == "mix") {
        return mix_;
    }
    else if (name == "type") {
        return static_cast<float>(currentType_);
    }
    else if (name == "blend_enabled") {
        return blendEnabled_ ? 1.0f : 0.0f;
    }
    else if (name == "blend_amount") {
        return blendAmount_;
    }
    else if (name == "blend_type") {
        return static_cast<float>(blendType_);
    }
    
    // Forward to the active filter
    if (currentFilter_) {
        return currentFilter_->getParameter(name);
    }
    
    return 0.0f;
}

} // namespace AIMusicHardware