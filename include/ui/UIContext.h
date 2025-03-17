#pragma once

#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>

#include "DisplayManager.h"
#include "Font.h"

namespace AIMusicHardware {

// Forward declarations
class Synthesizer;
class EffectProcessor;
class Sequencer;
class HardwareInterface;
class AdaptiveSequencer;
class LLMInterface;

// Input event types
enum class InputEventType {
    ButtonPress,
    ButtonRelease,
    EncoderRotate,
    EncoderPress,
    EncoderRelease,
    TouchPress,
    TouchMove,
    TouchRelease
};

// Input event data
struct InputEvent {
    InputEventType type;
    int id;         // Control ID
    int value;      // For encoders: rotation amount, For touch/buttons: X position
    int value2;     // For touch/buttons: Y position
    bool handled;   // Set to true if the event has been handled
    
    InputEvent() 
        : type(InputEventType::ButtonPress), id(0), value(0), value2(0), handled(false) {}
    InputEvent(InputEventType t, int i, int v1 = 0, int v2 = 0) 
        : type(t), id(i), value(v1), value2(v2), handled(false) {}
};

// UI component base class
class UIComponent {
public:
    UIComponent(const std::string& id);
    virtual ~UIComponent();
    
    // Basic properties
    const std::string& getId() const { return id_; }
    void setPosition(int x, int y) { x_ = x; y_ = y; }
    void setSize(int width, int height) { width_ = width; height_ = height; }
    Rect getBounds() const { return Rect(x_, y_, width_, height_); }
    
    // Visibility and enable state
    void setVisible(bool visible) { visible_ = visible; }
    bool isVisible() const { return visible_; }
    void setEnabled(bool enabled) { enabled_ = enabled; }
    bool isEnabled() const { return enabled_; }
    
    // Core methods
    virtual void update(float deltaTime) = 0;
    virtual void render(DisplayManager* display) = 0;
    virtual bool handleInput(const InputEvent& event) = 0;
    
    // Parent-child relationship
    void addChild(std::unique_ptr<UIComponent> child);
    UIComponent* getChild(const std::string& id);
    void removeChild(const std::string& id);
    
protected:
    std::string id_;
    int x_, y_;
    int width_, height_;
    bool visible_;
    bool enabled_;
    std::vector<std::unique_ptr<UIComponent>> children_;
    
    // Helper for rendering children
    void renderChildren(DisplayManager* display);
    
    // Helper for processing input to children
    bool handleChildrenInput(const InputEvent& event);
};

// Screen class - represents a full UI screen
class Screen : public UIComponent {
public:
    Screen(const std::string& id);
    virtual ~Screen();
    
    // Screen can set its own background color
    void setBackgroundColor(const Color& color) { backgroundColor_ = color; }
    const Color& getBackgroundColor() const { return backgroundColor_; }
    
    // Screen activation/deactivation
    virtual void onActivate() {}
    virtual void onDeactivate() {}
    
private:
    Color backgroundColor_;
};

// UI Context class - manages screens and input
class UIContext {
public:
    UIContext();
    ~UIContext();
    
    // Initialize the UI system
    bool initialize(int width, int height);
    void shutdown();
    
    // Screen management
    void addScreen(std::unique_ptr<Screen> screen);
    Screen* getScreen(const std::string& id);
    void setActiveScreen(const std::string& id);
    const std::string& getActiveScreenId() const { return activeScreenId_; }
    
    // Update and render
    void update(float deltaTime);
    void render();
    
    // Input handling
    bool handleInput(const InputEvent& event);
    
    // Font management
    void registerFont(const std::string& name, std::unique_ptr<Font> font);
    Font* getFont(const std::string& name);
    
    // Theme
    void setThemeColor(const std::string& name, const Color& color);
    Color getThemeColor(const std::string& name) const;
    
    // System access
    DisplayManager* getDisplayManager() { return displayManager_.get(); }
    
    // System connections
    void connectSynthesizer(Synthesizer* synth) { synth_ = synth; }
    void connectEffectProcessor(EffectProcessor* effectProcessor) { effectProcessor_ = effectProcessor; }
    void connectSequencer(Sequencer* sequencer) { sequencer_ = sequencer; }
    void connectHardwareInterface(HardwareInterface* hardware) { hardware_ = hardware; }
    void connectAdaptiveSequencer(AdaptiveSequencer* adaptiveSequencer) { adaptiveSequencer_ = adaptiveSequencer; }
    void connectLLMInterface(LLMInterface* llmInterface) { llmInterface_ = llmInterface; }
    
    // Access to connected systems
    Synthesizer* getSynthesizer() const { return synth_; }
    EffectProcessor* getEffectProcessor() const { return effectProcessor_; }
    Sequencer* getSequencer() const { return sequencer_; }
    HardwareInterface* getHardwareInterface() const { return hardware_; }
    AdaptiveSequencer* getAdaptiveSequencer() const { return adaptiveSequencer_; }
    LLMInterface* getLLMInterface() const { return llmInterface_; }
    
private:
    // Display manager for drawing
    std::unique_ptr<DisplayManager> displayManager_;
    
    // Screen management
    std::unordered_map<std::string, std::unique_ptr<Screen>> screens_;
    std::string activeScreenId_;
    
    // Fonts
    std::unordered_map<std::string, std::unique_ptr<Font>> fonts_;
    
    // Theme colors
    std::unordered_map<std::string, Color> themeColors_;
    
    // Connected systems
    Synthesizer* synth_;
    EffectProcessor* effectProcessor_;
    Sequencer* sequencer_;
    HardwareInterface* hardware_;
    AdaptiveSequencer* adaptiveSequencer_;
    LLMInterface* llmInterface_;
};

} // namespace AIMusicHardware