#include "../../include/ui/GridLayout.h"
#include <algorithm>
#include <iostream>

namespace AIMusicHardware {

GridLayout::GridLayout(const std::string& id, int rows, int cols)
    : UIComponent(id)
    , rows_(rows)
    , cols_(cols) {
}

void GridLayout::addComponent(std::unique_ptr<UIComponent> component, 
                             int row, int col, 
                             int rowSpan, int colSpan) {
    if (!component) return;
    
    // Validate bounds
    if (row < 0 || row >= rows_ || col < 0 || col >= cols_) {
        std::cerr << "GridLayout: Invalid cell position (" << row << ", " << col << ")" << std::endl;
        return;
    }
    
    if (row + rowSpan > rows_ || col + colSpan > cols_) {
        std::cerr << "GridLayout: Component spans outside grid bounds" << std::endl;
        return;
    }
    
    GridCell cell;
    cell.row = row;
    cell.col = col;
    cell.rowSpan = rowSpan;
    cell.colSpan = colSpan;
    cell.component = std::move(component);
    
    cells_.push_back(std::move(cell));
    
    // Update layout immediately
    updateLayout();
}

void GridLayout::setCellMargins(int row, int col, int left, int top, int right, int bottom) {
    for (auto& cell : cells_) {
        if (cell.row == row && cell.col == col) {
            cell.marginLeft = left;
            cell.marginTop = top;
            cell.marginRight = right;
            cell.marginBottom = bottom;
            updateLayout();
            break;
        }
    }
}

void GridLayout::updateLayout() {
    if (cells_.empty()) return;
    
    // Calculate cell dimensions
    int availableWidth = width_ - (2 * padding_) - ((cols_ - 1) * horizontalSpacing_);
    int availableHeight = height_ - (2 * padding_) - ((rows_ - 1) * verticalSpacing_);
    
    int cellWidth = availableWidth / cols_;
    int cellHeight = availableHeight / rows_;
    
    // Position each component
    for (auto& cell : cells_) {
        if (!cell.component) continue;
        
        // Calculate cell bounds
        Rect bounds = calculateCellBounds(cell.row, cell.col, cell.rowSpan, cell.colSpan);
        
        // Apply margins
        int compX = bounds.x + cell.marginLeft;
        int compY = bounds.y + cell.marginTop;
        int compWidth = bounds.width - cell.marginLeft - cell.marginRight;
        int compHeight = bounds.height - cell.marginTop - cell.marginBottom;
        
        // Set component position and size
        cell.component->setPosition(compX, compY);
        cell.component->setSize(compWidth, compHeight);
    }
}

Rect GridLayout::calculateCellBounds(int row, int col, int rowSpan, int colSpan) const {
    // Calculate cell dimensions
    int availableWidth = width_ - (2 * padding_) - ((cols_ - 1) * horizontalSpacing_);
    int availableHeight = height_ - (2 * padding_) - ((rows_ - 1) * verticalSpacing_);
    
    int cellWidth = availableWidth / cols_;
    int cellHeight = availableHeight / rows_;
    
    // Calculate position
    int cellX = x_ + padding_ + (col * (cellWidth + horizontalSpacing_));
    int cellY = y_ + padding_ + (row * (cellHeight + verticalSpacing_));
    
    // Calculate size with span
    int spanWidth = (cellWidth * colSpan) + (horizontalSpacing_ * (colSpan - 1));
    int spanHeight = (cellHeight * rowSpan) + (verticalSpacing_ * (rowSpan - 1));
    
    return Rect(cellX, cellY, spanWidth, spanHeight);
}

void GridLayout::update(float deltaTime) {
    // Update all child components
    for (auto& cell : cells_) {
        if (cell.component && cell.component->isVisible()) {
            cell.component->update(deltaTime);
        }
    }
}

void GridLayout::render(DisplayManager* display) {
    if (!display) return;
    
    // Optional: Draw grid lines for debugging
    #ifdef DEBUG_GRID_LAYOUT
    // Draw grid background
    display->fillRect(x_, y_, width_, height_, Color(30, 30, 30));
    
    // Draw grid lines
    for (int row = 0; row <= rows_; ++row) {
        int y = y_ + padding_ + row * (height_ - 2 * padding_) / rows_;
        display->drawLine(x_ + padding_, y, x_ + width_ - padding_, y, Color(50, 50, 50));
    }
    
    for (int col = 0; col <= cols_; ++col) {
        int x = x_ + padding_ + col * (width_ - 2 * padding_) / cols_;
        display->drawLine(x, y_ + padding_, x, y_ + height_ - padding_, Color(50, 50, 50));
    }
    #endif
    
    // Render all components
    for (auto& cell : cells_) {
        if (cell.component && cell.component->isVisible()) {
            cell.component->render(display);
        }
    }
}

bool GridLayout::handleInput(const InputEvent& event) {
    // Forward input to child components
    for (auto& cell : cells_) {
        if (cell.component && cell.component->isVisible() && cell.component->isEnabled()) {
            if (cell.component->handleInput(event)) {
                return true;
            }
        }
    }
    return false;
}

std::vector<std::pair<UIComponent*, Rect>> GridLayout::getAbsolutePositions() const {
    std::vector<std::pair<UIComponent*, Rect>> positions;
    
    for (const auto& cell : cells_) {
        if (cell.component) {
            Rect bounds = cell.component->getBounds();
            positions.push_back({cell.component.get(), bounds});
        }
    }
    
    return positions;
}

// GridLayoutBuilder implementations
std::unique_ptr<GridLayout> GridLayoutBuilder::createSynthesizerLayout() {
    // Create a 6x8 grid for synthesizer UI
    auto layout = std::make_unique<GridLayout>("synth_layout", 6, 8);
    
    layout->setPadding(20);
    layout->setSpacing(15, 15);
    
    return layout;
}

std::unique_ptr<GridLayout> GridLayoutBuilder::createMixerLayout() {
    // Create a 4x8 grid for mixer UI
    auto layout = std::make_unique<GridLayout>("mixer_layout", 4, 8);
    
    layout->setPadding(10);
    layout->setSpacing(5, 10);
    
    return layout;
}

std::unique_ptr<GridLayout> GridLayoutBuilder::createEffectsLayout() {
    // Create a 3x6 grid for effects UI
    auto layout = std::make_unique<GridLayout>("effects_layout", 3, 6);
    
    layout->setPadding(15);
    layout->setSpacing(10, 10);
    
    return layout;
}

} // namespace AIMusicHardware