#include "../include/sequencer/Sequencer.h"
#include "../include/sequencer/ClockSource.h"
#include <iostream>
#include <cmath>
#include <thread>
#include <chrono>

using namespace AIMusicHardware;

int main() {
    std::cout << "ExternalClockDemo: driving sequencer via external clock pulses\n";

    Sequencer seq(120.0, 4);
    if (!seq.initialize()) {
        std::cerr << "Failed to initialize sequencer" << std::endl;
        return 1;
    }
    // Create a 1-bar pattern at 16th resolution and add 4 notes (quarters)
    size_t pat = seq.createPattern("Demo", 16);
    seq.setCurrentPattern(pat);
    for (int s = 0; s < 16; s += 4) {
        seq.setStep(pat, s, 60 + s / 4, 0.9f, 1.0f);
    }
    seq.setPerLoopDedupeEnabled(true);
    seq.setProbabilityMode(Sequencer::ProbabilityMode::PerLoopStable);
    seq.setWrapLongNotesAcrossLoop(true);

    // Simple print callbacks
    seq.setNoteCallbacks(
        [](int pitch, float vel, int ch, const Envelope&) {
            std::cout << "NoteOn  pitch=" << pitch << " vel=" << vel << " ch=" << ch << "\n";
        },
        [](int pitch, int ch) {
            std::cout << "NoteOff pitch=" << pitch << " ch=" << ch << "\n";
        }
    );

    // Attach an external clock source
    ClockSource clock;
    clock.setSampleRate(48000.0);
    clock.setTempo(120.0); // for internal calculations if needed
    clock.setPPQ(4.0);     // 4 pulses per beat => 16th notes at 4/4
    clock.setMode(ClockSource::Mode::External);
    seq.attachClockSource(&clock);

    // Synchronize seq with a notional audio engine
    seq.synchronizeWithAudioEngine(0.0, 48000.0);

    // Start playback
    seq.start();

    // Simulate audio callback blocks and generate external pulses
    const double bpm = 120.0;
    const double secondsPerBeat = 60.0 / bpm;
    const double pulsesPerBeat = 4.0; // 16th notes
    const double pulsePeriod = secondsPerBeat / pulsesPerBeat; // seconds per pulse

    double t = 0.0;
    double nextPulse = pulsePeriod;
    const double sampleRate = 48000.0;
    const int framesPerBlock = 480; // 10 ms blocks
    const double blockSec = framesPerBlock / sampleRate;

    for (int i = 0; i < 200; ++i) { // ~2 seconds
        // Advance time
        t += blockSec;

        // Emit any pulses that should occur in this block
        while (t >= nextPulse) {
            // Rising edge
            clock.onExternalClockSample(5.0f);
            // Falling edge
            clock.onExternalClockSample(0.0f);
            nextPulse += pulsePeriod;
        }

        // Drive the sequencer with this block's time
        seq.process(blockSec);

        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    seq.stop();
    std::cout << "Done." << std::endl;
    return 0;
}

