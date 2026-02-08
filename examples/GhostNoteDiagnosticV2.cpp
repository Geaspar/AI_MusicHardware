#include <iostream>
#include <vector>
#include <memory>
#include <chrono>
#include <thread>
#include <iomanip>
#include <cmath>
#include "../include/sequencer/Sequencer.h"

using namespace AIMusicHardware;

/**
 * Enhanced diagnostic test for ghost note issue
 * Tests: C4 at beats 1, 8, 11 in a 16-beat pattern
 * Expected behavior: Only these 3 notes should play per loop
 * Problem: Extra note appears after beat 11
 */

class GhostNoteDiagnosticV2 {
private:
    struct NoteEvent {
        enum Type { ON, OFF };
        Type type;
        double beat;
        int pitch;
        float velocity;
        double timestamp;  // Real time when event occurred
    };
    
    std::vector<NoteEvent> events_;
    std::vector<std::pair<double, int>> expectedNotes_;
    double patternLength_;
    double tolerance_;
    std::chrono::steady_clock::time_point startTime_;
    bool verboseMode_;
    
public:
    GhostNoteDiagnosticV2(double patternLength = 16.0, double tolerance = 0.05, bool verbose = true) 
        : patternLength_(patternLength), tolerance_(tolerance), verboseMode_(verbose) {
        startTime_ = std::chrono::steady_clock::now();
    }
    
    void addExpectedNote(double beat, int pitch) {
        expectedNotes_.push_back({beat, pitch});
    }
    
    void recordNoteOn(double beat, int pitch, float velocity) {
        auto now = std::chrono::steady_clock::now();
        double timestamp = std::chrono::duration<double>(now - startTime_).count();
        
        events_.push_back({NoteEvent::ON, beat, pitch, velocity, timestamp});
        
        if (verboseMode_) {
            std::cout << std::fixed << std::setprecision(3);
            std::cout << "[" << std::setw(8) << timestamp << "s] NOTE ON  @ beat " 
                     << std::setw(7) << beat << " | pitch=" << pitch 
                     << " vel=" << velocity;
            
            // Check if this is expected
            bool expected = isNoteExpected(beat, pitch);
            if (!expected) {
                std::cout << " *** GHOST NOTE ***";
            }
            std::cout << std::endl;
        }
    }
    
    void recordNoteOff(double beat, int pitch) {
        auto now = std::chrono::steady_clock::now();
        double timestamp = std::chrono::duration<double>(now - startTime_).count();
        
        events_.push_back({NoteEvent::OFF, beat, pitch, 0.0f, timestamp});
        
        if (verboseMode_) {
            std::cout << std::fixed << std::setprecision(3);
            std::cout << "[" << std::setw(8) << timestamp << "s] NOTE OFF @ beat " 
                     << std::setw(7) << beat << " | pitch=" << pitch << std::endl;
        }
    }
    
    bool isNoteExpected(double beat, int pitch) {
        // Normalize beat to pattern position (handle looping)
        double normalizedBeat = std::fmod(beat, patternLength_);
        if (normalizedBeat < 0) normalizedBeat += patternLength_;
        
        for (const auto& expected : expectedNotes_) {
            if (std::abs(normalizedBeat - expected.first) <= tolerance_ && 
                pitch == expected.second) {
                return true;
            }
            // Also check if we're at a loop boundary
            if (std::abs(beat - expected.first) <= tolerance_ && 
                pitch == expected.second) {
                return true;
            }
        }
        return false;
    }
    
