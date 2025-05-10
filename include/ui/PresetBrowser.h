#pragma once

#include "UIComponents.h"
#include "PresetManager.h"
#include <string>
#include <vector>
#include <functional>
#include <memory>

namespace AIMusicHardware {

// Forward declarations
class PresetListView;
class CategorySelector;
class SearchBox;
class SortSelector;
class PresetInfoPanel;

/**
 * @brief UI component for browsing, selecting, and managing presets
 */
class PresetBrowser : public UIComponent {
public:
    /**
     * @brief Constructor
     * 
     * @param id Component ID
     * @param presetManager Pointer to the preset manager
     */
    PresetBrowser(const std::string& id, PresetManager* presetManager);
    
    /**
     * @brief Destructor
     */
    virtual ~PresetBrowser();
    
    /**
     * @brief Set visibility of the browser
     * 
     * @param visible Whether the browser should be visible
     */
    void setVisible(bool visible) override;
    
    /**
     * @brief Update the browser
     * 
     * @param deltaTime Time since last update in seconds
     */
    void update(float deltaTime) override;
    
    /**
     * @brief Render the browser
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
     * @brief Refresh the preset list
     */
    void refreshPresetList();
    
    /**
     * @brief Set the current category filter
     * 
     * @param category Category name
     */
    void setCategory(const std::string& category);
    
    /**
     * @brief Set the search text filter
     * 
     * @param text Search text
     */
    void setSearchText(const std::string& text);
    
    /**
     * @brief Set the sort mode
     * 
     * @param mode Sort mode
     */
    void setSortMode(PresetSortMode mode);
    
    /**
     * @brief Get the currently selected preset
     * 
     * @return const PresetInfo& The selected preset
     */
    const PresetInfo& getSelectedPreset() const;
    
    /**
     * @brief Set callback for preset selection
     * 
     * @param callback Function to call when a preset is selected
     */
    using PresetSelectedCallback = std::function<void(const PresetInfo&)>;
    void setPresetSelectedCallback(PresetSelectedCallback callback);
    
    /**
     * @brief Set callback for browser close
     * 
     * @param callback Function to call when the browser is closed
     */
    using BrowserClosedCallback = std::function<void()>;
    void setBrowserClosedCallback(BrowserClosedCallback callback);
    
private:
    PresetManager* presetManager_;
    std::unique_ptr<PresetListView> listView_;
    std::unique_ptr<CategorySelector> categorySelector_;
    std::unique_ptr<SearchBox> searchBox_;
    std::unique_ptr<SortSelector> sortSelector_;
    std::unique_ptr<PresetInfoPanel> infoPanel_;
    std::unique_ptr<Button> loadButton_;
    std::unique_ptr<Button> cancelButton_;
    std::unique_ptr<Button> favoriteButton_;
    
    std::string currentCategory_;
    std::string searchText_;
    PresetSortMode sortMode_;
    std::vector<PresetInfo> filteredPresets_;
    PresetInfo selectedPreset_;
    
    PresetSelectedCallback presetSelectedCallback_;
    BrowserClosedCallback browserClosedCallback_;
    
    // Helper methods
    void createComponents();
    void layoutComponents();
    void filterPresets();
    void handlePresetSelection(const PresetInfo& preset);
    void handleCategoryChange(const std::string& category);
    void handleSearchTextChange(const std::string& text);
    void handleSortModeChange(PresetSortMode mode);
    void handleLoadButton();
    void handleCancelButton();
    void handleFavoriteButton();
};

/**
 * @brief UI component for displaying a list of presets
 */
class PresetListView : public UIComponent {
public:
    /**
     * @brief Constructor
     * 
     * @param id Component ID
     */
    PresetListView(const std::string& id);
    
    /**
     * @brief Destructor
     */
    virtual ~PresetListView();
    
    /**
     * @brief Update the list view
     * 
     * @param deltaTime Time since last update in seconds
     */
    void update(float deltaTime) override;
    
    /**
     * @brief Render the list view
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
     * @brief Set the list of presets to display
     * 
     * @param presets List of presets
     */
    void setPresets(const std::vector<PresetInfo>& presets);
    
    /**
     * @brief Set the selected preset index
     * 
     * @param index Index in the presets list
     */
    void setSelectedIndex(int index);
    
    /**
     * @brief Get the selected preset index
     * 
     * @return int The selected index
     */
    int getSelectedIndex() const;
    
    /**
     * @brief Get the selected preset
     * 
     * @return PresetInfo The selected preset
     */
    PresetInfo getSelectedPreset() const;
    
    /**
     * @brief Set callback for item selection
     * 
     * @param callback Function to call when an item is selected
     */
    using ItemSelectedCallback = std::function<void(const PresetInfo&)>;
    void setItemSelectedCallback(ItemSelectedCallback callback);
    
private:
    std::vector<PresetInfo> presets_;
    int selectedIndex_;
    int scrollOffset_;
    int visibleItems_;
    int itemHeight_;
    ItemSelectedCallback itemSelectedCallback_;
    
