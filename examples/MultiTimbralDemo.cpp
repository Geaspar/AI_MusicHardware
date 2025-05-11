#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <memory>
#include <vector>
#include <iomanip>

#ifdef HAVE_RTAUDIO
#include "../include/audio/AudioEngine.h"
#endif

#ifdef HAVE_RTMIDI
#include "../include/midi/MidiInterface.h"
#include "../include/midi/MidiManager.h"
#include "../include/midi/MultiTimbralMidiRouter.h"
#endif

#include "../include/synthesis/multitimbral/MultiTimbralEngine.h"
#include "../include/synthesis/multitimbral/ChannelSynthesizer.h"

using namespace AIMusicHardware;

// Terminal colors (ANSI escape codes) for nicer display
namespace Color {
    const std::string Reset = "\033[0m";
    const std::string Bold = "\033[1m";
    const std::string Red = "\033[31m";
    const std::string Green = "\033[32m";
    const std::string Yellow = "\033[33m";
    const std::string Blue = "\033[34m";
    const std::string Magenta = "\033[35m";
    const std::string Cyan = "\033[36m";
    
    // Channel colors (one for each MIDI channel)
    const std::vector<std::string> ChannelColors = {
        "\033[38;5;196m", // Channel 1 - Bright Red
        "\033[38;5;46m",  // Channel 2 - Bright Green
        "\033[38;5;21m",  // Channel 3 - Blue
        "\033[38;5;226m", // Channel 4 - Yellow
        "\033[38;5;201m", // Channel 5 - Magenta
        "\033[38;5;51m",  // Channel 6 - Cyan
        "\033[38;5;208m", // Channel 7 - Orange
        "\033[38;5;93m",  // Channel 8 - Purple
        "\033[38;5;124m", // Channel 9 - Dark Red
        "\033[38;5;34m",  // Channel 10 - Dark Green
        "\033[38;5;33m",  // Channel 11 - Light Blue
        "\033[38;5;214m", // Channel 12 - Dark Yellow
        "\033[38;5;127m", // Channel 13 - Dark Magenta
        "\033[38;5;39m",  // Channel 14 - Light Cyan
        "\033[38;5;202m", // Channel 15 - Dark Orange
        "\033[38;5;57m"   // Channel 16 - Dark Purple
    };
}

// Function to display note name
std::string getNoteName(int midiNote) {
    static const std::vector<std::string> noteNames = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
    int octave = (midiNote / 12) - 1;
    int noteIndex = midiNote % 12;
    return noteNames[noteIndex] + std::to_string(octave);
}

// Audio callback function for RtAudio
#ifdef HAVE_RTAUDIO
int audioCallback(void* outputBuffer, void* inputBuffer, unsigned int nFrames,
                 double streamTime, unsigned int status, void* userData) {
    // Cast user data to MultiTimbralEngine
    MultiTimbralEngine* engine = static_cast<MultiTimbralEngine*>(userData);
    
    // Process audio
    engine->process(static_cast<float*>(outputBuffer), nFrames);
    
    return 0;
}
#endif

