/**
 * Minimal Ghost Note Test
 * 
 * This test focuses on the core sequencer behavior without complex dependencies.
 * It simulates the exact scenario reported by the user and checks for extra notes.
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

class SimpleNoteTracker {
private:
    struct NoteEvent {
        double timestamp;
        int pitch;
        float velocity;
        bool isNoteOn;
        double beatPosition;
    };
    
    std::vector<NoteEvent> events_;
    double currentTime_;
    int sampleRate_;
    
public:
    SimpleNoteTracker(int sampleRate = 44100) : currentTime_(0.0), sampleRate_(sampleRate) {}
    
    void onNoteOn(int pitch, float velocity, int channel, const Envelope& env) {
        NoteEvent event;
        event.timestamp = currentTime_;
        event.pitch = pitch;
        event.velocity = velocity;
        event.isNoteOn = true;
        event.beatPosition = -1.0; // Will be filled by sequencer position
        
        events_.push_back(event);
        
        std::cout << "[TRACKER] Note ON: " << pitch << " velocity " << velocity 
                  << " at time " << std::fixed << std::setprecision(3) << currentTime_ << "s" << std::endl;
    }
    
    void onNoteOff(int pitch, int channel) {
        NoteEvent event;
        event.timestamp = currentTime_;
        event.pitch = pitch;
        event.velocity = 0.0f;
        event.isNoteOn = false;
        event.beatPosition = -1.0;
        
        events_.push_back(event);
        
        std::cout << "[TRACKER] Note OFF: " << pitch 
                  << " at time " << std::fixed << std::setprecision(3) << currentTime_ << "s" << std::endl;
    }
    
    void updateTime(double deltaTime) {
        currentTime_ += deltaTime;
    }
    
    bool analyzeResults() {
        std::cout << "\n=== NOTE TRACKING ANALYSIS ===" << std::endl;
        std::cout << "Total events: " << events_.size() << std::endl;
        
        // Count note-ons per pitch
        std::map<int, int> pitchCounts;
        for (const auto& event : events_) {
            if (event.isNoteOn) {
                pitchCounts[event.pitch]++;
            }
        }
        
        std::cout << "\nNote counts by pitch:" << std::endl;
        for (const auto& pair : pitchCounts) {
            std::cout << "  Pitch " << pair.first << ": " << pair.second << " notes" << std::endl;
        }
        
        // Check for unexpected notes
        bool hasGhostNotes = false;
        for (const auto& pair : pitchCounts) {
            // We expect exactly 3 notes per loop, so for 3 loops we expect 9 total notes
            int expectedTotalNotes = 3 * 3; // 3 loops × 3 notes per loop
            if (pair.second > expectedTotalNotes) {
                hasGhostNotes = true;
                std::cout << "❌ GHOST NOTE: Pitch " << pair.first 
                          << " has " << pair.second << " notes (expected " << expectedTotalNotes << ")" << std::endl;
            } else if (pair.second == expectedTotalNotes) {
                std::cout << "✅ Pitch " << pair.first << " has correct number of notes: " 
                          << pair.second << " (3 per loop × 3 loops)" << std::endl;
            }
        }
        
        if (!hasGhostNotes) {
            std::cout << "✅ No ghost notes detected" << std::endl;
        }
        
        // Show timeline
        std::cout << "\nEvent timeline:" << std::endl;
        for (const auto& event : events_) {
            std::cout << "  " << (event.isNoteOn ? "ON " : "OFF") 
                      << " Pitch " << event.pitch 
                      << " at " << std::fixed << std::setprecision(3) << event.timestamp << "s" << std::endl;
        }
        
        return hasGhostNotes;
    }
};

int main() {
    std::cout << "=== MINIMAL GHOST NOTE TEST ===" << std::endl;
    std::cout << "Testing sequencer with C4 at beats 1, 8, and 11..." << std::endl;
    
    // Create sequencer and tracker
    auto sequencer = std::make_unique<Sequencer>(44100);
    SimpleNoteTracker tracker(44100);
    
    // Initialize sequencer
    if (!sequencer->initialize()) {
        std::cout << "❌ Failed to initialize sequencer!" << std::endl;
        return 1;
    }
    std::cout << "✅ Sequencer initialized successfully" << std::endl;
    
    // Set up sequencer callbacks
    sequencer->setNoteCallbacks(
        [&](int pitch, float velocity, int channel, const Envelope& env) {
            tracker.onNoteOn(pitch, velocity, channel, env);
        },
        [&](int pitch, int channel) {
            tracker.onNoteOff(pitch, channel);
        }
    );
    
    // Create test pattern: C4 at beats 1, 8, and 11
    auto pattern = std::make_unique<Pattern>();
    pattern->setName("Minimal Ghost Test");
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
    std::cout << "Running test for 3 loops (48 beats)..." << std::endl;
    
    // Process for 3 loops
    const int samplesPerBuffer = 64;
    const double beatsPerLoop = 16.0;
    const double bpm = 120.0;
    const double samplesPerBeat = (44100.0 * 60.0) / bpm;
    const int totalSamples = static_cast<int>(3 * beatsPerLoop * samplesPerBeat);
    
    int loopCount = 0;
    int processCount = 0;
    for (int sample = 0; sample < totalSamples; sample += samplesPerBuffer) {
        // Convert samples to time in seconds
        double deltaTime = samplesPerBuffer / 44100.0;
        sequencer->process(deltaTime);
        processCount++;
        
        // Update tracker time
        tracker.updateTime(deltaTime);
        
        // Check for loop completion
        double currentBeat = sequencer->getPrecisePositionInBeats();
        static double lastBeat = 0.0;
        if (currentBeat < lastBeat) {
            loopCount++;
            std::cout << "[TRACKER] Loop " << loopCount << " completed at beat " 
                      << std::fixed << std::setprecision(2) << currentBeat << std::endl;
        }
        lastBeat = currentBeat;
        
        // Debug output every 1000 processes
        if (processCount % 1000 == 0) {
            std::cout << "[DEBUG] Processed " << processCount << " buffers, current beat: " 
                      << std::fixed << std::setprecision(2) << currentBeat << std::endl;
        }
        
        std::this_thread::sleep_for(std::chrono::microseconds(100));
    }
    
    sequencer->stop();
    
    // Analyze results
    bool hasGhostNotes = tracker.analyzeResults();
    
    std::cout << "\n=== TEST COMPLETE ===" << std::endl;
    if (hasGhostNotes) {
        std::cout << "❌ GHOST NOTES DETECTED - Issue not resolved!" << std::endl;
        return 1;
    } else {
        std::cout << "✅ No ghost notes detected - Issue appears resolved!" << std::endl;
        return 0;
    }
}