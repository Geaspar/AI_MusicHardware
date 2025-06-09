#include <iostream>
#include <memory>
#include <thread>
#include <chrono>
#include <SDL2/SDL.h>

// Core systems
#include "../include/audio/AudioEngine.h"
#include "../include/audio/Synthesizer.h"
#include "../include/ui/SynthKnob.h"
#include "../include/ui/DisplayManager.h"

using namespace AIMusicHardware;

// Simple SDL DisplayManager implementation
class SimpleDisplayManager : public DisplayManager {
public:
    SimpleDisplayManager(SDL_Renderer* renderer) : renderer_(renderer), width_(1200), height_(800) {}
    
    bool initialize(int width, int height) override {
        width_ = width;
        height_ = height;
        return renderer_ != nullptr;
    }
    
    void shutdown() override {
        renderer_ = nullptr;
    }
    
    void clear(const Color& color) override {
        if (!renderer_) return;
        SDL_SetRenderDrawColor(renderer_, color.r, color.g, color.b, color.a);
        SDL_RenderClear(renderer_);
    }
    
    void present() {
        if (!renderer_) return;
        SDL_RenderPresent(renderer_);
    }
    
    void drawRect(int x, int y, int width, int height, const Color& color) override {
        if (!renderer_) return;
        SDL_SetRenderDrawColor(renderer_, color.r, color.g, color.b, color.a);
        SDL_Rect rect = {x, y, width, height};
        SDL_RenderDrawRect(renderer_, &rect);
    }
    
    void fillRect(int x, int y, int width, int height, const Color& color) override {
        if (!renderer_) return;
        SDL_SetRenderDrawColor(renderer_, color.r, color.g, color.b, color.a);
        SDL_Rect rect = {x, y, width, height};
        SDL_RenderFillRect(renderer_, &rect);
    }
    
    void drawLine(int x1, int y1, int x2, int y2, const Color& color) override {
        if (!renderer_) return;
        SDL_SetRenderDrawColor(renderer_, color.r, color.g, color.b, color.a);
        SDL_RenderDrawLine(renderer_, x1, y1, x2, y2);
    }
    
    void drawCircle(int centerX, int centerY, int radius, const Color& color) override {
        if (!renderer_) return;
        SDL_SetRenderDrawColor(renderer_, color.r, color.g, color.b, color.a);
        
        for (int w = 0; w < radius * 2; w++) {
            for (int h = 0; h < radius * 2; h++) {
                int dx = radius - w;
                int dy = radius - h;
                if ((dx*dx + dy*dy) <= (radius * radius)) {
                    SDL_RenderDrawPoint(renderer_, centerX + dx, centerY + dy);
                }
            }
        }
    }
    
    void fillCircle(int centerX, int centerY, int radius, const Color& color) override {
        drawCircle(centerX, centerY, radius, color);
    }
    
    void drawText(int x, int y, const std::string& text, Font* font, const Color& color) override {
        if (!renderer_) return;
        
        // Simple text rendering as colored blocks
        SDL_SetRenderDrawColor(renderer_, color.r, color.g, color.b, color.a);
        
        for (size_t i = 0; i < text.length() && i < 30; ++i) {
            // Each character as a small rectangle
            SDL_Rect charRect = {x + static_cast<int>(i * 8), y, 6, 12};
            SDL_RenderFillRect(renderer_, &charRect);
        }
    }
    
    int getWidth() const override { return width_; }
    int getHeight() const override { return height_; }

private:
    SDL_Renderer* renderer_;
    int width_;
    int height_;
};

// Simple knob with built-in label
class LabeledKnob {
public:
    LabeledKnob(const std::string& name, const std::string& label, int x, int y, 
                float min = 0.0f, float max = 1.0f, float def = 0.5f)
        : knob_(std::make_unique<SynthKnob>(name, x, y, 80, min, max, def))
        , label_(label)
        , x_(x), y_(y) {
    }
    
