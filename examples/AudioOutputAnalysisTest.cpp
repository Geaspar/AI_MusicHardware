/**
 * Audio Output Analysis Test for Ghost Note Investigation
 * 
 * This test analyzes the actual audio output from the synthesizer to detect
 * ghost notes that might not be visible in sequencer-only diagnostic tests.
 * 
 * Key differences from previous tests:
 * 1. Uses the full synthesizer stack (not just sequencer callbacks)
 * 2. Analyzes actual audio samples for unexpected notes
 * 3. Tracks voice states and envelope phases
 * 4. Detects overlapping notes that shouldn't be there
 */

#include <iostream>
#include <memory>
#include <thread>
#include <chrono>
#include <vector>
#include <map>
#include <cmath>
#include <algorithm>
#include <fstream>
#include <iomanip>

// Core systems
#include "../include/audio/Synthesizer.h"
#include "../include/sequencer/Sequencer.h"
#include "../include/synthesis/voice/voice_manager.h"

using namespace AIMusicHardware;

class AudioAnalyzer {
private:
    struct NoteEvent {
        double timestamp;
        int pitch;
        float velocity;
        bool isNoteOn;
        double beatPosition;
    };
    
    struct VoiceState {
        int pitch;
        double startTime;
        double endTime;
        bool isActive;
        float currentLevel;
        double envelopePhase; // 0=attack, 1=decay, 2=sustain, 3=release
    };
    
    std::vector<NoteEvent> noteEvents_;
    std::map<int, VoiceState> activeVoices_;
    std::vector<float> audioBuffer_;
    double sampleRate_;
    double currentTime_;
    int expectedNotesPerLoop_;
    int actualNotesDetected_;
    bool ghostNoteDetected_;
    
public:
    AudioAnalyzer(double sampleRate = 44100.0) 
        : sampleRate_(sampleRate), currentTime_(0.0), 
          expectedNotesPerLoop_(3), actualNotesDetected_(0), ghostNoteDetected_(false) {
        audioBuffer_.reserve(1024);
    }
    
    void recordNoteEvent(int pitch, float velocity, bool isNoteOn, double beatPosition) {
        NoteEvent event;
        event.timestamp = currentTime_;
        event.pitch = pitch;
        event.velocity = velocity;
        event.isNoteOn = isNoteOn;
        event.beatPosition = beatPosition;
        noteEvents_.push_back(event);
        
        if (isNoteOn) {
            actualNotesDetected_++;
            std::cout << "[AUDIO] Note ON: " << pitch << " at beat " << std::fixed << std::setprecision(2) 
                      << beatPosition << " (time " << currentTime_ << ")" << std::endl;
        } else {
            std::cout << "[AUDIO] Note OFF: " << pitch << " at beat " << std::fixed << std::setprecision(2) 
                      << beatPosition << " (time " << currentTime_ << ")" << std::endl;
        }
    }
    
    void analyzeAudioSamples(const float* samples, int numSamples) {
        // Look for unexpected audio activity
        float maxLevel = 0.0f;
        for (int i = 0; i < numSamples; ++i) {
            maxLevel = std::max(maxLevel, std::abs(samples[i]));
        }
        
        // If we detect audio but no note events, it might be a ghost note
        if (maxLevel > 0.01f && noteEvents_.empty()) {
            ghostNoteDetected_ = true;
            std::cout << "[AUDIO] GHOST NOTE DETECTED: Audio level " << maxLevel 
                      << " but no note events recorded!" << std::endl;
        }
        
        currentTime_ += numSamples / sampleRate_;
    }
    
    void checkForOverlappingNotes() {
        std::map<int, int> pitchCounts;
        for (const auto& event : noteEvents_) {
            if (event.isNoteOn) {
                pitchCounts[event.pitch]++;
            }
        }
        
        for (const auto& pair : pitchCounts) {
            if (pair.second > expectedNotesPerLoop_) {
                ghostNoteDetected_ = true;
                std::cout << "[AUDIO] GHOST NOTE: Pitch " << pair.first 
                          << " triggered " << pair.second << " times (expected " 
                          << expectedNotesPerLoop_ << ")" << std::endl;
            }
        }
    }
    
