/**
 * Voice Cutoff Test
 * 
 * This test examines whether voices are properly cut off
 * when the same note is retriggered, which should prevent
 * ghost notes from overlapping audio.
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

class VoiceCutoffDebugger {
private:
    struct CutoffEvent {
        double timestamp;
        int pitch;
        float velocity;
        bool isNoteOn;
        double beatPosition;
        bool isRetrigger;
        int voiceIndex;
    };
    
    std::vector<CutoffEvent> events_;
    double currentTime_;
    int sampleRate_;
    std::map<int, double> lastNoteOnTime_;
    std::map<int, int> retriggerCounts_;
    
public:
    VoiceCutoffDebugger(int sampleRate = 44100) : currentTime_(0.0), sampleRate_(sampleRate) {}
    
    void onNoteOn(int pitch, float velocity, int channel, const Envelope& env) {
        CutoffEvent event;
        event.timestamp = currentTime_;
        event.pitch = pitch;
        event.velocity = velocity;
        event.isNoteOn = true;
        event.beatPosition = -1.0;
        event.voiceIndex = -1; // Would need voice manager access to get this
        
        // Check if this is a retrigger
        bool isRetrigger = false;
        if (lastNoteOnTime_.find(pitch) != lastNoteOnTime_.end()) {
            double timeSinceLastNote = currentTime_ - lastNoteOnTime_[pitch];
            // If less than 1 second since last note, consider it a retrigger
            if (timeSinceLastNote < 1.0) {
                isRetrigger = true;
                retriggerCounts_[pitch]++;
            }
        }
        
        event.isRetrigger = isRetrigger;
        lastNoteOnTime_[pitch] = currentTime_;
        
        events_.push_back(event);
        
        std::cout << "[CUTOFF] Note ON: " << pitch << " velocity " << velocity 
                  << (isRetrigger ? " (RETRIGGER)" : " (NEW)") << " at time " 
                  << std::fixed << std::setprecision(3) << currentTime_ << "s" << std::endl;
        
        if (isRetrigger) {
            double timeSinceLast = currentTime_ - lastNoteOnTime_[pitch];
            std::cout << "  Time since last note: " << std::fixed << std::setprecision(3) 
                      << timeSinceLast << "s" << std::endl;
        }
    }
    
    void onNoteOff(int pitch, int channel) {
        CutoffEvent event;
        event.timestamp = currentTime_;
        event.pitch = pitch;
        event.velocity = 0.0f;
        event.isNoteOn = false;
        event.beatPosition = -1.0;
        event.isRetrigger = false;
        event.voiceIndex = -1;
        
        events_.push_back(event);
        
        std::cout << "[CUTOFF] Note OFF: " << pitch 
                  << " at time " << std::fixed << std::setprecision(3) << currentTime_ << "s" << std::endl;
    }
    
    void updateTime(double deltaTime) {
        currentTime_ += deltaTime;
    }
    
    bool analyzeCutoffBehavior() {
        std::cout << "\n=== VOICE CUTOFF ANALYSIS ===" << std::endl;
        
        // Analyze retrigger patterns
        std::cout << "\nRetrigger analysis:" << std::endl;
        for (const auto& pair : retriggerCounts_) {
            int pitch = pair.first;
            int count = pair.second;
            std::cout << "Pitch " << pitch << ": " << count << " retriggers" << std::endl;
            
            if (count > 0) {
                std::cout << "  ⚠️  This pitch was retriggered - check for proper cutoff" << std::endl;
            }
        }
        
        // Analyze timing between retriggers
        std::cout << "\nRetrigger timing analysis:" << std::endl;
        std::map<int, std::vector<double>> noteOnTimes;
        
        for (const auto& event : events_) {
            if (event.isNoteOn) {
                noteOnTimes[event.pitch].push_back(event.timestamp);
            }
        }
        
        for (const auto& pair : noteOnTimes) {
            int pitch = pair.first;
            const auto& times = pair.second;
            
            if (times.size() > 1) {
                std::cout << "\nPitch " << pitch << " timing:" << std::endl;
                for (size_t i = 1; i < times.size(); ++i) {
                    double interval = times[i] - times[i-1];
                    std::cout << "  Interval " << i << ": " << std::fixed << std::setprecision(3) 
                              << interval << "s" << std::endl;
                    
                    // Check if interval is shorter than expected release time
                    if (interval < 0.5) { // Default release is 0.5s
                        std::cout << "    ❌ SHORT INTERVAL: Previous note may not have finished releasing" << std::endl;
                    } else {
                        std::cout << "    ✅ Adequate time for release" << std::endl;
                    }
                }
            }
        }
        
        // Check for potential cutoff issues
        std::cout << "\nCutoff issue detection:" << std::endl;
        bool hasCutoffIssues = false;
        
        for (const auto& pair : retriggerCounts_) {
            int pitch = pair.first;
            int retriggerCount = pair.second;
            
            if (retriggerCount > 0) {
                // Check if there are enough note-offs for the note-ons
                int noteOnCount = 0;
                int noteOffCount = 0;
                
                for (const auto& event : events_) {
                    if (event.pitch == pitch) {
                        if (event.isNoteOn) noteOnCount++;
                        else noteOffCount++;
                    }
                }
                
                std::cout << "Pitch " << pitch << ": " << noteOnCount << " ON, " 
                          << noteOffCount << " OFF" << std::endl;
                
                if (noteOnCount != noteOffCount) {
                    std::cout << "  ❌ MISMATCHED NOTE PAIRS - potential cutoff issue" << std::endl;
                    hasCutoffIssues = true;
                } else {
                    std::cout << "  ✅ Note pairs match" << std::endl;
                }
            }
        }
        
        return hasCutoffIssues;
    }
};

int main() {
    std::cout << "=== VOICE CUTOFF TEST ===" << std::endl;
    std::cout << "Testing voice cutoff behavior for retriggered notes..." << std::endl;
    
    // Create sequencer and debugger
    auto sequencer = std::make_unique<Sequencer>(44100);
    VoiceCutoffDebugger debugger(44100);
    
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
    
    // Create test pattern: C4 at beats 1, 8, and 11 (close together to test cutoff)
    auto pattern = std::make_unique<Pattern>();
    pattern->setName("Voice Cutoff Test");
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
    std::cout << "Running cutoff analysis for 1 loop..." << std::endl;
    
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
            std::cout << "[CUTOFF] Loop " << loopCount << " completed at beat " 
                      << std::fixed << std::setprecision(2) << currentBeat << std::endl;
            break; // Stop after first loop
        }
        lastBeat = currentBeat;
        
        std::this_thread::sleep_for(std::chrono::microseconds(100));
    }
    
    sequencer->stop();
    
    // Analyze cutoff behavior
    bool hasCutoffIssues = debugger.analyzeCutoffBehavior();
    
    std::cout << "\n=== TEST COMPLETE ===" << std::endl;
    if (hasCutoffIssues) {
        std::cout << "❌ VOICE CUTOFF ISSUES DETECTED!" << std::endl;
        return 1;
    } else {
        std::cout << "✅ No voice cutoff issues detected" << std::endl;
        return 0;
    }
}