    void render(DisplayManager* display) {
        // Render knob
        knob_->render(display);
        
        // Draw section background
        display->fillRect(x_ - 10, y_ - 30, 100, 130, Color(50, 50, 60, 180));
        display->drawRect(x_ - 10, y_ - 30, 100, 130, Color(80, 80, 90));
        
        // Draw label above knob
        display->drawText(x_ - 5, y_ - 25, label_, nullptr, Color(200, 200, 220));
        
        // Draw value below knob
        std::string valueStr = std::to_string(static_cast<int>(knob_->getValue() * 100)) + "%";
        display->drawText(x_ + 10, y_ + 85, valueStr, nullptr, Color(150, 200, 150));
    }
    
    void update(float deltaTime) {
        knob_->update(deltaTime);
    }
    
    bool handleInput(const InputEvent& event) {
        return knob_->handleInput(event);
    }
    
    SynthKnob* getKnob() { return knob_.get(); }

private:
    std::unique_ptr<SynthKnob> knob_;
    std::string label_;
    int x_, y_;
};

// Section header
void drawSection(DisplayManager* display, int x, int y, int width, const std::string& title) {
    // Section background
    display->fillRect(x, y, width, 200, Color(40, 40, 50, 200));
    display->drawRect(x, y, width, 200, Color(70, 70, 80));
    
    // Section title
    display->fillRect(x + 5, y + 5, width - 10, 25, Color(60, 60, 70));
    display->drawText(x + 15, y + 10, title, nullptr, Color(220, 220, 255));
}

