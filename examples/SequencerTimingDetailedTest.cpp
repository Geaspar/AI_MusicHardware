/**
 * Detailed Sequencer Timing Test
 * 
 * This test examines the exact timing calculations to understand
 * why notes are being triggered at unexpected times.
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

class DetailedTimingDebugger {
private:
    struct TimingEvent {
        double timestamp;
        double beatPosition;
        int pitch;
        bool isNoteOn;
        double expectedBeatTime;
    };
    
    std::vector<TimingEvent> events_;
    double currentTime_;
    int sampleRate_;
    
public:
    DetailedTimingDebugger(int sampleRate = 44100) : currentTime_(0.0), sampleRate_(sampleRate) {}
    
    void onNoteOn(int pitch, float velocity, int channel, const Envelope& env) {
        TimingEvent event;
        event.timestamp = currentTime_;
        event.beatPosition = -1.0; // Will be filled by sequencer position
        event.pitch = pitch;
        event.isNoteOn = true;
        event.expectedBeatTime = -1.0; // Will be calculated
        
        events_.push_back(event);
        
        std::cout << "[DETAILED] Note ON: " << pitch << " at time " << std::fixed << std::setprecision(3) 
                  << currentTime_ << "s" << std::endl;
    }
    
    void onNoteOff(int pitch, int channel) {
        TimingEvent event;
        event.timestamp = currentTime_;
        event.beatPosition = -1.0;
        event.pitch = pitch;
        event.isNoteOn = false;
        event.expectedBeatTime = -1.0;
        
        events_.push_back(event);
        
        std::cout << "[DETAILED] Note OFF: " << pitch << " at time " << std::fixed << std::setprecision(3) 
                  << currentTime_ << "s" << std::endl;
    }
    
    void updateTime(double deltaTime) {
        currentTime_ += deltaTime;
    }
    
    bool analyzeDetailedTiming() {
        std::cout << "\n=== DETAILED TIMING ANALYSIS ===" << std::endl;
        
        // Calculate expected timing for 120 BPM
        const double bpm = 120.0;
        const double beatsPerSecond = bpm / 60.0;
        const double secondsPerBeat = 1.0 / beatsPerSecond;
        
        std::cout << "BPM: " << bpm << std::endl;
        std::cout << "Beats per second: " << std::fixed << std::setprecision(3) << beatsPerSecond << std::endl;
        std::cout << "Seconds per beat: " << std::fixed << std::setprecision(3) << secondsPerBeat << std::endl;
        
        std::cout << "\nExpected note times:" << std::endl;
        std::vector<double> expectedBeats = {1.0, 8.0, 11.0};
        for (double beat : expectedBeats) {
            double expectedTime = beat * secondsPerBeat;
            std::cout << "  Beat " << beat << " -> " << std::fixed << std::setprecision(3) 
                      << expectedTime << "s" << std::endl;
        }
        
        std::cout << "\nActual note times:" << std::endl;
        for (const auto& event : events_) {
            if (event.isNoteOn) {
                std::cout << "  Note ON at " << std::fixed << std::setprecision(3) 
                          << event.timestamp << "s" << std::endl;
            }
        }
        
        // Check for timing discrepancies
        std::cout << "\nTiming analysis:" << std::endl;
        for (const auto& event : events_) {
            if (event.isNoteOn) {
                // Find the closest expected beat
                double closestBeat = -1;
                double minDiff = 1000.0;
                for (double expectedBeat : expectedBeats) {
                    double expectedTime = expectedBeat * secondsPerBeat;
                    double diff = std::abs(event.timestamp - expectedTime);
                    if (diff < minDiff) {
                        minDiff = diff;
                        closestBeat = expectedBeat;
                    }
                }
                
                if (minDiff < 0.1) { // Within 100ms
                    std::cout << "  ✅ Note at " << std::fixed << std::setprecision(3) 
                              << event.timestamp << "s matches beat " << closestBeat << std::endl;
                } else {
                    std::cout << "  ❌ Note at " << std::fixed << std::setprecision(3) 
                              << event.timestamp << "s doesn't match any expected beat (closest: " 
                              << closestBeat << ", diff: " << minDiff << "s)" << std::endl;
                }
            }
        }
        
        // Count notes
        std::map<int, int> pitchCounts;
        for (const auto& event : events_) {
            if (event.isNoteOn) {
                pitchCounts[event.pitch]++;
            }
        }
        
        std::cout << "\nNote counts:" << std::endl;
        for (const auto& pair : pitchCounts) {
            std::cout << "  Pitch " << pair.first << ": " << pair.second << " notes" << std::endl;
        }
        
        bool hasGhostNotes = false;
        for (const auto& pair : pitchCounts) {
            if (pair.second > 3) {
                hasGhostNotes = true;
                std::cout << "❌ GHOST NOTE: Pitch " << pair.first 
                          << " has " << pair.second << " notes (expected 3)" << std::endl;
            }
        }
        
        return hasGhostNotes;
    }
};

int main() {
    std::cout << "=== DETAILED SEQUENCER TIMING TEST ===" << std::endl;
    std::cout << "Analyzing exact timing calculations..." << std::endl;
    
    // Create sequencer and debugger
    auto sequencer = std::make_unique<Sequencer>(44100);
    DetailedTimingDebugger debugger(44100);
    
    // Initialize sequencer
    if (!sequencer->initialize()) {
        std::cout << "❌ Failed to initialize sequencer!" << std::endl;
        return 1;
    }
    std::cout << "✅ Sequencer initialized successfully" << std::endl;
    
    // Check initial tempo
    std::cout << "Initial tempo: " << sequencer->getTempo() << " BPM" << std::endl;
    
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
    pattern->setName("Detailed Timing Test");
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
    
    std::cout << "Tempo after setting: " << sequencer->getTempo() << " BPM" << std::endl;
    
    // Start playback
    sequencer->start();
    
    std::cout << "Sequencer playing: " << (sequencer->isPlaying() ? "YES" : "NO") << std::endl;
    std::cout << "Running detailed timing analysis for 1 loop..." << std::endl;
    
    // Process for 1 loop
    const int samplesPerBuffer = 64;
    const double beatsPerLoop = 16.0;
    const double bpm = 120.0;
    const double samplesPerBeat = (44100.0 * 60.0) / bpm;
    const int totalSamples = static_cast<int>(1 * beatsPerLoop * samplesPerBeat);
    
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
            std::cout << "[DETAILED] Loop " << loopCount << " completed at beat " 
                      << std::fixed << std::setprecision(2) << currentBeat << std::endl;
            break; // Stop after first loop
        }
        lastBeat = currentBeat;
        
        std::this_thread::sleep_for(std::chrono::microseconds(100));
    }
    
    sequencer->stop();
    
    // Analyze detailed timing
    bool hasGhostNotes = debugger.analyzeDetailedTiming();
    
    std::cout << "\n=== TEST COMPLETE ===" << std::endl;
    if (hasGhostNotes) {
        std::cout << "❌ TIMING ISSUES CONFIRMED!" << std::endl;
        return 1;
    } else {
        std::cout << "✅ Timing appears correct" << std::endl;
        return 0;
    }
}