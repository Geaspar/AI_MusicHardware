/**
 * Loop Boundary Timing Test
 * 
 * This test specifically examines the timing around loop boundaries
 * to identify why subsequent loops have timing drift.
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

class LoopBoundaryDebugger {
private:
    struct LoopEvent {
        int loopNumber;
        double timestamp;
        double beatPosition;
        int pitch;
        bool isNoteOn;
        double expectedTime;
        double timeError;
    };
    
    std::vector<LoopEvent> events_;
    double currentTime_;
    int sampleRate_;
    int currentLoop_;
    
public:
    LoopBoundaryDebugger(int sampleRate = 44100) : currentTime_(0.0), sampleRate_(sampleRate), currentLoop_(0) {}
    
    void onNoteOn(int pitch, float velocity, int channel, const Envelope& env) {
        LoopEvent event;
        event.loopNumber = currentLoop_;
        event.timestamp = currentTime_;
        event.beatPosition = -1.0; // Will be filled by sequencer position
        event.pitch = pitch;
        event.isNoteOn = true;
        event.expectedTime = -1.0; // Will be calculated
        event.timeError = -1.0;    // Will be calculated
        
        events_.push_back(event);
        
        std::cout << "[LOOP " << currentLoop_ << "] Note ON: " << pitch << " at time " 
                  << std::fixed << std::setprecision(3) << currentTime_ << "s" << std::endl;
    }
    
    void onNoteOff(int pitch, int channel) {
        LoopEvent event;
        event.loopNumber = currentLoop_;
        event.timestamp = currentTime_;
        event.beatPosition = -1.0;
        event.pitch = pitch;
        event.isNoteOn = false;
        event.expectedTime = -1.0;
        event.timeError = -1.0;
        
        events_.push_back(event);
        
        std::cout << "[LOOP " << currentLoop_ << "] Note OFF: " << pitch << " at time " 
                  << std::fixed << std::setprecision(3) << currentTime_ << "s" << std::endl;
    }
    
    void updateTime(double deltaTime) {
        currentTime_ += deltaTime;
    }
    
    void onLoopComplete() {
        currentLoop_++;
        std::cout << "[LOOP " << currentLoop_ << "] Loop completed at time " 
                  << std::fixed << std::setprecision(3) << currentTime_ << "s" << std::endl;
    }
    
    bool analyzeLoopTiming() {
        std::cout << "\n=== LOOP BOUNDARY TIMING ANALYSIS ===" << std::endl;
        
        // Calculate expected timing for 120 BPM
        const double bpm = 120.0;
        const double beatsPerSecond = bpm / 60.0;
        const double secondsPerBeat = 1.0 / beatsPerSecond;
        const double loopDuration = 16.0 * secondsPerBeat; // 16 beats per loop
        
        std::cout << "Loop duration: " << std::fixed << std::setprecision(3) << loopDuration << "s" << std::endl;
        
        // Group events by loop
        std::map<int, std::vector<LoopEvent*>> eventsByLoop;
        for (auto& event : events_) {
            if (event.isNoteOn) {
                eventsByLoop[event.loopNumber].push_back(&event);
            }
        }
        
        std::cout << "\nAnalyzing each loop:" << std::endl;
        
        for (const auto& pair : eventsByLoop) {
            int loopNum = pair.first;
            const auto& loopEvents = pair.second;
            
            std::cout << "\nLoop " << loopNum << ":" << std::endl;
            
            // Calculate expected times for this loop
            double loopStartTime = loopNum * loopDuration;
            std::vector<double> expectedBeats = {1.0, 8.0, 11.0};
            
            for (size_t i = 0; i < loopEvents.size() && i < expectedBeats.size(); ++i) {
                double expectedBeat = expectedBeats[i];
                double expectedTime = loopStartTime + (expectedBeat * secondsPerBeat);
                double actualTime = loopEvents[i]->timestamp;
                double timeError = actualTime - expectedTime;
                
                std::cout << "  Beat " << expectedBeat << ": expected " << std::fixed << std::setprecision(3) 
                          << expectedTime << "s, actual " << actualTime << "s, error " 
                          << timeError << "s" << std::endl;
                
                loopEvents[i]->expectedTime = expectedTime;
                loopEvents[i]->timeError = timeError;
            }
        }
        
        // Check for timing drift
        std::cout << "\nTiming drift analysis:" << std::endl;
        for (const auto& pair : eventsByLoop) {
            int loopNum = pair.first;
            const auto& loopEvents = pair.second;
            
            if (loopNum > 0) { // Skip first loop as reference
                double avgError = 0.0;
                for (const auto& event : loopEvents) {
                    avgError += event->timeError;
                }
                avgError /= loopEvents.size();
                
                std::cout << "Loop " << loopNum << " average timing error: " 
                          << std::fixed << std::setprecision(3) << avgError << "s" << std::endl;
                
                if (std::abs(avgError) > 0.1) { // More than 100ms error
                    std::cout << "  ❌ Significant timing drift detected!" << std::endl;
                } else {
                    std::cout << "  ✅ Timing appears correct" << std::endl;
                }
            }
        }
        
        // Count notes per loop
        std::cout << "\nNotes per loop:" << std::endl;
        for (const auto& pair : eventsByLoop) {
            int loopNum = pair.first;
            int noteCount = pair.second.size();
            std::cout << "Loop " << loopNum << ": " << noteCount << " notes" << std::endl;
            
            if (noteCount != 3) {
                std::cout << "  ❌ Expected 3 notes, got " << noteCount << std::endl;
            }
        }
        
        return false; // No ghost notes if we get here
    }
};

int main() {
    std::cout << "=== LOOP BOUNDARY TIMING TEST ===" << std::endl;
    std::cout << "Analyzing timing around loop boundaries..." << std::endl;
    
    // Create sequencer and debugger
    auto sequencer = std::make_unique<Sequencer>(44100);
    LoopBoundaryDebugger debugger(44100);
    
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
    pattern->setName("Loop Boundary Test");
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
    std::cout << "Running loop boundary analysis for 3 loops..." << std::endl;
    
    // Process for 3 loops
    const int samplesPerBuffer = 64;
    const double beatsPerLoop = 16.0;
    const double bpm = 120.0;
    const double samplesPerBeat = (44100.0 * 60.0) / bpm;
    const int totalSamples = static_cast<int>(3 * beatsPerLoop * samplesPerBeat);
    
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
            debugger.onLoopComplete();
        }
        lastBeat = currentBeat;
        
        std::this_thread::sleep_for(std::chrono::microseconds(100));
    }
    
    sequencer->stop();
    
    // Analyze loop timing
    debugger.analyzeLoopTiming();
    
    std::cout << "\n=== TEST COMPLETE ===" << std::endl;
    return 0;
}