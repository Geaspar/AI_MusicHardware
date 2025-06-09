#include "../../include/ui/PresetBrowserUIComponent.h"
#include "../../include/ui/parameters/ParameterManager.h"
#include <algorithm>
#include <cctype>

namespace AIMusicHardware {

// PresetListItem implementation
PresetListItem::PresetListItem(const std::string& id, const PresetInfo& preset, int index)
    : UIComponent(id)
    , preset_(preset)
    , index_(index) {
    height_ = ITEM_HEIGHT;
}

void PresetListItem::update(float deltaTime) {
    // No animation needed for now
}

void PresetListItem::render(DisplayManager* display) {
    if (!display) return;
    
    // Background color based on state
    Color bgColor;
    if (isSelected_) {
        bgColor = Color(60, 120, 180); // Blue selection
    } else if (isHovered_) {
        bgColor = Color(50, 50, 50); // Dark gray hover
    } else {
        bgColor = Color(30, 30, 30); // Darker background
    }
    
    display->fillRect(x_, y_, width_, height_, bgColor);
    
    // Draw preset name
    Color textColor = isSelected_ ? Color(255, 255, 255) : Color(200, 200, 200);
    display->drawText(x_ + PADDING, y_ + PADDING, preset_.name, nullptr, textColor);
    
    // Draw category on the right
    std::string categoryText = "[" + preset_.category + "]";
    int textWidth = categoryText.length() * 8; // Approximate
    display->drawText(x_ + width_ - textWidth - PADDING, y_ + PADDING, 
                     categoryText, nullptr, Color(150, 150, 150));
}

bool PresetListItem::handleInput(const InputEvent& event) {
    if (!visible_ || !enabled_) return false;
    
    bool inBounds = (event.value >= x_ && event.value < x_ + width_ &&
                    event.value2 >= y_ && event.value2 < y_ + height_);
    
    if (event.type == InputEventType::TouchMove) {
        isHovered_ = inBounds;
        return false; // Don't consume move events
    }
    
    if (event.type == InputEventType::TouchPress && inBounds) {
        if (clickCallback_) {
            clickCallback_(preset_);
        }
        return true;
    }
    
    return false;
}

// PresetListView implementation
PresetListView::PresetListView(const std::string& id)
    : UIComponent(id) {
}

void PresetListView::setPresets(const std::vector<PresetInfo>& presets) {
    items_.clear();
    
    for (size_t i = 0; i < presets.size(); ++i) {
        auto item = std::make_unique<PresetListItem>(
            "preset_" + std::to_string(i), presets[i], i);
        
        item->setClickCallback([this, i](const PresetInfo& preset) {
            selectPreset(i);
        });
        
        items_.push_back(std::move(item));
    }
    
    updateScrollBounds();
}

void PresetListView::clearPresets() {
    items_.clear();
    selectedIndex_ = -1;
    scrollOffset_ = 0.0f;
    maxScroll_ = 0.0f;
}

void PresetListView::selectPreset(int index) {
    if (index < 0 || index >= static_cast<int>(items_.size())) return;
    
    // Update selection state
    if (selectedIndex_ >= 0 && selectedIndex_ < static_cast<int>(items_.size())) {
        items_[selectedIndex_]->setSelected(false);
    }
    
    selectedIndex_ = index;
    items_[selectedIndex_]->setSelected(true);
    
    // Notify callback
    if (selectionCallback_) {
        selectionCallback_(items_[selectedIndex_]->getPreset());
    }
    
    ensureSelectedVisible();
}

void PresetListView::update(float deltaTime) {
    // Update visible items
    int itemY = y_ - static_cast<int>(scrollOffset_);
    
    for (auto& item : items_) {
        item->setPosition(x_, itemY);
        item->setSize(width_, PresetListItem::ITEM_HEIGHT);
        
        // Only update visible items
        bool isVisible = (itemY + PresetListItem::ITEM_HEIGHT > y_) && (itemY < y_ + height_);
        item->setVisible(isVisible);
        
        if (isVisible) {
            item->update(deltaTime);
        }
        
        itemY += PresetListItem::ITEM_HEIGHT;
    }
}

void PresetListView::render(DisplayManager* display) {
    if (!display) return;
    
    // Draw background
    display->fillRect(x_, y_, width_, height_, Color(20, 20, 20));
    
    // Set clipping region (simple scissor test)
    // Note: Real implementation would use proper clipping
    
    // Render visible items
    for (auto& item : items_) {
        if (item->isVisible()) {
            item->render(display);
        }
    }
    
    // Draw scrollbar if needed
    if (maxScroll_ > 0) {
        int scrollbarWidth = 10;
        int scrollbarX = x_ + width_ - scrollbarWidth - 2;
        
        // Scrollbar track
        display->fillRect(scrollbarX, y_, scrollbarWidth, height_, Color(40, 40, 40));
        
        // Scrollbar thumb
        float scrollRatio = scrollOffset_ / maxScroll_;
        float thumbHeight = (height_ * height_) / (height_ + maxScroll_);
        float thumbY = y_ + scrollRatio * (height_ - thumbHeight);
        
        display->fillRect(scrollbarX, static_cast<int>(thumbY), 
                         scrollbarWidth, static_cast<int>(thumbHeight), 
                         Color(80, 80, 80));
    }
}

bool PresetListView::handleInput(const InputEvent& event) {
    if (!visible_ || !enabled_) return false;
    
    bool inBounds = (event.value >= x_ && event.value < x_ + width_ &&
                    event.value2 >= y_ && event.value2 < y_ + height_);
    
    if (!inBounds) return false;
    
    // Handle scrolling (simplified - real implementation would track drag)
    if (event.type == InputEventType::TouchMove) {
        // Simple scroll: each pixel of vertical movement scrolls by 2 pixels
        static int lastY = event.value2;
        if (event.id == 0) { // Only track primary touch
            int deltaY = event.value2 - lastY;
            scrollOffset_ = std::clamp(scrollOffset_ - deltaY * 2.0f, 0.0f, maxScroll_);
            lastY = event.value2;
        }
    }
    
    // Let items handle their own input
    for (auto& item : items_) {
        if (item->isVisible() && item->handleInput(event)) {
            return true;
        }
    }
    
    return inBounds; // Consume event if in bounds
}

void PresetListView::updateScrollBounds() {
    float contentHeight = items_.size() * PresetListItem::ITEM_HEIGHT;
    maxScroll_ = std::max(0.0f, contentHeight - height_);
    scrollOffset_ = std::clamp(scrollOffset_, 0.0f, maxScroll_);
}

void PresetListView::ensureSelectedVisible() {
    if (selectedIndex_ < 0) return;
    
    float itemY = selectedIndex_ * PresetListItem::ITEM_HEIGHT;
    float itemBottom = itemY + PresetListItem::ITEM_HEIGHT;
    
    if (itemY < scrollOffset_) {
        scrollOffset_ = itemY;
    } else if (itemBottom > scrollOffset_ + height_) {
        scrollOffset_ = itemBottom - height_;
    }
}

// PresetSearchBox implementation
PresetSearchBox::PresetSearchBox(const std::string& id)
    : UIComponent(id) {
    height_ = 30;
}

void PresetSearchBox::update(float deltaTime) {
    if (hasFocus_) {
        cursorBlink_ += deltaTime;
        if (cursorBlink_ > 1.0f) cursorBlink_ -= 1.0f;
    }
}

void PresetSearchBox::render(DisplayManager* display) {
    if (!display) return;
    
    // Draw background
    Color bgColor = hasFocus_ ? Color(40, 40, 40) : Color(30, 30, 30);
    display->fillRect(x_, y_, width_, height_, bgColor);
    
    // Draw border
    Color borderColor = hasFocus_ ? Color(100, 150, 200) : Color(60, 60, 60);
    display->drawRect(x_, y_, width_, height_, borderColor);
    
    // Draw text or placeholder
    std::string displayText = searchText_.empty() ? placeholder_ : searchText_;
    Color textColor = searchText_.empty() ? Color(100, 100, 100) : Color(200, 200, 200);
    display->drawText(x_ + 5, y_ + 7, displayText, nullptr, textColor);
    
    // Draw cursor if focused
    if (hasFocus_ && cursorBlink_ < 0.5f) {
        int cursorX = x_ + 5 + searchText_.length() * 8;
        display->drawLine(cursorX, y_ + 5, cursorX, y_ + height_ - 5, Color(255, 255, 255));
    }
}

bool PresetSearchBox::handleInput(const InputEvent& event) {
    if (!visible_ || !enabled_) return false;
    
    bool inBounds = (event.value >= x_ && event.value < x_ + width_ &&
                    event.value2 >= y_ && event.value2 < y_ + height_);
    
    if (event.type == InputEventType::TouchPress) {
        hasFocus_ = inBounds;
        return inBounds;
    }
    
    // Handle keyboard input when focused
    if (hasFocus_ && event.type == InputEventType::ButtonPress) {
        // Simplified keyboard handling
        if (event.id == 8) { // Backspace
            if (!searchText_.empty()) {
                searchText_.pop_back();
                if (textChangeCallback_) {
                    textChangeCallback_(searchText_);
                }
            }
            return true;
        } else if (event.id >= 32 && event.id < 127) { // Printable characters
            searchText_ += static_cast<char>(event.id);
            if (textChangeCallback_) {
                textChangeCallback_(searchText_);
            }
            return true;
        }
    }
    
    return false;
}

// PresetCategoryFilter implementation
PresetCategoryFilter::PresetCategoryFilter(const std::string& id)
    : UIComponent(id) {
    height_ = 40;
}

void PresetCategoryFilter::setCategories(const std::vector<std::string>& categories) {
    categoryButtons_.clear();
    
    int buttonX = x_;
    for (const auto& category : categories) {
        auto button = std::make_unique<Button>("cat_" + category, category);
        button->setPosition(buttonX, y_);
        button->setSize(80, 30);
        
        // Capture category by value
        button->setClickCallback([this, category]() {
            selectCategory(category);
        });
        
        categoryButtons_.push_back(std::move(button));
        buttonX += 85;
    }
}

void PresetCategoryFilter::selectCategory(const std::string& category) {
    selectedCategory_ = category;
    
    // Update button states
    for (auto& button : categoryButtons_) {
        // Toggle state is handled internally by button click
    }
    
    if (categoryCallback_) {
        categoryCallback_(category);
    }
}

void PresetCategoryFilter::update(float deltaTime) {
    for (auto& button : categoryButtons_) {
        button->update(deltaTime);
    }
}

void PresetCategoryFilter::render(DisplayManager* display) {
    if (!display) return;
    
    for (auto& button : categoryButtons_) {
        button->render(display);
    }
}

bool PresetCategoryFilter::handleInput(const InputEvent& event) {
    for (auto& button : categoryButtons_) {
        if (button->handleInput(event)) {
            return true;
        }
    }
    return false;
}

// PresetBrowserUI implementation
PresetBrowserUI::PresetBrowserUI(const std::string& id)
    : UIComponent(id) {
    
    // Create sub-components
    searchBox_ = std::make_unique<PresetSearchBox>("search");
    categoryFilter_ = std::make_unique<PresetCategoryFilter>("categories");
    listView_ = std::make_unique<PresetListView>("list");
    presetInfoLabel_ = std::make_unique<Label>("info", "No preset selected");
    loadButton_ = std::make_unique<Button>("load", "Load");
    saveButton_ = std::make_unique<Button>("save", "Save");
    deleteButton_ = std::make_unique<Button>("delete", "Delete");
    
    // Set callbacks
    searchBox_->setTextChangeCallback([this](const std::string& text) {
        onSearchTextChanged(text);
    });
    
    categoryFilter_->setCategoryCallback([this](const std::string& category) {
        onCategoryChanged(category);
    });
    
    listView_->setSelectionCallback([this](const PresetInfo& preset) {
        onPresetSelected(preset);
    });
    
    loadButton_->setClickCallback([this]() {
        loadSelectedPreset();
    });
    
    // Don't call layoutComponents() here - it will be called when setPosition is called
}

PresetBrowserUI::~PresetBrowserUI() = default;

void PresetBrowserUI::initialize(PresetManager* presetManager, PresetDatabase* database) {
    presetManager_ = presetManager;
    database_ = database;
    
    std::cout << "PresetBrowserUI::initialize - Database has " 
              << (database_ ? database_->getAllPresets().size() : 0) << " presets" << std::endl;
    
    // Set up categories
    std::vector<std::string> categories = {"All", "Bass", "Lead", "Pad", "FX", "User"};
    categoryFilter_->setCategories(categories);
    categoryFilter_->selectCategory("All");
    
    refresh();
    
    std::cout << "PresetBrowserUI::initialize - After refresh, filteredPresets_ has " 
              << filteredPresets_.size() << " presets" << std::endl;
}

void PresetBrowserUI::refresh() {
    if (!database_) return;
    
    filterPresets();
}

void PresetBrowserUI::loadSelectedPreset() {
    if (selectedPreset_.name.empty()) return;
    
    if (presetManager_ && parameterManager_) {
        // Load preset data
        nlohmann::json presetData;
        if (presetManager_->loadPreset(selectedPreset_.name)) {
            // Update all parameters
            auto* rootGroup = parameterManager_->getRootGroup();
            if (rootGroup && presetData.contains("parameters")) {
                // TODO: Implement parameter loading from JSON
                // This would iterate through the JSON and update each parameter
            }
            
            if (loadCallback_) {
                loadCallback_(selectedPreset_);
            }
        }
    }
}

void PresetBrowserUI::saveAsNewPreset(const std::string& name, const std::string& category) {
    if (!presetManager_ || !parameterManager_) return;
    
    // Gather all parameters into JSON
    nlohmann::json presetData;
    presetData["name"] = name;
    presetData["category"] = category;
    presetData["parameters"] = nlohmann::json::object();
    
    // TODO: Implement parameter serialization
    // This would iterate through all parameters and save their values
    
    if (presetManager_->savePreset(name, presetData)) {
        refresh();
        
        if (saveCallback_) {
            saveCallback_(name, category);
        }
    }
}

void PresetBrowserUI::update(float deltaTime) {
    searchBox_->update(deltaTime);
    categoryFilter_->update(deltaTime);
    listView_->update(deltaTime);
    presetInfoLabel_->update(deltaTime);
    loadButton_->update(deltaTime);
    saveButton_->update(deltaTime);
    deleteButton_->update(deltaTime);
}

void PresetBrowserUI::render(DisplayManager* display) {
    if (!display) return;
    
    // Draw background
    display->fillRect(x_, y_, width_, height_, Color(25, 25, 25));
    
    // Draw border
    display->drawRect(x_, y_, width_, height_, Color(60, 60, 60));
    
    // Debug: Draw component position info
    display->drawText(x_ + 5, y_ + 5, "Preset Browser", nullptr, Color(255, 255, 255));
    
    // Render sub-components
    searchBox_->render(display);
    categoryFilter_->render(display);
    listView_->render(display);
    presetInfoLabel_->render(display);
    loadButton_->render(display);
    saveButton_->render(display);
    deleteButton_->render(display);
}

bool PresetBrowserUI::handleInput(const InputEvent& event) {
    if (!visible_ || !enabled_) return false;
    
    // Check sub-components in order
    if (searchBox_->handleInput(event)) return true;
    if (categoryFilter_->handleInput(event)) return true;
    if (listView_->handleInput(event)) return true;
    if (loadButton_->handleInput(event)) return true;
    if (saveButton_->handleInput(event)) return true;
    if (deleteButton_->handleInput(event)) return true;
    
    return false;
}

void PresetBrowserUI::filterPresets() {
    if (!database_) return;
    
    filteredPresets_.clear();
    
    std::string searchLower = searchBox_->getText();
    std::transform(searchLower.begin(), searchLower.end(), searchLower.begin(), ::tolower);
    
    std::string category = categoryFilter_->getSelectedCategory();
    
    // Get all presets and filter
    auto allPresets = database_->getAllPresets();
    
    for (const auto& preset : allPresets) {
        // Category filter
        if (category != "All" && preset.category != category) {
            continue;
        }
        
        // Search filter
        if (!searchLower.empty()) {
            std::string nameLower = preset.name;
            std::transform(nameLower.begin(), nameLower.end(), nameLower.begin(), ::tolower);
            
            if (nameLower.find(searchLower) == std::string::npos) {
                continue;
            }
        }
        
        filteredPresets_.push_back(preset);
    }
    
    listView_->setPresets(filteredPresets_);
}

void PresetBrowserUI::onSearchTextChanged(const std::string& text) {
    filterPresets();
}

void PresetBrowserUI::onCategoryChanged(const std::string& category) {
    filterPresets();
}

void PresetBrowserUI::onPresetSelected(const PresetInfo& preset) {
    selectedPreset_ = preset;
    
    // Update info label
    std::string info = preset.name + " by " + preset.author + 
                      " (" + preset.category + ")";
    presetInfoLabel_->setText(info);
    
    // Enable/disable buttons
    loadButton_->setEnabled(true);
    deleteButton_->setEnabled(preset.category == "User");
}

void PresetBrowserUI::layoutComponents() {
    // Layout sub-components within our bounds
    int padding = 10;
    int currentY = y_ + padding;
    
    // Search box at top
    searchBox_->setPosition(x_ + padding, currentY);
    searchBox_->setSize(width_ - 2 * padding, 30);
    currentY += 35;
    
    // Category filter
    categoryFilter_->setPosition(x_ + padding, currentY);
    categoryFilter_->setSize(width_ - 2 * padding, 40);
    currentY += 45;
    
    // List view (main area)
    int listHeight = height_ - 200; // Leave room for bottom controls
    listView_->setPosition(x_ + padding, currentY);
    listView_->setSize(width_ - 2 * padding, listHeight);
    currentY += listHeight + padding;
    
    // Info label
    presetInfoLabel_->setPosition(x_ + padding, currentY);
    currentY += 25;
    
    // Buttons at bottom
    int buttonWidth = 80;
    int buttonSpacing = 10;
    loadButton_->setPosition(x_ + padding, currentY);
    loadButton_->setSize(buttonWidth, 30);
    
    saveButton_->setPosition(x_ + padding + buttonWidth + buttonSpacing, currentY);
    saveButton_->setSize(buttonWidth, 30);
    
    deleteButton_->setPosition(x_ + padding + 2 * (buttonWidth + buttonSpacing), currentY);
    deleteButton_->setSize(buttonWidth, 30);
}

// PresetSaveDialogUI implementation
PresetSaveDialogUI::PresetSaveDialogUI(const std::string& id)
    : UIComponent(id) {
    
    width_ = 400;
    height_ = 300;
    
    saveButton_ = std::make_unique<Button>("save_btn", "Save");
    cancelButton_ = std::make_unique<Button>("cancel_btn", "Cancel");
    
    saveButton_->setClickCallback([this]() {
        if (!presetName_.empty() && saveCallback_) {
            saveCallback_(presetName_, presetCategory_, presetDescription_);
            hide();
        }
    });
    
    cancelButton_->setClickCallback([this]() {
        hide();
    });
}

void PresetSaveDialogUI::show() {
    visible_ = true;
    presetName_.clear();
    presetCategory_ = "User";
    presetDescription_.clear();
    focusedField_ = 0;
}

void PresetSaveDialogUI::hide() {
    visible_ = false;
}

void PresetSaveDialogUI::update(float deltaTime) {
    if (!visible_) return;
    
    saveButton_->update(deltaTime);
    cancelButton_->update(deltaTime);
}

void PresetSaveDialogUI::render(DisplayManager* display) {
    if (!visible_ || !display) return;
    
    // Center dialog on screen
    int dialogX = (display->getWidth() - width_) / 2;
    int dialogY = (display->getHeight() - height_) / 2;
    
    // Draw dialog background
    display->fillRect(dialogX, dialogY, width_, height_, Color(40, 40, 40));
    display->drawRect(dialogX, dialogY, width_, height_, Color(100, 100, 100));
    
    // Title
    display->drawText(dialogX + 20, dialogY + 20, "Save Preset", nullptr, Color(255, 255, 255));
    
    // Input fields
    int fieldY = dialogY + 60;
    drawInputField(display, "Name:", presetName_, dialogX + 20, fieldY, 300, focusedField_ == 0);
    
    fieldY += 50;
    drawInputField(display, "Category:", presetCategory_, dialogX + 20, fieldY, 300, focusedField_ == 1);
    
    fieldY += 50;
    drawInputField(display, "Description:", presetDescription_, dialogX + 20, fieldY, 300, focusedField_ == 2);
    
    // Buttons
    saveButton_->setPosition(dialogX + width_ - 180, dialogY + height_ - 50);
    saveButton_->setSize(70, 30);
    saveButton_->render(display);
    
    cancelButton_->setPosition(dialogX + width_ - 90, dialogY + height_ - 50);
    cancelButton_->setSize(70, 30);
    cancelButton_->render(display);
}

bool PresetSaveDialogUI::handleInput(const InputEvent& event) {
    if (!visible_) return false;
    
    if (saveButton_->handleInput(event)) return true;
    if (cancelButton_->handleInput(event)) return true;
    
    // Handle keyboard input for text fields
    if (event.type == InputEventType::ButtonPress) {
        std::string* currentField = nullptr;
        
        switch (focusedField_) {
            case 0: currentField = &presetName_; break;
            case 1: currentField = &presetCategory_; break;
            case 2: currentField = &presetDescription_; break;
        }
        
        if (currentField) {
            if (event.id == 8) { // Backspace
                if (!currentField->empty()) {
                    currentField->pop_back();
                }
                return true;
            } else if (event.id == 9) { // Tab
                focusedField_ = (focusedField_ + 1) % 3;
                return true;
            } else if (event.id >= 32 && event.id < 127) { // Printable
                *currentField += static_cast<char>(event.id);
                return true;
            }
        }
    }
    
    return true; // Modal dialog consumes all events
}

void PresetSaveDialogUI::drawInputField(DisplayManager* display, const std::string& label,
                                       const std::string& value, int x, int y, int width,
                                       bool focused) {
    // Label
    display->drawText(x, y, label, nullptr, Color(180, 180, 180));
    
    // Input box
    int boxY = y + 20;
    Color bgColor = focused ? Color(50, 50, 50) : Color(35, 35, 35);
    Color borderColor = focused ? Color(100, 150, 200) : Color(70, 70, 70);
    
    display->fillRect(x, boxY, width, 25, bgColor);
    display->drawRect(x, boxY, width, 25, borderColor);
    
    // Value text
    display->drawText(x + 5, boxY + 5, value, nullptr, Color(220, 220, 220));
    
    // Cursor if focused
    if (focused) {
        int cursorX = x + 5 + value.length() * 8;
        display->drawLine(cursorX, boxY + 5, cursorX, boxY + 20, Color(255, 255, 255));
    }
}

} // namespace AIMusicHardware