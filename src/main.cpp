#include <iostream>
#include <memory>
#include <thread>
#include <chrono>

#include "../include/audio/AudioEngine.h"
#include "../include/audio/Synthesizer.h"
#include "../include/effects/EffectProcessor.h"
#include "../include/sequencer/Sequencer.h"
#include "../include/midi/MidiInterface.h"
#include "../include/hardware/HardwareInterface.h"
#include "../include/ai/LLMInterface.h"
#include "../include/ui/UserInterface.h"

using namespace AIMusicHardware;

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
    auto llmInterface = std::make_unique<LLMInterface>();
    auto userInterface = std::make_unique<UserInterface>();
    
    // Initialize components
    if (!audioEngine->initialize()) {
        std::cerr << "Failed to initialize audio engine!" << std::endl;
        return 1;
    }
    
    if (!hardwareInterface->initialize()) {
        std::cerr << "Failed to initialize hardware interface! Continuing without hardware..." << std::endl;
    }
    
    std::string llmModelPath = "./models/llm_model.bin";
    if (!llmInterface->initialize(llmModelPath)) {
        std::cerr << "Failed to initialize LLM interface! AI features will be disabled." << std::endl;
    }
    
    if (!userInterface->initialize(1024, 768)) {
        std::cerr << "Failed to initialize user interface!" << std::endl;
        return 1;
    }
    
    // Connect components
    userInterface->connectSynthesizer(synthesizer.get());
    userInterface->connectEffectProcessor(effectProcessor.get());
    userInterface->connectSequencer(sequencer.get());
    userInterface->connectMidiHandler(midiHandler.get());
    userInterface->connectLLMInterface(llmInterface.get());
    userInterface->connectHardwareInterface(hardwareInterface.get());
    
    // Set up audio callback
    audioEngine->setAudioCallback([&](float* outputBuffer, int numFrames) {
        // Process sequencer
        sequencer->process(1.0 / audioEngine->getSampleRate() * numFrames);
        
        // Process synthesizer
        synthesizer->process(outputBuffer, numFrames);
        
        // Process effects
        effectProcessor->process(outputBuffer, numFrames);
    });
    
    // Set up MIDI handling
    midiInput->setCallback(midiHandler.get());
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
    
    // Main loop
    bool running = true;
    while (running) {
        // Update UI
        userInterface->update();
        userInterface->render();
        
        // Sleep to avoid consuming too much CPU
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
        
        // Check for quit condition
        // In a real application, this would be handled by the UI
        // running = checkIfStillRunning();
    }
    
    // Cleanup
    userInterface->shutdown();
    hardwareInterface->shutdown();
    audioEngine->shutdown();
    
    std::cout << "AI Music Hardware - Shutting down..." << std::endl;
    return 0;
}