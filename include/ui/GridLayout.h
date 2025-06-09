#pragma once

#include "UIContext.h"
#include <vector>
#include <memory>

namespace AIMusicHardware {

/**
 * @brief Simple grid-based layout system for UI components
 * 
 * This allows us to position components in a grid pattern without
 * manually calculating positions. Can be easily converted back to
 * absolute positioning for hardware implementation.
 */
class GridLayout : public UIComponent {
public:
    struct GridCell {
        int row;
        int col;
        int rowSpan;
        int colSpan;
        std::unique_ptr<UIComponent> component;
        int marginLeft = 5;
        int marginTop = 5;
        int marginRight = 5;
        int marginBottom = 5;
    };
    
    GridLayout(const std::string& id, int rows, int cols);
    virtual ~GridLayout() = default;
    
    /**
     * @brief Add a component to the grid
     * @param component The component to add (ownership transferred)
     * @param row Starting row (0-based)
     * @param col Starting column (0-based)
     * @param rowSpan Number of rows to span (default 1)
     * @param colSpan Number of columns to span (default 1)
     */
    void addComponent(std::unique_ptr<UIComponent> component, 
                     int row, int col, 
                     int rowSpan = 1, int colSpan = 1);
    
    /**
     * @brief Set margins for a specific cell
     */
    void setCellMargins(int row, int col, int left, int top, int right, int bottom);
    
    /**
     * @brief Set padding for the entire grid
     */
    void setPadding(int padding) { padding_ = padding; }
    
    /**
     * @brief Set spacing between grid cells
     */
    void setSpacing(int horizontal, int vertical) {
        horizontalSpacing_ = horizontal;
        verticalSpacing_ = vertical;
    }
    
    // UIComponent overrides
    void update(float deltaTime) override;
    void render(DisplayManager* display) override;
    bool handleInput(const InputEvent& event) override;
    
    // Override setSize to trigger relayout
    void setSize(int width, int height) {
        UIComponent::setSize(width, height);
        updateLayout();
    }
    
    // Override setPosition to trigger relayout
    void setPosition(int x, int y) {
        UIComponent::setPosition(x, y);
        updateLayout();
    }
    
    /**
     * @brief Get absolute position for hardware implementation
     * @return Vector of component positions for fixed layout export
     */
    std::vector<std::pair<UIComponent*, Rect>> getAbsolutePositions() const;
    
private:
    int rows_;
    int cols_;
    int padding_ = 10;
    int horizontalSpacing_ = 10;
    int verticalSpacing_ = 10;
    
    std::vector<GridCell> cells_;
    
    void updateLayout();
    Rect calculateCellBounds(int row, int col, int rowSpan, int colSpan) const;
};

/**
 * @brief Helper class to create common grid layouts
 */
class GridLayoutBuilder {
public:
    static std::unique_ptr<GridLayout> createSynthesizerLayout();
    static std::unique_ptr<GridLayout> createMixerLayout();
    static std::unique_ptr<GridLayout> createEffectsLayout();
};

} // namespace AIMusicHardware