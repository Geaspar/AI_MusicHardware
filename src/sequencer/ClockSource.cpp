#include "../../include/sequencer/ClockSource.h"
#include <algorithm>

namespace AIMusicHardware {

void ClockSource::updateBeatTime() {
    double bpm = std::max(1.0, tempo_.load(std::memory_order_relaxed));
    beatTimeSeconds_.store(60.0 / bpm, std::memory_order_relaxed);
}

void ClockSource::processAudio(double numSamples) {
    if (mode_.load(std::memory_order_relaxed) != Mode::Internal) {
        // Track time for external edge intervals
        timeSecAccum_ += numSamples / std::max(1.0, sampleRate_.load(std::memory_order_relaxed));
        return;
    }
    // Internal tempo accumulates beats continuously
    double sr = std::max(1.0, sampleRate_.load(std::memory_order_relaxed));
    double bt = beatTimeSeconds_.load(std::memory_order_relaxed);
    double sec = numSamples / sr;
    beatsAccum_ += sec / std::max(1e-9, bt);
}

void ClockSource::onExternalClockSample(float volts) {
    if (mode_.load(std::memory_order_relaxed) != Mode::External)
        return;
    bool newState = extState_;
    if (!extState_ && volts >= extHigh_) newState = true;
    else if (extState_ && volts <= extLow_) newState = false;
    if (newState && !extState_) {
        // Rising edge detected: compute time since last edge
        double now = timeSecAccum_;
        double dt = std::max(1e-6, now - extLastEdgeTimeSec_);
        extLastEdgeTimeSec_ = now;
        // Convert to beats (MIDI clock often 24 ppq -> 24 pulses per quarter note)
        double pulsesPerBeat = std::max(1.0, ppq_.load(std::memory_order_relaxed));
        beatsAccum_ += 1.0 / pulsesPerBeat;
    }
    extState_ = newState;
}

double ClockSource::popBeatIncrements() {
    double b = beatsAccum_;
    beatsAccum_ = 0.0;
    return b;
}

} // namespace AIMusicHardware

