/**
 * Regression test for sequencer loop boundary issues
 * Tests various edge cases that could cause ghost notes or stuck notes
 */

#include <iostream>
#include <memory>
#include <thread>
#include <chrono>
#include <vector>
#include <set>
#include <cmath>
#include <cassert>
#include "../include/sequencer/Sequencer.h"

using namespace AIMusicHardware;

class TestResult {
public:
    std::string testName;
    bool passed;
    std::string errorMessage;
    
    TestResult(const std::string& name) : testName(name), passed(true) {}
    
    void fail(const std::string& msg) {
        passed = false;
        errorMessage = msg;
    }
};

class SequencerRegressionTest {
private:
    std::vector<TestResult> results_;
    
    // Helper to track note events
    struct NoteTracker {
        std::vector<std::pair<int, double>> noteOns;  // pitch, beat
        std::vector<std::pair<int, double>> noteOffs; // pitch, beat
        std::set<int> activeNotes;
        bool hasGhostNotes = false;
        bool hasStuckNotes = false;
        
        void reset() {
            noteOns.clear();
            noteOffs.clear();
            activeNotes.clear();
            hasGhostNotes = false;
            hasStuckNotes = false;
        }
        
        void onNoteOn(int pitch, double beat) {
            noteOns.push_back({pitch, beat});
            if (activeNotes.count(pitch) > 0) {
                hasStuckNotes = true; // Note triggered while already active
            }
            activeNotes.insert(pitch);
        }
        
        void onNoteOff(int pitch, double beat) {
            noteOffs.push_back({pitch, beat});
            activeNotes.erase(pitch);
        }
        
        bool checkExpectedNotes(const std::vector<std::pair<int, double>>& expectedNotes, double tolerance = 0.1) {
            for (const auto& exp : expectedNotes) {
                bool found = false;
                for (const auto& actual : noteOns) {
                    if (actual.first == exp.first && 
                        std::abs(std::fmod(actual.second, 16.0) - exp.second) < tolerance) {
                        found = true;
                        break;
                    }
                }
                if (!found) return false;
            }
            
            // Check for unexpected notes (ghost notes)
            for (const auto& actual : noteOns) {
                double beatInPattern = std::fmod(actual.second, 16.0);
                if (beatInPattern < 0) beatInPattern += 16.0;
                
                bool expectedNote = false;
                for (const auto& exp : expectedNotes) {
                    if (actual.first == exp.first && 
                        std::abs(beatInPattern - exp.second) < tolerance) {
                        expectedNote = true;
                        break;
                    }
                }
                if (!expectedNote) {
                    hasGhostNotes = true;
                }
            }
            
            return true;
        }
    };
    
public:
    void runAllTests() {
        std::cout << "=== SEQUENCER LOOP BOUNDARY REGRESSION TESTS ===" << std::endl;
        std::cout << "Running comprehensive tests to prevent ghost notes...\n" << std::endl;
        
        // Test 1: Basic pattern with notes at specific beats
        testBasicPattern();
        
        // Test 2: Notes extending beyond pattern boundary
        testNotesExtendingBeyondLoop();
        
        // Test 3: Very short notes at loop boundary
        testShortNotesAtBoundary();
        
        // Test 4: Overlapping notes
        testOverlappingNotes();
        
        // Test 5: Notes at exact loop point
        testNotesAtExactLoopPoint();
        
        // Test 6: Empty pattern sections
        testSparsePattern();
        
        // Print results
        printResults();
    }
    
private:
    void testBasicPattern() {
        TestResult result("Basic Pattern (C4 at beats 1, 8, 11)");
        NoteTracker tracker;
        
        auto sequencer = std::make_unique<Sequencer>(120.0, 4);
        sequencer->initialize();
        
        auto pattern = std::make_unique<Pattern>("Basic");
        pattern->setLength(16.0);
        pattern->addNote(Note(60, 1.0f, 1.0, 1.0, 0));
        pattern->addNote(Note(60, 1.0f, 8.0, 1.0, 0));
        pattern->addNote(Note(60, 1.0f, 11.0, 1.0, 0));
        
        sequencer->addPattern(std::move(pattern));
        sequencer->setCurrentPattern(1);
        sequencer->setLooping(true);
        
        sequencer->setNoteCallbacks(
            [&](int pitch, float vel, int ch, const Envelope& env) {
                tracker.onNoteOn(pitch, sequencer->getPositionInBeats());
            },
            [&](int pitch, int ch) {
                tracker.onNoteOff(pitch, sequencer->getPositionInBeats());
            }
        );
        
        sequencer->start();
        
        // Run for 2 loops
        for (int i = 0; i < 2000; ++i) {
            sequencer->process(0.016); // ~60 FPS
            std::this_thread::sleep_for(std::chrono::microseconds(100));
        }
        
        sequencer->stop();
        
        // Check results
        std::vector<std::pair<int, double>> expected = {{60, 1.0}, {60, 8.0}, {60, 11.0}};
        if (!tracker.checkExpectedNotes(expected)) {
            result.fail("Missing expected notes");
        }
        if (tracker.hasGhostNotes) {
            result.fail("Ghost notes detected");
        }
        if (tracker.hasStuckNotes) {
            result.fail("Stuck notes detected");
        }
        
        results_.push_back(result);
    }
    
