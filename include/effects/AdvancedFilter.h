#pragma once

#include "EffectProcessor.h"
#include <memory>
#include <vector>
#include <string>
#include <unordered_map>

namespace AIMusicHardware {

/**
 * @brief FilterModel is a base class for all filter implementations
 * 
 * This is the core of the filter architecture. Each filter algorithm
 * should inherit from this class and implement its specific processing.
 */
class FilterModel {
public:
    FilterModel(int sampleRate = 44100);
    virtual ~FilterModel();
    
    // Core filter processing function
    virtual void process(float* buffer, int numFrames, int channels) = 0;
    
    // Filter parameter settings
    virtual void setParameter(const std::string& name, float value);
    virtual float getParameter(const std::string& name) const;
    
    // Sample rate handling
    virtual void setSampleRate(int sampleRate);
    int getSampleRate() const { return sampleRate_; }
    
    // Filter type identification
    virtual std::string getTypeName() const = 0;
    
protected:
    int sampleRate_;
    std::unordered_map<std::string, float> parameters_;
};

/**
 * @brief Advanced filter with multi-type support and blending capabilities
 * 
 * This filter extends the basic Effect class and uses different FilterModel
 * implementations for a wide range of filter types.
 */
class AdvancedFilter : public Effect {
public:
    enum class Type {
        // Standard biquad filters
        LowPass,
        HighPass, 
        BandPass,
        Notch,
        
        // Advanced filter types
        LadderLowPass,
        LadderHighPass,
        Comb,
        Phaser,
        Formant,
        
        // Number of filter types
        NumTypes
    };
    
    AdvancedFilter(int sampleRate = 44100, Type type = Type::LowPass);
    ~AdvancedFilter() override;
    
    void setSampleRate(int sampleRate) override;
    void process(float* buffer, int numFrames) override;
    void setParameter(const std::string& name, float value) override;
    float getParameter(const std::string& name) const override;
    
    std::string getName() const override;
    
    // New filter-specific methods
    void setFilterType(Type type);
    Type getFilterType() const { return currentType_; }
    
    // Blend between two filter types
    void setBlendMode(bool enabled) { blendEnabled_ = enabled; }
    bool getBlendMode() const { return blendEnabled_; }
    void setBlendType(Type type) { blendType_ = type; }
    Type getBlendType() const { return blendType_; }
    void setBlendAmount(float amount) { blendAmount_ = amount; }
    float getBlendAmount() const { return blendAmount_; }
    
private:
    // Helper to create the appropriate filter model
    std::unique_ptr<FilterModel> createFilterModel(Type type);
    
    // Active filter models
    Type currentType_;
    std::unique_ptr<FilterModel> currentFilter_;
    
    // Blending support
    bool blendEnabled_;
    Type blendType_;
    float blendAmount_; // 0.0 = only primary, 1.0 = only secondary
    std::unique_ptr<FilterModel> blendFilter_;
    
    // Temporary processing buffer for blending
    std::vector<float> tempBuffer_;
    
    // Common filter parameters
    float mix_; // Wet/dry mix
};

} // namespace AIMusicHardware