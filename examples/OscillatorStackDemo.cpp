#include <iostream>
#include <iomanip>
#include <vector>
#include <cmath>
#include <memory>
#include <string>
#include <chrono>
#include <thread>
#include <RtAudio.h>

#include "synthesis/wavetable/wavetable.h"
#include "synthesis/wavetable/oscillator_stack.h"
#include "synthesis/voice/stacked_voice.h"
#include "synthesis/voice/stacked_voice_manager.h"
#include "synthesis/modulators/envelope.h"

using namespace AIMusicHardware;

// Global audio parameters
constexpr int kSampleRate = 44100;
constexpr int kChannels = 2;  // Stereo output
constexpr int kBufferFrames = 256;

// Global voice manager for testing
std::unique_ptr<StackedVoiceManager> g_voiceManager;

// Global state
bool g_isRunning = true;
int g_unisonCount = 1;
float g_detuneAmount = 0.0f;
float g_stereoWidth = 0.0f;
float g_convergence = 0.0f;

// Audio callback function for RtAudio
int audioCallback(void *outputBuffer, void *inputBuffer, unsigned int nBufferFrames,
                 double streamTime, RtAudioStreamStatus status, void *userData) {
    float *buffer = static_cast<float*>(outputBuffer);
    
    // Clear buffer
    std::memset(buffer, 0, nBufferFrames * kChannels * sizeof(float));
    
    // Process voices
    g_voiceManager->process(buffer, nBufferFrames);
    
    return 0;
}

// Print the current unison settings
void printCurrentSettings() {
    std::cout << "----------------------------------------\n";
    std::cout << "Current Unison Settings:\n";
    std::cout << "  Oscillator Count: " << g_unisonCount << "\n";
    std::cout << "  Detune Amount: " << g_detuneAmount << " cents\n";
    std::cout << "  Stereo Width: " << g_stereoWidth << "\n";
    std::cout << "  Convergence: " << g_convergence << "\n";
    std::cout << "----------------------------------------\n";
}

// Print available commands
void printCommands() {
    std::cout << "\nCommands:\n";
    std::cout << "  1-8: Set oscillator count\n";
    std::cout << "  d <cents>: Set detune amount (0-100 cents)\n";
    std::cout << "  w <amount>: Set stereo width (0.0-1.0)\n";
    std::cout << "  c <amount>: Set convergence (0.0-1.0)\n";
    std::cout << "  p: Play a note (C4)\n";
    std::cout << "  s: Stop all notes\n";
    std::cout << "  q: Quit\n";
    std::cout << "  ?: Show commands\n";
}

int main() {
    std::cout << "==== Oscillator Stack Demo ====\n";
    std::cout << "Demonstrating oscillator stacking and unison features\n\n";
    
    // Create the voice manager
    g_voiceManager = std::make_unique<StackedVoiceManager>(kSampleRate, 16, g_unisonCount);

    // Configure envelope for all voices
    // Since we can't access the ADSR directly through StackedVoiceManager, we'll set attack/release through noteOn/Off timing
    
    // Initialize RtAudio
    RtAudio dac;
    if (dac.getDeviceCount() < 1) {
        std::cout << "No audio devices found!" << std::endl;
        return 1;
    }
    
    // Set up audio stream parameters
    RtAudio::StreamParameters parameters;
    parameters.deviceId = dac.getDefaultOutputDevice();
    parameters.nChannels = kChannels;
    parameters.firstChannel = 0;
    
    // Open audio stream
    try {
        unsigned int bufferFrames = kBufferFrames;
        dac.openStream(&parameters, nullptr, RTAUDIO_FLOAT32, kSampleRate, &bufferFrames,
                        &audioCallback, nullptr, nullptr);
        dac.startStream();
    } catch (std::exception& e) {
        std::cout << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    // Print initial state
    printCurrentSettings();
    printCommands();
    
    // Main loop for user commands
    std::string command;
    while (g_isRunning) {
        std::cout << "\n> ";
        std::getline(std::cin, command);
        
        if (command.empty()) continue;
        
        // Process command
        switch (command[0]) {
            case '1': case '2': case '3': case '4': case '5': case '6': case '7': case '8': {
                // Set oscillator count
                g_unisonCount = command[0] - '0';
                g_voiceManager->configureUnison(g_unisonCount, g_detuneAmount, g_stereoWidth, g_convergence);
                std::cout << "Set oscillator count to " << g_unisonCount << std::endl;
                break;
            }
            case 'd': {
                // Set detune amount
                if (command.length() > 1) {
                    try {
                        g_detuneAmount = std::stof(command.substr(2));
                        g_voiceManager->configureUnison(g_unisonCount, g_detuneAmount, g_stereoWidth, g_convergence);
                        std::cout << "Set detune amount to " << g_detuneAmount << " cents" << std::endl;
                    } catch (...) {
                        std::cout << "Invalid detune value. Use format: d 10.5" << std::endl;
                    }
                }
                break;
            }
            case 'w': {
                // Set stereo width
                if (command.length() > 1) {
                    try {
                        g_stereoWidth = std::stof(command.substr(2));
                        g_voiceManager->configureUnison(g_unisonCount, g_detuneAmount, g_stereoWidth, g_convergence);
                        std::cout << "Set stereo width to " << g_stereoWidth << std::endl;
                    } catch (...) {
                        std::cout << "Invalid width value. Use format: w 0.5" << std::endl;
                    }
                }
                break;
            }
            case 'c': {
                // Set convergence
                if (command.length() > 1) {
                    try {
                        g_convergence = std::stof(command.substr(2));
                        g_voiceManager->configureUnison(g_unisonCount, g_detuneAmount, g_stereoWidth, g_convergence);
                        std::cout << "Set convergence to " << g_convergence << std::endl;
                    } catch (...) {
                        std::cout << "Invalid convergence value. Use format: c 0.5" << std::endl;
                    }
                }
                break;
            }
            case 'p': {
                // Play a note (C4 = MIDI note 60)
                g_voiceManager->noteOn(60, 100);
                std::cout << "Playing note C4 with current settings" << std::endl;
                break;
            }
            case 's': {
                // Stop all notes
                g_voiceManager->allNotesOff();
                std::cout << "Stopped all notes" << std::endl;
                break;
            }
            case 'q': {
                // Quit
                g_isRunning = false;
                std::cout << "Exiting..." << std::endl;
                break;
            }
            case '?': {
                // Show commands
                printCommands();
                break;
            }
            default:
                std::cout << "Unknown command. Type ? for help." << std::endl;
        }
        
        // Show current settings after any command
        if (g_isRunning && command[0] != '?') {
            printCurrentSettings();
        }
    }
    
    // Clean up
    try {
        if (dac.isStreamOpen()) {
            dac.stopStream();
            dac.closeStream();
        }
    } catch (std::exception& e) {
        std::cout << "Error during cleanup: " << e.what() << std::endl;
    }
    
    return 0;
}