    void analyzePattern() {
        std::cout << "\n=== PATTERN ANALYSIS ===" << std::endl;
        std::cout << "Pattern length: " << patternLength_ << " beats" << std::endl;
        std::cout << "Total events recorded: " << events_.size() << std::endl;
        
        // Count loops
        int loopCount = 0;
        double lastBeat = 0.0;
        for (const auto& event : events_) {
            if (event.type == NoteEvent::ON && event.beat < lastBeat) {
                loopCount++;
            }
            lastBeat = event.beat;
        }
        std::cout << "Approximate loops detected: " << loopCount + 1 << std::endl;
        
        // Analyze each loop separately
        std::cout << "\n=== LOOP-BY-LOOP ANALYSIS ===" << std::endl;
        
        int currentLoop = 0;
        std::vector<NoteEvent> loopEvents;
        lastBeat = 0.0;
        
        for (const auto& event : events_) {
            if (event.type == NoteEvent::ON && event.beat < lastBeat) {
                // New loop started, analyze previous loop
                analyzeLoop(currentLoop, loopEvents);
                currentLoop++;
                loopEvents.clear();
            }
            loopEvents.push_back(event);
            if (event.type == NoteEvent::ON) {
                lastBeat = event.beat;
            }
        }
        // Analyze final loop
        if (!loopEvents.empty()) {
            analyzeLoop(currentLoop, loopEvents);
        }
        
        // Look for specific patterns
        findAnomalies();
    }
    
private:
    void analyzeLoop(int loopNum, const std::vector<NoteEvent>& loopEvents) {
        std::cout << "\nLoop " << loopNum << ":" << std::endl;
        
        // Count note ons
        int noteOnCount = 0;
        int ghostCount = 0;
        for (const auto& event : loopEvents) {
            if (event.type == NoteEvent::ON) {
                noteOnCount++;
                if (!isNoteExpected(event.beat, event.pitch)) {
                    ghostCount++;
                    std::cout << "  - GHOST at beat " << std::fixed << std::setprecision(3) 
                             << event.beat << " (t=" << event.timestamp << "s)" << std::endl;
                }
            }
        }
        
        std::cout << "  Notes triggered: " << noteOnCount;
        if (noteOnCount != static_cast<int>(expectedNotes_.size())) {
            std::cout << " (expected " << expectedNotes_.size() << ")";
        }
        if (ghostCount > 0) {
            std::cout << " - " << ghostCount << " GHOST NOTES!";
        }
        std::cout << std::endl;
    }
    
    void findAnomalies() {
        std::cout << "\n=== ANOMALY DETECTION ===" << std::endl;
        
        // Check for notes immediately after beat 11
        for (size_t i = 0; i < events_.size(); ++i) {
            if (events_[i].type == NoteEvent::ON) {
                double beat = events_[i].beat;
                
                // Look for notes right after beat 11
                if (beat > 11.0 && beat < 12.0 && !isNoteExpected(beat, events_[i].pitch)) {
                    std::cout << "SUSPICIOUS: Note at beat " << beat 
                             << " (just after beat 11)" << std::endl;
                    
                    // Check what happened before this
                    if (i > 0) {
                        std::cout << "  Previous event: ";
                        const auto& prev = events_[i-1];
                        if (prev.type == NoteEvent::ON) {
                            std::cout << "NOTE ON @ beat " << prev.beat;
                        } else {
                            std::cout << "NOTE OFF @ beat " << prev.beat;
                        }
                        std::cout << std::endl;
                    }
                }
                
                // Look for notes at loop boundary
                if (beat < 0.5 || (beat > patternLength_ - 0.5 && beat < patternLength_ + 0.5)) {
                    std::cout << "BOUNDARY NOTE: Beat " << beat 
                             << " (near loop point)" << std::endl;
                }
            }
        }
        
        // Check for double triggers
        for (size_t i = 1; i < events_.size(); ++i) {
            if (events_[i].type == NoteEvent::ON && events_[i-1].type == NoteEvent::ON) {
                if (events_[i].pitch == events_[i-1].pitch) {
                    double timeDiff = events_[i].timestamp - events_[i-1].timestamp;
                    if (timeDiff < 0.1) { // Less than 100ms apart
                        std::cout << "DOUBLE TRIGGER: Same pitch (" << events_[i].pitch 
                                 << ") triggered twice within " << timeDiff*1000 << "ms" << std::endl;
                    }
                }
            }
        }
    }
};

