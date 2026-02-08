/**
 * Envelope Ghost Note Test
 * 
 * This test examines the envelope and voice manager behavior
 * to identify envelope-related ghost note issues.
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

#include "../include/synthesis/voice/voice_manager.h"
#include "../include/sequencer/Sequencer.h"

using namespace AIMusicHardware;

class EnvelopeDebugger {
private:
    struct EnvelopeEvent {
        double timestamp;
        int pitch;
        float velocity;
        bool isNoteOn;
        double beatPosition;
        std::string envelopeStage;
        float envelopeValue;
    };
    
    std::vector<EnvelopeEvent> events_;
    double currentTime_;
    int sampleRate_;
    
public:
    EnvelopeDebugger(int sampleRate = 44100) : currentTime_(0.0), sampleRate_(sampleRate) {}
    
    void onNoteOn(int pitch, float velocity, int channel, const Envelope& env) {
        EnvelopeEvent event;
        event.timestamp = currentTime_;
        event.pitch = pitch;
        event.velocity = velocity;
        event.isNoteOn = true;
        event.beatPosition = -1.0; // Will be filled by sequencer position
        event.envelopeStage = "Attack";
        event.envelopeValue = 0.0f;
        
        events_.push_back(event);
        
        std::cout << "[ENVELOPE] Note ON: " << pitch << " velocity " << velocity 
                  << " at time " << std::fixed << std::setprecision(3) << currentTime_ << "s" << std::endl;
    }
    
    void onNoteOff(int pitch, int channel) {
        EnvelopeEvent event;
        event.timestamp = currentTime_;
        event.pitch = pitch;
        event.velocity = 0.0f;
        event.isNoteOn = false;
        event.beatPosition = -1.0;
        event.envelopeStage = "Release";
        event.envelopeValue = 0.0f;
        
        events_.push_back(event);
        
        std::cout << "[ENVELOPE] Note OFF: " << pitch 
                  << " at time " << std::fixed << std::setprecision(3) << currentTime_ << "s" << std::endl;
    }
    
    void updateTime(double deltaTime) {
        currentTime_ += deltaTime;
    }
    
    bool analyzeEnvelopeBehavior() {
        std::cout << "\n=== ENVELOPE BEHAVIOR ANALYSIS ===" << std::endl;
        
        // Group events by pitch
        std::map<int, std::vector<EnvelopeEvent*>> eventsByPitch;
        for (auto& event : events_) {
            eventsByPitch[event.pitch].push_back(&event);
        }
        
        std::cout << "\nAnalyzing envelope behavior by pitch:" << std::endl;
        
        for (const auto& pair : eventsByPitch) {
            int pitch = pair.first;
            const auto& pitchEvents = pair.second;
            
            std::cout << "\nPitch " << pitch << " events:" << std::endl;
            
            // Check for overlapping notes (note-on without corresponding note-off)
            int noteOnCount = 0;
            int noteOffCount = 0;
            std::vector<double> noteOnTimes;
            std::vector<double> noteOffTimes;
            
            for (const auto& event : pitchEvents) {
                if (event->isNoteOn) {
                    noteOnCount++;
                    noteOnTimes.push_back(event->timestamp);
                    std::cout << "  ON  at " << std::fixed << std::setprecision(3) 
                              << event->timestamp << "s" << std::endl;
                } else {
                    noteOffCount++;
                    noteOffTimes.push_back(event->timestamp);
                    std::cout << "  OFF at " << std::fixed << std::setprecision(3) 
                              << event->timestamp << "s" << std::endl;
                }
            }
            
            std::cout << "  Total ON: " << noteOnCount << ", Total OFF: " << noteOffCount << std::endl;
            
            // Check for mismatched note-on/note-off pairs
            if (noteOnCount != noteOffCount) {
                std::cout << "  ❌ MISMATCHED NOTE PAIRS: " << noteOnCount << " ON vs " 
                          << noteOffCount << " OFF" << std::endl;
            } else {
                std::cout << "  ✅ Note pairs match" << std::endl;
            }
            
            // Check for overlapping notes (note-on before previous note-off)
            bool hasOverlaps = false;
            for (size_t i = 1; i < noteOnTimes.size(); ++i) {
                if (noteOnTimes[i] < noteOffTimes[i-1]) {
                    hasOverlaps = true;
                    std::cout << "  ❌ OVERLAPPING NOTE: ON at " << std::fixed << std::setprecision(3)
                              << noteOnTimes[i] << "s before previous OFF at " 
                              << noteOffTimes[i-1] << "s" << std::endl;
                }
            }
            
            if (!hasOverlaps && noteOnTimes.size() > 1) {
                std::cout << "  ✅ No overlapping notes detected" << std::endl;
            }
            
            // Check for envelope timing issues
            if (noteOnTimes.size() > 3) {
                std::cout << "  ❌ TOO MANY NOTES: " << noteOnTimes.size() 
                          << " notes (expected 3 per loop)" << std::endl;
            }
        }
        
        // Overall analysis
        std::cout << "\nOverall analysis:" << std::endl;
        int totalNoteOns = 0;
        int totalNoteOffs = 0;
        for (const auto& event : events_) {
            if (event.isNoteOn) totalNoteOns++;
            else totalNoteOffs++;
        }
        
        std::cout << "Total note-ons: " << totalNoteOns << std::endl;
        std::cout << "Total note-offs: " << totalNoteOffs << std::endl;
        
        if (totalNoteOns != totalNoteOffs) {
            std::cout << "❌ ENVELOPE ISSUE: Mismatched note-on/note-off counts!" << std::endl;
            return true;
        }
        
        std::cout << "✅ Envelope behavior appears correct" << std::endl;
        return false;
    }
};

int main() {
    std::cout << "=== ENVELOPE GHOST NOTE TEST ===" << std::endl;
    std::cout << "Testing envelope and voice manager behavior..." << std::endl;
    
    // Create voice manager and sequencer
    auto voiceManager = std::make_unique<VoiceManager>(44100, 8);
    auto sequencer = std::make_unique<Sequencer>(44100);
    EnvelopeDebugger debugger(44100);
    
    // Initialize sequencer
    if (!sequencer->initialize()) {
        std::cout << "❌ Failed to initialize sequencer!" << std::endl;
        return 1;
    }
    std::cout << "✅ Sequencer initialized successfully" << std::endl;
    
    // Set up sequencer callbacks that also interact with voice manager
    sequencer->setNoteCallbacks(
        [&](int pitch, float velocity, int channel, const Envelope& env) {
            debugger.onNoteOn(pitch, velocity, channel, env);
            voiceManager->noteOn(pitch, velocity, channel);
        },
        [&](int pitch, int channel) {
            debugger.onNoteOff(pitch, channel);
            voiceManager->noteOff(pitch, channel);
        }
    );
    
    // Create test pattern: C4 at beats 1, 8, and 11
    auto pattern = std::make_unique<Pattern>();
    pattern->setName("Envelope Test");
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
    std::cout << "Running envelope analysis for 2 loops..." << std::endl;
    
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
            std::cout << "[ENVELOPE] Loop " << loopCount << " completed at beat " 
                      << std::fixed << std::setprecision(2) << currentBeat << std::endl;
        }
        lastBeat = currentBeat;
        
        std::this_thread::sleep_for(std::chrono::microseconds(100));
    }
    
    sequencer->stop();
    
    // Analyze envelope behavior
    bool hasEnvelopeIssues = debugger.analyzeEnvelopeBehavior();
    
    std::cout << "\n=== TEST COMPLETE ===" << std::endl;
    if (hasEnvelopeIssues) {
        std::cout << "❌ ENVELOPE ISSUES DETECTED!" << std::endl;
        return 1;
    } else {
        std::cout << "✅ No envelope issues detected" << std::endl;
        return 0;
    }
}