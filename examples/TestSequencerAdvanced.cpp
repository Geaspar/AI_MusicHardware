#include <iostream>
#include <memory>
#include <thread>
#include <chrono>
#include <iomanip>
#include <cmath>
#include <string>
#include <mutex>

#include "../include/sequencer/Sequencer.h"
#include "../include/sequencer/MidiFile.h"

// Create a mutex for thread-safe callbacks
std::mutex callbackMutex;

using namespace AIMusicHardware;

// Helper function to print menu and get user choice
int getUserChoice(const std::vector<std::string>& options, const std::string& prompt = "Choose an option:") {
    std::cout << "\n" << prompt << std::endl;
    
    for (size_t i = 0; i < options.size(); ++i) {
        std::cout << "[" << i + 1 << "] " << options[i] << std::endl;
    }
    std::cout << "[0] Exit" << std::endl;
    
    int choice;
    std::cout << "> ";
    
    // Improved input handling with retry loop
    while (true) {
        std::cin >> choice;
        
        // Handle invalid input
        if (std::cin.fail()) {
            std::cin.clear();
            std::cin.ignore(10000, '\n');
            std::cout << "Invalid input. Please enter a number: ";
        } else if (choice < 0 || choice > static_cast<int>(options.size())) {
            std::cout << "Invalid option. Please choose a number between 0 and " << options.size() << ": ";
        } else {
            break; // Valid input received
        }
    }
    
    return choice;
}

// Helper function to create a simple scale pattern
std::unique_ptr<Pattern> createScalePattern(const std::string& name = "Scale Pattern") {
    auto pattern = std::make_unique<Pattern>(name);
    
    // Add a C major scale: C4, D4, E4, F4, G4, A4, B4, C5
    int notes[] = {60, 62, 64, 65, 67, 69, 71, 72};
    
    for (int i = 0; i < 8; ++i) {
        Note note(notes[i], 0.8f, i * 0.5, 0.4, 0);
        pattern->addNote(note);
    }
    
    return pattern;
}

// Helper function to create a chord pattern
std::unique_ptr<Pattern> createChordPattern(const std::string& name = "Chord Pattern") {
    auto pattern = std::make_unique<Pattern>(name);
    
    // C Major chord (C, E, G)
    pattern->addNote(Note(60, 0.7f, 0.0, 1.0, 0));  // C4
    pattern->addNote(Note(64, 0.7f, 0.0, 1.0, 0));  // E4
    pattern->addNote(Note(67, 0.7f, 0.0, 1.0, 0));  // G4
    
    // F Major chord (F, A, C)
    pattern->addNote(Note(65, 0.7f, 1.0, 1.0, 0));  // F4
    pattern->addNote(Note(69, 0.7f, 1.0, 1.0, 0));  // A4
    pattern->addNote(Note(72, 0.7f, 1.0, 1.0, 0));  // C5
    
    // G Major chord (G, B, D)
    pattern->addNote(Note(67, 0.7f, 2.0, 1.0, 0));  // G4
    pattern->addNote(Note(71, 0.7f, 2.0, 1.0, 0));  // B4
    pattern->addNote(Note(74, 0.7f, 2.0, 1.0, 0));  // D5
    
    // C Major chord (C, E, G)
    pattern->addNote(Note(60, 0.7f, 3.0, 1.0, 0));  // C4
    pattern->addNote(Note(64, 0.7f, 3.0, 1.0, 0));  // E4
    pattern->addNote(Note(67, 0.7f, 3.0, 1.0, 0));  // G4
    
    return pattern;
}

