#include <iostream>
#include <vector>
#include <memory>
#include <chrono>
#include <thread>
#include "../../include/sequencer/Sequencer.h"

using namespace AIMusicHardware;

int main() {
    std::cout << "=== DETAILED GHOST NOTE TEST ===" << std::endl;
    std::cout << "Testing sequence: C4 at beats 1, 8, 11" << std::endl;
    
    // Create sequencer
    auto sequencer = std::make_unique<Sequencer>(120.0, 4); // 120 BPM, 4/4 time
    sequencer->initialize();
    // Match production ghost-note mitigation by enabling per-loop dedupe.
    sequencer->setPerLoopDedupeEnabled(true);
    
    // Create test pattern
    auto pattern = std::make_unique<Pattern>("Detailed Ghost Test Pattern");
    pattern->setLength(16.0); // 4 bars
    
    // Add notes at beats 1, 8, 11 (C4 = MIDI note 60)
    int pitch = 60; // C4
    double stepBeats = 1.0; // 1 beat per step
    
    // Beat 1
    pattern->addNote(Note(pitch, 1.0f, 1.0 * stepBeats, stepBeats, 0));
    
    // Beat 8
    pattern->addNote(Note(pitch, 1.0f, 8.0 * stepBeats, stepBeats, 0));
    
    // Beat 11
    pattern->addNote(Note(pitch, 1.0f, 11.0 * stepBeats, stepBeats, 0));
    
    sequencer->addPattern(std::move(pattern));
    sequencer->setCurrentPattern(1); // Use index 1, not 0
    
    // Set up detailed note callbacks
    int noteOnCount = 0;
    double lastNoteTime = -1.0;
    
    sequencer->setNoteCallbacks(
        [&](int notePitch, float velocity, int channel, const Envelope& env) {
            double currentBeat = sequencer->getPositionInBeats();
            noteOnCount++;
            
            double timeSinceLastNote = (lastNoteTime >= 0) ? (currentBeat - lastNoteTime) : 0.0;
            
            std::cout << "[NOTE ON #" << noteOnCount << "] Beat " << currentBeat 
                     << " -> Pitch " << notePitch 
                     << " (time since last: " << timeSinceLastNote << " beats)" << std::endl;
            
            lastNoteTime = currentBeat;
        },
        [&](int notePitch, int channel) {
            double currentBeat = sequencer->getPositionInBeats();
            std::cout << "[NOTE OFF] Beat " << currentBeat 
                     << " -> Pitch " << notePitch << std::endl;
        }
    );
    
    // Start sequencer
    sequencer->start();
    std::cout << "\nStarting sequencer playback...\n";
    
    // Deterministically run for exactly 2 loops (2 * 16 beats) without wall-clock timing.
    const double bpm = 120.0;
    const double secPerBeat = 60.0 / bpm;
    const double beatsPerLoop = 16.0;
    const int framesPerBeat = 60;
    const int maxLoops = 2;
    const int maxFrames = (int)(beatsPerLoop * framesPerBeat * maxLoops * 1.1); // small safety margin
    const double dt = secPerBeat / framesPerBeat;
    
    double lastPosition = -1.0;
    bool firstLoopComplete = false;
    int loopsSeen = 0;
    
    for (int frame = 0; frame < maxFrames && loopsSeen < maxLoops; ++frame) {
        sequencer->process(dt);
        
        double currentPosition = sequencer->getPositionInBeats();
        if (std::abs(currentPosition - lastPosition) > 0.01) {
            if (currentPosition < lastPosition && lastPosition > 10.0) {
                // We just looped
                ++loopsSeen;
                if (!firstLoopComplete) {
                    std::cout << "\n*** FIRST LOOP COMPLETE ***" << std::endl;
                    std::cout << "Notes triggered in first loop: " << noteOnCount << std::endl;
                    firstLoopComplete = true;
                } else {
                    std::cout << "\n*** SECOND LOOP COMPLETE ***" << std::endl;
                    std::cout << "Total notes triggered: " << noteOnCount << std::endl;
                }
            }
            lastPosition = currentPosition;
        }
    }
    
    // Stop and analyze
    sequencer->stop();
    std::cout << "\nSequencer stopped." << std::endl;
    std::cout << "Final note count: " << noteOnCount << std::endl;
    
    if (noteOnCount == 6) {
        std::cout << "\n*** SUCCESS: No ghost notes detected! ***" << std::endl;
    } else if (noteOnCount > 6) {
        std::cout << "\n*** GHOST NOTES DETECTED: " << (noteOnCount - 6) << " extra notes ***" << std::endl;
    } else {
        std::cout << "\n*** MISSING NOTES: Expected 6, got " << noteOnCount << " ***" << std::endl;
    }
    
    return 0;
}
