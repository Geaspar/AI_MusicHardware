/**
 * Sequencer Timing Debug Test
 * 
 * This test specifically examines the note triggering logic to identify
 * the exact timing bug that causes ghost notes.
 */

#include <iostream>
#include <memory>
#include <thread>
#include <chrono>
#include <vector>
#include <map>
#include <cmath>
#include <algorithm>
#include <iomanip>

#include "../include/sequencer/Sequencer.h"

using namespace AIMusicHardware;

class TimingDebugger {
private:
    struct TimingEvent {
        double timestamp;
        double previousPosition;
        double currentPosition;
        int pitch;
        bool isNoteOn;
        double noteStartTime;
        bool isLooping;
    };
    
    std::vector<TimingEvent> events_;
    double currentTime_;
    int sampleRate_;
    
public:
    TimingDebugger(int sampleRate = 44100) : currentTime_(0.0), sampleRate_(sampleRate) {}
    
    void onNoteOn(int pitch, float velocity, int channel, const Envelope& env) {
        TimingEvent event;
        event.timestamp = currentTime_;
        event.previousPosition = -1.0; // Will be filled by sequencer position
        event.currentPosition = -1.0;   // Will be filled by sequencer position
        event.pitch = pitch;
        event.isNoteOn = true;
        event.noteStartTime = -1.0;     // Will be filled by sequencer position
        event.isLooping = false;       // Will be filled by sequencer position
        
        events_.push_back(event);
        
        std::cout << "[TIMING] Note ON: " << pitch << " at time " << std::fixed << std::setprecision(3) 
                  << currentTime_ << "s" << std::endl;
    }
    
    void onNoteOff(int pitch, int channel) {
        TimingEvent event;
        event.timestamp = currentTime_;
        event.previousPosition = -1.0;
        event.currentPosition = -1.0;
        event.pitch = pitch;
        event.isNoteOn = false;
        event.noteStartTime = -1.0;
        event.isLooping = false;
        
        events_.push_back(event);
        
        std::cout << "[TIMING] Note OFF: " << pitch << " at time " << std::fixed << std::setprecision(3) 
                  << currentTime_ << "s" << std::endl;
    }
    
    void updateTime(double deltaTime) {
        currentTime_ += deltaTime;
    }
    
    bool analyzeTimingLogic() {
        std::cout << "\n=== TIMING LOGIC ANALYSIS ===" << std::endl;
        
        // Analyze the triggering conditions
        std::cout << "Expected notes: C4 at beats 1, 8, 11" << std::endl;
        std::cout << "Pattern length: 16 beats" << std::endl;
        std::cout << "\nAnalyzing triggering conditions:" << std::endl;
        
        // Simulate the triggering logic for each note
        const double EPSILON = 1e-6;
        std::vector<double> expectedNotes = {1.0, 8.0, 11.0};
        
        std::cout << "\nFor each expected note, check if it would trigger:" << std::endl;
        for (double noteStartTime : expectedNotes) {
            std::cout << "\nNote at beat " << noteStartTime << ":" << std::endl;
            
            // Simulate the triggering conditions
            // Normal case: noteStartsInFrame = (noteStartTime >= previousPosition - EPSILON) && (noteStartTime < currentPosition)
            // Looping case: inFirst = (noteStartTime >= previousPosition - EPSILON), inSecond = (noteStartTime < currentPosition)
            
            // Test with previousPosition = -EPSILON (what happens after looping)
            double previousPosition = -EPSILON;
            double currentPosition = 1.0; // Just after loop start
            
            bool normalCase = (noteStartTime >= previousPosition - EPSILON) && (noteStartTime < currentPosition);
            bool loopingCase = (noteStartTime >= previousPosition - EPSILON) || (noteStartTime < currentPosition);
            
            std::cout << "  Normal case: " << (normalCase ? "TRIGGERS" : "NO TRIGGER") << std::endl;
            std::cout << "  Looping case: " << (loopingCase ? "TRIGGERS" : "NO TRIGGER") << std::endl;
            std::cout << "  previousPosition - EPSILON = " << std::fixed << std::setprecision(6) 
                      << (previousPosition - EPSILON) << std::endl;
            std::cout << "  noteStartTime >= " << (previousPosition - EPSILON) << " = " 
                      << (noteStartTime >= previousPosition - EPSILON) << std::endl;
        }
        
        // Count actual events
        std::map<int, int> pitchCounts;
        for (const auto& event : events_) {
            if (event.isNoteOn) {
                pitchCounts[event.pitch]++;
            }
        }
        
        std::cout << "\nActual note counts:" << std::endl;
        for (const auto& pair : pitchCounts) {
            std::cout << "  Pitch " << pair.first << ": " << pair.second << " notes" << std::endl;
        }
        
        // Check for ghost notes
        bool hasGhostNotes = false;
        for (const auto& pair : pitchCounts) {
            if (pair.second > 3) { // We expect exactly 3 notes per loop
                hasGhostNotes = true;
                std::cout << "❌ GHOST NOTE: Pitch " << pair.first 
                          << " has " << pair.second << " notes (expected 3)" << std::endl;
            }
        }
        
        if (!hasGhostNotes) {
            std::cout << "✅ No ghost notes detected" << std::endl;
        }
        
        return hasGhostNotes;
    }
};

