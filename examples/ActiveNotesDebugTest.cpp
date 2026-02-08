/**
 * Test to debug active notes management in the sequencer
 * Focus on what happens to notes that cross the loop boundary
 */

#include <iostream>
#include <memory>
#include <thread>
#include <chrono>
#include <iomanip>
#include <set>
#include "../include/sequencer/Sequencer.h"

using namespace AIMusicHardware;

class ActiveNotesTracker {
private:
    std::set<int> activeNotes_;
    std::vector<std::string> log_;
    double patternLength_;
    
public:
    ActiveNotesTracker(double patternLength) : patternLength_(patternLength) {}
    
    void onNoteOn(int pitch, double beat) {
        if (activeNotes_.count(pitch) > 0) {
            std::string msg = "WARNING: Note " + std::to_string(pitch) + 
                             " triggered while already active at beat " + 
                             std::to_string(beat);
            log_.push_back(msg);
            std::cout << "*** " << msg << " ***" << std::endl;
        }
        activeNotes_.insert(pitch);
        
        std::cout << std::fixed << std::setprecision(3);
        std::cout << "NOTE ON:  pitch=" << pitch << " beat=" << std::setw(6) << beat;
        std::cout << " | Active notes: {";
        for (auto p : activeNotes_) {
            std::cout << p << " ";
        }
        std::cout << "}" << std::endl;
    }
    
    void onNoteOff(int pitch, double beat) {
        if (activeNotes_.count(pitch) == 0) {
            std::string msg = "WARNING: Note " + std::to_string(pitch) + 
                             " released but wasn't active at beat " + 
                             std::to_string(beat);
            log_.push_back(msg);
            std::cout << "*** " << msg << " ***" << std::endl;
        }
        activeNotes_.erase(pitch);
        
        std::cout << std::fixed << std::setprecision(3);
        std::cout << "NOTE OFF: pitch=" << pitch << " beat=" << std::setw(6) << beat;
        std::cout << " | Active notes: {";
        for (auto p : activeNotes_) {
            std::cout << p << " ";
        }
        std::cout << "}" << std::endl;
    }
    
    void checkForStuckNotes(double currentBeat) {
        if (!activeNotes_.empty()) {
            std::cout << "STUCK NOTES at beat " << currentBeat << ": {";
            for (auto p : activeNotes_) {
                std::cout << p << " ";
            }
            std::cout << "}" << std::endl;
        }
    }
    
    void printSummary() {
        std::cout << "\n=== ACTIVE NOTES TRACKING SUMMARY ===" << std::endl;
        if (log_.empty()) {
            std::cout << "No issues detected!" << std::endl;
        } else {
            std::cout << "Issues found:" << std::endl;
            for (const auto& msg : log_) {
                std::cout << "  - " << msg << std::endl;
            }
        }
        
        if (!activeNotes_.empty()) {
            std::cout << "\nWARNING: Notes still active at end: {";
            for (auto p : activeNotes_) {
                std::cout << p << " ";
            }
            std::cout << "}" << std::endl;
        }
    }
};

int main() {
    std::cout << "=== ACTIVE NOTES DEBUG TEST ===" << std::endl;
    std::cout << "Testing note lifecycle across loop boundaries" << std::endl;
    std::cout << "Pattern: Notes at beats 1, 8, 11 with different durations" << std::endl;
    std::cout << "===============================================\n" << std::endl;
    
    // Create sequencer
    auto sequencer = std::make_unique<Sequencer>(120.0, 4);
    sequencer->initialize();
    
    // Create test pattern with varying note durations
    auto pattern = std::make_unique<Pattern>("Debug Pattern");
    pattern->setLength(16.0);
    
    const int C4 = 60;
    const int D4 = 62;
    const int E4 = 64;
    
    // Add notes with different durations to test edge cases
    // Short note at beat 1
    pattern->addNote(Note(C4, 1.0f, 1.0, 0.5, 0));   // Beat 1, duration 0.5
    
    // Medium note at beat 8
    pattern->addNote(Note(D4, 1.0f, 8.0, 2.0, 0));   // Beat 8, duration 2.0
    
    // Note at beat 11 that could cause issues
    pattern->addNote(Note(E4, 1.0f, 11.0, 1.0, 0));  // Beat 11, duration 1.0
    
    // Add a note near the end to test loop boundary
    pattern->addNote(Note(C4, 1.0f, 15.0, 1.5, 0));  // Beat 15, extends past loop
    
    std::cout << "Pattern notes:" << std::endl;
    for (size_t i = 0; i < pattern->getNumNotes(); ++i) {
        const Note* note = pattern->getNote(i);
        if (note) {
            std::cout << "  Note " << i << ": pitch=" << note->pitch 
                     << " start=" << note->startTime 
                     << " end=" << (note->startTime + note->duration)
                     << " duration=" << note->duration << std::endl;
        }
    }
    std::cout << std::endl;
    
    // Add pattern
    sequencer->addPattern(std::move(pattern));
    sequencer->setCurrentPattern(1);
    sequencer->setLooping(true);
    
    // Create tracker
    ActiveNotesTracker tracker(16.0);
    
    // Set callbacks
    sequencer->setNoteCallbacks(
        [&](int pitch, float velocity, int channel, const Envelope& env) {
            double beat = sequencer->getPositionInBeats();
            tracker.onNoteOn(pitch, beat);
        },
        [&](int pitch, int channel) {
            double beat = sequencer->getPositionInBeats();
            tracker.onNoteOff(pitch, beat);
        }
    );
    
    // Transport callback to mark loop points
    sequencer->setTransportCallback(
        [&](double beat, int bar, int beatNum) {
            static int lastBar = -1;
            static double lastBeat = -1;
            
            // Detect loop
            if (beat < lastBeat) {
                std::cout << "\n>>> LOOP DETECTED: beat " << lastBeat 
                         << " -> " << beat << " <<<\n" << std::endl;
                tracker.checkForStuckNotes(beat);
            }
            
            // Print bar changes
            if (bar != lastBar) {
                std::cout << "\n=== Bar " << bar << " ===" << std::endl;
                lastBar = bar;
            }
            
            lastBeat = beat;
        }
    );
    
    // Run test
    std::cout << "Starting playback for 3 loops (24 seconds at 120 BPM)...\n" << std::endl;
    sequencer->start();
    
    const double frameTime = 1.0 / 60.0;
    const int totalFrames = 24 * 60; // 24 seconds
    
    for (int frame = 0; frame < totalFrames; ++frame) {
        sequencer->process(frameTime);
        std::this_thread::sleep_for(std::chrono::microseconds(16667));
        
        // Check for stuck notes periodically
        if (frame % 600 == 599) { // Every 10 seconds
            double beat = sequencer->getPositionInBeats();
            std::cout << "\n[Check at beat " << beat << "]" << std::endl;
            tracker.checkForStuckNotes(beat);
        }
    }
    
    // Stop
    sequencer->stop();
    std::cout << "\n\nPlayback stopped." << std::endl;
    
    // Final analysis
    tracker.printSummary();
    
    std::cout << "\n=== TEST COMPLETE ===" << std::endl;
    return 0;
}