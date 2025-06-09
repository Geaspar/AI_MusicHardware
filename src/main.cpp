#include <iostream>
#include <memory>
#include <thread>
#include <chrono>
#include <mutex>
#include <atomic>

#include "../include/audio/AudioEngine.h"
#include "../include/audio/Synthesizer.h"
#include "../include/effects/EffectProcessor.h"
#include "../include/sequencer/Sequencer.h"
#include "../include/midi/MidiInterface.h"
#include "../include/hardware/HardwareInterface.h"
#include "../include/ai/LLMInterface.h"
#include "../include/ui/UserInterface.h"

using namespace AIMusicHardware;

void sendAllNotesOff(MidiOutput* midiOutput) {
    // Send "All Notes Off" for all MIDI channels
    for (int ch = 0; ch < 16; ++ch) {
        for (int note = 0; note < 128; ++note) {
            midiOutput->sendNoteOff(ch, note);
        }
    }
}

int main(int argc, char* argv[]) {
    std::cout << "AI Music Hardware - Starting up..." << std::endl;
    
    // Create components
    auto audioEngine = std::make_unique<AudioEngine>();
    auto synthesizer = std::make_unique<Synthesizer>();
    auto effectProcessor = std::make_unique<EffectProcessor>();
    auto sequencer = std::make_unique<Sequencer>();
    auto midiInput = std::make_unique<MidiInput>();
    auto midiOutput = std::make_unique<MidiOutput>();
    auto midiHandler = std::make_unique<MidiHandler>();
    auto hardwareInterface = std::make_unique<HardwareInterface>();
    auto userInterface = std::make_unique<UserInterface>();
    
    // Initialize critical components first
    if (!synthesizer->initialize()) {
        std::cerr << "Failed to initialize synthesizer!" << std::endl;
        return 1;
    }
    
    if (!effectProcessor->initialize()) {
        std::cerr << "Failed to initialize effect processor!" << std::endl;
        return 1;
    }
    
    if (!sequencer->initialize()) {
        std::cerr << "Failed to initialize sequencer!" << std::endl;
        return 1;
    }
    
    if (!audioEngine->initialize()) {
        std::cerr << "Failed to initialize audio engine!" << std::endl;
        // No need to clean up as the unique_ptrs will handle destruction
        return 1;
    }
    
    if (!hardwareInterface->initialize()) {
        std::cerr << "Failed to initialize hardware interface! Continuing without hardware..." << std::endl;
    }
    
    // Allow model path to be specified via command-line argument
    std::string llmModelPath = (argc > 1) ? argv[1] : "./models/llm_model.bin";
    auto llmInterface = std::make_unique<LLMInterface>();
    if (!llmInterface->initialize(llmModelPath)) {
        std::cerr << "Failed to initialize LLM interface! AI features will be disabled." << std::endl;
    }
    
    if (!userInterface->initialize(1024, 768)) {
        std::cerr << "Failed to initialize user interface!" << std::endl;
        hardwareInterface->shutdown();
        audioEngine->shutdown();
        return 1;
    }
    
    // Connect components
    userInterface->connectSynthesizer(synthesizer.get());
    userInterface->connectEffectProcessor(effectProcessor.get());
    userInterface->connectSequencer(sequencer.get());
    userInterface->connectMidiHandler(midiHandler.get());
    userInterface->connectLLMInterface(llmInterface.get());
    userInterface->connectHardwareInterface(hardwareInterface.get());
    
    // Mutex for thread safety in audio callback
    std::mutex audioMutex;
    
    // Set up audio callback with thread safety
    audioEngine->setAudioCallback([&](float* outputBuffer, int numFrames) {
        std::lock_guard<std::mutex> lock(audioMutex);
        
        // Process sequencer
        sequencer->process(1.0 / audioEngine->getSampleRate() * numFrames);
        
        // Process synthesizer
        synthesizer->process(outputBuffer, numFrames);
        
        // Process effects
        effectProcessor->process(outputBuffer, numFrames);
    });
    
    // Set up MIDI handling with explicit lambda (clearer callback signature)
    midiInput->setCallback([&](const MidiMessage& msg) {
        midiHandler->processMessage(msg);
    });
    
    midiHandler->setNoteOnCallback([&](int channel, int note, int velocity) {
        synthesizer->noteOn(note, velocity / 127.0f);
    });
    
    midiHandler->setNoteOffCallback([&](int channel, int note) {
        synthesizer->noteOff(note);
    });
    
    // Set up sequencer note callbacks
    sequencer->setNoteCallbacks(
        [&](int pitch, float velocity, int channel) {
            synthesizer->noteOn(pitch, velocity);
            midiOutput->sendNoteOn(channel, pitch, static_cast<int>(velocity * 127.0f));
        },
        [&](int pitch, int channel) {
            synthesizer->noteOff(pitch);
            midiOutput->sendNoteOff(channel, pitch);
        }
    );
    
    // Main loop with proper timing
    bool running = true;
    auto lastFrameTime = std::chrono::high_resolution_clock::now();
    
    while (running) {
        auto now = std::chrono::high_resolution_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastFrameTime);
        
        if (elapsed.count() >= 16) {  // Target ~60 FPS
            // Update UI with lock for shared data
            {
                std::lock_guard<std::mutex> lock(audioMutex);
                userInterface->update();
            }
            
            userInterface->render();
            lastFrameTime = now;
            
            // Check for quit condition from UI
            running = !userInterface->shouldQuit();
        }
    }
    
    // Send "All Notes Off" before shutdown
    sendAllNotesOff(midiOutput.get());
    
    // Cleanup
    std::cout << "AI Music Hardware - Shutting down..." << std::endl;
    
    // Stop audio engine first to prevent callbacks during shutdown
    std::cout << "Stopping audio engine..." << std::endl;
    audioEngine->shutdown();
    
    // Stop hardware interface
    std::cout << "Stopping hardware interface..." << std::endl;
    hardwareInterface->shutdown();
    
    // Clear connections in UI before shutdown
    userInterface->connectSynthesizer(nullptr);
    userInterface->connectEffectProcessor(nullptr);
    userInterface->connectSequencer(nullptr);
    userInterface->connectMidiHandler(nullptr);
    userInterface->connectLLMInterface(nullptr);
    userInterface->connectHardwareInterface(nullptr);
    
    // Shutdown UI
    std::cout << "Shutting down UI..." << std::endl;
    userInterface->shutdown();
    
    std::cout << "Shutdown complete." << std::endl;
    
    return 0;
}