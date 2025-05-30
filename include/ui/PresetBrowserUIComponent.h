#pragma once

#include "UIComponents.h"
#include "presets/PresetManager.h"
#include "presets/PresetDatabase.h"
#include "ParameterBridge.h"
#include "parameters/ParameterManager.h"
#include <memory>
#include <vector>
#include <functional>

namespace AIMusicHardware {

/**
 * @brief Preset list item for displaying in browser
 */
class PresetListItem : public UIComponent {
public:
    PresetListItem(const std::string& id, const PresetInfo& preset, int index);
    
    void setSelected(bool selected) { isSelected_ = selected; }
    bool isSelected() const { return isSelected_; }
    
    const PresetInfo& getPreset() const { return preset_; }
    int getIndex() const { return index_; }
    
    void update(float deltaTime) override;
    void render(DisplayManager* display) override;
    bool handleInput(const InputEvent& event) override;
    
    // Callback when item is clicked
    using ClickCallback = std::function<void(const PresetInfo&)>;
    void setClickCallback(ClickCallback callback) { clickCallback_ = callback; }
    
private:
    PresetInfo preset_;
    int index_;
    bool isSelected_ = false;
    bool isHovered_ = false;
    ClickCallback clickCallback_;
    
public:
    // Visual parameters
    static constexpr int ITEM_HEIGHT = 30;
    static constexpr int PADDING = 5;
    
private:
};

/**
 * @brief Scrollable list of presets
 */
class PresetListView : public UIComponent {
public:
    PresetListView(const std::string& id);
    
    void setPresets(const std::vector<PresetInfo>& presets);
    void clearPresets();
    
    void selectPreset(int index);
    int getSelectedIndex() const { return selectedIndex_; }
    
    void update(float deltaTime) override;
    void render(DisplayManager* display) override;
    bool handleInput(const InputEvent& event) override;
    
    // Callback when selection changes
    using SelectionCallback = std::function<void(const PresetInfo&)>;
    void setSelectionCallback(SelectionCallback callback) { selectionCallback_ = callback; }
    
private:
    std::vector<std::unique_ptr<PresetListItem>> items_;
    int selectedIndex_ = -1;
    float scrollOffset_ = 0.0f;
    float maxScroll_ = 0.0f;
    SelectionCallback selectionCallback_;
    
    void updateScrollBounds();
    void ensureSelectedVisible();
};

/**
 * @brief Search box for filtering presets
 */
class PresetSearchBox : public UIComponent {
public:
    PresetSearchBox(const std::string& id);
    
    void setText(const std::string& text) { searchText_ = text; }
    const std::string& getText() const { return searchText_; }
    
    void setPlaceholder(const std::string& placeholder) { placeholder_ = placeholder; }
    
    void update(float deltaTime) override;
    void render(DisplayManager* display) override;
    bool handleInput(const InputEvent& event) override;
    
    // Callback when search text changes
    using TextChangeCallback = std::function<void(const std::string&)>;
    void setTextChangeCallback(TextChangeCallback callback) { textChangeCallback_ = callback; }
    
private:
    std::string searchText_;
    std::string placeholder_ = "Search presets...";
    bool hasFocus_ = false;
    float cursorBlink_ = 0.0f;
    TextChangeCallback textChangeCallback_;
};

/**
 * @brief Category filter buttons
 */
class PresetCategoryFilter : public UIComponent {
public:
    PresetCategoryFilter(const std::string& id);
    
    void setCategories(const std::vector<std::string>& categories);
    void selectCategory(const std::string& category);
    const std::string& getSelectedCategory() const { return selectedCategory_; }
    
    void update(float deltaTime) override;
    void render(DisplayManager* display) override;
    bool handleInput(const InputEvent& event) override;
    
    // Callback when category changes
    using CategoryCallback = std::function<void(const std::string&)>;
    void setCategoryCallback(CategoryCallback callback) { categoryCallback_ = callback; }
    
private:
    std::vector<std::unique_ptr<Button>> categoryButtons_;
    std::string selectedCategory_ = "All";
    CategoryCallback categoryCallback_;
};

/**
 * @brief Main preset browser UI component
 */
class PresetBrowserUI : public UIComponent {
public:
    PresetBrowserUI(const std::string& id);
    ~PresetBrowserUI();
    
    /**
     * @brief Initialize with preset manager and database
     */
    void initialize(PresetManager* presetManager, PresetDatabase* database);
    
    /**
     * @brief Set parameter manager for updating parameters on preset load
     */
    void setParameterManager(EnhancedParameterManager* paramManager) {
        parameterManager_ = paramManager;
    }
    
    /**
     * @brief Refresh the preset list
     */
    void refresh();
    
    /**
     * @brief Load the selected preset
     */
    void loadSelectedPreset();
    
    /**
     * @brief Save current state as new preset
     */
    void saveAsNewPreset(const std::string& name, const std::string& category);
    
    void update(float deltaTime) override;
    void render(DisplayManager* display) override;
    bool handleInput(const InputEvent& event) override;
    
    /**
     * @brief Callbacks for preset actions
     */
    using PresetLoadCallback = std::function<void(const PresetInfo&)>;
    using PresetSaveCallback = std::function<void(const std::string&, const std::string&)>;
    
    void setPresetLoadCallback(PresetLoadCallback callback) { loadCallback_ = callback; }
    void setPresetSaveCallback(PresetSaveCallback callback) { saveCallback_ = callback; }
    
private:
    // Sub-components
    std::unique_ptr<PresetSearchBox> searchBox_;
    std::unique_ptr<PresetCategoryFilter> categoryFilter_;
    std::unique_ptr<PresetListView> listView_;
    std::unique_ptr<Label> presetInfoLabel_;
    std::unique_ptr<Button> loadButton_;
    std::unique_ptr<Button> saveButton_;
    std::unique_ptr<Button> deleteButton_;
    
    // Data
    PresetManager* presetManager_ = nullptr;
    PresetDatabase* database_ = nullptr;
    EnhancedParameterManager* parameterManager_ = nullptr;
    
    std::vector<PresetInfo> filteredPresets_;
    PresetInfo selectedPreset_;
    
    // Callbacks
    PresetLoadCallback loadCallback_;
    PresetSaveCallback saveCallback_;
    
    // Internal methods
    void filterPresets();
    void onSearchTextChanged(const std::string& text);
    void onCategoryChanged(const std::string& category);
    void onPresetSelected(const PresetInfo& preset);
    void layoutComponents();
};

/**
 * @brief Preset save dialog
 */
class PresetSaveDialogUI : public UIComponent {
public:
    PresetSaveDialogUI(const std::string& id);
    
    void show();
    void hide();
    bool isVisible() const { return visible_; }
    
    void update(float deltaTime) override;
    void render(DisplayManager* display) override;
    bool handleInput(const InputEvent& event) override;
    
    // Callback when save is confirmed
    using SaveCallback = std::function<void(const std::string& name, const std::string& category, const std::string& description)>;
    void setSaveCallback(SaveCallback callback) { saveCallback_ = callback; }
    
private:
    bool visible_ = false;
    
    // Input fields
    std::string presetName_;
    std::string presetCategory_ = "User";
    std::string presetDescription_;
    int focusedField_ = 0; // 0=name, 1=category, 2=description
    
    // UI elements
    std::unique_ptr<Button> saveButton_;
    std::unique_ptr<Button> cancelButton_;
    
    SaveCallback saveCallback_;
    
    void drawInputField(DisplayManager* display, const std::string& label, 
                       const std::string& value, int x, int y, int width, 
                       bool focused);
};

} // namespace AIMusicHardware