int main() {
    std::cout << "Starting Simple UI Demo with Parameter Automation..." << std::endl;
    
    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) {
        std::cerr << "Failed to initialize SDL: " << SDL_GetError() << std::endl;
        return 1;
    }
    
    // Create window
    SDL_Window* window = SDL_CreateWindow(
        "AIMusicHardware - Parameter Automation Demo",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        1200, 800,
        SDL_WINDOW_SHOWN
    );
    
    if (!window) {
        std::cerr << "Failed to create window: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return 1;
    }
    
    // Create renderer
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        std::cerr << "Failed to create renderer: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }
    
    // Create display manager
    auto displayManager = std::make_unique<SimpleDisplayManager>(renderer);
    displayManager->initialize(1200, 800);
    
    // Initialize synthesizer
    auto synthesizer = std::make_unique<Synthesizer>();
    synthesizer->initialize();
    
    // Create organized knobs with proper spacing and labels
    std::vector<std::unique_ptr<LabeledKnob>> knobs;
    
    // Oscillator section
    knobs.push_back(std::make_unique<LabeledKnob>("freq", "FREQUENCY", 80, 80, 20.0f, 20000.0f, 440.0f));
    knobs.push_back(std::make_unique<LabeledKnob>("wave", "WAVEFORM", 200, 80, 0.0f, 4.0f, 0.0f));
    knobs.push_back(std::make_unique<LabeledKnob>("detune", "DETUNE", 320, 80, -50.0f, 50.0f, 0.0f));
    
    // Filter section  
    knobs.push_back(std::make_unique<LabeledKnob>("cutoff", "CUTOFF", 480, 80, 20.0f, 20000.0f, 1000.0f));
    knobs.push_back(std::make_unique<LabeledKnob>("res", "RESONANCE", 600, 80, 0.0f, 1.0f, 0.1f));
    
    // Envelope section
    knobs.push_back(std::make_unique<LabeledKnob>("attack", "ATTACK", 760, 80, 0.001f, 3.0f, 0.01f));
    knobs.push_back(std::make_unique<LabeledKnob>("decay", "DECAY", 880, 80, 0.001f, 3.0f, 0.1f));
    knobs.push_back(std::make_unique<LabeledKnob>("sustain", "SUSTAIN", 760, 220, 0.0f, 1.0f, 0.7f));
    knobs.push_back(std::make_unique<LabeledKnob>("release", "RELEASE", 880, 220, 0.001f, 5.0f, 0.3f));
    
    // Master section
    knobs.push_back(std::make_unique<LabeledKnob>("volume", "VOLUME", 1050, 80, 0.0f, 1.0f, 0.7f));
    
    // Set up parameter automation - demonstrate smooth parameter changes
    auto lastAutomationTime = std::chrono::steady_clock::now();
    int automationIndex = 0;
    
    std::cout << "UI Demo ready!" << std::endl;
    std::cout << "Controls:" << std::endl;
    std::cout << "- Drag knobs with mouse" << std::endl;
    std::cout << "- Watch automatic parameter automation demo" << std::endl;
    std::cout << "- Press ESC to exit" << std::endl;
    
    // Main loop
    bool running = true;
    SDL_Event event;
    auto lastTime = std::chrono::high_resolution_clock::now();
    
    while (running) {
        auto currentTime = std::chrono::high_resolution_clock::now();
        float deltaTime = std::chrono::duration<float>(currentTime - lastTime).count();
        lastTime = currentTime;
        
        // Handle events
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT || 
                (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE)) {
                running = false;
            }
            
            // Convert SDL events to UI events and handle knob input
            InputEvent uiEvent;
            switch (event.type) {
                case SDL_MOUSEBUTTONDOWN:
                    uiEvent.type = InputEventType::TouchPress;
                    uiEvent.value = event.button.x;
                    uiEvent.value2 = event.button.y;
                    for (auto& knob : knobs) {
                        knob->handleInput(uiEvent);
                    }
                    break;
                    
                case SDL_MOUSEBUTTONUP:
                    uiEvent.type = InputEventType::TouchRelease;
                    uiEvent.value = event.button.x;
                    uiEvent.value2 = event.button.y;
                    for (auto& knob : knobs) {
                        knob->handleInput(uiEvent);
                    }
                    break;
                    
                case SDL_MOUSEMOTION:
                    if (event.motion.state & SDL_BUTTON_LMASK) {
                        uiEvent.type = InputEventType::TouchMove;
                        uiEvent.value = event.motion.x;
                        uiEvent.value2 = event.motion.y;
                        for (auto& knob : knobs) {
                            knob->handleInput(uiEvent);
                        }
                    }
                    break;
            }
        }
        
        // Automatic parameter automation demo every 3 seconds
        auto now = std::chrono::steady_clock::now();
        if (std::chrono::duration_cast<std::chrono::seconds>(now - lastAutomationTime).count() >= 3) {
            if (automationIndex < knobs.size()) {
                // Smoothly animate to random value using setValueFromAutomation
                float randomValue = static_cast<float>(rand()) / RAND_MAX;
                knobs[automationIndex]->getKnob()->setValueFromAutomation(randomValue);
                
                std::cout << "Automating " << knobs[automationIndex]->getKnob()->getId() 
                          << " to " << randomValue << std::endl;
                
                automationIndex = (automationIndex + 1) % knobs.size();
            }
            lastAutomationTime = now;
        }
        
        // Update knobs
        for (auto& knob : knobs) {
            knob->update(deltaTime);
        }
        
        // Render
        displayManager->clear(Color(30, 30, 35));
        
        // Draw title
        displayManager->fillRect(10, 10, 1180, 40, Color(50, 50, 70));
        displayManager->drawText(20, 20, "AIMusicHardware - Parameter Automation Demo", nullptr, Color(220, 220, 255));
        
        // Draw sections
        drawSection(displayManager.get(), 50, 60, 290, "OSCILLATOR");
        drawSection(displayManager.get(), 450, 60, 170, "FILTER");
        drawSection(displayManager.get(), 730, 60, 180, "ENVELOPE");
        drawSection(displayManager.get(), 1020, 60, 120, "MASTER");
        
        // Render all knobs
        for (auto& knob : knobs) {
            knob->render(displayManager.get());
        }
        
        // Draw instructions
        displayManager->drawText(50, 720, "Mouse: Drag knobs to control parameters", nullptr, Color(150, 150, 180));
        displayManager->drawText(50, 740, "Auto: Watch parameters smoothly animate every 3 seconds", nullptr, Color(150, 150, 180));
        displayManager->drawText(50, 760, "Features: Green rings show automation, smooth transitions", nullptr, Color(150, 150, 180));
        
        displayManager->present();
        
        // Small delay
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }
    
    // Cleanup
    std::cout << "Shutting down..." << std::endl;
    
    knobs.clear();
    displayManager.reset();
    synthesizer.reset();
    
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    
    std::cout << "Demo completed successfully." << std::endl;
    return 0;
}