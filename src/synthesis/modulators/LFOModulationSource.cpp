#include "synthesis/modulators/LFOModulationSource.h"

namespace AIMusicHardware {

LFOModulationSource::LFOModulationSource(const std::string& name, float sampleRate)
    : ModulationSource(name)
    , lfo_(sampleRate)
    , currentValue_(0.0f) {
    setBipolar(true); // LFOs are bipolar by default
}

float LFOModulationSource::getValue() const {
    return currentValue_;
}

void LFOModulationSource::update() {
    currentValue_ = lfo_.process();
}

} // namespace AIMusicHardware