    // Helper methods
    void renderPresetItem(DisplayManager* display, const PresetInfo& preset, 
                        int index, bool selected);
    void ensureSelectedVisible();
    int getItemAt(int y) const;
};

/**
 * @brief UI component for selecting a preset category
 */
class CategorySelector : public UIComponent {
public:
    /**
     * @brief Constructor
     * 
     * @param id Component ID
     */
    CategorySelector(const std::string& id);
    
    /**
     * @brief Destructor
     */
    virtual ~CategorySelector();
    
    /**
     * @brief Update the category selector
     * 
     * @param deltaTime Time since last update in seconds
     */
    void update(float deltaTime) override;
    
    /**
     * @brief Render the category selector
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
     * @brief Set the list of available categories
     * 
     * @param categories List of category names
     */
    void setCategories(const std::vector<std::string>& categories);
    
    /**
     * @brief Set the selected category
     * 
     * @param category Category name
     */
    void setSelectedCategory(const std::string& category);
    
    /**
     * @brief Get the selected category
     * 
     * @return std::string The selected category
     */
    std::string getSelectedCategory() const;
    
    /**
     * @brief Set callback for category selection
     * 
     * @param callback Function to call when a category is selected
     */
    using CategorySelectedCallback = std::function<void(const std::string&)>;
    void setCategorySelectedCallback(CategorySelectedCallback callback);
    
private:
    std::vector<std::string> categories_;
    std::string selectedCategory_;
    CategorySelectedCallback categorySelectedCallback_;
    std::vector<std::unique_ptr<Button>> categoryButtons_;
    
    // Helper methods
    void createCategoryButtons();
    void handleCategoryButtonClicked(const std::string& category);
};

/**
 * @brief UI component for displaying detailed preset information
 */
class PresetInfoPanel : public UIComponent {
public:
    /**
     * @brief Constructor
     * 
     * @param id Component ID
     */
    PresetInfoPanel(const std::string& id);
    
    /**
     * @brief Destructor
     */
    virtual ~PresetInfoPanel();
    
    /**
     * @brief Update the info panel
     * 
     * @param deltaTime Time since last update in seconds
     */
    void update(float deltaTime) override;
    
    /**
     * @brief Render the info panel
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
     * @brief Set the preset to display information for
     * 
     * @param preset The preset
     */
    void setPreset(const PresetInfo& preset);
    
private:
    PresetInfo preset_;
    std::unique_ptr<Label> nameLabel_;
    std::unique_ptr<Label> authorLabel_;
    std::unique_ptr<Label> categoryLabel_;
    std::unique_ptr<Label> createdLabel_;
    std::unique_ptr<Label> commentsLabel_;
    
    // Helper methods
    void createComponents();
    void updateComponents();
};

/**
 * @brief UI component for text search input
 */
class SearchBox : public UIComponent {
public:
    /**
     * @brief Constructor
     * 
     * @param id Component ID
     */
    SearchBox(const std::string& id);
    
    /**
     * @brief Destructor
     */
    virtual ~SearchBox();
    
    /**
     * @brief Update the search box
     * 
     * @param deltaTime Time since last update in seconds
     */
    void update(float deltaTime) override;
    
    /**
     * @brief Render the search box
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
     * @brief Set the search text
     * 
     * @param text Search text
     */
    void setText(const std::string& text);
    
    /**
     * @brief Get the search text
     * 
     * @return std::string The search text
     */
    std::string getText() const;
    
    /**
     * @brief Set callback for text change
     * 
     * @param callback Function to call when the text changes
     */
    using TextChangedCallback = std::function<void(const std::string&)>;
    void setTextChangedCallback(TextChangedCallback callback);
    
private:
    std::string text_;
    bool focused_;
    TextChangedCallback textChangedCallback_;
    
    // Helper methods
    void handleTextInput(const std::string& text);
    void handleBackspace();
};

/**
 * @brief UI component for selecting preset sort mode
 */
class SortSelector : public UIComponent {
public:
    /**
     * @brief Constructor
     * 
     * @param id Component ID
     */
    SortSelector(const std::string& id);
    
    /**
     * @brief Destructor
     */
    virtual ~SortSelector();
    
    /**
     * @brief Update the sort selector
     * 
     * @param deltaTime Time since last update in seconds
     */
    void update(float deltaTime) override;
    
    /**
     * @brief Render the sort selector
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
     * @brief Set the sort mode
     * 
     * @param mode Sort mode
     */
    void setSortMode(PresetSortMode mode);
    
    /**
     * @brief Get the sort mode
     * 
     * @return PresetSortMode The sort mode
     */
    PresetSortMode getSortMode() const;
    
    /**
     * @brief Set callback for sort mode change
     * 
     * @param callback Function to call when the sort mode changes
     */
    using SortModeChangedCallback = std::function<void(PresetSortMode)>;
    void setSortModeChangedCallback(SortModeChangedCallback callback);
    
private:
    PresetSortMode sortMode_;
    bool dropdownOpen_;
    SortModeChangedCallback sortModeChangedCallback_;
    std::unique_ptr<Button> sortButton_;
    std::vector<std::unique_ptr<Button>> modeButtons_;
    
    // Helper methods
    void toggleDropdown();
    void createModeButtons();
    void handleModeButtonClicked(PresetSortMode mode);
    std::string getSortModeText(PresetSortMode mode) const;
};

} // namespace AIMusicHardware