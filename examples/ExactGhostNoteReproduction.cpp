/**
 * Exact reproduction of the ghost note issue as described:
 * - Pattern with notes at beats 1, 8, and 11
 * - Ghost note appears AFTER beat 11
 * - This test simulates the exact conditions with the full synthesizer stack
 */

#include <iostream>
#include <memory>
#include <thread>
#include <chrono>
#include <iomanip>
#include <vector>
#include "../include/audio/Synthesizer.h"
#include "../include/sequencer/Sequencer.h"
#include "../include/synthesis/voice/voice_manager.h"

using namespace AIMusicHardware;

class GhostNoteMonitor {
public:
    struct Event {
        enum Type { NOTE_ON, NOTE_OFF, POSITION };
        Type type;
        double timestamp;
        double beat;
        int pitch;
        float velocity;
        int bar;
        int beatInBar;
    };
    
private:
    std::vector<Event> events_;
    std::chrono::steady_clock::time_point startTime_;
    double lastReportedBeat_ = -1.0;
    bool detectingGhost_ = false;
    double ghostDetectionWindow_ = 2.0; // beats after 11 to watch
    
public:
    GhostNoteMonitor() : startTime_(std::chrono::steady_clock::now()) {}
    
    void onNoteOn(int pitch, float velocity, double beat) {
        double timestamp = getTimestamp();
        events_.push_back({Event::NOTE_ON, timestamp, beat, pitch, velocity, 0, 0});
        
        // Detailed output for debugging
        std::cout << std::fixed << std::setprecision(3);
        std::cout << "[" << std::setw(7) << timestamp << "s] NOTE ON  | beat=" 
                 << std::setw(8) << beat << " pitch=" << pitch;
        
        // Check if this is potentially a ghost note
        double beatInPattern = std::fmod(beat, 16.0);
        if (beatInPattern < 0) beatInPattern += 16.0;
        
        // Expected notes are at beats 1, 8, 11
        bool isExpected = (std::abs(beatInPattern - 1.0) < 0.1 ||
                          std::abs(beatInPattern - 8.0) < 0.1 ||
                          std::abs(beatInPattern - 11.0) < 0.1);
        
        if (!isExpected) {
            std::cout << " *** GHOST NOTE ***";
            
            // Special attention to notes after beat 11
            if (beatInPattern > 11.0 && beatInPattern < 13.0) {
                std::cout << " (AFTER BEAT 11!)";
            }
        }
        std::cout << std::endl;
        
        // Track if we just played beat 11
        if (std::abs(beatInPattern - 11.0) < 0.1) {
            detectingGhost_ = true;
            std::cout << ">>> Watching for ghost notes after beat 11..." << std::endl;
        }
        
        // If we're in the detection window after beat 11
        if (detectingGhost_ && beatInPattern > 11.1 && beatInPattern < (11.0 + ghostDetectionWindow_)) {
            std::cout << "!!! GHOST NOTE DETECTED: " << (beatInPattern - 11.0) 
                     << " beats after beat 11" << std::endl;
        }
        
        // Reset detection after we've passed the window
        if (beatInPattern < 1.0 || beatInPattern > 13.0) {
            detectingGhost_ = false;
        }
    }
    
    void onNoteOff(int pitch, double beat) {
        double timestamp = getTimestamp();
        events_.push_back({Event::NOTE_OFF, timestamp, beat, pitch, 0, 0, 0});
        
        std::cout << std::fixed << std::setprecision(3);
        std::cout << "[" << std::setw(7) << timestamp << "s] NOTE OFF | beat=" 
                 << std::setw(8) << beat << " pitch=" << pitch << std::endl;
    }
    
    void onTransport(double beat, int bar, int beatInBar) {
        // Only report significant position changes
        if (std::abs(beat - lastReportedBeat_) > 0.5) {
            double timestamp = getTimestamp();
            events_.push_back({Event::POSITION, timestamp, beat, 0, 0, bar, beatInBar});
            
            // Print bar changes
            static int lastBar = -1;
            if (bar != lastBar) {
                std::cout << "=== Bar " << bar << " ===" << std::endl;
                lastBar = bar;
            }
            
            lastReportedBeat_ = beat;
        }
    }
    
    void analyze() {
        std::cout << "\n\n=== ANALYSIS RESULTS ===" << std::endl;
        std::cout << "Total events: " << events_.size() << std::endl;
        
        // Count note ons per beat position
        std::map<int, int> notesByBeat;
        int ghostCount = 0;
        
        for (const auto& event : events_) {
            if (event.type == Event::NOTE_ON) {
                double beatInPattern = std::fmod(event.beat, 16.0);
                if (beatInPattern < 0) beatInPattern += 16.0;
                
                int beatRounded = static_cast<int>(beatInPattern + 0.5);
                notesByBeat[beatRounded]++;
                
                // Check for ghosts
                if (beatRounded != 1 && beatRounded != 8 && beatRounded != 11) {
                    ghostCount++;
                    std::cout << "Ghost note found at beat " << beatInPattern 
                             << " (rounded to " << beatRounded << ")" << std::endl;
                }
            }
        }
        
        std::cout << "\nNotes by beat position:" << std::endl;
        for (const auto& [beat, count] : notesByBeat) {
            std::cout << "  Beat " << std::setw(2) << beat << ": " << count << " notes";
            if (beat != 1 && beat != 8 && beat != 11 && beat != 0 && beat != 16) {
                std::cout << " (UNEXPECTED)";
            }
            std::cout << std::endl;
        }
        
        std::cout << "\nTotal ghost notes: " << ghostCount << std::endl;
    }
    
private:
    double getTimestamp() {
        auto now = std::chrono::steady_clock::now();
        return std::chrono::duration<double>(now - startTime_).count();
    }
};

