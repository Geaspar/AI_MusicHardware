/**
 * Voice Manager Ghost Note Test
 * 
 * This test specifically investigates the voice manager's behavior
 * when handling sequencer note-on/note-off events to identify
 * potential sources of ghost notes.
 */

#include <iostream>
#include <memory>
#include <thread>
#include <chrono>
#include <vector>
#include <map>
#include <cmath>
#include <algorithm>

#include "../include/synthesis/voice/voice_manager.h"
#include "../include/sequencer/Sequencer.h"

using namespace AIMusicHardware;

class VoiceManagerMonitor {
private:
    struct VoiceActivity {
        int pitch;
        double startTime;
        double endTime;
        bool isActive;
        int voiceIndex;
    };
    
    std::vector<VoiceActivity> voiceActivities_;
    std::map<int, int> pitchToVoiceIndex_;
    double currentTime_;
    int sampleRate_;
    
public:
    VoiceManagerMonitor(int sampleRate = 44100) : currentTime_(0.0), sampleRate_(sampleRate) {}
    
    void onNoteOn(int pitch, float velocity, int channel) {
        VoiceActivity activity;
        activity.pitch = pitch;
        activity.startTime = currentTime_;
        activity.isActive = true;
        activity.voiceIndex = -1; // Will be filled by voice manager
        
        voiceActivities_.push_back(activity);
        
        std::cout << "[VOICE] Note ON: " << pitch << " at time " << std::fixed << std::setprecision(3) 
                  << currentTime_ << "s" << std::endl;
    }
    
    void onNoteOff(int pitch, int channel) {
        // Find the most recent active voice for this pitch
        for (auto it = voiceActivities_.rbegin(); it != voiceActivities_.rend(); ++it) {
            if (it->pitch == pitch && it->isActive) {
                it->endTime = currentTime_;
                it->isActive = false;
                std::cout << "[VOICE] Note OFF: " << pitch << " at time " << std::fixed << std::setprecision(3) 
                          << currentTime_ << "s (duration: " << (currentTime_ - it->startTime) << "s)" << std::endl;
                break;
            }
        }
    }
    
    void updateTime(double deltaTime) {
        currentTime_ += deltaTime;
    }
    
    void checkForOverlappingVoices() {
        std::cout << "\n=== VOICE OVERLAP ANALYSIS ===" << std::endl;
        
        std::map<int, std::vector<VoiceActivity*>> pitches;
        for (auto& activity : voiceActivities_) {
            pitches[activity.pitch].push_back(&activity);
        }
        
        bool hasOverlaps = false;
        for (const auto& pair : pitches) {
            int pitch = pair.first;
            const auto& activities = pair.second;
            
            std::cout << "Pitch " << pitch << " activities:" << std::endl;
            for (size_t i = 0; i < activities.size(); ++i) {
                const auto& act = *activities[i];
                std::cout << "  " << i << ": " << (act.isActive ? "ACTIVE" : "ENDED") 
                          << " from " << std::fixed << std::setprecision(3) << act.startTime 
                          << "s to " << act.endTime << "s" << std::endl;
                
                // Check for overlaps with previous activities
                for (size_t j = 0; j < i; ++j) {
                    const auto& prevAct = *activities[j];
                    if (prevAct.isActive || prevAct.endTime > act.startTime) {
                        hasOverlaps = true;
                        std::cout << "    ⚠️  OVERLAP with activity " << j << std::endl;
                    }
                }
            }
        }
        
        if (hasOverlaps) {
            std::cout << "\n❌ VOICE OVERLAPS DETECTED - This could cause ghost notes!" << std::endl;
        } else {
            std::cout << "\n✅ No voice overlaps detected" << std::endl;
        }
    }
    
    void printSummary() {
        std::cout << "\n=== VOICE MANAGER SUMMARY ===" << std::endl;
        std::cout << "Total voice activities: " << voiceActivities_.size() << std::endl;
        
        int activeCount = 0;
        for (const auto& activity : voiceActivities_) {
            if (activity.isActive) activeCount++;
        }
        std::cout << "Currently active voices: " << activeCount << std::endl;
        
        checkForOverlappingVoices();
    }
};

int main() {
    std::cout << "=== VOICE MANAGER GHOST NOTE TEST ===" << std::endl;
    std::cout << "Testing voice manager behavior with sequencer events..." << std::endl;
    
    // Create voice manager and sequencer
    auto voiceManager = std::make_unique<VoiceManager>(44100, 8);
    auto sequencer = std::make_unique<Sequencer>(44100);
    
    // Create monitor
    VoiceManagerMonitor monitor(44100);
    
    // Set up sequencer callbacks that monitor voice manager
    sequencer->setNoteCallbacks(
        [&](int pitch, float velocity, int channel, const Envelope& env) {
            monitor.onNoteOn(pitch, velocity, channel);
            voiceManager->noteOn(pitch, velocity, channel);
        },
        [&](int pitch, int channel) {
            monitor.onNoteOff(pitch, channel);
            voiceManager->noteOff(pitch, channel);
        }
    );
    
    // Create test pattern: C4 at beats 1, 8, and 11
    auto pattern = std::make_unique<Pattern>();
    pattern->setName("Voice Manager Test");
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
    
    std::cout << "Running voice manager test for 2 loops..." << std::endl;
    
    // Process for 2 loops
    const int samplesPerBuffer = 64;
    const double beatsPerLoop = 16.0;
    const double bpm = 120.0;
    const double samplesPerBeat = (44100.0 * 60.0) / bpm;
    const int totalSamples = static_cast<int>(2 * beatsPerLoop * samplesPerBeat);
    
    for (int sample = 0; sample < totalSamples; sample += samplesPerBuffer) {
        sequencer->process(samplesPerBuffer);
        
        // Update monitor time
        double deltaTime = samplesPerBuffer / 44100.0;
        monitor.updateTime(deltaTime);
        
        // Check for loop completion
        double currentBeat = sequencer->getPrecisePositionInBeats();
        static double lastBeat = 0.0;
        if (currentBeat < lastBeat) {
            std::cout << "[VOICE] Loop completed at beat " << std::fixed << std::setprecision(2) 
                      << currentBeat << std::endl;
        }
        lastBeat = currentBeat;
        
        std::this_thread::sleep_for(std::chrono::microseconds(100));
    }
    
    sequencer->stop();
    
    // Analyze results
    monitor.printSummary();
    
    std::cout << "\n=== TEST COMPLETE ===" << std::endl;
    return 0;
}