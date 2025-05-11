#include "../../../include/ui/presets/PresetSelector.h"
#include "../../../include/ui/DisplayManager.h"
#include <algorithm>
#include <iostream>

namespace AIMusicHardware {

PresetSelector::PresetSelector(PresetManager* presetManager)
    : UIComponent("PresetSelector"),
      presetManager_(presetManager) {

    width_ = 800;
    height_ = 40; // Default width and height
    updateLayout();
}

PresetSelector::~PresetSelector() {
    // Nothing specific to clean up
}

void PresetSelector::render(DisplayManager* display) {
    if (!visible_ || !display) return;

    // Get the component's position
    int absX = x_;
    int absY = y_;
    
    // Background
    display->fillRect(absX, absY, width_, height_, Color(40, 40, 40));
    
    // Previous button
    Color prevColor = prevButtonPressed_ ? Color(100, 100, 100) : Color(60, 60, 60);
    display->fillRect(absX + prevButtonArea_.x, absY + prevButtonArea_.y,
                     prevButtonArea_.width, prevButtonArea_.height, prevColor);
    display->drawText(absX + prevButtonArea_.x + prevButtonArea_.width/2 - 5,
                     absY + prevButtonArea_.y + prevButtonArea_.height/2 - 8,
                     "<", nullptr, Color(200, 200, 200));
    
    // Next button
    Color nextColor = nextButtonPressed_ ? Color(100, 100, 100) : Color(60, 60, 60);
    display->fillRect(absX + nextButtonArea_.x, absY + nextButtonArea_.y,
                     nextButtonArea_.width, nextButtonArea_.height, nextColor);
    display->drawText(absX + nextButtonArea_.x + nextButtonArea_.width/2 - 5,
                     absY + nextButtonArea_.y + nextButtonArea_.height/2 - 8,
                     ">", nullptr, Color(200, 200, 200));
    
    // Save button
    Color saveColor = saveButtonPressed_ ? Color(100, 100, 100) : Color(60, 60, 60);
    display->fillRect(absX + saveButtonArea_.x, absY + saveButtonArea_.y,
                     saveButtonArea_.width, saveButtonArea_.height, saveColor);
    display->drawText(absX + saveButtonArea_.x + saveButtonArea_.width/2 - 15,
                     absY + saveButtonArea_.y + saveButtonArea_.height/2 - 8,
                     "Save", nullptr, Color(200, 200, 200));
    
    // Preset name
    std::string presetName = presetManager_ ? presetManager_->getCurrentPresetName() : "No Preset";
    display->drawText(absX + nameArea_.x + 10, absY + nameArea_.y + nameArea_.height/2 - 8,
                     presetName, nullptr, Color(200, 200, 200));
}

void PresetSelector::update(float deltaTime) {
    // Nothing to update in this component
}

bool PresetSelector::handleInput(const InputEvent& event) {
    if (!visible_ || !enabled_) return false;

    // Check if the event is within this component's bounds
    Point touchPoint(event.value, event.value2);
    if (!getBounds().contains(touchPoint)) return false;

    // Calculate event position relative to this component
    int relX = event.value - x_;
    int relY = event.value2 - y_;
    
    // Handle the event based on its type
    switch (event.type) {
        case InputEventType::TouchPress:
            // Previous button
            if (prevButtonArea_.contains(Point(relX, relY))) {
                prevButtonPressed_ = true;
                return true;
            }
            // Next button
            else if (nextButtonArea_.contains(Point(relX, relY))) {
                nextButtonPressed_ = true;
                return true;
            }
            // Save button
            else if (saveButtonArea_.contains(Point(relX, relY))) {
                saveButtonPressed_ = true;
                return true;
            }
            break;
            
        case InputEventType::TouchRelease:
            // Previous button
            if (prevButtonPressed_ && prevButtonArea_.contains(Point(relX, relY))) {
                loadPreviousPreset();
            }
            // Next button
            else if (nextButtonPressed_ && nextButtonArea_.contains(Point(relX, relY))) {
                loadNextPreset();
            }
            // Save button
            else if (saveButtonPressed_ && saveButtonArea_.contains(Point(relX, relY))) {
                requestSave();
            }
            
            // Reset button states
            prevButtonPressed_ = false;
            nextButtonPressed_ = false;
            saveButtonPressed_ = false;
            return true;
            
        default:
            break;
    }
    
    return false;
}

void PresetSelector::setOnSaveRequested(std::function<void()> callback) {
    onSaveRequested_ = callback;
}

void PresetSelector::setOnPresetChanged(std::function<void(const std::string&)> callback) {
    onPresetChanged_ = callback;
}

void PresetSelector::updateLayout() {
    // Define button sizes and spacing
    const int buttonWidth = 40;
    const int buttonHeight = 30;
    const int buttonMargin = 5;
    
    // Calculate positions
    prevButtonArea_ = {buttonMargin, (height_ - buttonHeight) / 2, buttonWidth, buttonHeight};
    nextButtonArea_ = {prevButtonArea_.x + prevButtonArea_.width + buttonMargin, 
                       (height_ - buttonHeight) / 2, buttonWidth, buttonHeight};
    
    saveButtonArea_ = {width_ - buttonWidth - buttonMargin, 
                       (height_ - buttonHeight) / 2, buttonWidth, buttonHeight};
    
    // Name area is in the middle
    nameArea_ = {nextButtonArea_.x + nextButtonArea_.width + buttonMargin, 
                (height_ - buttonHeight) / 2, 
                saveButtonArea_.x - (nextButtonArea_.x + nextButtonArea_.width) - 2 * buttonMargin, 
                buttonHeight};
}

void PresetSelector::loadPreviousPreset() {
    if (presetManager_) {
        if (presetManager_->loadPreviousPreset()) {
            if (onPresetChanged_) {
                onPresetChanged_(presetManager_->getCurrentPresetPath());
            }
        }
    }
}

void PresetSelector::loadNextPreset() {
    if (presetManager_) {
        if (presetManager_->loadNextPreset()) {
            if (onPresetChanged_) {
                onPresetChanged_(presetManager_->getCurrentPresetPath());
            }
        }
    }
}

void PresetSelector::requestSave() {
    if (onSaveRequested_) {
        onSaveRequested_();
    }
}

} // namespace AIMusicHardware