    void testNotesExtendingBeyondLoop() {
        TestResult result("Notes Extending Beyond Loop Boundary");
        NoteTracker tracker;
        
        auto sequencer = std::make_unique<Sequencer>(120.0, 4);
        sequencer->initialize();
        
        auto pattern = std::make_unique<Pattern>("Extended");
        pattern->setLength(16.0);
        // Note at beat 15 with duration 2.0 (extends to beat 17, beyond pattern)
        pattern->addNote(Note(60, 1.0f, 15.0, 2.0, 0));
        // Note at beat 14 with duration 1.5
        pattern->addNote(Note(62, 1.0f, 14.0, 1.5, 0));
        
        sequencer->addPattern(std::move(pattern));
        sequencer->setCurrentPattern(1);
        sequencer->setLooping(true);
        
        sequencer->setNoteCallbacks(
            [&](int pitch, float vel, int ch, const Envelope& env) {
                tracker.onNoteOn(pitch, sequencer->getPositionInBeats());
            },
            [&](int pitch, int ch) {
                tracker.onNoteOff(pitch, sequencer->getPositionInBeats());
            }
        );
        
        sequencer->start();
        
        // Run for 2 loops
        for (int i = 0; i < 2000; ++i) {
            sequencer->process(0.016);
            std::this_thread::sleep_for(std::chrono::microseconds(100));
        }
        
        sequencer->stop();
        
        // These notes should NOT cause stuck notes at loop boundary
        if (tracker.hasStuckNotes) {
            result.fail("Notes extending beyond loop caused stuck notes");
        }
        
        results_.push_back(result);
    }
    
    void testShortNotesAtBoundary() {
        TestResult result("Short Notes at Loop Boundary");
        NoteTracker tracker;
        
        auto sequencer = std::make_unique<Sequencer>(120.0, 4);
        sequencer->initialize();
        
        auto pattern = std::make_unique<Pattern>("Short");
        pattern->setLength(16.0);
        // Very short note right at the end
        pattern->addNote(Note(60, 1.0f, 15.9, 0.05, 0));
        // Short note at the beginning
        pattern->addNote(Note(62, 1.0f, 0.0, 0.1, 0));
        
        sequencer->addPattern(std::move(pattern));
        sequencer->setCurrentPattern(1);
        sequencer->setLooping(true);
        
        sequencer->setNoteCallbacks(
            [&](int pitch, float vel, int ch, const Envelope& env) {
                tracker.onNoteOn(pitch, sequencer->getPositionInBeats());
            },
            [&](int pitch, int ch) {
                tracker.onNoteOff(pitch, sequencer->getPositionInBeats());
            }
        );
        
        sequencer->start();
        
        // Run for 2 loops
        for (int i = 0; i < 2000; ++i) {
            sequencer->process(0.016);
            std::this_thread::sleep_for(std::chrono::microseconds(100));
        }
        
        sequencer->stop();
        
        if (tracker.hasStuckNotes) {
            result.fail("Short notes at boundary caused stuck notes");
        }
        
        results_.push_back(result);
    }
    
    void testOverlappingNotes() {
        TestResult result("Overlapping Notes");
        NoteTracker tracker;
        
        auto sequencer = std::make_unique<Sequencer>(120.0, 4);
        sequencer->initialize();
        
        auto pattern = std::make_unique<Pattern>("Overlap");
        pattern->setLength(16.0);
        // Two notes of same pitch that overlap
        pattern->addNote(Note(60, 1.0f, 10.0, 3.0, 0)); // Ends at 13
        pattern->addNote(Note(60, 1.0f, 12.0, 2.0, 0)); // Starts at 12, ends at 14
        
        sequencer->addPattern(std::move(pattern));
        sequencer->setCurrentPattern(1);
        sequencer->setLooping(true);
        
        int noteOnCount = 0;
        sequencer->setNoteCallbacks(
            [&](int pitch, float vel, int ch, const Envelope& env) {
                tracker.onNoteOn(pitch, sequencer->getPositionInBeats());
                noteOnCount++;
            },
            [&](int pitch, int ch) {
                tracker.onNoteOff(pitch, sequencer->getPositionInBeats());
            }
        );
        
        sequencer->start();
        
        // Run for 1 loop
        for (int i = 0; i < 1000; ++i) {
            sequencer->process(0.016);
            std::this_thread::sleep_for(std::chrono::microseconds(100));
        }
        
        sequencer->stop();
        
        // Should handle overlapping notes correctly
        if (noteOnCount != 2 && noteOnCount != 4) { // 2 per loop
            result.fail("Incorrect number of note triggers for overlapping notes");
        }
        
        results_.push_back(result);
    }
    
