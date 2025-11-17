#include <iostream>
#include <vector>
#include <memory>
#include <chrono>
#include <thread>
#include "../../include/sequencer/Sequencer.h"

using namespace AIMusicHardware;

class GhostNoteDetector {
private:
    std::vector<std::pair<double, int>> expectedNotes;
    std::vector<std::pair<double, int>> actualNotes;
    double tolerance;
    
public:
    GhostNoteDetector(double tol = 0.1) 
        : tolerance(tol) {}
    
    void addExpectedNote(double beat, int pitch) {
        expectedNotes.push_back({beat, pitch});
    }
    
    void recordActualNote(double beat, int pitch) {
        actualNotes.push_back({beat, pitch});
        std::cout << "[DETECTED] Note at beat " << beat << ", pitch " << pitch << std::endl;
    }
    
    void analyze() {
        std::cout << "\n=== GHOST NOTE ANALYSIS ===" << std::endl;
        std::cout << "Expected notes (per loop): " << expectedNotes.size() << std::endl;
        std::cout << "Actual notes: " << actualNotes.size() << std::endl;
        
        // Check for missing expected notes (single-loop window)
        for (const auto& expected : expectedNotes) {
            bool found = false;
            for (const auto& actual : actualNotes) {
                if (std::abs(actual.first - expected.first) <= tolerance && 
                    actual.second == expected.second) {
                    found = true;
                    break;
                }
            }
            if (!found) {
                std::cout << "[MISSING] Expected note at beat " << expected.first 
                         << ", pitch " << expected.second << " was NOT detected" << std::endl;
            }
        }
        
        // Check for unexpected notes (ghost notes) within the first loop window
        for (const auto& actual : actualNotes) {
            bool expected = false;
            for (const auto& exp : expectedNotes) {
                if (std::abs(actual.first - exp.first) <= tolerance && 
                    actual.second == exp.second) {
                    expected = true;
                    break;
                }
            }
            if (!expected) {
                std::cout << "[GHOST] Unexpected note at beat " << actual.first 
                         << ", pitch " << actual.second << std::endl;
            }
        }
        
        // Detailed timing analysis
        std::cout << "\n=== DETAILED TIMING ANALYSIS ===" << std::endl;
        std::cout << "Expected sequence:" << std::endl;
        for (const auto& note : expectedNotes) {
            std::cout << "  Beat " << note.first << " -> Pitch " << note.second << std::endl;
        }
        
        std::cout << "\nActual sequence:" << std::endl;
        for (const auto& note : actualNotes) {
            std::cout << "  Beat " << note.first << " -> Pitch " << note.second << std::endl;
        }
    }
};

int main() {
    std::cout << "=== GHOST NOTE TEST ===" << std::endl;
    std::cout << "Testing sequence: C4 at beats 1, 8, 11" << std::endl;
    
    // Create sequencer
    auto sequencer = std::make_unique<Sequencer>(120.0, 4); // 120 BPM, 4/4 time
    sequencer->initialize();
    // Enable per-loop dedupe so a note at an exact boundary is not re-fired
    // in the very next frame; this matches the recommended production setting
    // when chasing ghost notes at loop edges.
    sequencer->setPerLoopDedupeEnabled(true);
    
    // Create test pattern
    auto pattern = std::make_unique<Pattern>("Ghost Test Pattern");
    pattern->setLength(16.0); // 4 bars
    
    // Add notes at beats 1, 8, 11 (C4 = MIDI note 60)
    int pitch = 60; // C4
    double stepBeats = 1.0; // 1 beat per step
    
    // Beat 1 (0-indexed would be 0, but we want beat 1)
    pattern->addNote(Note(pitch, 1.0f, 1.0 * stepBeats, stepBeats, 0));
    
    // Beat 8
    pattern->addNote(Note(pitch, 1.0f, 8.0 * stepBeats, stepBeats, 0));
    
    // Beat 11
    pattern->addNote(Note(pitch, 1.0f, 11.0 * stepBeats, stepBeats, 0));
    
    sequencer->addPattern(std::move(pattern));
    sequencer->setCurrentPattern(1); // Use index 1, not 0 (0 is the default empty pattern)
    
    // Set up ghost note detector
    GhostNoteDetector detector(0.1);
    detector.addExpectedNote(1.0, pitch);
    detector.addExpectedNote(8.0, pitch);
    detector.addExpectedNote(11.0, pitch);
    
    // Set up note callbacks
    sequencer->setNoteCallbacks(
        [&](int notePitch, float velocity, int channel, const Envelope& env) {
            double currentBeat = sequencer->getPositionInBeats();
            detector.recordActualNote(currentBeat, notePitch);
            std::cout << "[NOTE ON] Beat " << currentBeat << " -> Pitch " << notePitch 
                     << " (velocity " << velocity << ")" << std::endl;
        },
        [&](int notePitch, int channel) {
            double currentBeat = sequencer->getPositionInBeats();
            std::cout << "[NOTE OFF] Beat " << currentBeat << " -> Pitch " << notePitch << std::endl;
        }
    );
    
    // Start sequencer
    sequencer->start();
    std::cout << "\nStarting sequencer playback (single-loop window)...\n";
    
    // Drive the sequencer deterministically for exactly one 16-beat loop.
    const double bpm = 120.0;
    const double secPerBeat = 60.0 / bpm;
    const double beatsPerLoop = 16.0;
    const int framesPerBeat = 60;           // simulation frames per beat
    const int totalFrames = (int)(beatsPerLoop * framesPerBeat);
    const double dt = secPerBeat / framesPerBeat;
    
    for (int frame = 0; frame < totalFrames; ++frame) {
        sequencer->process(dt);
        if (frame % (framesPerBeat * 4) == 0) { // roughly every bar
            double pos = sequencer->getPositionInBeats();
            std::cout << "[POS] Current beat: " << pos << std::endl;
        }
    }
    
    // Stop and analyze
    sequencer->stop();
    std::cout << "\nStopping sequencer..." << std::endl;
    
    // Analyze results
    detector.analyze();
    
    std::cout << "\n=== TEST COMPLETE ===" << std::endl;
    return 0;
}