// Handles MIDI messages
#ifdef HAVE_RTMIDI
void midiCallback(const std::vector<unsigned char>* message, void* userData) {
    MultiTimbralEngine* engine = static_cast<MultiTimbralEngine*>(userData);
    
    // Extract MIDI message data
    if (message->size() < 2) return;
    
    unsigned char status = (*message)[0];
    unsigned char data1 = (*message)[1];
    unsigned char data2 = message->size() > 2 ? (*message)[2] : 0;
    
    // Extract channel (lower 4 bits of status byte)
    int channel = status & 0x0F;
    
    // Extract message type (upper 4 bits of status byte)
    int messageType = status & 0xF0;
    
    // Display MIDI message with color based on channel
    std::cout << Color::ChannelColors[channel] << "MIDI CH" << (channel + 1) << ": ";
    
    // Process based on message type
    switch (messageType) {
        case 0x90: // Note On
            if (data2 > 0) {
                std::cout << "Note On: " << getNoteName(data1) << " (" << (int)data1 
                          << ") Velocity: " << (int)data2 << Color::Reset << std::endl;
                engine->noteOn(data1, data2 / 127.0f, channel);
            } else {
                // Note On with velocity 0 is equivalent to Note Off
                std::cout << "Note Off: " << getNoteName(data1) << " (" << (int)data1 
                          << ")" << Color::Reset << std::endl;
                engine->noteOff(data1, channel);
            }
            break;
            
        case 0x80: // Note Off
            std::cout << "Note Off: " << getNoteName(data1) << " (" << (int)data1 
                      << ")" << Color::Reset << std::endl;
            engine->noteOff(data1, channel);
            break;
            
        case 0xB0: // Control Change
            std::cout << "Control Change: CC" << (int)data1 << " Value: " 
                      << (int)data2 << Color::Reset << std::endl;
            engine->controlChange(data1, data2, channel);
            break;
            
        case 0xC0: // Program Change
            std::cout << "Program Change: " << (int)data1 << Color::Reset << std::endl;
            engine->programChange(data1, channel);
            break;
            
        case 0xE0: // Pitch Bend
            {
                // Combine bytes to get 14-bit value
                int bendValue = ((data2 << 7) | data1) - 8192;
                float normalizedBend = bendValue / 8192.0f; // Range: -1.0 to +1.0
                std::cout << "Pitch Bend: " << normalizedBend << Color::Reset << std::endl;
                engine->pitchBend(normalizedBend, channel);
            }
            break;
            
        case 0xD0: // Channel Pressure
            std::cout << "Channel Pressure: " << (int)data1 << Color::Reset << std::endl;
            engine->channelPressure(data1 / 127.0f, channel);
            break;
            
        case 0xA0: // Polyphonic Key Pressure (Aftertouch)
            std::cout << "Aftertouch: Note " << getNoteName(data1) << " (" << (int)data1 
                      << ") Pressure: " << (int)data2 << Color::Reset << std::endl;
            engine->aftertouch(data1, data2 / 127.0f, channel);
            break;
            
        default:
            std::cout << "Unhandled MIDI message: " << std::hex 
                      << (int)status << std::dec << Color::Reset << std::endl;
            break;
    }
}
#endif

// Display menu and get user choice
int displayMenu() {
    std::cout << std::endl;
    std::cout << Color::Bold << Color::Cyan << "=== Multi-Timbral Synthesizer Demo ===" << Color::Reset << std::endl;
    std::cout << "1. " << Color::Green << "Show active channels" << Color::Reset << std::endl;
    std::cout << "2. " << Color::Green << "Activate/deactivate a channel" << Color::Reset << std::endl;
    std::cout << "3. " << Color::Green << "Set channel volume/pan" << Color::Reset << std::endl;
    std::cout << "4. " << Color::Green << "Set channel priority" << Color::Reset << std::endl;
    std::cout << "5. " << Color::Green << "Set voice allocation strategy" << Color::Reset << std::endl;
    std::cout << "6. " << Color::Green << "Setup keyboard split" << Color::Reset << std::endl;
    std::cout << "7. " << Color::Green << "Setup layered channels" << Color::Reset << std::endl;
    std::cout << "8. " << Color::Green << "Clear performance config" << Color::Reset << std::endl;
    std::cout << "9. " << Color::Green << "Play test notes" << Color::Reset << std::endl;
    std::cout << "0. " << Color::Red << "Exit" << Color::Reset << std::endl;
    std::cout << Color::Bold << "Enter your choice: " << Color::Reset;
    
    int choice;
    std::cin >> choice;
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    
    return choice;
}

// Show active channels
void showActiveChannels(MultiTimbralEngine* engine) {
    std::cout << Color::Bold << Color::Cyan << "Channel Status:" << Color::Reset << std::endl;
    std::cout << "┌─────┬─────────┬──────────┬─────────┬─────────┬───────────────┐" << std::endl;
    std::cout << "│ CH  │ Active  │ Priority │ Volume  │   Pan   │ Preset        │" << std::endl;
    std::cout << "├─────┼─────────┼──────────┼─────────┼─────────┼───────────────┤" << std::endl;
    
    for (int i = 0; i < 16; ++i) {
        bool active = engine->isChannelActive(i);
        int priority = engine->getChannelPriority(i);
        float volume = engine->getChannelVolume(i);
        float pan = engine->getChannelPan(i);
        std::string presetName = engine->getChannelPresetName(i);
        
        std::cout << "│ " << Color::ChannelColors[i] << std::setw(2) << (i + 1) << Color::Reset 
                  << " │ " << (active ? Color::Green + "Yes" + Color::Reset : Color::Red + "No " + Color::Reset)
                  << " │ " << std::setw(8) << priority
                  << " │ " << std::fixed << std::setprecision(2) << std::setw(6) << volume
                  << " │ " << std::setw(7) << pan
                  << " │ " << std::setw(13) << (presetName.empty() ? "Default" : presetName) << " │" << std::endl;
    }
    
    std::cout << "└─────┴─────────┴──────────┴─────────┴─────────┴───────────────┘" << std::endl;
}