// Helper function to create an arpeggio pattern
std::unique_ptr<Pattern> createArpeggioPattern(const std::string& name = "Arpeggio Pattern") {
    auto pattern = std::make_unique<Pattern>(name);
    
    // C Major arpeggio (C, E, G, C)
    pattern->addNote(Note(60, 0.7f, 0.0, 0.25, 0));  // C4
    pattern->addNote(Note(64, 0.7f, 0.25, 0.25, 0)); // E4
    pattern->addNote(Note(67, 0.7f, 0.5, 0.25, 0));  // G4
    pattern->addNote(Note(72, 0.7f, 0.75, 0.25, 0)); // C5
    
    // F Major arpeggio (F, A, C, F)
    pattern->addNote(Note(65, 0.7f, 1.0, 0.25, 0));  // F4
    pattern->addNote(Note(69, 0.7f, 1.25, 0.25, 0)); // A4
    pattern->addNote(Note(72, 0.7f, 1.5, 0.25, 0));  // C5
    pattern->addNote(Note(77, 0.7f, 1.75, 0.25, 0)); // F5
    
    // G Major arpeggio (G, B, D, G)
    pattern->addNote(Note(67, 0.7f, 2.0, 0.25, 0));  // G4
    pattern->addNote(Note(71, 0.7f, 2.25, 0.25, 0)); // B4
    pattern->addNote(Note(74, 0.7f, 2.5, 0.25, 0));  // D5
    pattern->addNote(Note(79, 0.7f, 2.75, 0.25, 0)); // G5
    
    // C Major arpeggio (C, E, G, C)
    pattern->addNote(Note(60, 0.7f, 3.0, 0.25, 0));  // C4
    pattern->addNote(Note(64, 0.7f, 3.25, 0.25, 0)); // E4
    pattern->addNote(Note(67, 0.7f, 3.5, 0.25, 0));  // G4
    pattern->addNote(Note(72, 0.7f, 3.75, 0.25, 0)); // C5
    
    return pattern;
}

// Helper function to create a bassline pattern
std::unique_ptr<Pattern> createBassPattern(const std::string& name = "Bass Pattern") {
    auto pattern = std::make_unique<Pattern>(name);
    
    // Simple bassline in C
    pattern->addNote(Note(36, 0.9f, 0.0, 0.5, 0));   // C2
    pattern->addNote(Note(36, 0.7f, 1.0, 0.5, 0));   // C2
    pattern->addNote(Note(41, 0.9f, 2.0, 0.5, 0));   // F2
    pattern->addNote(Note(43, 0.9f, 3.0, 0.5, 0));   // G2
    
    return pattern;
}

// Helper function to create a drum pattern
std::unique_ptr<Pattern> createDrumPattern(const std::string& name = "Drum Pattern") {
    auto pattern = std::make_unique<Pattern>(name);
    
    // Basic 4/4 drum pattern on the General MIDI drum map
    int kick = 36;    // Bass Drum 1
    int snare = 40;   // Electric Snare
    int hihat = 42;   // Closed Hi-Hat
    
    // Kicks on beats 1 and 3
    pattern->addNote(Note(kick, 0.9f, 0.0, 0.25, 9));
    pattern->addNote(Note(kick, 0.9f, 2.0, 0.25, 9));
    
    // Snares on beats 2 and 4
    pattern->addNote(Note(snare, 0.8f, 1.0, 0.25, 9));
    pattern->addNote(Note(snare, 0.8f, 3.0, 0.25, 9));
    
    // Hi-hats on every eighth note
    for (int i = 0; i < 8; ++i) {
        pattern->addNote(Note(hihat, 0.7f, i * 0.5, 0.25, 9));
    }
    
    return pattern;
}

// Helper function to print a pattern's contents
void printPattern(const Pattern* pattern) {
    if (!pattern) {
        std::cout << "Invalid pattern!" << std::endl;
        return;
    }
    
    std::cout << "Pattern: " << pattern->getName() << std::endl;
    std::cout << "Length: " << pattern->getLength() << " beats" << std::endl;
    std::cout << "Notes: " << pattern->getNumNotes() << std::endl;
    
    std::cout << "----------------------------------------------------" << std::endl;
    std::cout << std::setw(5) << "Note" << std::setw(10) << "Pitch" 
              << std::setw(10) << "Start" << std::setw(10) << "Duration" 
              << std::setw(10) << "Velocity" << std::setw(10) << "Channel" << std::endl;
    std::cout << "----------------------------------------------------" << std::endl;
    
    for (size_t i = 0; i < pattern->getNumNotes(); ++i) {
        const Note* note = pattern->getNote(i);
        if (!note) {
            std::cout << "Error: Null note encountered at index " << i << std::endl;
            continue;
        }
        
        std::cout << std::setw(5) << i 
                  << std::setw(10) << note->pitch
                  << std::setw(10) << std::fixed << std::setprecision(2) << note->startTime
                  << std::setw(10) << std::fixed << std::setprecision(2) << note->duration
                  << std::setw(10) << std::fixed << std::setprecision(2) << note->velocity
                  << std::setw(10) << note->channel
                  << std::endl;
    }
    std::cout << "----------------------------------------------------" << std::endl;
}

