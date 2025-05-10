#pragma once

#include <string>
#include <memory>
#include <map>
#include <vector>
#include <functional>

namespace AIMusicHardware {

// Forward declarations
class Synthesizer;
class EffectProcessor;
class Sequencer;
class MidiHandler;
class LLMInterface;
class HardwareInterface;
class UIContext;
class Screen;
class UIComponent;
class Label;
class Button;
class Knob;
class WaveformDisplay;
class SequencerGrid;
class EnvelopeEditor;
class InputEvent;

// Main UI class - interface to our custom UI system
class UserInterface {
public:
    UserInterface();
    ~UserInterface();
    
    // Basic UI lifecycle
    bool initialize(int width, int height);
    void shutdown();
    void update(float deltaTime = 0.016f); // Default to ~60fps
    void render();
    
    // System connections
    void connectSynthesizer(Synthesizer* synth);
    void connectEffectProcessor(EffectProcessor* effects);
    void connectSequencer(Sequencer* sequencer);
    void connectMidiHandler(MidiHandler* midiHandler);
    void connectLLMInterface(LLMInterface* llmInterface);
    void connectHardwareInterface(HardwareInterface* hardware);
    
    // Screen management
    void addScreen(std::unique_ptr<Screen> screen);
    Screen* getScreen(const std::string& id);
    void setCurrentScreen(const std::string& screenName);
    std::string getCurrentScreen() const;
    
    // UI component access
    UIComponent* getComponent(const std::string& screenId, const std::string& componentId);
    
    // Hard key input handling (for hardware buttons, encoders, etc.)
    bool handleInput(const InputEvent& event);
    
    // Common UI operations
    Label* createLabel(const std::string& screenId, const std::string& id, 
                     const std::string& text, int x, int y);
    
    Button* createButton(const std::string& screenId, const std::string& id, 
                       const std::string& text, int x, int y, int width, int height,
                       std::function<void()> callback = nullptr);
    
    Knob* createKnob(const std::string& screenId, const std::string& id, 
                   const std::string& label, int x, int y, int size,
                   float minValue, float maxValue, float defaultValue,
                   std::function<void(float)> callback = nullptr);
    
    WaveformDisplay* createWaveformDisplay(const std::string& screenId, const std::string& id,
                                        int x, int y, int width, int height);
    
    SequencerGrid* createSequencerGrid(const std::string& screenId, const std::string& id,
                                     int x, int y, int width, int height,
                                     int rows, int columns);
    
    EnvelopeEditor* createEnvelopeEditor(const std::string& screenId, const std::string& id,
                                       int x, int y, int width, int height);
    
    // Theme management
    void setThemeColor(const std::string& name, uint8_t r, uint8_t g, uint8_t b);
    
    // Layout management
    void loadLayout(const std::string& filename);
    void saveLayout(const std::string& filename) const;
    void resetToDefaultLayout();
    
    // AI Assistant specific UI
    void showAIAssistantSuggestion(const std::string& suggestion);
    void showParameterSuggestions(const std::map<std::string, float>& parameters);
    void showPatternSuggestion(const std::vector<std::pair<int, float>>& notes);
    
    // Voice input UI
    void beginVoiceInput();
    void endVoiceInput();
    void showTranscribedText(const std::string& text);
    
    // Quit handling
    bool shouldQuit() const;
    void setQuitFlag(bool quit);
    
    // FOR TESTING ONLY: Get access to the underlying UIContext
    // This method is only for test purposes and should not be used in production code
    void* getUIContextForTesting() { return uiContext_.get(); }

private:
    std::unique_ptr<UIContext> uiContext_;
    bool quitFlag_;
    
    // Creates standard screens and initializes the UI
    void createDefaultScreens();
};

} // namespace AIMusicHardware