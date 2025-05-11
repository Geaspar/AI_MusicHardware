#include <iostream>
#include <iomanip>
#include <vector>
#include <cmath>
#include <memory>
#include <string>
#include <chrono>
#include <thread>
#include <RtAudio.h>

#include "effects/AdvancedFilter.h"
#include "effects/LadderFilter.h"
#include "effects/CombFilter.h"
#include "effects/FormantFilter.h"

using namespace AIMusicHardware;

// Global audio parameters
constexpr int kSampleRate = 44100;
constexpr int kChannels = 2;  // Stereo output
constexpr int kBufferFrames = 256;

// Global filter for testing
std::unique_ptr<AdvancedFilter> g_filter;

// Oscillator state
float g_phase = 0.0f;
float g_phaseIncrement = 0.0f;
bool g_isRunning = true;
bool g_audioEnabled = true;

// Filter parameters
float g_frequency = 1000.0f;
float g_resonance = 0.5f;
float g_filterMix = 1.0f;
AdvancedFilter::Type g_currentFilterType = AdvancedFilter::Type::LowPass;
bool g_blendEnabled = false;
float g_blendAmount = 0.0f;
AdvancedFilter::Type g_blendFilterType = AdvancedFilter::Type::HighPass;

// Audio source type
enum class SourceType {
    Sine,
    Sawtooth,
    Square,
    Triangle,
    Noise,
    Impulse
};

SourceType g_sourceType = SourceType::Sawtooth;
float g_sourceFrequency = 220.0f;  // A3

// Audio callback function for RtAudio
int audioCallback(void *outputBuffer, void *inputBuffer, unsigned int nBufferFrames,
                 double streamTime, RtAudioStreamStatus status, void *userData) {
    float *buffer = static_cast<float*>(outputBuffer);
    
    // Clear buffer
    std::memset(buffer, 0, nBufferFrames * kChannels * sizeof(float));
    
    if (!g_audioEnabled) {
        return 0;
    }
    
    // Generate source waveform
    for (unsigned int i = 0; i < nBufferFrames; ++i) {
        float sample = 0.0f;
        
        switch (g_sourceType) {
            case SourceType::Sine:
                sample = 0.5f * std::sin(g_phase * 2.0f * PI);
                break;
                
            case SourceType::Sawtooth:
                sample = 0.5f * (2.0f * (g_phase - std::floor(g_phase + 0.5f)));
                break;
                
            case SourceType::Square:
                sample = 0.5f * (g_phase < 0.5f ? 1.0f : -1.0f);
                break;
                
            case SourceType::Triangle:
                sample = 0.5f * (1.0f - 4.0f * std::abs(std::round(g_phase) - g_phase));
                break;
                
            case SourceType::Noise:
                sample = 0.5f * (std::rand() / static_cast<float>(RAND_MAX) * 2.0f - 1.0f);
                break;
                
            case SourceType::Impulse:
                // Impulses every 0.25 seconds
                sample = ((static_cast<int>(streamTime * 4) % 4) == 0 && i == 0) ? 0.8f : 0.0f;
                break;
        }
        
        // Set left and right channels
        buffer[i * 2] = buffer[i * 2 + 1] = sample;
        
        // Update oscillator phase
        g_phase += g_phaseIncrement;
        if (g_phase >= 1.0f) g_phase -= 1.0f;
    }
    
    // Process through the filter
    if (g_filter) {
        g_filter->process(buffer, nBufferFrames);
    }
    
    return 0;
}

void updateFilterParameters() {
    if (!g_filter) return;
    
    // Set common parameters
    g_filter->setParameter("frequency", g_frequency);
    g_filter->setParameter("resonance", g_resonance);
    g_filter->setParameter("mix", g_filterMix);
    
    // Set the filter type
    g_filter->setFilterType(g_currentFilterType);
    
    // Set blend parameters
    g_filter->setBlendMode(g_blendEnabled);
    g_filter->setBlendType(g_blendFilterType);
    g_filter->setParameter("blend_amount", g_blendAmount);
    
    // Type-specific parameters (can add more as needed)
    switch (g_currentFilterType) {
        case AdvancedFilter::Type::LadderLowPass:
        case AdvancedFilter::Type::LadderHighPass:
            g_filter->setParameter("drive", 1.5f);  // Slight overdrive for ladder filter
            break;
            
        case AdvancedFilter::Type::Comb:
            g_filter->setParameter("delay_time", 5.0f);          // 5ms delay
            g_filter->setParameter("feedback", 0.7f);            // Medium feedback
            g_filter->setParameter("mod_amount", 0.0f);          // No modulation
            break;
            
        case AdvancedFilter::Type::Phaser:
            g_filter->setParameter("delay_time", 2.0f);          // 2ms delay
            g_filter->setParameter("feedback", 0.7f);            // Medium feedback
            g_filter->setParameter("mod_amount", 1.5f);          // Some modulation
            g_filter->setParameter("mod_rate", 0.2f);            // Slow modulation
            break;
            
        case AdvancedFilter::Type::Formant:
            g_filter->setParameter("vowel", 0.0f);               // 'A' vowel
            g_filter->setParameter("morph", 0.0f);               // No morphing
            g_filter->setParameter("gender", 0.5f);              // Neutral gender
            g_filter->setParameter("resonance", 0.8f);           // High resonance
            break;
    }
}

