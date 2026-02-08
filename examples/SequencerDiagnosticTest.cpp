#include <iostream>
#include <vector>
#include <memory>
#include <chrono>
#include <thread>
#include "../../include/sequencer/Sequencer.h"

using namespace AIMusicHardware;

int main() {
    std::cout << "=== SEQUENCER DIAGNOSTIC TEST ===" << std::endl;
    
    // Create sequencer
    auto sequencer = std::make_unique<Sequencer>(120.0, 4); // 120 BPM, 4/4 time
    std::cout << "Sequencer created with tempo: " << sequencer->getTempo() << " BPM" << std::endl;
    
    // Initialize
    bool initResult = sequencer->initialize();
    std::cout << "Sequencer initialization: " << (initResult ? "SUCCESS" : "FAILED") << std::endl;
    
    // Check initial state
    std::cout << "Initial state - Playing: " << (sequencer->isPlaying() ? "YES" : "NO") << std::endl;
    std::cout << "Initial position: " << sequencer->getPositionInBeats() << " beats" << std::endl;
    std::cout << "Number of patterns: " << sequencer->getNumPatterns() << std::endl;
    
    // Create test pattern
    auto pattern = std::make_unique<Pattern>("Diagnostic Test Pattern");
    pattern->setLength(16.0); // 4 bars
    std::cout << "Pattern created with length: " << pattern->getLength() << " beats" << std::endl;
    
    // Add notes at beats 1, 8, 11 (C4 = MIDI note 60)
    int pitch = 60; // C4
    double stepBeats = 1.0; // 1 beat per step
    
    std::cout << "Adding notes..." << std::endl;
    
    // Beat 1
    Note note1(pitch, 1.0f, 1.0 * stepBeats, stepBeats, 0);
    pattern->addNote(note1);
    std::cout << "Added note 1 at beat " << note1.startTime << ", pitch " << note1.pitch << std::endl;
    
    // Beat 8
    Note note2(pitch, 1.0f, 8.0 * stepBeats, stepBeats, 0);
    pattern->addNote(note2);
    std::cout << "Added note 2 at beat " << note2.startTime << ", pitch " << note2.pitch << std::endl;
    
    // Beat 11
    Note note3(pitch, 1.0f, 11.0 * stepBeats, stepBeats, 0);
    pattern->addNote(note3);
    std::cout << "Added note 3 at beat " << note3.startTime << ", pitch " << note3.pitch << std::endl;
    
    std::cout << "Pattern has " << pattern->getNumNotes() << " notes" << std::endl;
    
    // Add pattern to sequencer
    sequencer->addPattern(std::move(pattern));
    std::cout << "Pattern added to sequencer. Total patterns: " << sequencer->getNumPatterns() << std::endl;
    
    // Set current pattern
    sequencer->setCurrentPattern(0);
    std::cout << "Current pattern index: " << sequencer->getCurrentPatternIndex() << std::endl;
    
    // Get the pattern back and verify
    Pattern* currentPattern = sequencer->getPattern(0);
    if (currentPattern) {
        std::cout << "Current pattern name: " << currentPattern->getName() << std::endl;
        std::cout << "Current pattern length: " << currentPattern->getLength() << " beats" << std::endl;
        std::cout << "Current pattern notes: " << currentPattern->getNumNotes() << std::endl;
        
        // List all notes in the pattern
        for (size_t i = 0; i < currentPattern->getNumNotes(); ++i) {
            const Note* note = currentPattern->getNote(i);
            if (note) {
                std::cout << "  Note " << i << ": pitch=" << note->pitch 
                         << ", start=" << note->startTime 
                         << ", duration=" << note->duration << std::endl;
            }
        }
    } else {
        std::cout << "ERROR: Could not retrieve current pattern!" << std::endl;
        return 1;
    }
    
    // Set up note callbacks
    int noteOnCount = 0;
    int noteOffCount = 0;
    
    sequencer->setNoteCallbacks(
        [&](int notePitch, float velocity, int channel, const Envelope& env) {
            double currentBeat = sequencer->getPositionInBeats();
            noteOnCount++;
            std::cout << "[NOTE ON #" << noteOnCount << "] Beat " << currentBeat 
                     << " -> Pitch " << notePitch 
                     << " (velocity " << velocity << ", channel " << channel << ")" << std::endl;
        },
        [&](int notePitch, int channel) {
            double currentBeat = sequencer->getPositionInBeats();
            noteOffCount++;
            std::cout << "[NOTE OFF #" << noteOffCount << "] Beat " << currentBeat 
                     << " -> Pitch " << notePitch << " (channel " << channel << ")" << std::endl;
        }
    );
    
    // Start sequencer
    std::cout << "\nStarting sequencer..." << std::endl;
    sequencer->start();
    std::cout << "Sequencer started. Playing: " << (sequencer->isPlaying() ? "YES" : "NO") << std::endl;
    
    // Run for a few seconds with detailed logging
    auto startTime = std::chrono::high_resolution_clock::now();
    auto endTime = startTime + std::chrono::seconds(10);
    
    int frameCount = 0;
    double lastPosition = -1.0;
    
    while (std::chrono::high_resolution_clock::now() < endTime) {
        // Process sequencer at 60 FPS
        sequencer->process(1.0 / 60.0);
        
        // Check for position changes
        double currentPosition = sequencer->getPositionInBeats();
        if (std::abs(currentPosition - lastPosition) > 0.01) {
            std::cout << "[POS] Beat " << currentPosition << std::endl;
            lastPosition = currentPosition;
        }
        
        // Print detailed info every 2 seconds
        if (frameCount % 120 == 0) { // Every 2 seconds at 60 FPS
            std::cout << "[STATUS] Frame " << frameCount 
                     << ", Position: " << currentPosition 
                     << ", Notes ON: " << noteOnCount 
                     << ", Notes OFF: " << noteOffCount << std::endl;
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
        frameCount++;
    }
    
    // Stop and final report
    sequencer->stop();
    std::cout << "\nSequencer stopped." << std::endl;
    std::cout << "Final position: " << sequencer->getPositionInBeats() << " beats" << std::endl;
    std::cout << "Total note-ons: " << noteOnCount << std::endl;
    std::cout << "Total note-offs: " << noteOffCount << std::endl;
    
    if (noteOnCount == 0) {
        std::cout << "\n*** DIAGNOSIS: No notes were triggered! ***" << std::endl;
        std::cout << "Possible causes:" << std::endl;
        std::cout << "1. Pattern notes are not at the expected beat positions" << std::endl;
        std::cout << "2. Sequencer timing is not advancing properly" << std::endl;
        std::cout << "3. Note detection logic has an issue" << std::endl;
    } else if (noteOnCount > 3) {
        std::cout << "\n*** DIAGNOSIS: More notes than expected - possible ghost notes! ***" << std::endl;
    } else {
        std::cout << "\n*** DIAGNOSIS: Expected number of notes detected ***" << std::endl;
    }
    
    return 0;
}