// Activate/deactivate a channel
void toggleChannelActive(MultiTimbralEngine* engine) {
    std::cout << Color::Bold << "Enter channel number (1-16): " << Color::Reset;
    int channelNum;
    std::cin >> channelNum;
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    
    if (channelNum < 1 || channelNum > 16) {
        std::cout << Color::Red << "Invalid channel number!" << Color::Reset << std::endl;
        return;
    }
    
    int channel = channelNum - 1;
    bool currentState = engine->isChannelActive(channel);
    
    std::cout << "Channel " << channelNum << " is currently " 
              << (currentState ? "active" : "inactive") << "." << std::endl;
    std::cout << "Toggle? (y/n): ";
    char response;
    std::cin >> response;
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    
    if (response == 'y' || response == 'Y') {
        engine->setChannelActive(channel, !currentState);
        std::cout << Color::Green << "Channel " << channelNum << " is now " 
                  << (!currentState ? "active" : "inactive") << "." << Color::Reset << std::endl;
    }
}

// Set channel volume/pan
void setChannelVolumePan(MultiTimbralEngine* engine) {
    std::cout << Color::Bold << "Enter channel number (1-16): " << Color::Reset;
    int channelNum;
    std::cin >> channelNum;
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    
    if (channelNum < 1 || channelNum > 16) {
        std::cout << Color::Red << "Invalid channel number!" << Color::Reset << std::endl;
        return;
    }
    
    int channel = channelNum - 1;
    
    // Volume
    std::cout << "Current volume: " << engine->getChannelVolume(channel) << std::endl;
    std::cout << "Enter new volume (0.0-1.0): ";
    float volume;
    std::cin >> volume;
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    
    if (volume >= 0.0f && volume <= 1.0f) {
        engine->setChannelVolume(channel, volume);
        std::cout << Color::Green << "Volume updated." << Color::Reset << std::endl;
    } else {
        std::cout << Color::Red << "Invalid volume value!" << Color::Reset << std::endl;
    }
    
    // Pan
    std::cout << "Current pan: " << engine->getChannelPan(channel) << std::endl;
    std::cout << "Enter new pan (-1.0=left, 0.0=center, 1.0=right): ";
    float pan;
    std::cin >> pan;
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    
    if (pan >= -1.0f && pan <= 1.0f) {
        engine->setChannelPan(channel, pan);
        std::cout << Color::Green << "Pan updated." << Color::Reset << std::endl;
    } else {
        std::cout << Color::Red << "Invalid pan value!" << Color::Reset << std::endl;
    }
}

// Set channel priority
void setChannelPriority(MultiTimbralEngine* engine) {
    std::cout << Color::Bold << "Enter channel number (1-16): " << Color::Reset;
    int channelNum;
    std::cin >> channelNum;
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    
    if (channelNum < 1 || channelNum > 16) {
        std::cout << Color::Red << "Invalid channel number!" << Color::Reset << std::endl;
        return;
    }
    
    int channel = channelNum - 1;
    
    std::cout << "Current priority: " << engine->getChannelPriority(channel) << std::endl;
    std::cout << "Enter new priority (1-10): ";
    int priority;
    std::cin >> priority;
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    
    if (priority >= 1 && priority <= 10) {
        engine->setChannelPriority(channel, priority);
        std::cout << Color::Green << "Priority updated." << Color::Reset << std::endl;
    } else {
        std::cout << Color::Red << "Invalid priority value!" << Color::Reset << std::endl;
    }
}