// Print the current settings
void printCurrentSettings() {
    std::cout << "----------------------------------------\n";
    std::cout << "Current Settings:\n";
    std::cout << "  Source Type: ";
    switch (g_sourceType) {
        case SourceType::Sine: std::cout << "Sine"; break;
        case SourceType::Sawtooth: std::cout << "Sawtooth"; break;
        case SourceType::Square: std::cout << "Square"; break;
        case SourceType::Triangle: std::cout << "Triangle"; break;
        case SourceType::Noise: std::cout << "Noise"; break;
        case SourceType::Impulse: std::cout << "Impulse"; break;
    }
    std::cout << " (" << g_sourceFrequency << " Hz)\n";
    
    std::cout << "  Filter Type: ";
    switch (g_currentFilterType) {
        case AdvancedFilter::Type::LowPass: std::cout << "Biquad Low Pass"; break;
        case AdvancedFilter::Type::HighPass: std::cout << "Biquad High Pass"; break;
        case AdvancedFilter::Type::BandPass: std::cout << "Biquad Band Pass"; break;
        case AdvancedFilter::Type::Notch: std::cout << "Biquad Notch"; break;
        case AdvancedFilter::Type::LadderLowPass: std::cout << "Ladder Low Pass"; break;
        case AdvancedFilter::Type::LadderHighPass: std::cout << "Ladder High Pass"; break;
        case AdvancedFilter::Type::Comb: std::cout << "Comb Filter"; break;
        case AdvancedFilter::Type::Phaser: std::cout << "Phaser"; break;
        case AdvancedFilter::Type::Formant: std::cout << "Formant Filter"; break;
        default: std::cout << "Unknown"; break;
    }
    
    std::cout << "\n  Frequency: " << g_frequency << " Hz";
    std::cout << "\n  Resonance: " << g_resonance;
    std::cout << "\n  Filter Mix: " << g_filterMix * 100.0f << "%";
    
    if (g_blendEnabled) {
        std::cout << "\n  Blend Enabled: ";
        switch (g_blendFilterType) {
            case AdvancedFilter::Type::LowPass: std::cout << "Biquad Low Pass"; break;
            case AdvancedFilter::Type::HighPass: std::cout << "Biquad High Pass"; break;
            case AdvancedFilter::Type::BandPass: std::cout << "Biquad Band Pass"; break;
            case AdvancedFilter::Type::Notch: std::cout << "Biquad Notch"; break;
            case AdvancedFilter::Type::LadderLowPass: std::cout << "Ladder Low Pass"; break;
            case AdvancedFilter::Type::LadderHighPass: std::cout << "Ladder High Pass"; break;
            case AdvancedFilter::Type::Comb: std::cout << "Comb Filter"; break;
            case AdvancedFilter::Type::Phaser: std::cout << "Phaser"; break;
            case AdvancedFilter::Type::Formant: std::cout << "Formant Filter"; break;
            default: std::cout << "Unknown"; break;
        }
        std::cout << " (Blend: " << g_blendAmount * 100.0f << "%)";
    }
    
    std::cout << "\n----------------------------------------\n";
}

// Print available commands
void printCommands() {
    std::cout << "\nCommands:\n";
    std::cout << "  1-9: Set filter type\n";
    std::cout << "    1: Biquad Low Pass\n";
    std::cout << "    2: Biquad High Pass\n";
    std::cout << "    3: Biquad Band Pass\n";
    std::cout << "    4: Biquad Notch\n";
    std::cout << "    5: Ladder Low Pass\n";
    std::cout << "    6: Ladder High Pass\n";
    std::cout << "    7: Comb Filter\n";
    std::cout << "    8: Phaser\n";
    std::cout << "    9: Formant Filter\n";
    std::cout << "  s#: Set source type (1=Sine, 2=Saw, 3=Square, 4=Triangle, 5=Noise, 6=Impulse)\n";
    std::cout << "  f <freq>: Set filter frequency (20-20000Hz)\n";
    std::cout << "  r <resonance>: Set resonance (0.0-1.0)\n";
    std::cout << "  m <mix>: Set filter mix (0.0-1.0)\n";
    std::cout << "  n <note>: Play MIDI note (0-127)\n";
    std::cout << "  b <0|1>: Toggle blend mode\n";
    std::cout << "  bt <1-9>: Set blend filter type\n";
    std::cout << "  ba <amount>: Set blend amount (0.0-1.0)\n";
    std::cout << "  p <0|1>: Toggle audio processing\n";
    std::cout << "  q: Quit\n";
    std::cout << "  ?: Show commands\n";
}

// Helper to convert MIDI note to frequency
float midiNoteToFrequency(int note) {
    return 440.0f * std::pow(2.0f, (note - 69) / 12.0f);
}

