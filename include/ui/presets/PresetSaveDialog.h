#pragma once

#include "../../ui/UIComponents.h"
#include "PresetManager.h"
#include <functional>
#include <string>
#include <vector>

namespace AIMusicHardware {

/**
 * @class PresetSaveDialog
 * @brief Dialog UI for saving presets with metadata
 * 
 * This component provides fields for entering preset name, author, category,
 * and description when saving a new preset.
 */
class PresetSaveDialog : public UIComponent {
public:
    /**
     * @brief Construct a new Preset Save Dialog
     * @param presetManager Pointer to the preset manager
     */
    PresetSaveDialog(PresetManager* presetManager);
    
    /**
     * @brief Destructor
     */
    ~PresetSaveDialog() override;
    
    /**
     * @brief Render the save dialog
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
     * @brief Show the dialog
     */
    void show();
    
    /**
     * @brief Hide the dialog
     */
    void hide();
    
    /**
     * @brief Check if the dialog is visible
     * @return true if visible
     */
    bool isVisible() const;
    
    /**
     * @brief Set a callback for when save is complete
     * @param callback Function to call when save is complete
     */
    void setOnSaveComplete(std::function<void()> callback);
    
    /**
     * @brief Set a callback for when save is canceled
     * @param callback Function to call when save is canceled
     */
    void setOnCancel(std::function<void()> callback);
    
private:
    // The preset manager to use
    PresetManager* presetManager_;
    
    // Dialog state
    bool visible_ = false;
    
    // Input field values
    std::string nameInput_;
    std::string authorInput_;
    std::string descriptionInput_;
    std::string selectedCategory_;
    
    // Currently focused field
    enum class Field {
        Name,
        Author,
        Description,
        Category,
        SaveButton,
        CancelButton,
        None
    };
    Field focusedField_ = Field::Name;
    
    // Category options
    std::vector<std::string> categories_;
    int selectedCategoryIndex_ = 0;
    
    // Callbacks
    std::function<void()> onSaveComplete_;
    std::function<void()> onCancel_;
    
    // UI element rectangles
    Rect nameInputArea_;
    Rect authorInputArea_;
    Rect descriptionInputArea_;
    Rect categorySelectArea_;
    Rect saveButtonArea_;
    Rect cancelButtonArea_;
    
    // UI state
    bool saveButtonPressed_ = false;
    bool cancelButtonPressed_ = false;
    
    // Helper methods
    void updateLayout();
    void saveCurrentPreset();
    void cancelSave();
    void cycleFocus(bool forward);
    void updateSelectedCategory(int direction);
    void handleCharacterInput(char c);
    void handleDelete();
    std::string& getCurrentInputField();
};

} // namespace AIMusicHardware