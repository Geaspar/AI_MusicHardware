#pragma once

#include "synthesis/modulators/modulation_matrix.h"
#include "synthesis/modulators/LFO.h"
#include <memory>

namespace AIMusicHardware {

// Adapter class to make LFO work with the existing ModulationSource interface
class LFOModulationSource : public ModulationSource {
public:
    LFOModulationSource(const std::string& name, float sampleRate);
    ~LFOModulationSource() override = default;
    
    // ModulationSource interface
    float getValue() const override;
    void update() override;
    
    // Access to underlying LFO
    LFO& getLFO() { return lfo_; }
    const LFO& getLFO() const { return lfo_; }
    
    // Convenience methods
    void setRate(float hz) { lfo_.setRate(hz); }
    void setDepth(float depth) { lfo_.setDepth(depth); }
    void setShape(LFO::WaveShape shape) { lfo_.setShape(shape); }
    void setSyncMode(LFO::SyncMode mode) { lfo_.setSyncMode(mode); }
    void setPhase(float phase) { lfo_.setPhase(phase); }
    void trigger() { lfo_.trigger(); }
    void reset() { lfo_.reset(); }
    
private:
    LFO lfo_;
    float currentValue_;
};

} // namespace AIMusicHardware