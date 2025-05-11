#include "../../../include/ui/presets/PresetSaveDialog.h"
#include "../../../include/ui/DisplayManager.h"
#include <filesystem>
#include <iostream>
#include <cctype> // for tolower

namespace AIMusicHardware {

namespace fs = std::filesystem;

PresetSaveDialog::PresetSaveDialog(PresetManager* presetManager)
    : UIComponent("PresetSaveDialog"),
      presetManager_(presetManager),
      nameInput_("New Preset"),
      authorInput_(""),
      descriptionInput_(""),
      selectedCategory_("Other") {
    
    // Set up categories
    categories_ = PresetManager::getAvailableCategories();
    if (!categories_.empty()) {
        selectedCategory_ = categories_[0];
    }
    
    // Set default size
    setSize(600, 400);
    updateLayout();
}

PresetSaveDialog::~PresetSaveDialog() {
    // Nothing specific to clean up
}

void PresetSaveDialog::render(DisplayManager* display) {
    if (!visible_ || !display) return;
    
    // Get the component's absolute position
    int absX = getAbsoluteX();
    int absY = getAbsoluteY();
    
    // Overlay background
    display->fillRect(0, 0, display->getWidth(), display->getHeight(), 
                     Color(0, 0, 0, 128)); // Semi-transparent black
    
    // Dialog background
    display->fillRect(absX, absY, width_, height_, Color(40, 40, 40));
    display->drawRect(absX, absY, width_, height_, Color(80, 80, 80)); // Border
    
    // Title
    display->drawText(absX + 20, absY + 30, "Save Preset", Color(220, 220, 220));
    
    // Name field
    display->drawText(absX + nameInputArea_.x, absY + nameInputArea_.y - 20, 
                     "Name:", Color(180, 180, 180));
    Color nameBoxColor = (focusedField_ == Field::Name) ? 
                        Color(60, 60, 100) : Color(60, 60, 60);
    display->fillRect(absX + nameInputArea_.x, absY + nameInputArea_.y,
                     nameInputArea_.width, nameInputArea_.height, nameBoxColor);
    display->drawText(absX + nameInputArea_.x + 10, absY + nameInputArea_.y + 20,
                     nameInput_, Color(220, 220, 220));
    
    // Author field
    display->drawText(absX + authorInputArea_.x, absY + authorInputArea_.y - 20,
                     "Author:", Color(180, 180, 180));
    Color authorBoxColor = (focusedField_ == Field::Author) ?
                         Color(60, 60, 100) : Color(60, 60, 60);
    display->fillRect(absX + authorInputArea_.x, absY + authorInputArea_.y,
                     authorInputArea_.width, authorInputArea_.height, authorBoxColor);
    display->drawText(absX + authorInputArea_.x + 10, absY + authorInputArea_.y + 20,
                     authorInput_, Color(220, 220, 220));
    
    // Category field
    display->drawText(absX + categorySelectArea_.x, absY + categorySelectArea_.y - 20,
                     "Category:", Color(180, 180, 180));
    Color categoryBoxColor = (focusedField_ == Field::Category) ?
                           Color(60, 60, 100) : Color(60, 60, 60);
    display->fillRect(absX + categorySelectArea_.x, absY + categorySelectArea_.y,
                     categorySelectArea_.width, categorySelectArea_.height, categoryBoxColor);
    display->drawText(absX + categorySelectArea_.x + 10, absY + categorySelectArea_.y + 20,
                     selectedCategory_, Color(220, 220, 220));
    
    // Description field
    display->drawText(absX + descriptionInputArea_.x, absY + descriptionInputArea_.y - 20,
                     "Description:", Color(180, 180, 180));
    Color descBoxColor = (focusedField_ == Field::Description) ?
                       Color(60, 60, 100) : Color(60, 60, 60);
    display->fillRect(absX + descriptionInputArea_.x, absY + descriptionInputArea_.y,
                     descriptionInputArea_.width, descriptionInputArea_.height, descBoxColor);
    display->drawText(absX + descriptionInputArea_.x + 10, absY + descriptionInputArea_.y + 20,
                     descriptionInput_, Color(220, 220, 220));
    
    // Save button
    Color saveColor = (focusedField_ == Field::SaveButton) ?
                     Color(60, 100, 60) : Color(60, 80, 60);
    if (saveButtonPressed_) saveColor = Color(80, 120, 80);
    display->fillRect(absX + saveButtonArea_.x, absY + saveButtonArea_.y,
                     saveButtonArea_.width, saveButtonArea_.height, saveColor);
    display->drawText(absX + saveButtonArea_.x + saveButtonArea_.width/2 - 20,
                     absY + saveButtonArea_.y + saveButtonArea_.height/2,
                     "Save", Color(220, 220, 220));
    
    // Cancel button
    Color cancelColor = (focusedField_ == Field::CancelButton) ?
                       Color(100, 60, 60) : Color(80, 60, 60);
    if (cancelButtonPressed_) cancelColor = Color(120, 80, 80);
    display->fillRect(absX + cancelButtonArea_.x, absY + cancelButtonArea_.y,
                     cancelButtonArea_.width, cancelButtonArea_.height, cancelColor);
    display->drawText(absX + cancelButtonArea_.x + cancelButtonArea_.width/2 - 30,
                     absY + cancelButtonArea_.y + cancelButtonArea_.height/2,
                     "Cancel", Color(220, 220, 220));
    
    // Instructions
    display->drawText(absX + 20, absY + height_ - 40,
                     "Tab: Next field | Shift+Tab: Previous field | Enter: Confirm | Esc: Cancel",
                     Color(150, 150, 150));
}

bool PresetSaveDialog::handleInput(const InputEvent& event) {
    if (!visible_ || !enabled_) return false;
    
    // Handle keyboard input for text fields
    if (event.type == InputEvent::Type::KeyPress) {
        // Tab to cycle focus
        if (event.keyCode == InputEvent::KeyCode::Tab) {
            cycleFocus(!event.shift); // Shift+Tab cycles backward
            return true;
        }
        
        // Enter to confirm current field or save
        if (event.keyCode == InputEvent::KeyCode::Enter) {
            if (focusedField_ == Field::SaveButton) {
                saveCurrentPreset();
                return true;
            }
            else if (focusedField_ == Field::CancelButton) {
                cancelSave();
                return true;
            }
            else {
                // Move to next field
                cycleFocus(true);
                return true;
            }
        }
        
        // Escape to cancel
        if (event.keyCode == InputEvent::KeyCode::Escape) {
            cancelSave();
            return true;
        }
        
        // Left/Right arrow keys for category selection
        if (focusedField_ == Field::Category) {
            if (event.keyCode == InputEvent::KeyCode::Left) {
                updateSelectedCategory(-1);
                return true;
            }
            else if (event.keyCode == InputEvent::KeyCode::Right) {
                updateSelectedCategory(1);
                return true;
            }
        }
        
        // Backspace for text field editing
        if (event.keyCode == InputEvent::KeyCode::Backspace) {
            handleDelete();
            return true;
        }
        
        // Character input for text fields
        if (event.character != 0 && (focusedField_ == Field::Name ||
                                   focusedField_ == Field::Author ||
                                   focusedField_ == Field::Description)) {
            handleCharacterInput(event.character);
            return true;
        }
    }
    
    // Handle touch input
    if (event.type == InputEvent::Type::TouchPress) {
        // Calculate event position relative to this component
        int relX = event.x - getAbsoluteX();
        int relY = event.y - getAbsoluteY();
        
        // Check if touch is in save button
        if (saveButtonArea_.contains(relX, relY)) {
            saveButtonPressed_ = true;
            focusedField_ = Field::SaveButton;
            return true;
        }
        
        // Check if touch is in cancel button
        if (cancelButtonArea_.contains(relX, relY)) {
            cancelButtonPressed_ = true;
            focusedField_ = Field::CancelButton;
            return true;
        }
        
        // Check if touch is in name input
        if (nameInputArea_.contains(relX, relY)) {
            focusedField_ = Field::Name;
            return true;
        }
        
        // Check if touch is in author input
        if (authorInputArea_.contains(relX, relY)) {
            focusedField_ = Field::Author;
            return true;
        }
        
        // Check if touch is in description input
        if (descriptionInputArea_.contains(relX, relY)) {
            focusedField_ = Field::Description;
            return true;
        }
        
        // Check if touch is in category select
        if (categorySelectArea_.contains(relX, relY)) {
            focusedField_ = Field::Category;
            return true;
        }
    }
    
    if (event.type == InputEvent::Type::TouchRelease) {
        // Calculate event position relative to this component
        int relX = event.x - getAbsoluteX();
        int relY = event.y - getAbsoluteY();
        
        // Check if released in save button
        if (saveButtonPressed_ && saveButtonArea_.contains(relX, relY)) {
            saveCurrentPreset();
        }
        
        // Check if released in cancel button
        if (cancelButtonPressed_ && cancelButtonArea_.contains(relX, relY)) {
            cancelSave();
        }
        
        // Reset button states
        saveButtonPressed_ = false;
        cancelButtonPressed_ = false;
        return true;
    }
    
    return false;
}

void PresetSaveDialog::show() {
    visible_ = true;
    
    // Initialize inputs with current preset data if available
    if (presetManager_) {
        std::string currentName = presetManager_->getCurrentPresetName();
        std::string currentAuthor = presetManager_->getCurrentPresetAuthor();
        std::string currentCategory = presetManager_->getCurrentPresetCategory();
        std::string currentDescription = presetManager_->getCurrentPresetDescription();
        
        // Only use preset values if they exist
        if (!currentName.empty()) {
            nameInput_ = currentName;
        } else {
            nameInput_ = "New Preset";
        }
        
        if (!currentAuthor.empty()) {
            authorInput_ = currentAuthor;
        }
        
        if (!currentCategory.empty()) {
            selectedCategory_ = currentCategory;
            
            // Find index of the category
            auto it = std::find(categories_.begin(), categories_.end(), currentCategory);
            if (it != categories_.end()) {
                selectedCategoryIndex_ = static_cast<int>(it - categories_.begin());
            }
        }
        
        if (!currentDescription.empty()) {
            descriptionInput_ = currentDescription;
        }
    }
    
    // Set initial focus on name field
    focusedField_ = Field::Name;
}

void PresetSaveDialog::hide() {
    visible_ = false;
}

bool PresetSaveDialog::isVisible() const {
    return visible_;
}

void PresetSaveDialog::setOnSaveComplete(std::function<void()> callback) {
    onSaveComplete_ = callback;
}

void PresetSaveDialog::setOnCancel(std::function<void()> callback) {
    onCancel_ = callback;
}

void PresetSaveDialog::updateLayout() {
    // Field sizes
    const int fieldWidth = width_ - 40;
    const int fieldHeight = 40;
    const int fieldX = 20;
    
    // Input fields
    nameInputArea_ = {fieldX, 80, fieldWidth, fieldHeight};
    authorInputArea_ = {fieldX, 160, fieldWidth, fieldHeight};
    categorySelectArea_ = {fieldX, 240, fieldWidth, fieldHeight};
    descriptionInputArea_ = {fieldX, 320, fieldWidth, fieldHeight};
    
    // Buttons
    const int buttonWidth = 120;
    const int buttonHeight = 40;
    const int buttonY = height_ - 60;
    
    saveButtonArea_ = {width_ - buttonWidth*2 - 40, buttonY, buttonWidth, buttonHeight};
    cancelButtonArea_ = {width_ - buttonWidth - 20, buttonY, buttonWidth, buttonHeight};
}

void PresetSaveDialog::saveCurrentPreset() {
    if (!presetManager_) {
        std::cerr << "Cannot save preset: no preset manager" << std::endl;
        return;
    }
    
    // Validate name
    if (nameInput_.empty()) {
        std::cerr << "Cannot save preset: name is empty" << std::endl;
        return;
    }
    
    // Create safe filename
    std::string safeName = nameInput_;
    std::transform(safeName.begin(), safeName.end(), safeName.begin(), 
                  [](unsigned char c) {
                      if (std::isalnum(c) || c == ' ' || c == '_' || c == '-') {
                          return std::tolower(c);
                      }
                      return '_';
                  });
    
    // Replace spaces with underscores
    std::replace(safeName.begin(), safeName.end(), ' ', '_');
    
    // Build file path
    fs::path presetDir = fs::path(PresetManager::getUserPresetsDirectory());
    if (!selectedCategory_.empty()) {
        presetDir /= selectedCategory_;
    }
    
    fs::path presetFile = presetDir / (safeName + ".preset");
    
    // Save the preset
    if (presetManager_->savePreset(presetFile.string(), nameInput_,
                                  authorInput_, selectedCategory_,
                                  descriptionInput_)) {
        std::cout << "Preset saved: " << presetFile.string() << std::endl;
        
        // Notify save complete
        if (onSaveComplete_) {
            onSaveComplete_();
        }
        
        // Close dialog
        hide();
    } else {
        std::cerr << "Failed to save preset" << std::endl;
    }
}

void PresetSaveDialog::cancelSave() {
    // Notify cancel
    if (onCancel_) {
        onCancel_();
    }
    
    // Close dialog
    hide();
}

void PresetSaveDialog::cycleFocus(bool forward) {
    if (forward) {
        // Forward cycle: Name -> Author -> Category -> Description -> Save -> Cancel -> Name
        switch (focusedField_) {
            case Field::Name: focusedField_ = Field::Author; break;
            case Field::Author: focusedField_ = Field::Category; break;
            case Field::Category: focusedField_ = Field::Description; break;
            case Field::Description: focusedField_ = Field::SaveButton; break;
            case Field::SaveButton: focusedField_ = Field::CancelButton; break;
            case Field::CancelButton: focusedField_ = Field::Name; break;
            default: focusedField_ = Field::Name; break;
        }
    } else {
        // Backward cycle: Name -> Cancel -> Save -> Description -> Category -> Author -> Name
        switch (focusedField_) {
            case Field::Name: focusedField_ = Field::CancelButton; break;
            case Field::Author: focusedField_ = Field::Name; break;
            case Field::Category: focusedField_ = Field::Author; break;
            case Field::Description: focusedField_ = Field::Category; break;
            case Field::SaveButton: focusedField_ = Field::Description; break;
            case Field::CancelButton: focusedField_ = Field::SaveButton; break;
            default: focusedField_ = Field::Name; break;
        }
    }
}

void PresetSaveDialog::updateSelectedCategory(int direction) {
    if (categories_.empty()) return;
    
    // Update index with wrapping
    selectedCategoryIndex_ = (selectedCategoryIndex_ + direction + categories_.size()) % categories_.size();
    selectedCategory_ = categories_[selectedCategoryIndex_];
}

void PresetSaveDialog::handleCharacterInput(char c) {
    // Get reference to current input field
    std::string& field = getCurrentInputField();
    
    // Append character
    field += c;
}

void PresetSaveDialog::handleDelete() {
    // Get reference to current input field
    std::string& field = getCurrentInputField();
    
    // Remove last character if field is not empty
    if (!field.empty()) {
        field.pop_back();
    }
}

std::string& PresetSaveDialog::getCurrentInputField() {
    switch (focusedField_) {
        case Field::Name: return nameInput_;
        case Field::Author: return authorInput_;
        case Field::Description: return descriptionInput_;
        default: return nameInput_; // Default fallback
    }
}

} // namespace AIMusicHardware