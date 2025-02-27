#pragma once

#include <memory>
#include <string>
#include <vector>
#include <map>

namespace AIMusicHardware {

class Effect {
public:
    Effect(int sampleRate = 44100);
    virtual ~Effect();
    
    virtual void process(float* buffer, int numFrames) = 0;
    virtual void setParameter(const std::string& name, float value) = 0;
    virtual float getParameter(const std::string& name) const = 0;
    
    virtual void setSampleRate(int sampleRate);
    int getSampleRate() const { return sampleRate_; }
    
    virtual std::string getName() const = 0;
    
protected:
    int sampleRate_;
};

class EffectProcessor {
public:
    EffectProcessor(int sampleRate = 44100);
    ~EffectProcessor();
    
    void addEffect(std::unique_ptr<Effect> effect);
    void removeEffect(size_t index);
    void clearEffects();
    
    Effect* getEffect(size_t index);
    size_t getNumEffects() const;
    
    void process(float* buffer, int numFrames);
    void setSampleRate(int sampleRate);
    
private:
    std::vector<std::unique_ptr<Effect>> effects_;
    int sampleRate_;
    std::vector<float> tempBuffer_;
};

// Time-based effects
class Delay : public Effect {
public:
    Delay(int sampleRate = 44100);
    ~Delay() override;
    
    void process(float* buffer, int numFrames) override;
    void setParameter(const std::string& name, float value) override;
    float getParameter(const std::string& name) const override;
    std::string getName() const override { return "Delay"; }
    
private:
    float delayTime_; // in seconds
    float feedback_;  // 0-1
    float mix_;       // 0-1 (dry to wet)
    std::vector<float> delayBuffer_;
    size_t writePos_;
};

class Reverb : public Effect {
public:
    Reverb(int sampleRate = 44100);
    ~Reverb() override;
    
    void process(float* buffer, int numFrames) override;
    void setParameter(const std::string& name, float value) override;
    float getParameter(const std::string& name) const override;
    std::string getName() const override { return "Reverb"; }
    
private:
    float roomSize_;
    float damping_;
    float wetLevel_;
    float dryLevel_;
    float width_;
    
    class Impl;
    std::unique_ptr<Impl> pimpl_;
};

// Forward declarations of other effect classes
// These are defined in their own header files

// Modulation effects
class Modulation;

// Tone-shaping effects
class Saturation;
class BassBoost;

// Effect factory function
inline std::unique_ptr<Effect> createEffect(const std::string& type, int sampleRate) {
    if (type == "Delay") {
        return std::make_unique<Delay>(sampleRate);
    }
    else if (type == "Reverb") {
        return std::make_unique<Reverb>(sampleRate);
    }
    // Other effect types need to include their respective headers
    return nullptr;
}

} // namespace AIMusicHardware