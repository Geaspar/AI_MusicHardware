#pragma once

// Include all effect classes for easy access

#include "EffectProcessor.h"
#include "EffectUtils.h"
#include "Saturation.h"
#include "BassBoost.h"
#include "Modulation.h"
#include "Filter.h"
#include "Distortion.h"
#include "BitCrusher.h"
#include "Compressor.h"
#include "Phaser.h"
#include "EQ.h"

namespace AIMusicHardware {

// Extended effect factory function that includes all effect types
inline std::unique_ptr<Effect> createEffectComplete(const std::string& type, int sampleRate) {
    if (type == "Delay") {
        return std::make_unique<Delay>(sampleRate);
    }
    else if (type == "Reverb") {
        return std::make_unique<Reverb>(sampleRate);
    }
    else if (type == "Saturation") {
        return std::make_unique<Saturation>(sampleRate);
    }
    else if (type == "BassBoost") {
        return std::make_unique<BassBoost>(sampleRate);
    }
    else if (type == "Modulation") {
        return std::make_unique<Modulation>(sampleRate);
    }
    else if (type == "LowPassFilter") {
        return std::make_unique<Filter>(sampleRate, Filter::Type::LowPass);
    }
    else if (type == "HighPassFilter") {
        return std::make_unique<Filter>(sampleRate, Filter::Type::HighPass);
    }
    else if (type == "BandPassFilter") {
        return std::make_unique<Filter>(sampleRate, Filter::Type::BandPass);
    }
    else if (type == "NotchFilter") {
        return std::make_unique<Filter>(sampleRate, Filter::Type::Notch);
    }
    else if (type == "Distortion") {
        return std::make_unique<Distortion>(sampleRate);
    }
    else if (type == "BitCrusher") {
        return std::make_unique<BitCrusher>(sampleRate);
    }
    else if (type == "Compressor") {
        return std::make_unique<Compressor>(sampleRate);
    }
    else if (type == "Phaser") {
        return std::make_unique<Phaser>(sampleRate);
    }
    else if (type == "EQ") {
        return std::make_unique<EQ>(sampleRate);
    }
    return nullptr;
}

// Helper function to get a list of all available effect types
inline std::vector<std::string> getAvailableEffects() {
    return {
        "Delay",
        "Reverb",
        "Saturation",
        "BassBoost",
        "Modulation",
        "LowPassFilter",
        "HighPassFilter",
        "BandPassFilter",
        "NotchFilter",
        "Distortion",
        "BitCrusher",
        "Compressor",
        "Phaser",
        "EQ"
    };
}

// Helper to categorize effects
enum class EffectCategory {
    Dynamics,    // Compressor
    Distortion,  // Saturation, Distortion, BitCrusher
    Filter,      // Filters, EQ
    Modulation,  // Phaser, Chorus, Flanger
    TimeBased,   // Delay, Reverb
    Utility      // Gain, Analyzer
};

// Get effects by category
inline std::vector<std::string> getEffectsByCategory(EffectCategory category) {
    switch (category) {
        case EffectCategory::Dynamics:
            return {"Compressor"};
            
        case EffectCategory::Distortion:
            return {"Saturation", "Distortion", "BitCrusher"};
            
        case EffectCategory::Filter:
            return {"LowPassFilter", "HighPassFilter", "BandPassFilter", "NotchFilter", "EQ", "BassBoost"};
            
        case EffectCategory::Modulation:
            return {"Phaser", "Modulation"};
            
        case EffectCategory::TimeBased:
            return {"Delay", "Reverb"};
            
        case EffectCategory::Utility:
            return {};
            
        default:
            return {};
    }
}

} // namespace AIMusicHardware