// Helper function to print the song arrangement
void printSongArrangement(const Sequencer* sequencer) {
    std::cout << "\n=== Song Arrangement ===" << std::endl;
    std::cout << "Total Length: " << sequencer->getSongLength() << " beats" << std::endl;
    std::cout << "Pattern Instances: " << sequencer->getNumPatternInstances() << std::endl;
    
    if (sequencer->getNumPatternInstances() == 0) {
        std::cout << "No patterns in arrangement." << std::endl;
        return;
    }
    
    // Pagination settings
    const size_t pageSize = 5; // Show 5 items per page
    size_t startIdx = 0;
    
    while (startIdx < sequencer->getNumPatternInstances()) {
        std::cout << "----------------------------------------------------" << std::endl;
        std::cout << std::setw(5) << "Idx" << std::setw(20) << "Pattern" 
                  << std::setw(10) << "Start" << std::setw(10) << "End" << std::endl;
        std::cout << "----------------------------------------------------" << std::endl;
        
        // Calculate end index for current page
        size_t endIdx = std::min(startIdx + pageSize, sequencer->getNumPatternInstances());
        
        // Print current page of pattern instances
        for (size_t i = startIdx; i < endIdx; ++i) {
            auto instanceOpt = sequencer->getPatternInstance(i);
            if (!instanceOpt) {
                std::cout << std::setw(5) << i << std::setw(20) << "Invalid Instance" << std::endl;
                continue;
            }
            
            // Create a local copy of the PatternInstance from the optional
            PatternInstance instance = instanceOpt.value();
            
            const Pattern* pattern = sequencer->getPattern(instance.patternIndex);
            std::string patternName = pattern ? pattern->getName() : "Unknown";
            
            std::cout << std::setw(5) << i 
                      << std::setw(20) << patternName
                      << std::setw(10) << std::fixed << std::setprecision(2) << instance.startBeat
                      << std::setw(10) << std::fixed << std::setprecision(2) << instance.endBeat
                      << std::endl;
        }
        std::cout << "----------------------------------------------------" << std::endl;
        
        // Move to next page if there are more instances
        startIdx += pageSize;
        if (startIdx < sequencer->getNumPatternInstances()) {
            std::cout << "Showing " << (startIdx - pageSize + 1) << "-" << endIdx
                      << " of " << sequencer->getNumPatternInstances() << " instances." << std::endl;
            std::cout << "Press Enter to view more patterns...";
            std::cin.ignore(10000, '\n');
            std::cin.get();
        }
    }
}

