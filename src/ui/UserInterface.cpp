#include "../../include/ui/UserInterface.h"
#include "../../include/ui/UIContext.h"
#include "../../include/ui/UIComponents.h"
#include "../../include/audio/Synthesizer.h"
#include "../../include/effects/EffectProcessor.h"
#include "../../include/sequencer/Sequencer.h"
#include "../../include/midi/MidiInterface.h"
#include "../../include/ai/LLMInterface.h"
#include "../../include/hardware/HardwareInterface.h"

#include <iostream>
#include <fstream>

namespace AIMusicHardware {

// UserInterface implementation
UserInterface::UserInterface() 
    : quitFlag_(false) {
}

UserInterface::~UserInterface() {
    shutdown();
}

bool UserInterface::initialize(int width, int height) {
    // Create and initialize the UI context
    uiContext_ = std::make_unique<UIContext>();
    if (!uiContext_->initialize(width, height)) {
        std::cerr << "Failed to initialize UI context" << std::endl;
        return false;
    }
    
    // Create default screens
    createDefaultScreens();
    
    return true;
}

void UserInterface::shutdown() {
    // Clean up UI resources
    if (uiContext_) {
        uiContext_->shutdown();
        uiContext_.reset();
    }
}

void UserInterface::update(float deltaTime) {
    // Update the UI context
    if (uiContext_) {
        uiContext_->update(deltaTime);
    }
}

void UserInterface::render() {
    // Render the UI
    if (uiContext_) {
        uiContext_->render();
    }
}

void UserInterface::connectSynthesizer(Synthesizer* synth) {
    if (uiContext_) {
        uiContext_->connectSynthesizer(synth);
    }
}

void UserInterface::connectEffectProcessor(EffectProcessor* effects) {
    if (uiContext_) {
        uiContext_->connectEffectProcessor(effects);
    }
}

void UserInterface::connectSequencer(Sequencer* sequencer) {
    if (uiContext_) {
        uiContext_->connectSequencer(sequencer);
    }
}

void UserInterface::connectMidiHandler(MidiHandler* midiHandler) {
    // Store MIDI handler reference (UIContext doesn't have MIDI handler yet)
    // This would be used for MIDI-specific UI elements
}

void UserInterface::connectLLMInterface(LLMInterface* llmInterface) {
    if (uiContext_) {
        uiContext_->connectLLMInterface(llmInterface);
    }
}

void UserInterface::connectHardwareInterface(HardwareInterface* hardware) {
    if (uiContext_) {
        uiContext_->connectHardwareInterface(hardware);
    }
}

void UserInterface::addScreen(std::unique_ptr<Screen> screen) {
    if (uiContext_ && screen) {
        uiContext_->addScreen(std::move(screen));
    }
}

Screen* UserInterface::getScreen(const std::string& id) {
    if (uiContext_) {
        return uiContext_->getScreen(id);
    }
    return nullptr;
}

void UserInterface::setCurrentScreen(const std::string& screenName) {
    if (uiContext_) {
        uiContext_->setActiveScreen(screenName);
    }
}

std::string UserInterface::getCurrentScreen() const {
    if (uiContext_) {
        return uiContext_->getActiveScreenId();
    }
    return "";
}

UIComponent* UserInterface::getComponent(const std::string& screenId, const std::string& componentId) {
    Screen* screen = getScreen(screenId);
    if (screen) {
        return screen->getChild(componentId);
    }
    return nullptr;
}

bool UserInterface::handleInput(const InputEvent& event) {
    if (uiContext_) {
        return uiContext_->handleInput(event);
    }
    return false;
}

Label* UserInterface::createLabel(const std::string& screenId, const std::string& id, 
                               const std::string& text, int x, int y) {
    Screen* screen = getScreen(screenId);
    if (!screen) {
        return nullptr;
    }
    
    // Create label and add to screen
    auto label = std::make_unique<Label>(id, text);
    label->setPosition(x, y);
    
    // Get pointer before we move the unique_ptr
    Label* labelPtr = label.get();
    
    // Add to screen
    screen->addChild(std::move(label));
    
    return labelPtr;
}

Button* UserInterface::createButton(const std::string& screenId, const std::string& id, 
                                 const std::string& text, int x, int y, int width, int height,
                                 std::function<void()> callback) {
    Screen* screen = getScreen(screenId);
    if (!screen) {
        return nullptr;
    }
    
    // Create button and add to screen
    auto button = std::make_unique<Button>(id, text);
    button->setPosition(x, y);
    button->setSize(width, height);
    
    if (callback) {
        button->setClickCallback(callback);
    }
    
    // Get pointer before we move the unique_ptr
    Button* buttonPtr = button.get();
    
    // Add to screen
    screen->addChild(std::move(button));
    
    return buttonPtr;
}

Knob* UserInterface::createKnob(const std::string& screenId, const std::string& id, 
                             const std::string& label, int x, int y, int size,
                             float minValue, float maxValue, float defaultValue,
                             std::function<void(float)> callback) {
    Screen* screen = getScreen(screenId);
    if (!screen) {
        return nullptr;
    }
    
    // Create knob and add to screen
    auto knob = std::make_unique<Knob>(id, label);
    knob->setPosition(x, y);
    knob->setSize(size, size);
    knob->setRange(minValue, maxValue);
    knob->setValue(defaultValue);
    
    if (callback) {
        knob->setValueChangeCallback(callback);
    }
    
    // Get pointer before we move the unique_ptr
    Knob* knobPtr = knob.get();
    
    // Add to screen
    screen->addChild(std::move(knob));
    
    return knobPtr;
}

WaveformDisplay* UserInterface::createWaveformDisplay(const std::string& screenId, const std::string& id,
                                                  int x, int y, int width, int height) {
    Screen* screen = getScreen(screenId);
    if (!screen) {
        return nullptr;
    }
    
    // Create waveform display and add to screen
    auto waveform = std::make_unique<WaveformDisplay>(id);
    waveform->setPosition(x, y);
    waveform->setSize(width, height);
    
    // Get pointer before we move the unique_ptr
    WaveformDisplay* waveformPtr = waveform.get();
    
    // Add to screen
    screen->addChild(std::move(waveform));
    
    return waveformPtr;
}

SequencerGrid* UserInterface::createSequencerGrid(const std::string& screenId, const std::string& id,
                                              int x, int y, int width, int height,
                                              int rows, int columns) {
    Screen* screen = getScreen(screenId);
    if (!screen) {
        return nullptr;
    }
    
    // Create sequencer grid and add to screen
    auto grid = std::make_unique<SequencerGrid>(id, rows, columns);
    grid->setPosition(x, y);
    grid->setSize(width, height);
    
    // Get pointer before we move the unique_ptr
    SequencerGrid* gridPtr = grid.get();
    
    // Add to screen
    screen->addChild(std::move(grid));
    
    return gridPtr;
}

EnvelopeEditor* UserInterface::createEnvelopeEditor(const std::string& screenId, const std::string& id,
                                                int x, int y, int width, int height) {
    Screen* screen = getScreen(screenId);
    if (!screen) {
        return nullptr;
    }
    
    // Create envelope editor and add to screen
    auto envelope = std::make_unique<EnvelopeEditor>(id);
    envelope->setPosition(x, y);
    envelope->setSize(width, height);
    
    // Get pointer before we move the unique_ptr
    EnvelopeEditor* envelopePtr = envelope.get();
    
    // Add to screen
    screen->addChild(std::move(envelope));
    
    return envelopePtr;
}

void UserInterface::setThemeColor(const std::string& name, uint8_t r, uint8_t g, uint8_t b) {
    if (uiContext_) {
        uiContext_->setThemeColor(name, Color(r, g, b));
    }
}

void UserInterface::loadLayout(const std::string& filename) {
    // Implementation will load UI layout from file
    // This would involve reading a JSON/XML file and creating UI elements accordingly
}

void UserInterface::saveLayout(const std::string& filename) const {
    // Implementation will save current UI layout to file
    // This would involve serializing the current UI state to JSON/XML
}

void UserInterface::resetToDefaultLayout() {
    // Reset to default layout by recreating the default screens
    if (uiContext_) {
        uiContext_->shutdown();
        uiContext_->initialize(uiContext_->getDisplayManager()->getWidth(), 
                            uiContext_->getDisplayManager()->getHeight());
        createDefaultScreens();
    }
}

// AI Assistant functionality
void UserInterface::showAIAssistantSuggestion(const std::string& suggestion) {
    // Get AI suggestion screen
    Screen* aiScreen = getScreen("ai_assistant");
    if (!aiScreen) {
        return;
    }
    
    // Find the suggestion label and update it
    Label* suggestionLabel = dynamic_cast<Label*>(aiScreen->getChild("suggestion_text"));
    if (suggestionLabel) {
        suggestionLabel->setText(suggestion);
    }
    
    // Show the AI assistant screen
    setCurrentScreen("ai_assistant");
}

void UserInterface::showParameterSuggestions(const std::map<std::string, float>& parameters) {
    // Implementation to show parameter suggestions from the AI
    // This would create or update UI elements on the AI suggestion screen
}

void UserInterface::showPatternSuggestion(const std::vector<std::pair<int, float>>& notes) {
    // Implementation to show pattern suggestions from the AI
    // This would create or update a sequencer grid on the AI suggestion screen
}

// Voice input functionality
void UserInterface::beginVoiceInput() {
    // Start voice recording
    // This would update UI to show recording state
}

void UserInterface::endVoiceInput() {
    // End voice recording
    // This would update UI to show processing state
}

void UserInterface::showTranscribedText(const std::string& text) {
    // Show transcribed text in the UI
    // This would update a label on the voice input screen
}

// Quit handling
bool UserInterface::shouldQuit() const {
    return quitFlag_;
}

void UserInterface::setQuitFlag(bool quit) {
    quitFlag_ = quit;
}

// Private methods
// Private concrete Screen class for the default implementation
class DefaultScreen : public Screen {
public:
    DefaultScreen(const std::string& id) : Screen(id) {}
};

void UserInterface::createDefaultScreens() {
    if (!uiContext_) {
        return;
    }
    
    // Define default screens
    
    // Main/Home screen
    auto mainScreen = std::make_unique<DefaultScreen>("main");
    mainScreen->setBackgroundColor(uiContext_->getThemeColor("background"));
    
    // Add a label to main screen
    auto titleLabel = std::make_unique<Label>("title", "AI Music Hardware");
    titleLabel->setPosition(10, 10);
    titleLabel->setTextColor(uiContext_->getThemeColor("highlight"));
    mainScreen->addChild(std::move(titleLabel));
    
    // Add screen navigation buttons
    auto synthButton = std::make_unique<Button>("synth_button", "Synth");
    synthButton->setPosition(20, 60);
    synthButton->setSize(100, 40);
    synthButton->setClickCallback([this]() {
        this->setCurrentScreen("synth");
    });
    mainScreen->addChild(std::move(synthButton));
    
    auto seqButton = std::make_unique<Button>("seq_button", "Sequencer");
    seqButton->setPosition(140, 60);
    seqButton->setSize(100, 40);
    seqButton->setClickCallback([this]() {
        this->setCurrentScreen("sequencer");
    });
    mainScreen->addChild(std::move(seqButton));
    
    auto effectsButton = std::make_unique<Button>("effects_button", "Effects");
    effectsButton->setPosition(260, 60);
    effectsButton->setSize(100, 40);
    effectsButton->setClickCallback([this]() {
        this->setCurrentScreen("effects");
    });
    mainScreen->addChild(std::move(effectsButton));
    
    auto aiButton = std::make_unique<Button>("ai_button", "AI Assistant");
    aiButton->setPosition(380, 60);
    aiButton->setSize(100, 40);
    aiButton->setClickCallback([this]() {
        this->setCurrentScreen("ai_assistant");
    });
    mainScreen->addChild(std::move(aiButton));
    
    // Add a waveform display
    auto waveform = std::make_unique<WaveformDisplay>("main_waveform");
    waveform->setPosition(20, 120);
    waveform->setSize(760, 200);
    waveform->setWaveformColor(uiContext_->getThemeColor("accent"));
    waveform->setBackgroundColor(Color(30, 30, 30));
    waveform->showGrid(true);
    mainScreen->addChild(std::move(waveform));
    
    // Add the main screen to the context
    uiContext_->addScreen(std::move(mainScreen));
    
    // Create other screens (just placeholders for now)
    
    // Synth screen
    auto synthScreen = std::make_unique<DefaultScreen>("synth");
    synthScreen->setBackgroundColor(uiContext_->getThemeColor("background"));
    
    // Add a back button
    auto synthBackButton = std::make_unique<Button>("back_button", "Back");
    synthBackButton->setPosition(10, 10);
    synthBackButton->setSize(60, 30);
    synthBackButton->setClickCallback([this]() {
        this->setCurrentScreen("main");
    });
    synthScreen->addChild(std::move(synthBackButton));
    
    // Add a title
    auto synthTitle = std::make_unique<Label>("title", "Synthesizer");
    synthTitle->setPosition(100, 15);
    synthTitle->setTextColor(uiContext_->getThemeColor("highlight"));
    synthScreen->addChild(std::move(synthTitle));
    
    // Add synth controls
    int knobSize = 60;
    int knobY = 100;
    int knobSpacing = 90;
    int knobX = 40;
    
    // Create several knobs for synth parameters
    for (int i = 0; i < 8; i++) {
        auto knob = std::make_unique<Knob>("knob_" + std::to_string(i), "Param " + std::to_string(i+1));
        knob->setPosition(knobX + i * knobSpacing, knobY);
        knob->setSize(knobSize, knobSize);
        knob->setRange(0.0f, 1.0f);
        knob->setValue(0.5f);
        knob->setColor(uiContext_->getThemeColor("accent"));
        synthScreen->addChild(std::move(knob));
    }
    
    // Add envelope editor
    auto envelope = std::make_unique<EnvelopeEditor>("envelope");
    envelope->setPosition(50, 200);
    envelope->setSize(700, 150);
    envelope->setLineColor(uiContext_->getThemeColor("highlight"));
    envelope->setBackgroundColor(Color(30, 30, 30));
    synthScreen->addChild(std::move(envelope));
    
    uiContext_->addScreen(std::move(synthScreen));
    
    // Sequencer screen
    auto seqScreen = std::make_unique<DefaultScreen>("sequencer");
    seqScreen->setBackgroundColor(uiContext_->getThemeColor("background"));
    
    // Add a back button
    auto seqBackButton = std::make_unique<Button>("back_button", "Back");
    seqBackButton->setPosition(10, 10);
    seqBackButton->setSize(60, 30);
    seqBackButton->setClickCallback([this]() {
        this->setCurrentScreen("main");
    });
    seqScreen->addChild(std::move(seqBackButton));
    
    // Add a title
    auto seqTitle = std::make_unique<Label>("title", "Sequencer");
    seqTitle->setPosition(100, 15);
    seqTitle->setTextColor(uiContext_->getThemeColor("highlight"));
    seqScreen->addChild(std::move(seqTitle));
    
    // Add sequencer grid
    auto seqGrid = std::make_unique<SequencerGrid>("seq_grid", 8, 16);
    seqGrid->setPosition(50, 60);
    seqGrid->setSize(700, 300);
    seqGrid->setActiveColor(uiContext_->getThemeColor("highlight"));
    seqGrid->setInactiveColor(Color(60, 60, 60));
    seqGrid->setCursorColor(uiContext_->getThemeColor("accent"));
    seqScreen->addChild(std::move(seqGrid));
    
    // Add transport controls
    auto playButton = std::make_unique<Button>("play_button", "Play");
    playButton->setPosition(100, 400);
    playButton->setSize(80, 40);
    seqScreen->addChild(std::move(playButton));
    
    auto stopButton = std::make_unique<Button>("stop_button", "Stop");
    stopButton->setPosition(200, 400);
    stopButton->setSize(80, 40);
    seqScreen->addChild(std::move(stopButton));
    
    auto recordButton = std::make_unique<Button>("record_button", "Record");
    recordButton->setPosition(300, 400);
    recordButton->setSize(80, 40);
    seqScreen->addChild(std::move(recordButton));
    
    // Tempo control
    auto tempoKnob = std::make_unique<Knob>("tempo_knob", "Tempo");
    tempoKnob->setPosition(450, 400);
    tempoKnob->setSize(60, 60);
    tempoKnob->setRange(60.0f, 200.0f);
    tempoKnob->setValue(120.0f);
    tempoKnob->setColor(uiContext_->getThemeColor("highlight"));
    seqScreen->addChild(std::move(tempoKnob));
    
    uiContext_->addScreen(std::move(seqScreen));
    
    // Effects screen (placeholder)
    auto effectsScreen = std::make_unique<DefaultScreen>("effects");
    effectsScreen->setBackgroundColor(uiContext_->getThemeColor("background"));
    
    // Add a back button
    auto effectsBackButton = std::make_unique<Button>("back_button", "Back");
    effectsBackButton->setPosition(10, 10);
    effectsBackButton->setSize(60, 30);
    effectsBackButton->setClickCallback([this]() {
        this->setCurrentScreen("main");
    });
    effectsScreen->addChild(std::move(effectsBackButton));
    
    // Add a title
    auto effectsTitle = std::make_unique<Label>("title", "Effects");
    effectsTitle->setPosition(100, 15);
    effectsTitle->setTextColor(uiContext_->getThemeColor("highlight"));
    effectsScreen->addChild(std::move(effectsTitle));
    
    uiContext_->addScreen(std::move(effectsScreen));
    
    // AI Assistant screen (placeholder)
    auto aiScreen = std::make_unique<DefaultScreen>("ai_assistant");
    aiScreen->setBackgroundColor(uiContext_->getThemeColor("background"));
    
    // Add a back button
    auto aiBackButton = std::make_unique<Button>("back_button", "Back");
    aiBackButton->setPosition(10, 10);
    aiBackButton->setSize(60, 30);
    aiBackButton->setClickCallback([this]() {
        this->setCurrentScreen("main");
    });
    aiScreen->addChild(std::move(aiBackButton));
    
    // Add a title
    auto aiTitle = std::make_unique<Label>("title", "AI Assistant");
    aiTitle->setPosition(100, 15);
    aiTitle->setTextColor(uiContext_->getThemeColor("highlight"));
    aiScreen->addChild(std::move(aiTitle));
    
    // Add suggestion text area
    auto suggestionLabel = std::make_unique<Label>("suggestion_text", "AI suggestions will appear here");
    suggestionLabel->setPosition(50, 100);
    suggestionLabel->setTextColor(uiContext_->getThemeColor("foreground"));
    aiScreen->addChild(std::move(suggestionLabel));
    
    uiContext_->addScreen(std::move(aiScreen));
}

} // namespace AIMusicHardware