int main() {
    std::cout << "==== Advanced Filter Demo ====\n";
    std::cout << "Demonstrating various filter types and blending\n\n";
    
    // Create the filter
    g_filter = std::make_unique<AdvancedFilter>(kSampleRate, g_currentFilterType);
    
    // Update filter parameters
    updateFilterParameters();
    
    // Set initial source frequency
    g_phaseIncrement = g_sourceFrequency / kSampleRate;
    
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
            case '1': case '2': case '3': case '4': case '5': 
            case '6': case '7': case '8': case '9': {
                // Set filter type
                int typeIdx = command[0] - '1';
                if (typeIdx >= 0 && typeIdx < static_cast<int>(AdvancedFilter::Type::NumTypes)) {
                    g_currentFilterType = static_cast<AdvancedFilter::Type>(typeIdx);
                    updateFilterParameters();
                    std::cout << "Changed filter type" << std::endl;
                }
                break;
            }
            case 's': {
                // Change source type
                if (command.length() > 1) {
                    int sourceIdx = command[1] - '1';
                    if (sourceIdx >= 0 && sourceIdx < 6) {
                        g_sourceType = static_cast<SourceType>(sourceIdx);
                        std::cout << "Changed source type" << std::endl;
                    }
                }
                break;
            }
            case 'f': {
                // Set filter frequency
                if (command.length() > 1) {
                    try {
                        float freq = std::stof(command.substr(2));
                        g_frequency = std::clamp(freq, 20.0f, 20000.0f);
                        g_filter->setParameter("frequency", g_frequency);
                        std::cout << "Set filter frequency to " << g_frequency << " Hz" << std::endl;
                    } catch (...) {
                        std::cout << "Invalid frequency value. Use format: f 1000" << std::endl;
                    }
                }
                break;
            }
            case 'r': {
                // Set resonance
                if (command.length() > 1) {
                    try {
                        float res = std::stof(command.substr(2));
                        g_resonance = std::clamp(res, 0.0f, 1.0f);
                        g_filter->setParameter("resonance", g_resonance);
                        std::cout << "Set resonance to " << g_resonance << std::endl;
                    } catch (...) {
                        std::cout << "Invalid resonance value. Use format: r 0.7" << std::endl;
                    }
                }
                break;
            }
            case 'm': {
                // Set mix
                if (command.length() > 1) {
                    try {
                        float mix = std::stof(command.substr(2));
                        g_filterMix = std::clamp(mix, 0.0f, 1.0f);
                        g_filter->setParameter("mix", g_filterMix);
                        std::cout << "Set filter mix to " << g_filterMix * 100.0f << "%" << std::endl;
                    } catch (...) {
                        std::cout << "Invalid mix value. Use format: m 0.5" << std::endl;
                    }
                }
                break;
            }
            case 'n': {
                // Set note (MIDI)
                if (command.length() > 1) {
                    try {
                        int midiNote = std::stoi(command.substr(2));
                        midiNote = std::clamp(midiNote, 0, 127);
                        g_sourceFrequency = midiNoteToFrequency(midiNote);
                        g_phaseIncrement = g_sourceFrequency / kSampleRate;
                        std::cout << "Playing MIDI note " << midiNote << " (" << g_sourceFrequency << " Hz)" << std::endl;
                    } catch (...) {
                        std::cout << "Invalid note value. Use format: n 60" << std::endl;
                    }
                }
                break;
            }
            case 'b': {
                // Toggle blend mode or set blend type
                if (command.length() > 1) {
                    if (command[1] == 't' && command.length() > 2) {
                        // Set blend filter type
                        try {
                            int typeIdx = command[2] - '1';
                            if (typeIdx >= 0 && typeIdx < static_cast<int>(AdvancedFilter::Type::NumTypes)) {
                                g_blendFilterType = static_cast<AdvancedFilter::Type>(typeIdx);
                                g_filter->setBlendType(g_blendFilterType);
                                std::cout << "Set blend filter type" << std::endl;
                            }
                        } catch (...) {
                            std::cout << "Invalid blend type. Use format: bt 5" << std::endl;
                        }
                    }
                    else if (command[1] == 'a' && command.length() > 2) {
                        // Set blend amount
                        try {
                            float amount = std::stof(command.substr(3));
                            g_blendAmount = std::clamp(amount, 0.0f, 1.0f);
                            g_filter->setParameter("blend_amount", g_blendAmount);
                            std::cout << "Set blend amount to " << g_blendAmount * 100.0f << "%" << std::endl;
                        } catch (...) {
                            std::cout << "Invalid blend amount. Use format: ba 0.5" << std::endl;
                        }
                    }
                    else {
                        // Toggle blend enabled
                        int enabled = std::stoi(command.substr(1));
                        g_blendEnabled = (enabled != 0);
                        g_filter->setBlendMode(g_blendEnabled);
                        std::cout << "Blend mode " << (g_blendEnabled ? "enabled" : "disabled") << std::endl;
                    }
                }
                break;
            }
            case 'p': {
                // Toggle audio processing
                if (command.length() > 1) {
                    int enabled = std::stoi(command.substr(1));
                    g_audioEnabled = (enabled != 0);
                    std::cout << "Audio " << (g_audioEnabled ? "enabled" : "disabled") << std::endl;
                }
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