int main() {
    std::cout << "=== EXACT GHOST NOTE REPRODUCTION TEST ===" << std::endl;
    std::cout << "Setup: C4 at beats 1, 8, 11" << std::endl;
    std::cout << "Looking for: Extra note after beat 11" << std::endl;
    std::cout << "==========================================\n" << std::endl;
    
    // Create synthesizer (includes voice manager)
    auto synth = std::make_unique<Synthesizer>(44100);
    synth->initialize();
    
    // Create sequencer
    auto sequencer = std::make_unique<Sequencer>(120.0, 4);
    sequencer->initialize();
    
    // Create the pattern exactly as user describes
    auto pattern = std::make_unique<Pattern>("Test Pattern");
    pattern->setLength(16.0); // 16 beats = 4 bars at 4/4
    
    const int C4 = 60;
    
    // Add notes with 1-beat duration (full gate)
    pattern->addNote(Note(C4, 1.0f, 1.0, 1.0, 0));   // Beat 1
    pattern->addNote(Note(C4, 1.0f, 8.0, 1.0, 0));   // Beat 8
    pattern->addNote(Note(C4, 1.0f, 11.0, 1.0, 0));  // Beat 11
    
    std::cout << "Pattern info:" << std::endl;
    std::cout << "  Length: " << pattern->getLength() << " beats" << std::endl;
    std::cout << "  Notes: " << pattern->getNumNotes() << std::endl;
    
    // Verify notes
    for (size_t i = 0; i < pattern->getNumNotes(); ++i) {
        const Note* note = pattern->getNote(i);
        if (note) {
            std::cout << "  Note " << i << ": pitch=" << note->pitch 
                     << " start=" << note->startTime 
                     << " duration=" << note->duration << std::endl;
        }
    }
    std::cout << std::endl;
    
    // Add pattern and set as current
    sequencer->addPattern(std::move(pattern));
    sequencer->setCurrentPattern(1); // Our pattern is at index 1
    sequencer->setLooping(true);
    
    // Create monitor
    GhostNoteMonitor monitor;
    
    // Wire sequencer to synthesizer with monitoring
    sequencer->setNoteCallbacks(
        [&](int pitch, float velocity, int channel, const Envelope& env) {
            double beat = sequencer->getPositionInBeats();
            monitor.onNoteOn(pitch, velocity, beat);
            
            // Also trigger the synthesizer
            synth->noteOn(pitch, velocity, channel);
        },
        [&](int pitch, int channel) {
            double beat = sequencer->getPositionInBeats();
            monitor.onNoteOff(pitch, beat);
            
            // Also release in synthesizer
            synth->noteOff(pitch, channel);
        }
    );
    
    // Set transport callback
    sequencer->setTransportCallback(
        [&](double beat, int bar, int beatNum) {
            monitor.onTransport(beat, bar, beatNum);
        }
    );
    
    // Start playback
    std::cout << "Starting playback for 20 seconds...\n" << std::endl;
    sequencer->start();
    
    // Process audio in real-time simulation
    const int sampleRate = 44100;
    const int blockSize = 256;
    const double blockTime = static_cast<double>(blockSize) / sampleRate;
    const int totalBlocks = static_cast<int>(20.0 / blockTime); // 20 seconds
    
    std::vector<float> audioBuffer(blockSize * 2); // Stereo
    
    for (int block = 0; block < totalBlocks; ++block) {
        // Process sequencer
        sequencer->process(blockTime);
        
        // Process synthesizer (this would normally go to audio output)
        synth->process(audioBuffer.data(), blockSize);
        
        // Sleep to simulate real-time
        std::this_thread::sleep_for(std::chrono::microseconds(
            static_cast<int>(blockTime * 1000000)
        ));
        
        // Progress indicator
        if (block % 200 == 0 && block > 0) {
            double elapsed = block * blockTime;
            std::cout << "\n[" << elapsed << " seconds elapsed]\n" << std::endl;
        }
    }
    
    // Stop everything
    sequencer->stop();
    synth->allNotesOff();
    
    std::cout << "\n\nPlayback complete." << std::endl;
    
    // Analyze results
    monitor.analyze();
    
    std::cout << "\n=== TEST COMPLETE ===" << std::endl;
    return 0;
}