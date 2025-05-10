#pragma once

#include "UIComponents.h"
#include "PresetManager.h"
#include <string>
#include <memory>
#include <functional>

namespace AIMusicHardware {

// Forward declarations
class PresetBrowser;
class PresetSaveDialog;

/**
 * @brief UI component for selecting presets in the main interface
 * 
 * This class provides a compact interface for navigating presets and
 * accessing the preset browser. It typically appears in the header of
 * the main UI and allows for quick preset navigation without needing
 * to open the full browser.
 */
class PresetSelector : public UIComponent {
public:
    /**
     * @brief Constructor
     * 
     * @param id Component ID
     * @param presetManager Pointer to the preset manager
     */
    PresetSelector(const std::string& id, PresetManager* presetManager);
    
    /**
     * @brief Destructor
     */
    virtual ~PresetSelector();
    
    /**
     * @brief Update the selector
     * 
     * @param deltaTime Time since last update in seconds
     */
    void update(float deltaTime) override;
    
    /**
     * @brief Render the selector
     * 
     * @param display Pointer to the display manager
     */
    void render(DisplayManager* display) override;
    
    /**
     * @brief Handle input events
     * 
     * @param event The input event
     * @return true if the event was handled
     */
    bool handleInput(const InputEvent& event) override;
    
    /**
     * @brief Show the preset browser
     */
    void showBrowser();
    
    /**
     * @brief Hide the preset browser
     */
    void hideBrowser();
    
    /**
     * @brief Show the save dialog
     */
    void showSaveDialog();
    
    /**
     * @brief Hide the save dialog
     */
    void hideSaveDialog();
    
    /**
     * @brief Load the next preset
     * 
     * @return true if successful
     */
    bool nextPreset();
    
    /**
     * @brief Load the previous preset
     * 
     * @return true if successful
     */
    bool previousPreset();
    
    /**
     * @brief Set callback for preset change
     * 
     * @param callback Function to call when a preset is loaded
     */
    using PresetChangedCallback = std::function<void(const PresetInfo&)>;
    void setPresetChangedCallback(PresetChangedCallback callback);
    
private:
    PresetManager* presetManager_;
    std::unique_ptr<Button> currentPresetButton_;
    std::unique_ptr<Button> prevButton_;
    std::unique_ptr<Button> nextButton_;
    std::unique_ptr<Button> saveButton_;
    std::unique_ptr<Button> browseButton_;
    
    // Modal dialogs
    std::unique_ptr<PresetBrowser> browser_;
    std::unique_ptr<PresetSaveDialog> saveDialog_;
    
    PresetChangedCallback presetChangedCallback_;
    
    // Helper methods
    void createComponents();
    void layoutComponents();
    void updateCurrentPresetDisplay();
    
    // Button handlers
    void handleCurrentPresetButton();
    void handlePrevButton();
    void handleNextButton();
    void handleSaveButton();
    void handleBrowseButton();
    
    // Dialog callbacks
    void handlePresetSelected(const PresetInfo& preset);
    void handleBrowserClosed();
    void handleSaveCompleted(bool success);
};

} // namespace AIMusicHardware