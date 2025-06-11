#pragma once

#include "UIComponents.h"
#include <vector>
#include <string>
#include <functional>

namespace AIMusicHardware {

class DropdownMenu : public UIComponent {
public:
    DropdownMenu(const std::string& id, const std::string& placeholder = "Select...");
    virtual ~DropdownMenu() = default;
    
    // Add items to the dropdown
    void addItem(const std::string& item);
    void addItems(const std::vector<std::string>& items);
    void clearItems();
    
    // Selection
    void selectItem(int index);
    void selectItem(const std::string& item);
    int getSelectedIndex() const { return selectedIndex_; }
    std::string getSelectedItem() const;
    
    // Callbacks
    void setSelectionCallback(std::function<void(int index, const std::string& item)> callback) {
        selectionCallback_ = callback;
    }
    
    // UI Component overrides
    void update(float deltaTime) override;
    void render(DisplayManager* display) override;
    bool handleInput(const InputEvent& event) override;
    
    // Special rendering for dropdown list (to be called after all other components)
    void renderDropdownList(DisplayManager* display);
    bool isDropdownOpen() const { return isOpen_; }
    
    // Appearance
    void setMaxVisibleItems(int count) { maxVisibleItems_ = count; }
    void setItemHeight(int height) { itemHeight_ = height; }
    
protected:
    std::vector<std::string> items_;
    int selectedIndex_ = -1;
    bool isOpen_ = false;
    std::string placeholder_;
    
    // Visual properties
    int itemHeight_ = 25;
    int maxVisibleItems_ = 8;
    int scrollOffset_ = 0;
    int hoveredIndex_ = -1;
    bool openUpwards_ = false;  // Auto-detected based on position
    
    // Callbacks
    std::function<void(int index, const std::string& item)> selectionCallback_;
    
    // Helper methods
    int getDropdownHeight() const;
    void ensureSelectedVisible();
    bool shouldOpenUpwards() const;
};

// Specialized preset dropdown with categories
class PresetDropdown : public DropdownMenu {
public:
    struct PresetItem {
        std::string name;
        std::string category;
        std::string fullPath;
        
        std::string getDisplayName() const {
            return "[" + category + "] " + name;
        }
    };
    
    PresetDropdown(const std::string& id);
    
    void addPreset(const std::string& name, const std::string& category, const std::string& fullPath);
    void clearPresets();
    
    // Get selected preset info
    PresetItem getSelectedPreset() const;
    
    // Filter by category
    void filterByCategory(const std::string& category);
    void clearCategoryFilter();
    
private:
    std::vector<PresetItem> presets_;
    std::string categoryFilter_;
    
    void updateDisplayItems();
};

} // namespace AIMusicHardware