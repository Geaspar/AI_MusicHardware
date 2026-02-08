/**
 * Envelope Retrigger Test
 * 
 * This test examines what happens when the same note is triggered
 * multiple times with different envelope parameters, which might
 * be causing the ghost note issue.
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

class EnvelopeRetriggerDebugger {
private:
    struct RetriggerEvent {
        double timestamp;
        int pitch;
        float velocity;
        bool isNoteOn;
        double beatPosition;
        Envelope envelope;
        int triggerCount;
    };
    
    std::vector<RetriggerEvent> events_;
    double currentTime_;
    int sampleRate_;
    std::map<int, int> pitchTriggerCounts_;
    
public:
    EnvelopeRetriggerDebugger(int sampleRate = 44100) : currentTime_(0.0), sampleRate_(sampleRate) {}
    
    void onNoteOn(int pitch, float velocity, int channel, const Envelope& env) {
        RetriggerEvent event;
        event.timestamp = currentTime_;
        event.pitch = pitch;
        event.velocity = velocity;
        event.isNoteOn = true;
        event.beatPosition = -1.0; // Will be filled by sequencer position
        event.envelope = env;
        
        // Count triggers for this pitch
        pitchTriggerCounts_[pitch]++;
        event.triggerCount = pitchTriggerCounts_[pitch];
        
        events_.push_back(event);
        
        std::cout << "[RETRIGGER] Note ON: " << pitch << " velocity " << velocity 
                  << " (trigger #" << event.triggerCount << ") at time " 
                  << std::fixed << std::setprecision(3) << currentTime_ << "s" << std::endl;
        std::cout << "  Envelope: A=" << std::fixed << std::setprecision(3) << env.attack 
                  << "s D=" << env.decay << "s S=" << env.sustain 
                  << " R=" << env.release << "s" << std::endl;
    }
    
    void onNoteOff(int pitch, int channel) {
        RetriggerEvent event;
        event.timestamp = currentTime_;
        event.pitch = pitch;
        event.velocity = 0.0f;
        event.isNoteOn = false;
        event.beatPosition = -1.0;
        event.envelope = Envelope(); // Default envelope
        event.triggerCount = 0;
        
        events_.push_back(event);
        
        std::cout << "[RETRIGGER] Note OFF: " << pitch 
                  << " at time " << std::fixed << std::setprecision(3) << currentTime_ << "s" << std::endl;
    }
    
    void updateTime(double deltaTime) {
        currentTime_ += deltaTime;
    }
    
    bool analyzeRetriggerBehavior() {
        std::cout << "\n=== ENVELOPE RETRIGGER ANALYSIS ===" << std::endl;
        
        // Analyze trigger counts
        std::cout << "\nTrigger counts by pitch:" << std::endl;
        for (const auto& pair : pitchTriggerCounts_) {
            int pitch = pair.first;
            int count = pair.second;
            std::cout << "  Pitch " << pitch << ": " << count << " triggers" << std::endl;
            
            if (count > 3) {
                std::cout << "    ❌ TOO MANY TRIGGERS: Expected 3 per loop" << std::endl;
            } else if (count == 3) {
                std::cout << "    ✅ Correct number of triggers" << std::endl;
            }
        }
        
        // Analyze envelope consistency
        std::cout << "\nEnvelope consistency analysis:" << std::endl;
        std::map<int, std::vector<Envelope>> envelopesByPitch;
        
        for (const auto& event : events_) {
            if (event.isNoteOn) {
                envelopesByPitch[event.pitch].push_back(event.envelope);
            }
        }
        
        for (const auto& pair : envelopesByPitch) {
            int pitch = pair.first;
            const auto& envelopes = pair.second;
            
            std::cout << "\nPitch " << pitch << " envelopes:" << std::endl;
            
            // Check if all envelopes are the same
            bool allSame = true;
            if (envelopes.size() > 1) {
                const Envelope& first = envelopes[0];
                for (size_t i = 1; i < envelopes.size(); ++i) {
                    const Envelope& current = envelopes[i];
                    if (current.attack != first.attack || 
                        current.decay != first.decay || 
                        current.sustain != first.sustain || 
                        current.release != first.release) {
                        allSame = false;
                        break;
                    }
                }
            }
            
            if (allSame) {
                std::cout << "  ✅ All envelopes are identical" << std::endl;
                if (!envelopes.empty()) {
                    const Envelope& env = envelopes[0];
                    std::cout << "    A=" << std::fixed << std::setprecision(3) << env.attack 
                              << "s D=" << env.decay << "s S=" << env.sustain 
                              << " R=" << env.release << "s" << std::endl;
                }
            } else {
                std::cout << "  ❌ Envelopes are different!" << std::endl;
                for (size_t i = 0; i < envelopes.size(); ++i) {
                    const Envelope& env = envelopes[i];
                    std::cout << "    Trigger " << (i+1) << ": A=" << std::fixed << std::setprecision(3) 
                              << env.attack << "s D=" << env.decay << "s S=" << env.sustain 
                              << " R=" << env.release << "s" << std::endl;
                }
            }
        }
        
        // Check for potential envelope retrigger issues
        std::cout << "\nPotential retrigger issues:" << std::endl;
        bool hasIssues = false;
        
        for (const auto& pair : pitchTriggerCounts_) {
            int pitch = pair.first;
            int count = pair.second;
            
            if (count > 3) {
                std::cout << "  ❌ Pitch " << pitch << " triggered " << count 
                          << " times - this could cause envelope conflicts" << std::endl;
                hasIssues = true;
            }
        }
        
        if (!hasIssues) {
            std::cout << "  ✅ No obvious retrigger issues detected" << std::endl;
        }
        
        return hasIssues;
    }
};

int main() {
    std::cout << "=== ENVELOPE RETRIGGER TEST ===" << std::endl;
    std::cout << "Testing envelope behavior with repeated note triggers..." << std::endl;
    
    // Create sequencer and debugger
    auto sequencer = std::make_unique<Sequencer>(44100);
    EnvelopeRetriggerDebugger debugger(44100);
    
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
    pattern->setName("Envelope Retrigger Test");
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
    std::cout << "Running retrigger analysis for 2 loops..." << std::endl;
    
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
            std::cout << "[RETRIGGER] Loop " << loopCount << " completed at beat " 
                      << std::fixed << std::setprecision(2) << currentBeat << std::endl;
        }
        lastBeat = currentBeat;
        
        std::this_thread::sleep_for(std::chrono::microseconds(100));
    }
    
    sequencer->stop();
    
    // Analyze retrigger behavior
    bool hasRetriggerIssues = debugger.analyzeRetriggerBehavior();
    
    std::cout << "\n=== TEST COMPLETE ===" << std::endl;
    if (hasRetriggerIssues) {
        std::cout << "❌ ENVELOPE RETRIGGER ISSUES DETECTED!" << std::endl;
        return 1;
    } else {
        std::cout << "✅ No envelope retrigger issues detected" << std::endl;
        return 0;
    }
}