    void testNotesAtExactLoopPoint() {
        TestResult result("Notes at Exact Loop Point");
        NoteTracker tracker;
        
        auto sequencer = std::make_unique<Sequencer>(120.0, 4);
        sequencer->initialize();
        
        auto pattern = std::make_unique<Pattern>("Exact");
        pattern->setLength(16.0);
        // Note exactly at beat 0
        pattern->addNote(Note(60, 1.0f, 0.0, 1.0, 0));
        // Note exactly at beat 16 (should be treated as beat 0 of next loop)
        pattern->addNote(Note(62, 1.0f, 16.0, 1.0, 0));
        
        sequencer->addPattern(std::move(pattern));
        sequencer->setCurrentPattern(1);
        sequencer->setLooping(true);
        
        sequencer->setNoteCallbacks(
            [&](int pitch, float vel, int ch, const Envelope& env) {
                tracker.onNoteOn(pitch, sequencer->getPositionInBeats());
            },
            [&](int pitch, int ch) {
                tracker.onNoteOff(pitch, sequencer->getPositionInBeats());
            }
        );
        
        sequencer->start();
        
        // Run for 2 loops
        for (int i = 0; i < 2000; ++i) {
            sequencer->process(0.016);
            std::this_thread::sleep_for(std::chrono::microseconds(100));
        }
        
        sequencer->stop();
        
        // Note at beat 16 should not play (it's beyond pattern length)
        bool foundBeat16 = false;
        for (const auto& noteOn : tracker.noteOns) {
            if (noteOn.first == 62) {
                foundBeat16 = true;
                break;
            }
        }
        
        if (foundBeat16) {
            result.fail("Note at beat 16 (beyond pattern) was incorrectly triggered");
        }
        
        if (tracker.hasStuckNotes) {
            result.fail("Notes at exact loop point caused stuck notes");
        }
        
        results_.push_back(result);
    }
    
    void testSparsePattern() {
        TestResult result("Sparse Pattern with Empty Sections");
        NoteTracker tracker;
        
        auto sequencer = std::make_unique<Sequencer>(120.0, 4);
        sequencer->initialize();
        
        auto pattern = std::make_unique<Pattern>("Sparse");
        pattern->setLength(16.0);
        // Only one note in the entire pattern
        pattern->addNote(Note(60, 1.0f, 7.5, 0.5, 0));
        
        sequencer->addPattern(std::move(pattern));
        sequencer->setCurrentPattern(1);
        sequencer->setLooping(true);
        
        int noteCount = 0;
        sequencer->setNoteCallbacks(
            [&](int pitch, float vel, int ch, const Envelope& env) {
                tracker.onNoteOn(pitch, sequencer->getPositionInBeats());
                noteCount++;
            },
            [&](int pitch, int ch) {
                tracker.onNoteOff(pitch, sequencer->getPositionInBeats());
            }
        );
        
        sequencer->start();
        
        // Run for 3 loops
        for (int i = 0; i < 3000; ++i) {
            sequencer->process(0.016);
            std::this_thread::sleep_for(std::chrono::microseconds(100));
        }
        
        sequencer->stop();
        
        // Should trigger exactly once per loop
        if (noteCount < 2 || noteCount > 4) { // Allow some tolerance
            result.fail("Sparse pattern triggered incorrect number of notes");
        }
        
        if (tracker.hasGhostNotes || tracker.hasStuckNotes) {
            result.fail("Sparse pattern caused ghost or stuck notes");
        }
        
        results_.push_back(result);
    }
    
    void printResults() {
        std::cout << "\n=== TEST RESULTS ===" << std::endl;
        
        int passed = 0;
        int failed = 0;
        
        for (const auto& result : results_) {
            if (result.passed) {
                std::cout << "✓ " << result.testName << std::endl;
                passed++;
            } else {
                std::cout << "✗ " << result.testName << std::endl;
                std::cout << "  Error: " << result.errorMessage << std::endl;
                failed++;
            }
        }
        
        std::cout << "\n";
        std::cout << "Total: " << (passed + failed) << " tests" << std::endl;
        std::cout << "Passed: " << passed << std::endl;
        std::cout << "Failed: " << failed << std::endl;
        
        if (failed == 0) {
            std::cout << "\n🎉 All tests passed! No ghost note issues detected." << std::endl;
        } else {
            std::cout << "\n⚠️  Some tests failed. Please review the errors above." << std::endl;
        }
    }
};

int main() {
    SequencerRegressionTest test;
    test.runAllTests();
    return 0;
}