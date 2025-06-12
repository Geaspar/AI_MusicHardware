#pragma once

namespace AIMusicHardware {

// Abstract interface for modulation sources
class ModulationSource {
public:
    virtual ~ModulationSource() = default;
    
    // Get current modulation value (typically -1 to 1 or 0 to 1)
    virtual float getValue() const = 0;
    
    // Process one sample
    virtual float process() = 0;
    
    // Reset to initial state
    virtual void reset() = 0;
    
    // Trigger (for triggered sources)
    virtual void trigger() {}
    
    // Get human-readable name
    virtual const char* getName() const = 0;
    
    // Check if source is bipolar (-1 to 1) or unipolar (0 to 1)
    virtual bool isBipolar() const = 0;
};

// Concrete modulation source types
enum class ModulationSourceType {
    None,
    LFO1,
    LFO2,
    LFO3,
    LFO4,
    Envelope1,
    Envelope2,
    Velocity,
    Aftertouch,
    ModWheel,
    PitchBend,
    Expression,
    Breath,
    Random,
    Count
};

// Helper to get source name
inline const char* getModulationSourceName(ModulationSourceType type) {
    switch (type) {
        case ModulationSourceType::None: return "None";
        case ModulationSourceType::LFO1: return "LFO 1";
        case ModulationSourceType::LFO2: return "LFO 2";
        case ModulationSourceType::LFO3: return "LFO 3";
        case ModulationSourceType::LFO4: return "LFO 4";
        case ModulationSourceType::Envelope1: return "Envelope 1";
        case ModulationSourceType::Envelope2: return "Envelope 2";
        case ModulationSourceType::Velocity: return "Velocity";
        case ModulationSourceType::Aftertouch: return "Aftertouch";
        case ModulationSourceType::ModWheel: return "Mod Wheel";
        case ModulationSourceType::PitchBend: return "Pitch Bend";
        case ModulationSourceType::Expression: return "Expression";
        case ModulationSourceType::Breath: return "Breath";
        case ModulationSourceType::Random: return "Random";
        default: return "Unknown";
    }
}

} // namespace AIMusicHardware