// Main program
int main() {
    std::cout << "===== Advanced Sequencer Test =====" << std::endl;
    
    // Create sequencer
    auto sequencer = std::make_unique<Sequencer>(120.0, 4); // 120 BPM, 4/4 time
    
    // Create patterns
    sequencer->addPattern(createScalePattern("C Major Scale"));
    sequencer->addPattern(createChordPattern("C-F-G-C Progression"));
    sequencer->addPattern(createArpeggioPattern("C-F-G-C Arpeggios"));
    sequencer->addPattern(createBassPattern("Bass Pattern"));
    sequencer->addPattern(createDrumPattern("Basic Drum Pattern"));
    
    // Create MIDI exporter
    MidiFile midiExporter;
    
    // Setup for note callbacks (just for debugging)
    sequencer->setNoteCallbacks(
        // Note on callback (thread-safe)
        [](int pitch, float velocity, int channel, const Envelope& env) {
            std::lock_guard<std::mutex> lock(callbackMutex);
            try {
                std::cout << "Note On: " << pitch << " Velocity: " << velocity 
                          << " Channel: " << channel 
                          << " Env: A=" << env.attack << " D=" << env.decay 
                          << " S=" << env.sustain << " R=" << env.release << std::endl;
            } catch (const std::exception& e) {
                std::cerr << "Error in noteOnCallback: " << e.what() << std::endl;
            }
        },
        // Note off callback (thread-safe)
        [](int pitch, int channel) {
            std::lock_guard<std::mutex> lock(callbackMutex);
            try {
                std::cout << "Note Off: " << pitch << " Channel: " << channel << std::endl;
            } catch (const std::exception& e) {
                std::cerr << "Error in noteOffCallback: " << e.what() << std::endl;
            }
        }
    );
    
    // Setup transport callback with thread safety
    sequencer->setTransportCallback(
        [](double positionInBeats, int bar, int beat) {
            // Only print occasionally to avoid console spam
            // Use fmod for floating point modulo calculation
            if (std::fmod(positionInBeats * 4, 4.0) < 0.01) {
                std::lock_guard<std::mutex> lock(callbackMutex);
                try {
                    std::cout << "\rPosition: " << std::fixed << std::setprecision(2) 
                              << positionInBeats << " | Bar: " << bar 
                              << " Beat: " << beat << std::flush;
                } catch (const std::exception& e) {
                    std::cerr << "Error in transportCallback: " << e.what() << std::endl;
                }
            }
        }
    );
    
    // Main menu loop
    bool running = true;
    while (running) {
        std::vector<std::string> mainOptions = {
            "Manage Patterns",
            "Edit Song Arrangement",
            "Export to MIDI",
            "Apply Quantization/Swing",
            "Playback Test"
        };
        
        int choice = getUserChoice(mainOptions, "Advanced Sequencer Test Menu:");
        
        switch (choice) {
            case 0:
                running = false;
                break;
                
            case 1: { // Manage Patterns
                bool managingPatterns = true;
                while (managingPatterns) {
                    std::vector<std::string> patternOptions;
                    for (size_t i = 0; i < sequencer->getNumPatterns(); ++i) {
                        Pattern* pattern = sequencer->getPattern(i);
                        patternOptions.push_back(pattern ? pattern->getName() : "Unknown Pattern");
                    }
                    patternOptions.push_back("Create New Pattern");
                    
                    int patternChoice = getUserChoice(patternOptions, "Select Pattern to View/Edit:");
                    
                    if (patternChoice == 0) {
                        managingPatterns = false;
                    } else if (patternChoice > 0 && patternChoice <= static_cast<int>(sequencer->getNumPatterns())) {
                        Pattern* selectedPattern = sequencer->getPattern(patternChoice - 1);
                        printPattern(selectedPattern);
                        
                        // Could add pattern editing functionality here
                        std::cout << "Pattern editing not implemented in this demo." << std::endl;
                        std::cout << "Press Enter to continue...";
                        std::cin.ignore(10000, '\n');
                        std::cin.get();
                    } else if (patternChoice == static_cast<int>(patternOptions.size())) {
                        // This is safe because getUserChoice now validates the choice range
                        // This matches the index of "Create New Pattern" option
                        std::string patternName;
                        std::cout << "Enter name for new pattern: ";
                        std::cin.ignore(10000, '\n');
                        std::getline(std::cin, patternName);
                        
                        // Create a new empty pattern
                        auto newPattern = std::make_unique<Pattern>(patternName);
                        sequencer->addPattern(std::move(newPattern));
                        
                        std::cout << "New pattern created: " << patternName << std::endl;
                    }
                }
                break;
            }
                
            case 2: { // Edit Song Arrangement
                bool editingArrangement = true;
                while (editingArrangement) {
                    std::vector<std::string> arrangementOptions = {
                        "View Current Arrangement",
                        "Add Pattern to Song",
                        "Remove Pattern from Song",
                        "Clear Song Arrangement",
                        "Set Playback Mode"
                    };
                    
                    int arrangementChoice = getUserChoice(arrangementOptions, "Song Arrangement Menu:");
                    
                    if (arrangementChoice == 0) {
                        editingArrangement = false;
                    } else if (arrangementChoice == 1) {
                        printSongArrangement(sequencer.get());
                        
                        std::cout << "Press Enter to continue...";
                        std::cin.ignore(10000, '\n');
                        std::cin.get();
                    } else if (arrangementChoice == 2) {
                        // Add pattern to song
                        std::vector<std::string> patternOptions;
                        for (size_t i = 0; i < sequencer->getNumPatterns(); ++i) {
                            Pattern* pattern = sequencer->getPattern(i);
                            patternOptions.push_back(pattern ? pattern->getName() : "Unknown Pattern");
                        }
                        
                        int patternChoice = getUserChoice(patternOptions, "Select Pattern to Add:");
                        
                        if (patternChoice > 0 && patternChoice <= static_cast<int>(sequencer->getNumPatterns())) {
                            double startBeat;
                            std::cout << "Enter start position (in beats): ";
                            std::cin >> startBeat;
                            
                            if (std::cin.fail()) {
                                std::cin.clear();
                                std::cin.ignore(10000, '\n');
                                std::cout << "Invalid input. Using 0.0 as start position." << std::endl;
                                startBeat = 0.0;
                            }
                            
                            sequencer->addPatternToSong(patternChoice - 1, startBeat);
                            std::cout << "Pattern added to song arrangement." << std::endl;
                        }
                    } else if (arrangementChoice == 3) {
                        // Remove pattern from song
                        if (sequencer->getNumPatternInstances() == 0) {
                            std::cout << "No patterns in song arrangement to remove." << std::endl;
                        } else {
                            printSongArrangement(sequencer.get());
                            
                            int instanceIdx;
                            std::cout << "Enter index of pattern instance to remove: ";
                            std::cin >> instanceIdx;
                            
                            if (std::cin.fail() || instanceIdx < 0 || instanceIdx >= static_cast<int>(sequencer->getNumPatternInstances())) {
                                std::cin.clear();
                                std::cin.ignore(10000, '\n');
                                std::cout << "Invalid index." << std::endl;
                            } else {
                                sequencer->removePatternFromSong(instanceIdx);
                                std::cout << "Pattern instance removed from song arrangement." << std::endl;
                            }
                        }
                    } else if (arrangementChoice == 4) {
                        // Clear song arrangement
                        std::cout << "Are you sure you want to clear the song arrangement? (y/n): ";
                        char confirm;
                        std::cin >> confirm;
                        
                        if (confirm == 'y' || confirm == 'Y') {
                            sequencer->clearSong();
                            std::cout << "Song arrangement cleared." << std::endl;
                        }
                    } else if (arrangementChoice == 5) {
                        // Set playback mode
                        std::vector<std::string> modeOptions = {
                            "Single Pattern Mode",
                            "Song Arrangement Mode"
                        };
                        
                        int modeChoice = getUserChoice(modeOptions, "Select Playback Mode:");
                        
                        if (modeChoice == 1) {
                            sequencer->setPlaybackMode(PlaybackMode::SinglePattern);
                            std::cout << "Playback mode set to Single Pattern." << std::endl;
                        } else if (modeChoice == 2) {
                            sequencer->setPlaybackMode(PlaybackMode::Song);
                            std::cout << "Playback mode set to Song Arrangement." << std::endl;
                        }
                    }
                }
                break;
            }
                
            case 3: { // Export to MIDI
                std::vector<std::string> exportOptions = {
                    "Export Current Pattern",
                    "Export All Patterns as Separate Tracks",
                    "Export Song Arrangement"
                };
                
                int exportChoice = getUserChoice(exportOptions, "MIDI Export Menu:");
                
                if (exportChoice == 1) {
                    // Export current pattern
                    Pattern* currentPattern = sequencer->getPattern(sequencer->getCurrentPatternIndex());
                    if (!currentPattern) {
                        std::cout << "No current pattern to export!" << std::endl;
                        break;
                    }
                    
                    std::string filename = "output_pattern.mid";
                    if (midiExporter.exportPattern(*currentPattern, filename, sequencer->getTempo())) {
                        std::cout << "Pattern exported to " << filename << std::endl;
                    } else {
                        std::cout << "Failed to export pattern!" << std::endl;
                    }
                } else if (exportChoice == 2) {
                    // Export all patterns
                    std::vector<Pattern*> allPatterns;
                    for (size_t i = 0; i < sequencer->getNumPatterns(); ++i) {
                        Pattern* pattern = sequencer->getPattern(i);
                        if (pattern) {
                            allPatterns.push_back(pattern);
                        }
                    }
                    
                    if (allPatterns.empty()) {
                        std::cout << "No patterns to export!" << std::endl;
                        break;
                    }
                    
                    std::string filename = "output_all_patterns.mid";
                    if (midiExporter.exportPatterns(allPatterns, filename, sequencer->getTempo())) {
                        std::cout << "All patterns exported to " << filename << std::endl;
                    } else {
                        std::cout << "Failed to export patterns!" << std::endl;
                    }
                } else if (exportChoice == 3) {
                    // Export song arrangement
                    if (sequencer->getNumPatternInstances() == 0) {
                        std::cout << "No song arrangement to export!" << std::endl;
                        break;
                    }
                    
                    std::cout << "Song arrangement export not fully implemented in this demo." << std::endl;
                    std::cout << "This would require extending the MidiFile class to handle pattern instances." << std::endl;
                }
                
                break;
            }
                
            case 4: { // Apply Quantization/Swing
                std::vector<std::string> editOptions = {
                    "Quantize Pattern",
                    "Apply Swing"
                };
                
                int editChoice = getUserChoice(editOptions, "Pattern Edit Menu:");
                
                if (editChoice == 0) {
                    break;
                }
                
                // Select pattern to edit
                std::vector<std::string> patternOptions;
                for (size_t i = 0; i < sequencer->getNumPatterns(); ++i) {
                    Pattern* pattern = sequencer->getPattern(i);
                    patternOptions.push_back(pattern ? pattern->getName() : "Unknown Pattern");
                }
                
                int patternChoice = getUserChoice(patternOptions, "Select Pattern to Edit:");
                
                if (patternChoice <= 0 || patternChoice > static_cast<int>(sequencer->getNumPatterns())) {
                    break;
                }
                
                Pattern* selectedPattern = sequencer->getPattern(patternChoice - 1);
                if (!selectedPattern) {
                    std::cout << "Invalid pattern selection!" << std::endl;
                    break;
                }
                
                if (editChoice == 1) {
                    // Quantize
                    double gridSize;
                    std::cout << "Enter grid size for quantization (e.g., 0.25 for 16th notes): ";
                    std::cin >> gridSize;
                    
                    if (std::cin.fail() || gridSize <= 0.0) {
                        std::cin.clear();
                        std::cin.ignore(10000, '\n');
                        std::cout << "Invalid grid size. Using 0.25 (16th notes)." << std::endl;
                        gridSize = 0.25;
                    }
                    
                    selectedPattern->quantize(gridSize);
                    std::cout << "Pattern quantized to grid size " << gridSize << std::endl;
                    
                } else if (editChoice == 2) {
                    // Swing
                    double swingAmount;
                    std::cout << "Enter swing amount (0.0-0.5, where 0.33 is typical): ";
                    std::cin >> swingAmount;
                    
                    if (std::cin.fail() || swingAmount < 0.0 || swingAmount > 0.5) {
                        std::cin.clear();
                        std::cin.ignore(10000, '\n');
                        std::cout << "Invalid swing amount. Using 0.33." << std::endl;
                        swingAmount = 0.33;
                    }
                    
                    double gridSize;
                    std::cout << "Enter grid size for swing (e.g., 0.25 for 16th notes): ";
                    std::cin >> gridSize;
                    
                    if (std::cin.fail() || gridSize <= 0.0) {
                        std::cin.clear();
                        std::cin.ignore(10000, '\n');
                        std::cout << "Invalid grid size. Using 0.25 (16th notes)." << std::endl;
                        gridSize = 0.25;
                    }
                    
                    selectedPattern->applySwing(swingAmount, gridSize);
                    std::cout << "Swing applied with amount " << swingAmount 
                              << " on grid size " << gridSize << std::endl;
                }
                
                break;
            }
                
            case 5: { // Playback Test
                // Simple playback simulation since we don't have real-time audio
                
                std::cout << "\nPlayback Test (simulated)" << std::endl;
                std::cout << "Mode: " << (sequencer->getPlaybackMode() == PlaybackMode::SinglePattern ? 
                                        "Single Pattern" : "Song Arrangement") << std::endl;
                
                if (sequencer->getPlaybackMode() == PlaybackMode::SinglePattern) {
                    Pattern* currentPattern = sequencer->getPattern(sequencer->getCurrentPatternIndex());
                    std::cout << "Current Pattern: " << (currentPattern ? currentPattern->getName() : "None") << std::endl;
                } else {
                    std::cout << "Song Length: " << sequencer->getSongLength() << " beats" << std::endl;
                }
                
                std::cout << "Press Enter to start playback, press Enter again to stop..." << std::endl;
                std::cin.ignore(10000, '\n');
                std::cin.get();
                
                // Start playback
                sequencer->start();
                
                // Simulate playback by processing time slices
                const double frameDuration = 0.01; // 10ms frames
                bool playing = true;
                
                while (playing && sequencer->isPlaying()) {
                    sequencer->process(frameDuration);
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                    
                    // Check if user pressed Enter to stop
                    if (std::cin.peek() == '\n') {
                        std::cin.ignore(10000, '\n');
                        playing = false;
                    }
                }
                
                // Stop playback
                sequencer->stop();
                std::cout << "\nPlayback stopped." << std::endl;
                
                break;
            }
                
            default:
                std::cout << "Invalid choice. Please try again." << std::endl;
        }
    }
    
    std::cout << "\nAdvanced Sequencer Test completed!" << std::endl;
    return 0;
}