#pragma once

#include <vector>
#include <string>
#include <functional>
#include <memory>
#include <map>

namespace AIMusicHardware {

// Forward declarations
class Synthesizer;
class EffectProcessor;
class Sequencer;
class MidiHandler;
class LLMInterface;
class HardwareInterface;

enum class UIElementType {
    Button,
    Slider,
    Knob,
    Display,
    Grid,
    Menu,
    TextInput,
    WaveformDisplay,
    PatternEditor,
    KeyboardInput,
    AIAssistantPanel
};

class UIElement {
public:
    UIElement(const std::string& id, UIElementType type);
    virtual ~UIElement();
    
    std::string getId() const;
    UIElementType getType() const;
    
    virtual void render() = 0;
    virtual void update() = 0;
    virtual bool handleInput(int x, int y, bool pressed) = 0;
    
protected:
    std::string id_;
    UIElementType type_;
};

class UserInterface {
public:
    UserInterface();
    ~UserInterface();
    
    bool initialize(int width, int height);
    void shutdown();
    
    void update();
    void render();
    
    // UI Element management
    void addElement(std::unique_ptr<UIElement> element);
    UIElement* getElementById(const std::string& id);
    void removeElement(const std::string& id);
    
    // System connections
    void connectSynthesizer(Synthesizer* synth);
    void connectEffectProcessor(EffectProcessor* effects);
    void connectSequencer(Sequencer* sequencer);
    void connectMidiHandler(MidiHandler* midiHandler);
    void connectLLMInterface(LLMInterface* llmInterface);
    void connectHardwareInterface(HardwareInterface* hardware);
    
    // Layout management
    void loadLayout(const std::string& filename);
    void saveLayout(const std::string& filename) const;
    void resetToDefaultLayout();
    
    // Screen management
    void setCurrentScreen(const std::string& screenName);
    std::string getCurrentScreen() const;
    void addScreen(const std::string& screenName);
    void removeScreen(const std::string& screenName);
    
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
    
private:
    class Impl;
    std::unique_ptr<Impl> pimpl_;
    
    int width_;
    int height_;
    std::string currentScreen_;
    std::map<std::string, std::vector<std::unique_ptr<UIElement>>> screens_;
    bool quitFlag_ = false;
    
    Synthesizer* synth_;
    EffectProcessor* effects_;
    Sequencer* sequencer_;
    MidiHandler* midiHandler_;
    LLMInterface* llmInterface_;
    HardwareInterface* hardware_;
};

} // namespace AIMusicHardware