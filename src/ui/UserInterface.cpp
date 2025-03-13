#include "../../include/ui/UserInterface.h"

namespace AIMusicHardware {

UserInterface::UserInterface() 
    : isInitialized_(false),
      width_(800),
      height_(600),
      synthesizer_(nullptr),
      effectProcessor_(nullptr),
      sequencer_(nullptr),
      midiHandler_(nullptr),
      llmInterface_(nullptr),
      hardwareInterface_(nullptr) {
}

UserInterface::~UserInterface() {
    shutdown();
}

bool UserInterface::initialize(int width, int height) {
    // Stub implementation - will be expanded with actual UI framework
    width_ = width;
    height_ = height;
    isInitialized_ = true;
    return true;
}

void UserInterface::shutdown() {
    if (isInitialized_) {
        // Clean up UI resources
        isInitialized_ = false;
    }
}

void UserInterface::update() {
    if (!isInitialized_) {
        return;
    }
    
    // Update UI state based on audio engine, etc.
}

void UserInterface::render() {
    if (!isInitialized_) {
        return;
    }
    
    // Render the UI
}

void UserInterface::connectSynthesizer(Synthesizer* synthesizer) {
    synthesizer_ = synthesizer;
}

void UserInterface::connectEffectProcessor(EffectProcessor* effectProcessor) {
    effectProcessor_ = effectProcessor;
}

void UserInterface::connectSequencer(Sequencer* sequencer) {
    sequencer_ = sequencer;
}

void UserInterface::connectMidiHandler(MidiHandler* midiHandler) {
    midiHandler_ = midiHandler;
}

void UserInterface::connectLLMInterface(LLMInterface* llmInterface) {
    llmInterface_ = llmInterface;
}

void UserInterface::connectHardwareInterface(HardwareInterface* hardwareInterface) {
    hardwareInterface_ = hardwareInterface;
}

} // namespace AIMusicHardware