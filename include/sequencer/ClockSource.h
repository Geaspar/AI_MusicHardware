#pragma once

#include <atomic>

namespace AIMusicHardware {

// Simple musical clock source with internal tempo or external edge-driven mode.
class ClockSource {
public:
    enum class Mode { Internal, External };

    void setMode(Mode m) { mode_.store(m, std::memory_order_relaxed); }
    Mode getMode() const { return mode_.load(std::memory_order_relaxed); }

    void setTempo(double bpm) { tempo_.store(bpm, std::memory_order_relaxed); updateBeatTime(); }
    double getTempo() const { return tempo_.load(std::memory_order_relaxed); }

    void setSampleRate(double sr) { sampleRate_.store(sr, std::memory_order_relaxed); updateBeatTime(); }
    double getSampleRate() const { return sampleRate_.load(std::memory_order_relaxed); }

    void setPPQ(double ppq) { ppq_.store(ppq, std::memory_order_relaxed); }
    double getPPQ() const { return ppq_.load(std::memory_order_relaxed); }

    // Call per audio callback to advance internal phase
    void processAudio(double numSamples);

    // External clock edge input (voltage) with simple hysteresis
    void onExternalClockSample(float volts);

    // Return beats elapsed since last read (consumes accumulator)
    double popBeatIncrements();

    // Seconds per beat at current tempo
    double getBeatTimeSeconds() const { return beatTimeSeconds_.load(std::memory_order_relaxed); }

private:
    void updateBeatTime();

    std::atomic<Mode> mode_{Mode::Internal};
    std::atomic<double> tempo_{120.0};
    std::atomic<double> sampleRate_{48000.0};
    std::atomic<double> ppq_{24.0}; // MIDI clock default
    std::atomic<double> beatTimeSeconds_{0.5};

    // Internal accumulators
    double internalPhaseSamples_ = 0.0;
    double beatsAccum_ = 0.0;

    // External Schmitt trigger state
    bool extState_ = false;
    double extLastEdgeTimeSec_ = 0.0;
    double timeSecAccum_ = 0.0;
    float extLow_ = 0.1f, extHigh_ = 2.0f; // simple hysteresis
};

} // namespace AIMusicHardware