// Set voice allocation strategy
void setVoiceAllocationStrategy(MultiTimbralEngine* engine) {
    std::cout << Color::Bold << "Voice Allocation Strategies:" << Color::Reset << std::endl;
    std::cout << "1. Equal - Divide voices equally among active channels" << std::endl;
    std::cout << "2. Priority Based - Allocate more voices to higher priority channels" << std::endl;
    std::cout << "3. Dynamic - Allocate based on recent usage patterns" << std::endl;
    std::cout << "Enter strategy number: ";
    
    int choice;
    std::cin >> choice;
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    
    VoiceAllocationStrategy strategy;
    
    switch (choice) {
        case 1:
            strategy = VoiceAllocationStrategy::Equal;
            break;
        case 2:
            strategy = VoiceAllocationStrategy::PriorityBased;
            break;
        case 3:
            strategy = VoiceAllocationStrategy::Dynamic;
            break;
        default:
            std::cout << Color::Red << "Invalid choice!" << Color::Reset << std::endl;
            return;
    }
    
    engine->setVoiceAllocationStrategy(strategy);
    std::cout << Color::Green << "Voice allocation strategy updated." << Color::Reset << std::endl;
}

// Setup keyboard split
void setupKeyboardSplit(MultiTimbralEngine* engine, MultiTimbralMidiRouter* router = nullptr) {
    std::cout << Color::Bold << "Setup Keyboard Split" << Color::Reset << std::endl;

    std::cout << "Enter split point (MIDI note number, 0-127): ";
    int splitPoint;
    std::cin >> splitPoint;
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

    if (splitPoint < 0 || splitPoint > 127) {
        std::cout << Color::Red << "Invalid split point!" << Color::Reset << std::endl;
        return;
    }

    std::cout << "Split point set to " << getNoteName(splitPoint) << " (" << splitPoint << ")" << std::endl;

    std::cout << "Enter lower channel (1-16): ";
    int lowerChannel;
    std::cin >> lowerChannel;
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

    if (lowerChannel < 1 || lowerChannel > 16) {
        std::cout << Color::Red << "Invalid channel number!" << Color::Reset << std::endl;
        return;
    }

    std::cout << "Enter upper channel (1-16): ";
    int upperChannel;
    std::cin >> upperChannel;
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

    if (upperChannel < 1 || upperChannel > 16) {
        std::cout << Color::Red << "Invalid channel number!" << Color::Reset << std::endl;
        return;
    }

    // Convert to 0-based channel numbers
    int lowerChannel0 = lowerChannel - 1;
    int upperChannel0 = upperChannel - 1;

    // Configure both the engine and the router if available
    engine->setupKeyboardSplit(splitPoint, lowerChannel0, upperChannel0);

    if (router) {
        router->setupKeyboardSplit(splitPoint, lowerChannel0, upperChannel0);
    }

    std::cout << Color::Green << "Keyboard split configured." << Color::Reset << std::endl;
    std::cout << "Notes below " << getNoteName(splitPoint) << " will play on channel " << lowerChannel << std::endl;
    std::cout << "Notes at or above " << getNoteName(splitPoint) << " will play on channel " << upperChannel << std::endl;
}

// Setup layered channels
void setupLayeredChannels(MultiTimbralEngine* engine, MultiTimbralMidiRouter* router = nullptr) {
    std::cout << Color::Bold << "Setup Layered Channels" << Color::Reset << std::endl;
    std::cout << "Enter channels to layer (1-16, comma separated, e.g., 1,2,3): ";

    std::string input;
    std::getline(std::cin, input);

    std::vector<int> channels;
    std::stringstream ss(input);
    std::string channelStr;

    while (std::getline(ss, channelStr, ',')) {
        try {
            int channel = std::stoi(channelStr);
            if (channel >= 1 && channel <= 16) {
                channels.push_back(channel - 1); // Convert to 0-based index
            } else {
                std::cout << Color::Yellow << "Ignoring invalid channel: " << channel << Color::Reset << std::endl;
            }
        } catch (const std::exception& e) {
            std::cout << Color::Yellow << "Ignoring invalid input: " << channelStr << Color::Reset << std::endl;
        }
    }

    if (channels.empty()) {
        std::cout << Color::Red << "No valid channels specified!" << Color::Reset << std::endl;
        return;
    }

    // Configure both the engine and the router if available
    engine->setupLayeredChannels(channels);

    if (router) {
        router->setupLayeredChannels(channels);
    }

    std::cout << Color::Green << "Channels layered: ";
    for (size_t i = 0; i < channels.size(); ++i) {
        std::cout << (channels[i] + 1);
        if (i < channels.size() - 1) {
            std::cout << ", ";
        }
    }
    std::cout << Color::Reset << std::endl;
}

