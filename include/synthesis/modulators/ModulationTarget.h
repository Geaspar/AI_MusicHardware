#pragma once

namespace AIMusicHardware {

// Modulation target types
enum class ModulationTargetType {
    None,
    // Oscillator targets
    OscPitch,
    OscFine,
    OscShape,
    OscPulseWidth,
    OscLevel,
    OscPan,
    
    // Filter targets  
    FilterCutoff,
    FilterResonance,
    FilterDrive,
    FilterMix,
    
    // Envelope targets
    EnvAttack,
    EnvDecay,
    EnvSustain,
    EnvRelease,
    
    // Amp targets
    Volume,
    Pan,
    
    // Effect targets
    FxMix,
    FxParam1,
    FxParam2,
    FxParam3,
    
    // LFO targets (for LFO cross-modulation)
    LFORate,
    LFODepth,
    
    Count
};

// Helper to get target name
inline const char* getModulationTargetName(ModulationTargetType type) {
    switch (type) {
        case ModulationTargetType::None: return "None";
        case ModulationTargetType::OscPitch: return "Pitch";
        case ModulationTargetType::OscFine: return "Fine Tune";
        case ModulationTargetType::OscShape: return "Wave Shape";
        case ModulationTargetType::OscPulseWidth: return "Pulse Width";
        case ModulationTargetType::OscLevel: return "Osc Level";
        case ModulationTargetType::OscPan: return "Osc Pan";
        case ModulationTargetType::FilterCutoff: return "Filter Cutoff";
        case ModulationTargetType::FilterResonance: return "Filter Resonance";
        case ModulationTargetType::FilterDrive: return "Filter Drive";
        case ModulationTargetType::FilterMix: return "Filter Mix";
        case ModulationTargetType::EnvAttack: return "Env Attack";
        case ModulationTargetType::EnvDecay: return "Env Decay";
        case ModulationTargetType::EnvSustain: return "Env Sustain";
        case ModulationTargetType::EnvRelease: return "Env Release";
        case ModulationTargetType::Volume: return "Volume";
        case ModulationTargetType::Pan: return "Pan";
        case ModulationTargetType::FxMix: return "FX Mix";
        case ModulationTargetType::FxParam1: return "FX Param 1";
        case ModulationTargetType::FxParam2: return "FX Param 2";
        case ModulationTargetType::FxParam3: return "FX Param 3";
        case ModulationTargetType::LFORate: return "LFO Rate";
        case ModulationTargetType::LFODepth: return "LFO Depth";
        default: return "Unknown";
    }
}

// Modulation routing slot
struct ModulationSlot {
    ModulationSourceType source = ModulationSourceType::None;
    ModulationTargetType target = ModulationTargetType::None;
    float amount = 0.0f;  // -1 to 1
    bool active = false;
};

} // namespace AIMusicHardware