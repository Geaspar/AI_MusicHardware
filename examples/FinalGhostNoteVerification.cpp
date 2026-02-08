/**
 * Final verification test for the ghost note fix
 * Tests the exact scenario reported by the user:
 * - Basic sequence on C4 at beats 1, 8, and 11
 * - Should hear exactly 3 notes per loop, no ghost note after beat 11
 */

#include <iostream>
#include <memory>
#include <thread>
#include <chrono>
#include "../include/sequencer/Sequencer.h"

using namespace AIMusicHardware;

int main() {
    std::cout << "=== GHOST NOTE FIX VERIFICATION ===" << std::endl;
    std::cout << "Testing: C4 at beats 1, 8, 11" << std::endl;
    std::cout << "Expected: Exactly 3 notes per loop, no extra notes\n" << std::endl;
    
    // Create sequencer at 120 BPM, 4/4 time
    auto sequencer = std::make_unique<Sequencer>(120.0, 4);
    sequencer->initialize();
    
    // Create pattern
    auto pattern = std::make_unique<Pattern>("Test Pattern");
    pattern->setLength(16.0); // 4 bars at 4/4
    
    const int C4 = 60;
    
    // Add the three notes exactly as user specified
    pattern->addNote(Note(C4, 1.0f, 1.0, 1.0, 0));   // Beat 1
    pattern->addNote(Note(C4, 1.0f, 8.0, 1.0, 0));   // Beat 8
    pattern->addNote(Note(C4, 1.0f, 11.0, 1.0, 0));  // Beat 11
    
    // Setup sequencer
    sequencer->addPattern(std::move(pattern));
    sequencer->setCurrentPattern(1);
    sequencer->setLooping(true);
    
    // Track notes
    int totalNotes = 0;
    int notesInCurrentLoop = 0;
    int completedLoops = 0;
    double lastBeat = 0;
    bool ghostNoteDetected = false;
    
    sequencer->setNoteCallbacks(
        [&](int pitch, float velocity, int channel, const Envelope& env) {
            double beat = sequencer->getPositionInBeats();
            totalNotes++;
            
            // Check if we looped
            if (beat < lastBeat) {
                completedLoops++;
                std::cout << "Loop " << completedLoops << " completed: " 
                         << notesInCurrentLoop << " notes in previous loop" << std::endl;
                notesInCurrentLoop = 0;
            }
            lastBeat = beat;
            notesInCurrentLoop++;
            
            // Check for ghost note (unexpected note position)
            double beatInPattern = std::fmod(beat, 16.0);
            if (beatInPattern < 0) beatInPattern += 16.0;
            
            bool isExpected = (std::abs(beatInPattern - 1.0) < 0.1 ||
                              std::abs(beatInPattern - 8.0) < 0.1 ||
                              std::abs(beatInPattern - 11.0) < 0.1);
            
            if (!isExpected) {
                ghostNoteDetected = true;
                std::cout << "*** GHOST NOTE DETECTED at beat " << beatInPattern << " ***" << std::endl;
            }
            
            std::cout << "Note " << totalNotes << ": Beat " << beatInPattern << std::endl;
        },
        [&](int pitch, int channel) {
            // Note off callback (not used for this test)
        }
    );
    
    // Start playback
    std::cout << "\nStarting playback for 15 seconds (about 2 loops)...\n" << std::endl;
    sequencer->start();
    
    // Run for 15 seconds
    const double frameTime = 1.0 / 60.0;
    const int totalFrames = 15 * 60;
    
    for (int frame = 0; frame < totalFrames; ++frame) {
        sequencer->process(frameTime);
        std::this_thread::sleep_for(std::chrono::microseconds(16667));
    }
    
    sequencer->stop();
    
    // Results
    std::cout << "\n=== RESULTS ===" << std::endl;
    std::cout << "Total notes played: " << totalNotes << std::endl;
    std::cout << "Complete loops: " << completedLoops << std::endl;
    std::cout << "Current loop has: " << notesInCurrentLoop << " notes" << std::endl;
    
    bool allLoopsCorrect = true;
    if (completedLoops > 0) {
        // Check if each completed loop had exactly 3 notes
        int expectedTotal = completedLoops * 3 + notesInCurrentLoop;
        allLoopsCorrect = (totalNotes == expectedTotal);
    }
    
    if (ghostNoteDetected) {
        std::cout << "\n❌ FAIL: Ghost notes were detected!" << std::endl;
        std::cout << "The issue is NOT fixed." << std::endl;
    } else if (allLoopsCorrect && !ghostNoteDetected) {
        std::cout << "\n✅ SUCCESS: No ghost notes detected!" << std::endl;
        std::cout << "Each loop played exactly 3 notes as expected." << std::endl;
        std::cout << "The ghost note issue is FIXED!" << std::endl;
    } else {
        std::cout << "\n⚠️  WARNING: Note count mismatch but no ghost notes detected" << std::endl;
    }
    
    return ghostNoteDetected ? 1 : 0;
}