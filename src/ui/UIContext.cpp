#include "../../include/ui/UIContext.h"
#include <algorithm>
#include <stdexcept>

namespace AIMusicHardware {

//
// UIComponent implementation
//
UIComponent::UIComponent(const std::string& id)
    : id_(id), x_(0), y_(0), width_(0), height_(0), visible_(true), enabled_(true) {
}

UIComponent::~UIComponent() {
}

void UIComponent::addChild(std::unique_ptr<UIComponent> child) {
    if (child) {
        children_.push_back(std::move(child));
    }
}

UIComponent* UIComponent::getChild(const std::string& id) {
    for (auto& child : children_) {
        if (child->getId() == id) {
            return child.get();
        }
    }
    return nullptr;
}

void UIComponent::removeChild(const std::string& id) {
    auto it = std::remove_if(children_.begin(), children_.end(),
                           [&id](const std::unique_ptr<UIComponent>& child) {
                               return child->getId() == id;
                           });
    
    if (it != children_.end()) {
        children_.erase(it, children_.end());
    }
}

void UIComponent::renderChildren(DisplayManager* display) {
    for (auto& child : children_) {
        if (child->isVisible()) {
            child->render(display);
        }
    }
}

bool UIComponent::handleChildrenInput(const InputEvent& event) {
    // Process children in reverse order (top-most first)
    for (auto it = children_.rbegin(); it != children_.rend(); ++it) {
        auto& child = *it;
        if (child->isVisible() && child->isEnabled()) {
            if (child->handleInput(event)) {
                return true;
            }
        }
    }
    return false;
}

//
// Screen implementation
//
Screen::Screen(const std::string& id)
    : UIComponent(id), backgroundColor_(Color::Black()) {
}

Screen::~Screen() {
}

void Screen::update(float deltaTime) {
    // Update all children
    for (auto& child : children_) {
        if (child->isVisible()) {
            child->update(deltaTime);
        }
    }
}

void Screen::render(DisplayManager* display) {
    if (!display) {
        return;
    }
    
    // Draw background
    display->fillRect(x_, y_, width_, height_, backgroundColor_);
    
    // Render all children
    renderChildren(display);
}

bool Screen::handleInput(const InputEvent& event) {
    // Handle input for children
    return handleChildrenInput(event);
}

//
// UIContext implementation
//
UIContext::UIContext()
    : activeScreenId_(""), synth_(nullptr), effectProcessor_(nullptr),
      sequencer_(nullptr), hardware_(nullptr), adaptiveSequencer_(nullptr),
      llmInterface_(nullptr) {
}

UIContext::~UIContext() {
    shutdown();
}

bool UIContext::initialize(int width, int height) {
    // Create and initialize the display manager
    displayManager_ = std::make_unique<DisplayManager>();
    if (!displayManager_->initialize(width, height)) {
        return false;
    }
    
    // Set up default theme colors
    themeColors_["background"] = Color(40, 40, 40);
    themeColors_["foreground"] = Color(230, 230, 230);
    themeColors_["highlight"] = Color(255, 120, 0);
    themeColors_["accent"] = Color(0, 180, 255);
    themeColors_["warning"] = Color(255, 60, 60);
    themeColors_["success"] = Color(60, 200, 60);
    
    return true;
}

void UIContext::shutdown() {
    // Clean up resources
    screens_.clear();
    fonts_.clear();
    
    if (displayManager_) {
        displayManager_->shutdown();
        displayManager_.reset();
    }
}

void UIContext::addScreen(std::unique_ptr<Screen> screen) {
    if (screen) {
        std::string id = screen->getId();
        screens_[id] = std::move(screen);
        
        // If this is the first screen, make it active
        if (activeScreenId_.empty()) {
            setActiveScreen(id);
        }
    }
}

Screen* UIContext::getScreen(const std::string& id) {
    auto it = screens_.find(id);
    if (it != screens_.end()) {
        return it->second.get();
    }
    return nullptr;
}

void UIContext::setActiveScreen(const std::string& id) {
    // Deactivate current screen
    if (!activeScreenId_.empty()) {
        Screen* currentScreen = getScreen(activeScreenId_);
        if (currentScreen) {
            currentScreen->onDeactivate();
        }
    }
    
    // Activate new screen
    Screen* newScreen = getScreen(id);
    if (newScreen) {
        activeScreenId_ = id;
        newScreen->onActivate();
    }
}

void UIContext::update(float deltaTime) {
    // Update active screen
    Screen* activeScreen = getScreen(activeScreenId_);
    if (activeScreen) {
        activeScreen->update(deltaTime);
    }
}

void UIContext::render() {
    // Clear the display
    Screen* activeScreen = getScreen(activeScreenId_);
    if (activeScreen) {
        displayManager_->clear(activeScreen->getBackgroundColor());
        
        // Render the active screen
        activeScreen->render(displayManager_.get());
    } else {
        displayManager_->clear();
    }
    
    // Swap buffers to display the frame
    displayManager_->swapBuffers();
}

bool UIContext::handleInput(const InputEvent& event) {
    // Pass the input to the active screen
    Screen* activeScreen = getScreen(activeScreenId_);
    if (activeScreen) {
        return activeScreen->handleInput(event);
    }
    return false;
}

void UIContext::registerFont(const std::string& name, std::unique_ptr<Font> font) {
    if (font) {
        fonts_[name] = std::move(font);
    }
}

Font* UIContext::getFont(const std::string& name) {
    auto it = fonts_.find(name);
    if (it != fonts_.end()) {
        return it->second.get();
    }
    return nullptr;
}

void UIContext::setThemeColor(const std::string& name, const Color& color) {
    themeColors_[name] = color;
}

Color UIContext::getThemeColor(const std::string& name) const {
    auto it = themeColors_.find(name);
    if (it != themeColors_.end()) {
        return it->second;
    }
    return Color(255, 255, 255); // Default to white if theme color not found
}

} // namespace AIMusicHardware