    void printAnalysis() {
        std::cout << "\n=== AUDIO ANALYSIS RESULTS ===" << std::endl;
        std::cout << "Total note events: " << noteEvents_.size() << std::endl;
        std::cout << "Expected notes per loop: " << expectedNotesPerLoop_ << std::endl;
        std::cout << "Actual notes detected: " << actualNotesDetected_ << std::endl;
        
        if (ghostNoteDetected_) {
            std::cout << "❌ GHOST NOTES DETECTED in audio output!" << std::endl;
        } else {
            std::cout << "✅ No ghost notes detected in audio analysis" << std::endl;
        }
        
        // Show note timing
        std::cout << "\nNote Event Timeline:" << std::endl;
        for (const auto& event : noteEvents_) {
            std::cout << "  " << (event.isNoteOn ? "ON " : "OFF") 
                      << " Pitch " << event.pitch 
                      << " at beat " << std::fixed << std::setprecision(2) << event.beatPosition
                      << " (time " << std::fixed << std::setprecision(3) << event.timestamp << ")" << std::endl;
        }
    }
    
    bool hasGhostNotes() const { return ghostNoteDetected_; }
};

int main() {
    std::cout << "=== AUDIO OUTPUT ANALYSIS TEST ===" << std::endl;
    std::cout << "Testing ghost note detection in actual audio output..." << std::endl;
    
    // Create synthesizer and sequencer
    auto synthesizer = std::make_unique<Synthesizer>(44100);
    auto sequencer = std::make_unique<Sequencer>(44100);
    
    // Create audio analyzer
    AudioAnalyzer analyzer(44100.0);
    
    // Set up sequencer callbacks that also record audio events
    sequencer->setNoteCallbacks(
        [&](int pitch, float velocity, int channel, const Envelope& env) {
            double beatPos = sequencer->getPrecisePositionInBeats();
            analyzer.recordNoteEvent(pitch, velocity, true, beatPos);
            synthesizer->noteOn(pitch, velocity, channel);
        },
        [&](int pitch, int channel) {
            double beatPos = sequencer->getPrecisePositionInBeats();
            analyzer.recordNoteEvent(pitch, 0.0f, false, beatPos);
            synthesizer->noteOff(pitch);
        }
    );
    
    // Create the test pattern: C4 at beats 1, 8, and 11
    auto pattern = std::make_unique<Pattern>();
    pattern->setName("Ghost Test Pattern");
    pattern->setLength(16.0); // 16 beats
    
    // Add notes
    Note note1(60, 0.8f, 1.0, 1.0f);   // C4 at beat 1
    Note note2(60, 0.8f, 8.0, 1.0f);   // C4 at beat 8
    Note note3(60, 0.8f, 11.0, 1.0f);   // C4 at beat 11
    pattern->addNote(note1);
    pattern->addNote(note2);
    pattern->addNote(note3);
    
    sequencer->addPattern(std::move(pattern));
    sequencer->setCurrentPattern(1); // Our pattern is at index 1
    sequencer->setLooping(true);
    sequencer->setTempo(120.0);
    
    // Start playback
    sequencer->start();
    
    std::cout << "Starting audio analysis for 4 loops (32 beats)..." << std::endl;
    
    // Simulate audio processing for 4 loops
    const int samplesPerBuffer = 64;
    const double beatsPerLoop = 16.0;
    const double bpm = 120.0;
    const double samplesPerBeat = (44100.0 * 60.0) / bpm;
    const int totalSamples = static_cast<int>(4 * beatsPerLoop * samplesPerBeat);
    
    std::vector<float> leftBuffer(samplesPerBuffer);
    std::vector<float> rightBuffer(samplesPerBuffer);
    
    for (int sample = 0; sample < totalSamples; sample += samplesPerBuffer) {
        // Process sequencer
        sequencer->process(samplesPerBuffer);
        
        // Generate audio (mono output)
        synthesizer->process(leftBuffer.data(), samplesPerBuffer);
        
        // Analyze audio output
        analyzer.analyzeAudioSamples(leftBuffer.data(), samplesPerBuffer);
        
        // Check if we've completed a loop
        double currentBeat = sequencer->getPrecisePositionInBeats();
        static double lastBeat = 0.0;
        if (currentBeat < lastBeat) {
            std::cout << "[AUDIO] Loop completed at beat " << std::fixed << std::setprecision(2) 
                      << currentBeat << std::endl;
        }
        lastBeat = currentBeat;
        
        // Small delay to prevent overwhelming the system
        std::this_thread::sleep_for(std::chrono::microseconds(100));
    }
    
    // Stop playback
    sequencer->stop();
    
    // Analyze results
    analyzer.checkForOverlappingNotes();
    analyzer.printAnalysis();
    
    if (analyzer.hasGhostNotes()) {
        std::cout << "\n❌ GHOST NOTE ISSUE CONFIRMED in audio output!" << std::endl;
        std::cout << "The sequencer fix is not working in the full audio pipeline." << std::endl;
        return 1;
    } else {
        std::cout << "\n✅ No ghost notes detected in audio output analysis." << std::endl;
        std::cout << "The sequencer fix appears to be working correctly." << std::endl;
        return 0;
    }
}