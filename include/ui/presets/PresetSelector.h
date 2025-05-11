#pragma once

#include "../../ui/UIComponents.h"
#include "PresetManager.h"
#include <functional>

namespace AIMusicHardware {

/**
 * @class PresetSelector
 * @brief A compact UI component for selecting and navigating presets
 * 
 * The PresetSelector provides a simple interface for displaying the current preset
 * and navigating through available presets using next/previous buttons.
 */
class PresetSelector : public UIComponent {
public:
    /**
     * @brief Construct a new Preset Selector
     * @param presetManager Pointer to the preset manager
     */
    PresetSelector(PresetManager* presetManager);
    
    /**
     * @brief Destructor
     */
    ~PresetSelector() override;
    
    /**
     * @brief Render the preset selector
     * @param display The display manager to render to
     */
    void render(DisplayManager* display) override;
    
    /**
     * @brief Handle input events
     * @param event The input event to handle
     * @return true if the event was handled
     */
    bool handleInput(const InputEvent& event) override;

    /**
     * @brief Update the component state
     * @param deltaTime Time elapsed since last update
     */
    void update(float deltaTime) override;
    
    /**
     * @brief Set a callback for when the save button is pressed
     * @param callback Function to call when save is requested
     */
    void setOnSaveRequested(std::function<void()> callback);
    
    /**
     * @brief Set a callback for when a preset is changed
     * @param callback Function to call when a preset is loaded
     */
    void setOnPresetChanged(std::function<void(const std::string&)> callback);
    
private:
    // The preset manager to use
    PresetManager* presetManager_;
    
    // Callback functions
    std::function<void()> onSaveRequested_;
    std::function<void(const std::string&)> onPresetChanged_;
    
    // UI element rectangles
    Rect prevButtonArea_;
    Rect nextButtonArea_;
    Rect nameArea_;
    Rect saveButtonArea_;
    
    // UI state
    bool prevButtonPressed_ = false;
    bool nextButtonPressed_ = false;
    bool saveButtonPressed_ = false;
    
    // Helper methods
    void updateLayout();
    void loadPreviousPreset();
    void loadNextPreset();
    void requestSave();
};

} // namespace AIMusicHardware