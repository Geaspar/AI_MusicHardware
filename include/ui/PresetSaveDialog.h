#pragma once

#include "UIComponents.h"
#include "PresetManager.h"
#include <string>
#include <functional>
#include <memory>

namespace AIMusicHardware {

// Forward declarations
class TextInput;
class TextArea;
class CategoryDropdown;

/**
 * @brief UI dialog for saving presets with metadata
 */
class PresetSaveDialog : public UIComponent {
public:
    /**
     * @brief Constructor
     * 
     * @param id Component ID
     * @param presetManager Pointer to the preset manager
     */
    PresetSaveDialog(const std::string& id, PresetManager* presetManager);
    
    /**
     * @brief Destructor
     */
    virtual ~PresetSaveDialog();
    
    /**
     * @brief Update the dialog
     * 
     * @param deltaTime Time since last update in seconds
     */
    void update(float deltaTime) override;
    
    /**
     * @brief Render the dialog
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
     * @brief Show the dialog
     * 
     * @param initialName Initial preset name
     * @param initialCategory Initial category
     */
    void show(const std::string& initialName = "", const std::string& initialCategory = "");
    
    /**
     * @brief Hide the dialog
     */
    void hide();
    
    /**
     * @brief Check if the dialog is visible
     * 
     * @return true if visible
     */
    bool isVisible() const;
    
    /**
     * @brief Set callback for save completion
     * 
     * @param callback Function to call when save is completed
     */
    using SaveCompletedCallback = std::function<void(bool success)>;
    void setSaveCompletedCallback(SaveCompletedCallback callback);
    
private:
    PresetManager* presetManager_;
    std::unique_ptr<TextInput> nameInput_;
    std::unique_ptr<CategoryDropdown> categorySelector_;
    std::unique_ptr<TextInput> authorInput_;
    std::unique_ptr<TextArea> commentsInput_;
    std::unique_ptr<Button> saveButton_;
    std::unique_ptr<Button> cancelButton_;
    
    std::string presetName_;
    std::string category_;
    std::string author_;
    std::string comments_;
    bool visible_;
    
    SaveCompletedCallback saveCompletedCallback_;
    
    // Helper methods
    void createComponents();
    void layoutComponents();
    void handleSave();
    void handleCancel();
    bool validateInputs();
    
    // Component callbacks
    void handleNameChanged(const std::string& text);
    void handleCategoryChanged(const std::string& category);
    void handleAuthorChanged(const std::string& text);
    void handleCommentsChanged(const std::string& text);
};

/**
 * @brief UI component for text input
 */
class TextInput : public UIComponent {
public:
    /**
     * @brief Constructor
     * 
     * @param id Component ID
     * @param label Optional label
     */
    TextInput(const std::string& id, const std::string& label = "");
    
    /**
     * @brief Destructor
     */
    virtual ~TextInput();
    
    /**
     * @brief Update the text input
     * 
     * @param deltaTime Time since last update in seconds
     */
    void update(float deltaTime) override;
    
    /**
     * @brief Render the text input
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
     * @brief Set the input text
     * 
     * @param text Text value
     */
    void setText(const std::string& text);
    
    /**
     * @brief Get the input text
     * 
     * @return std::string Text value
     */
    std::string getText() const;
    
    /**
     * @brief Set the input label
     * 
     * @param label Label text
     */
    void setLabel(const std::string& label);
    
    /**
     * @brief Get the input label
     * 
     * @return std::string Label text
     */
    std::string getLabel() const;
    
    /**
     * @brief Set the input focus
     * 
     * @param focused Whether the input should have focus
     */
    void setFocused(bool focused);
    
    /**
     * @brief Check if the input has focus
     * 
     * @return true if focused
     */
    bool isFocused() const;
    
    /**
     * @brief Set callback for text change
     * 
     * @param callback Function to call when the text changes
     */
    using TextChangedCallback = std::function<void(const std::string&)>;
    void setTextChangedCallback(TextChangedCallback callback);
    
private:
    std::string text_;
    std::string label_;
    int cursorPosition_;
    bool focused_;
    int blinkTimer_;
    bool showCursor_;
    TextChangedCallback textChangedCallback_;
    
    // Helper methods
    void handleTextInput(const std::string& text);
    void handleBackspace();
    void handleDelete();
    void handleArrowKeys(int direction);
    void drawCursor(DisplayManager* display, int x, int y);
};

/**
 * @brief UI component for multi-line text input
 */
class TextArea : public UIComponent {
public:
    /**
     * @brief Constructor
     * 
     * @param id Component ID
     * @param label Optional label
     */
    TextArea(const std::string& id, const std::string& label = "");
    
    /**
     * @brief Destructor
     */
    virtual ~TextArea();
    
    /**
     * @brief Update the text area
     * 
     * @param deltaTime Time since last update in seconds
     */
    void update(float deltaTime) override;
    
    /**
     * @brief Render the text area
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
     * @brief Set the text
     * 
     * @param text Text value
     */
    void setText(const std::string& text);
    
    /**
     * @brief Get the text
     * 
     * @return std::string Text value
     */
    std::string getText() const;
    
    /**
     * @brief Set the label
     * 
     * @param label Label text
     */
    void setLabel(const std::string& label);
    
    /**
     * @brief Get the label
     * 
     * @return std::string Label text
     */
    std::string getLabel() const;
    
    /**
     * @brief Set the input focus
     * 
     * @param focused Whether the text area should have focus
     */
    void setFocused(bool focused);
    
    /**
     * @brief Check if the text area has focus
     * 
     * @return true if focused
     */
    bool isFocused() const;
    
    /**
     * @brief Set callback for text change
     * 
     * @param callback Function to call when the text changes
     */
    using TextChangedCallback = std::function<void(const std::string&)>;
    void setTextChangedCallback(TextChangedCallback callback);
    
private:
    std::vector<std::string> lines_;
    std::string label_;
    int cursorLine_;
    int cursorColumn_;
    int scrollOffset_;
    bool focused_;
    int blinkTimer_;
    bool showCursor_;
    TextChangedCallback textChangedCallback_;
    
    // Helper methods
    void handleTextInput(const std::string& text);
    void handleBackspace();
    void handleDelete();
    void handleArrowKeys(int direction);
    void handleNewLine();
    void drawCursor(DisplayManager* display, int x, int y);
    void ensureCursorVisible();
    std::string getAllText() const;
};

/**
 * @brief UI component for selecting a category from a dropdown
 */
class CategoryDropdown : public UIComponent {
public:
    /**
     * @brief Constructor
     * 
     * @param id Component ID
     * @param label Optional label
     */
    CategoryDropdown(const std::string& id, const std::string& label = "");
    
    /**
     * @brief Destructor
     */
    virtual ~CategoryDropdown();
    
    /**
     * @brief Update the dropdown
     * 
     * @param deltaTime Time since last update in seconds
     */
    void update(float deltaTime) override;
    
    /**
     * @brief Render the dropdown
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
     * @brief Set the available categories
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
     * @return std::string Category name
     */
    std::string getSelectedCategory() const;
    
    /**
     * @brief Set the label
     * 
     * @param label Label text
     */
    void setLabel(const std::string& label);
    
    /**
     * @brief Get the label
     * 
     * @return std::string Label text
     */
    std::string getLabel() const;
    
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
    std::string label_;
    bool dropdownOpen_;
    int scrollOffset_;
    CategorySelectedCallback categorySelectedCallback_;
    
    // Helper methods
    void toggleDropdown();
    void selectCategory(const std::string& category);
    void drawDropdownList(DisplayManager* display);
    int getCategoryAt(int y) const;
};

} // namespace AIMusicHardware