int main() {
    std::cout << "=== SEQUENCER TIMING DEBUG TEST ===" << std::endl;
    std::cout << "Analyzing the note triggering logic for timing bugs..." << std::endl;
    
    // Create sequencer and debugger
    auto sequencer = std::make_unique<Sequencer>(44100);
    TimingDebugger debugger(44100);
    
    // Initialize sequencer
    if (!sequencer->initialize()) {
        std::cout << "❌ Failed to initialize sequencer!" << std::endl;
        return 1;
    }
    std::cout << "✅ Sequencer initialized successfully" << std::endl;
    
    // Set up sequencer callbacks
    sequencer->setNoteCallbacks(
        [&](int pitch, float velocity, int channel, const Envelope& env) {
            debugger.onNoteOn(pitch, velocity, channel, env);
        },
        [&](int pitch, int channel) {
            debugger.onNoteOff(pitch, channel);
        }
    );
    
    // Create test pattern: C4 at beats 1, 8, and 11
    auto pattern = std::make_unique<Pattern>();
    pattern->setName("Timing Debug Test");
    pattern->setLength(16.0);
    
    Note note1(60, 0.8f, 1.0, 1.0f);   // C4 at beat 1
    Note note2(60, 0.8f, 8.0, 1.0f);   // C4 at beat 8
    Note note3(60, 0.8f, 11.0, 1.0f);   // C4 at beat 11
    pattern->addNote(note1);
    pattern->addNote(note2);
    pattern->addNote(note3);
    
    sequencer->addPattern(std::move(pattern));
    sequencer->setCurrentPattern(1);
    sequencer->setLooping(true);
    sequencer->setTempo(120.0);
    
    // Start playback
    sequencer->start();
    
    std::cout << "Sequencer playing: " << (sequencer->isPlaying() ? "YES" : "NO") << std::endl;
    std::cout << "Running timing analysis for 2 loops..." << std::endl;
    
    // Process for 2 loops
    const int samplesPerBuffer = 64;
    const double beatsPerLoop = 16.0;
    const double bpm = 120.0;
    const double samplesPerBeat = (44100.0 * 60.0) / bpm;
    const int totalSamples = static_cast<int>(2 * beatsPerLoop * samplesPerBeat);
    
    int loopCount = 0;
    for (int sample = 0; sample < totalSamples; sample += samplesPerBuffer) {
        // Convert samples to time in seconds
        double deltaTime = samplesPerBuffer / 44100.0;
        sequencer->process(deltaTime);
        
        // Update debugger time
        debugger.updateTime(deltaTime);
        
        // Check for loop completion
        double currentBeat = sequencer->getPrecisePositionInBeats();
        static double lastBeat = 0.0;
        if (currentBeat < lastBeat) {
            loopCount++;
            std::cout << "[TIMING] Loop " << loopCount << " completed at beat " 
                      << std::fixed << std::setprecision(2) << currentBeat << std::endl;
        }
        lastBeat = currentBeat;
        
        std::this_thread::sleep_for(std::chrono::microseconds(100));
    }
    
    sequencer->stop();
    
    // Analyze timing logic
    bool hasGhostNotes = debugger.analyzeTimingLogic();
    
    std::cout << "\n=== TEST COMPLETE ===" << std::endl;
    if (hasGhostNotes) {
        std::cout << "❌ TIMING BUG CONFIRMED - Issue identified!" << std::endl;
        return 1;
    } else {
        std::cout << "✅ No timing issues detected" << std::endl;
        return 0;
    }
}