// Clear performance configuration
void clearPerformanceConfig(MultiTimbralEngine* engine, MultiTimbralMidiRouter* router = nullptr) {
    engine->clearPerformanceConfig();

    if (router) {
        router->clearPerformanceConfig();
    }

    std::cout << Color::Green << "Performance configuration cleared." << Color::Reset << std::endl;
}

// Play test notes to demonstrate multi-timbral functionality
void playTestNotes(MultiTimbralEngine* engine) {
    std::cout << Color::Bold << "Playing test notes on active channels..." << Color::Reset << std::endl;
    
    // Play a simple scale on each active channel
    const int notes[] = {60, 62, 64, 65, 67, 69, 71, 72}; // C4 to C5 major scale
    const int numNotes = 8;
    
    for (int i = 0; i < 16; ++i) {
        if (engine->isChannelActive(i)) {
            std::cout << Color::ChannelColors[i] << "Playing scale on channel " << (i + 1) << Color::Reset << std::endl;
            
            for (int j = 0; j < numNotes; ++j) {
                int note = notes[j];
                std::cout << Color::ChannelColors[i] << "CH" << (i + 1) << ": " 
                          << getNoteName(note) << Color::Reset << std::endl;
                
                engine->noteOn(note, 0.7f, i);
                std::this_thread::sleep_for(std::chrono::milliseconds(300));
                engine->noteOff(note, i);
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
            
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
    }
    
    std::cout << Color::Green << "Test notes complete." << Color::Reset << std::endl;
}

int main(int argc, char* argv[]) {
    std::cout << Color::Bold << Color::Cyan;
    std::cout << "=============================================" << std::endl;
    std::cout << "      AIMusicHardware Multi-Timbral Demo     " << std::endl;
    std::cout << "=============================================" << std::endl;
    std::cout << Color::Reset << std::endl;
    
    // Create the multi-timbral engine
    std::unique_ptr<MultiTimbralEngine> engine = std::make_unique<MultiTimbralEngine>(44100, 64);
    
    // Initialize the engine
    if (!engine->initialize()) {
        std::cerr << Color::Red << "Failed to initialize multi-timbral engine!" << Color::Reset << std::endl;
        return 1;
    }
    
    std::cout << "Multi-timbral engine initialized with 64 total voices." << std::endl;
    
    // Initialize audio engine
#ifdef HAVE_RTAUDIO
    std::unique_ptr<AudioEngine> audioEngine = std::make_unique<AudioEngine>();
    if (!audioEngine->initialize()) {
        std::cerr << Color::Red << "Failed to initialize audio engine!" << Color::Reset << std::endl;
        return 1;
    }
    
    // Set audio callback
    audioEngine->setCallback(audioCallback, engine.get());
    
    // Start audio
    if (!audioEngine->start()) {
        std::cerr << Color::Red << "Failed to start audio engine!" << Color::Reset << std::endl;
        return 1;
    }
    
    std::cout << Color::Green << "Audio engine started." << Color::Reset << std::endl;
#else
    std::cout << Color::Yellow << "RtAudio not available. No audio output." << Color::Reset << std::endl;
#endif
    
    // Initialize MIDI
#ifdef HAVE_RTMIDI
    // Create the MIDI router for multi-timbral operation
    std::unique_ptr<MultiTimbralMidiRouter> midiRouter = std::make_unique<MultiTimbralMidiRouter>(engine.get());

    // Set debug mode for MIDI visualization
    midiRouter->setDebugMode(true);

    // Open first available MIDI input port
    if (midiRouter->getMidiInputDevices().size() > 0) {
        midiRouter->openMidiInput(0);
        std::cout << Color::Green << "MIDI input connected using MultiTimbralMidiRouter" << Color::Reset << std::endl;

        // List available MIDI devices
        std::cout << "Available MIDI input devices:" << std::endl;
        auto devices = midiRouter->getMidiInputDevices();
        for (size_t i = 0; i < devices.size(); ++i) {
            std::cout << "  " << (i + 1) << ": " << devices[i] << std::endl;
        }
    } else {
        std::cout << Color::Yellow << "No MIDI input ports available." << Color::Reset << std::endl;
    }
#else
    std::cout << Color::Yellow << "RtMidi not available. No MIDI input." << Color::Reset << std::endl;
#endif
    
    // Set up default channels
    engine->setChannelActive(0, true);  // Channel 1
    engine->setChannelActive(1, true);  // Channel 2
    engine->setChannelActive(2, true);  // Channel 3
    
    // Different parameters for different channels
    ChannelSynthesizer* ch1 = engine->getChannelSynth(0);
    if (ch1) {
        ch1->setName("Bass");
        ch1->setMonophonic(true);
        ch1->setTransposition(-12); // One octave down
    }
    
    ChannelSynthesizer* ch2 = engine->getChannelSynth(1);
    if (ch2) {
        ch2->setName("Lead");
        ch2->setMonophonic(false);
    }
    
    ChannelSynthesizer* ch3 = engine->getChannelSynth(2);
    if (ch3) {
        ch3->setName("Pad");
        ch3->setMonophonic(false);
        ch3->setTransposition(12); // One octave up
    }
    
    // Set different volumes and pans
    engine->setChannelVolume(0, 0.8f);
    engine->setChannelPan(0, -0.3f);
    
    engine->setChannelVolume(1, 0.7f);
    engine->setChannelPan(1, 0.0f);
    
    engine->setChannelVolume(2, 0.6f);
    engine->setChannelPan(2, 0.3f);
    
    std::cout << std::endl;
    std::cout << Color::Bold << "Default setup:" << Color::Reset << std::endl;
    std::cout << "- Channel 1: Bass (mono, -12 semitones, panned left)" << std::endl;
    std::cout << "- Channel 2: Lead (poly, centered)" << std::endl;
    std::cout << "- Channel 3: Pad (poly, +12 semitones, panned right)" << std::endl;
    std::cout << std::endl;
    
    std::cout << Color::Bold << "Available commands:" << Color::Reset << std::endl;
    std::cout << "- View and configure channels" << std::endl;
    std::cout << "- Set up keyboard splits and layers" << std::endl;
    std::cout << "- Adjust voice allocation strategies" << std::endl;
    std::cout << "- Play test notes on each channel" << std::endl;
    std::cout << std::endl;
    
#ifdef HAVE_RTMIDI
    std::cout << Color::Bold << "MIDI Input:" << Color::Reset << std::endl;
    std::cout << "- Connect a MIDI keyboard to control the synth" << std::endl;
    std::cout << "- MIDI messages are automatically routed to appropriate channels" << std::endl;
    std::cout << "- When split mode is active, notes are routed based on the split point" << std::endl;
    std::cout << "- When layer mode is active, notes are routed to all layered channels" << std::endl;
    std::cout << std::endl;
#endif
    
    // Main menu loop
    bool running = true;
    while (running) {
        int choice = displayMenu();

        switch (choice) {
            case 0: // Exit
                running = false;
                break;

            case 1: // Show active channels
                showActiveChannels(engine.get());
                break;

            case 2: // Activate/deactivate a channel
                toggleChannelActive(engine.get());
                break;

            case 3: // Set channel volume/pan
                setChannelVolumePan(engine.get());
                break;

            case 4: // Set channel priority
                setChannelPriority(engine.get());
                break;

            case 5: // Set voice allocation strategy
                setVoiceAllocationStrategy(engine.get());
                break;

            case 6: // Setup keyboard split
#ifdef HAVE_RTMIDI
                setupKeyboardSplit(engine.get(), midiRouter.get());
#else
                setupKeyboardSplit(engine.get());
#endif
                break;

            case 7: // Setup layered channels
#ifdef HAVE_RTMIDI
                setupLayeredChannels(engine.get(), midiRouter.get());
#else
                setupLayeredChannels(engine.get());
#endif
                break;

            case 8: // Clear performance config
#ifdef HAVE_RTMIDI
                clearPerformanceConfig(engine.get(), midiRouter.get());
#else
                clearPerformanceConfig(engine.get());
#endif
                break;

            case 9: // Play test notes
                playTestNotes(engine.get());
                break;

            default:
                std::cout << Color::Red << "Invalid choice!" << Color::Reset << std::endl;
                break;
        }
    }
    
    // Cleanup
    std::cout << Color::Cyan << "Shutting down..." << Color::Reset << std::endl;
    
#ifdef HAVE_RTAUDIO
    if (audioEngine) {
        audioEngine->stop();
    }
#endif
    
    return 0;
}