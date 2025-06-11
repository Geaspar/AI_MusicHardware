#include "../../include/ui/DropdownMenu.h"
#include <algorithm>

namespace AIMusicHardware {

DropdownMenu::DropdownMenu(const std::string& id, const std::string& placeholder)
    : UIComponent(id)
    , placeholder_(placeholder) {
    height_ = 30; // Default closed height
}

void DropdownMenu::addItem(const std::string& item) {
    items_.push_back(item);
}

void DropdownMenu::addItems(const std::vector<std::string>& items) {
    items_.insert(items_.end(), items.begin(), items.end());
}

void DropdownMenu::clearItems() {
    items_.clear();
    selectedIndex_ = -1;
    scrollOffset_ = 0;
}

void DropdownMenu::selectItem(int index) {
    if (index >= 0 && index < static_cast<int>(items_.size())) {
        selectedIndex_ = index;
        if (selectionCallback_) {
            selectionCallback_(index, items_[index]);
        }
    }
}

void DropdownMenu::selectItem(const std::string& item) {
    auto it = std::find(items_.begin(), items_.end(), item);
    if (it != items_.end()) {
        selectItem(std::distance(items_.begin(), it));
    }
}

std::string DropdownMenu::getSelectedItem() const {
    if (selectedIndex_ >= 0 && selectedIndex_ < static_cast<int>(items_.size())) {
        return items_[selectedIndex_];
    }
    return "";
}

void DropdownMenu::update(float deltaTime) {
    // Could add animations here
}

void DropdownMenu::render(DisplayManager* display) {
    if (!display || !visible_) return;
    
    // Determine if we should open upwards (when near bottom of screen)
    openUpwards_ = shouldOpenUpwards();
    
    // Draw main dropdown box
    Color bgColor = isOpen_ ? Color(50, 50, 60) : Color(40, 40, 50);
    Color borderColor = isOpen_ ? Color(100, 150, 200) : Color(80, 80, 100);
    
    display->fillRect(x_, y_, width_, height_, bgColor);
    display->drawRect(x_, y_, width_, height_, borderColor);
    
    // Draw selected item or placeholder
    std::string displayText = placeholder_;
    if (selectedIndex_ >= 0 && selectedIndex_ < static_cast<int>(items_.size())) {
        displayText = items_[selectedIndex_];
    }
    
    Color textColor = selectedIndex_ >= 0 ? Color(220, 220, 220) : Color(150, 150, 150);
    display->drawText(x_ + 10, y_ + 8, displayText, nullptr, textColor);
    
    // Draw dropdown arrow
    int arrowX = x_ + width_ - 25;
    int arrowY = y_ + height_ / 2;
    if (isOpen_) {
        if (openUpwards_) {
            // Down arrow when open upwards
            display->drawLine(arrowX, arrowY - 3, arrowX + 5, arrowY + 3, Color(200, 200, 200));
            display->drawLine(arrowX + 5, arrowY + 3, arrowX + 10, arrowY - 3, Color(200, 200, 200));
        } else {
            // Up arrow when open downwards
            display->drawLine(arrowX, arrowY + 3, arrowX + 5, arrowY - 3, Color(200, 200, 200));
            display->drawLine(arrowX + 5, arrowY - 3, arrowX + 10, arrowY + 3, Color(200, 200, 200));
        }
    } else {
        // Show direction arrow when closed
        if (openUpwards_) {
            // Up arrow to indicate upward opening
            display->drawLine(arrowX, arrowY + 3, arrowX + 5, arrowY - 3, Color(200, 200, 200));
            display->drawLine(arrowX + 5, arrowY - 3, arrowX + 10, arrowY + 3, Color(200, 200, 200));
        } else {
            // Down arrow to indicate downward opening
            display->drawLine(arrowX, arrowY - 3, arrowX + 5, arrowY + 3, Color(200, 200, 200));
            display->drawLine(arrowX + 5, arrowY + 3, arrowX + 10, arrowY - 3, Color(200, 200, 200));
        }
    }
    
    // Note: Dropdown list is now rendered in renderDropdownList() for proper z-ordering
}

void DropdownMenu::renderDropdownList(DisplayManager* display) {
    if (!display || !visible_ || !isOpen_ || items_.empty()) return;
    
    Color borderColor = Color(100, 150, 200);
    int dropdownHeight = getDropdownHeight();
    int listY;
    
    if (openUpwards_) {
        // Position list above the dropdown button
        listY = y_ - dropdownHeight;
    } else {
        // Position list below the dropdown button
        listY = y_ + height_;
    }
    
    // Background for dropdown
    display->fillRect(x_, listY, width_, dropdownHeight, Color(35, 35, 40));
    display->drawRect(x_, listY, width_, dropdownHeight, borderColor);
    
    // Draw visible items
    int visibleItems = std::min(static_cast<int>(items_.size()), maxVisibleItems_);
    for (int i = 0; i < visibleItems; ++i) {
        int itemIndex = i + scrollOffset_;
        if (itemIndex >= static_cast<int>(items_.size())) break;
        
        int itemY = listY + i * itemHeight_;
        
        // Highlight hovered or selected item
        if (itemIndex == hoveredIndex_) {
            display->fillRect(x_ + 1, itemY + 1, width_ - 2, itemHeight_ - 2, Color(60, 60, 80));
        } else if (itemIndex == selectedIndex_) {
            display->fillRect(x_ + 1, itemY + 1, width_ - 2, itemHeight_ - 2, Color(50, 80, 120));
        }
        
        // Draw item text
        display->drawText(x_ + 10, itemY + 5, items_[itemIndex], nullptr, Color(200, 200, 200));
    }
    
    // Draw scrollbar if needed
    if (static_cast<int>(items_.size()) > maxVisibleItems_) {
        int scrollbarX = x_ + width_ - 10;
        int scrollbarHeight = dropdownHeight - 4;
        int thumbHeight = (maxVisibleItems_ * scrollbarHeight) / items_.size();
        int thumbY = listY + 2 + (scrollOffset_ * (scrollbarHeight - thumbHeight)) / (items_.size() - maxVisibleItems_);
        
        display->fillRect(scrollbarX, listY + 2, 6, scrollbarHeight, Color(40, 40, 40));
        display->fillRect(scrollbarX, thumbY, 6, thumbHeight, Color(80, 80, 80));
    }
}

bool DropdownMenu::handleInput(const InputEvent& event) {
    if (!visible_ || !enabled_) return false;
    
    bool inMainBounds = (event.value >= x_ && event.value < x_ + width_ &&
                        event.value2 >= y_ && event.value2 < y_ + height_);
    
    if (event.type == InputEventType::TouchPress) {
        if (inMainBounds) {
            isOpen_ = !isOpen_;
            return true;
        } else if (isOpen_) {
            // Check if click is in dropdown list
            int dropdownHeight = getDropdownHeight();
            int listY = openUpwards_ ? y_ - dropdownHeight : y_ + height_;
            
            bool inDropdownBounds = (event.value >= x_ && event.value < x_ + width_ &&
                                   event.value2 >= listY && event.value2 < listY + dropdownHeight);
            
            if (inDropdownBounds) {
                int clickY = event.value2 - listY;
                int clickedIndex = scrollOffset_ + clickY / itemHeight_;
                if (clickedIndex >= 0 && clickedIndex < static_cast<int>(items_.size())) {
                    selectItem(clickedIndex);
                    isOpen_ = false;
                }
                return true;
            } else {
                // Click outside - close dropdown
                isOpen_ = false;
                return false;
            }
        }
    } else if (event.type == InputEventType::TouchMove && isOpen_) {
        // Update hovered item
        int dropdownHeight = getDropdownHeight();
        int listY = openUpwards_ ? y_ - dropdownHeight : y_ + height_;
        
        bool inDropdownBounds = (event.value >= x_ && event.value < x_ + width_ &&
                               event.value2 >= listY && event.value2 < listY + dropdownHeight);
        
        if (inDropdownBounds) {
            int hoverY = event.value2 - listY;
            hoveredIndex_ = scrollOffset_ + hoverY / itemHeight_;
            if (hoveredIndex_ >= static_cast<int>(items_.size())) {
                hoveredIndex_ = -1;
            }
        } else {
            hoveredIndex_ = -1;
        }
    } else if (event.type == InputEventType::EncoderRotate && isOpen_) {
        // Handle scrolling
        if (static_cast<int>(items_.size()) > maxVisibleItems_) {
            scrollOffset_ = std::max(0, std::min(scrollOffset_ - static_cast<int>(event.value),
                                                static_cast<int>(items_.size()) - maxVisibleItems_));
            return true;
        }
    }
    
    return false;
}

int DropdownMenu::getDropdownHeight() const {
    return std::min(static_cast<int>(items_.size()), maxVisibleItems_) * itemHeight_ + 4;
}

void DropdownMenu::ensureSelectedVisible() {
    if (selectedIndex_ < 0) return;
    
    if (selectedIndex_ < scrollOffset_) {
        scrollOffset_ = selectedIndex_;
    } else if (selectedIndex_ >= scrollOffset_ + maxVisibleItems_) {
        scrollOffset_ = selectedIndex_ - maxVisibleItems_ + 1;
    }
}

bool DropdownMenu::shouldOpenUpwards() const {
    // Check if dropdown would go off screen if opened downwards
    // Assume screen height is 800 pixels (standard for this application)
    const int screenHeight = 800;
    int dropdownHeight = getDropdownHeight();
    
    // If dropdown would extend past screen bottom, open upwards
    if (y_ + height_ + dropdownHeight > screenHeight - 20) { // 20px margin
        // But only if there's enough space above
        return y_ - dropdownHeight > 20;
    }
    
    return false;
}

// PresetDropdown implementation
PresetDropdown::PresetDropdown(const std::string& id)
    : DropdownMenu(id, "Select Preset...") {
}

void PresetDropdown::addPreset(const std::string& name, const std::string& category, const std::string& fullPath) {
    PresetItem preset{name, category, fullPath};
    presets_.push_back(preset);
    updateDisplayItems();
}

void PresetDropdown::clearPresets() {
    presets_.clear();
    clearItems();
}

PresetDropdown::PresetItem PresetDropdown::getSelectedPreset() const {
    int idx = getSelectedIndex();
    if (idx >= 0 && idx < static_cast<int>(presets_.size())) {
        // Need to map from filtered index to actual preset index
        std::string selectedDisplay = getSelectedItem();
        for (const auto& preset : presets_) {
            if (preset.getDisplayName() == selectedDisplay) {
                return preset;
            }
        }
    }
    return PresetItem{"", "", ""};
}

void PresetDropdown::filterByCategory(const std::string& category) {
    categoryFilter_ = category;
    updateDisplayItems();
}

void PresetDropdown::clearCategoryFilter() {
    categoryFilter_.clear();
    updateDisplayItems();
}

void PresetDropdown::updateDisplayItems() {
    clearItems();
    
    for (const auto& preset : presets_) {
        if (categoryFilter_.empty() || preset.category == categoryFilter_) {
            addItem(preset.getDisplayName());
        }
    }
}

} // namespace AIMusicHardware