int main() {
    std::cout << "=== GHOST NOTE DIAGNOSTIC V2 ===" << std::endl;
    std::cout << "Testing: C4 at beats 1, 8, 11 in 16-beat pattern" << std::endl;
    std::cout << "Tempo: 120 BPM, Time Signature: 4/4" << std::endl;
    std::cout << "=========================================\n" << std::endl;
    
    // Create sequencer
    auto sequencer = std::make_unique<Sequencer>(120.0, 4);
    sequencer->initialize();
    
    // Create pattern with specific length
    auto pattern = std::make_unique<Pattern>("Test Pattern");
    pattern->setLength(16.0); // Exactly 4 bars at 4/4
    
    // Add notes: C4 at beats 1, 8, 11
    const int C4 = 60;
    const float velocity = 1.0f;
    const double noteDuration = 0.5; // Half beat duration
    
    // Note: beats are 0-indexed in the pattern
    pattern->addNote(Note(C4, velocity, 1.0, noteDuration, 0));  // Beat 1
    pattern->addNote(Note(C4, velocity, 8.0, noteDuration, 0));  // Beat 8
    pattern->addNote(Note(C4, velocity, 11.0, noteDuration, 0)); // Beat 11
    
    std::cout << "Pattern created with " << pattern->getNumNotes() << " notes" << std::endl;
    std::cout << "Pattern length: " << pattern->getLength() << " beats\n" << std::endl;
    
    // Add pattern to sequencer
    sequencer->addPattern(std::move(pattern));
    // Note: initialize() creates a default empty pattern at index 0,
    // so our pattern is at index 1
    sequencer->setCurrentPattern(1); // Use our pattern at index 1
    sequencer->setLooping(true);
    
    // Create diagnostic tracker
    GhostNoteDiagnosticV2 diagnostic(16.0, 0.05, true);
    diagnostic.addExpectedNote(1.0, C4);
    diagnostic.addExpectedNote(8.0, C4);
    diagnostic.addExpectedNote(11.0, C4);
    
    // Set up callbacks
    sequencer->setNoteCallbacks(
        [&](int pitch, float velocity, int channel, const Envelope& env) {
            double beat = sequencer->getPositionInBeats();
            diagnostic.recordNoteOn(beat, pitch, velocity);
        },
        [&](int pitch, int channel) {
            double beat = sequencer->getPositionInBeats();
            diagnostic.recordNoteOff(beat, pitch);
        }
    );
    
    // Set up transport callback for position monitoring
    sequencer->setTransportCallback(
        [](double positionInBeats, int bar, int beat) {
            static int lastBar = -1;
            if (bar != lastBar) {
                std::cout << "--- Bar " << bar << " ---" << std::endl;
                lastBar = bar;
            }
        }
    );
    
    // Start playback
    std::cout << "Starting playback..." << std::endl;
    std::cout << "Will run for 30 seconds to capture multiple loops\n" << std::endl;
    
    sequencer->start();
    
    // Run for 30 seconds at 60 FPS
    const double frameTime = 1.0 / 60.0;
    const int totalFrames = 30 * 60; // 30 seconds
    
    for (int frame = 0; frame < totalFrames; ++frame) {
        sequencer->process(frameTime);
        std::this_thread::sleep_for(std::chrono::microseconds(16667)); // ~60 FPS
        
        // Print progress every 5 seconds
        if (frame % 300 == 0 && frame > 0) {
            double elapsed = frame * frameTime;
            std::cout << "\n[Progress: " << elapsed << " seconds elapsed]\n" << std::endl;
        }
    }
    
    // Stop playback
    sequencer->stop();
    std::cout << "\n\nPlayback stopped." << std::endl;
    
    // Analyze results
    diagnostic.analyzePattern();
    
    std::cout << "\n=== TEST COMPLETE ===" << std::endl;
    return 0;
}