#include <SDL2/SDL.h>
#include <iostream>
#include <cmath>
#include <memory>
#include "../include/ui/UIContext.h"
#include "../include/ui/UIComponents.h"
#include "../include/ui/UserInterface.h"

using namespace AIMusicHardware;

// Function to translate SDL events to InputEvent objects
InputEvent translateSDLEvent(const SDL_Event& sdlEvent) {
    InputEvent event;
    
    switch (sdlEvent.type) {
        case SDL_MOUSEBUTTONDOWN:
            event.type = InputEventType::TouchPress;
            event.id = 0; // Default touch ID
            event.value = sdlEvent.button.x;
            event.value2 = sdlEvent.button.y;
            break;
            
        case SDL_MOUSEBUTTONUP:
            event.type = InputEventType::TouchRelease;
            event.id = 0; // Default touch ID
            event.value = sdlEvent.button.x;
            event.value2 = sdlEvent.button.y;
            break;
            
        case SDL_MOUSEMOTION:
            if (sdlEvent.motion.state & SDL_BUTTON_LMASK) {
                event.type = InputEventType::TouchMove;
                event.id = 0; // Default touch ID
                event.value = sdlEvent.motion.x;
                event.value2 = sdlEvent.motion.y;
            }
            break;
            
        case SDL_KEYDOWN:
            event.type = InputEventType::ButtonPress;
            event.id = sdlEvent.key.keysym.sym; // Use SDL key code
            break;
            
        case SDL_KEYUP:
            event.type = InputEventType::ButtonRelease;
            event.id = sdlEvent.key.keysym.sym; // Use SDL key code
            break;
            
        default:
            // Event not handled - set invalid type
            break;
    }
    
    return event;
}

// Custom SDL-based DisplayManager implementation for testing
class SDLDisplayManager : public DisplayManager {
public:
    SDLDisplayManager(SDL_Renderer* renderer) : renderer_(renderer), width_(800), height_(600) {}
    
    bool initialize(int width, int height) override {
        width_ = width;
        height_ = height;
        return renderer_ != nullptr;
    }
    
    void shutdown() override {
        // SDL renderer is managed externally
    }
    
    void clear(const Color& color = Color(0, 0, 0)) override {
        SDL_SetRenderDrawColor(renderer_, color.r, color.g, color.b, color.a);
        SDL_RenderClear(renderer_);
    }
    
    void swapBuffers() override {
        SDL_RenderPresent(renderer_);
    }
    
    void drawLine(int x1, int y1, int x2, int y2, const Color& color) override {
        SDL_SetRenderDrawColor(renderer_, color.r, color.g, color.b, color.a);
        SDL_RenderDrawLine(renderer_, x1, y1, x2, y2);
    }
    
    void drawRect(int x, int y, int width, int height, const Color& color) override {
        SDL_Rect rect = { x, y, width, height };
        SDL_SetRenderDrawColor(renderer_, color.r, color.g, color.b, color.a);
        SDL_RenderDrawRect(renderer_, &rect);
    }
    
    void fillRect(int x, int y, int width, int height, const Color& color) override {
        SDL_Rect rect = { x, y, width, height };
        SDL_SetRenderDrawColor(renderer_, color.r, color.g, color.b, color.a);
        SDL_RenderFillRect(renderer_, &rect);
    }
    
    // Improved text rendering (in a real implementation, this would use SDL_ttf)
    void drawText(int x, int y, const std::string& text, Font* font, const Color& color) override {
        SDL_SetRenderDrawColor(renderer_, color.r, color.g, color.b, color.a);
        
        // Draw text background for better visibility
        int charWidth = 8;
        int charHeight = 16;
        int textWidth = static_cast<int>(text.length() * charWidth);
        SDL_Rect textRect = { x, y, textWidth, charHeight };
        
        // Draw character-by-character for better visualization
        for (size_t i = 0; i < text.length(); i++) {
            int charX = x + i * charWidth;
            
            // Create a simple character visualization
            if (text[i] != ' ') {
                // Different patterns for different characters
                if (isalpha(text[i])) {
                    // Letters get a more elaborate pattern
                    SDL_Rect charRect = { charX, y, charWidth - 1, charHeight };
                    SDL_RenderDrawRect(renderer_, &charRect);
                    
                    // Add a line in the middle for lowercase
                    if (islower(text[i])) {
                        SDL_RenderDrawLine(renderer_, charX, y + charHeight/2, 
                                         charX + charWidth - 2, y + charHeight/2);
                    }
                } else if (isdigit(text[i])) {
                    // Digits get a filled rectangle
                    SDL_Rect digitRect = { charX + 1, y + 2, charWidth - 3, charHeight - 4 };
                    SDL_RenderDrawRect(renderer_, &digitRect);
                } else {
                    // Symbols get a simple line
                    SDL_RenderDrawLine(renderer_, charX, y + charHeight/2, 
                                     charX + charWidth - 2, y + charHeight/2);
                }
            }
        }
    }
    
    int getWidth() const override { return width_; }
    int getHeight() const override { return height_; }
    
private:
    SDL_Renderer* renderer_;
    int width_;
    int height_;
};

int main(int argc, char* argv[]) {
    std::cout << "Starting AI Music Hardware UI Test with interactive support" << std::endl;
    
    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cerr << "SDL could not initialize! SDL_Error: " << SDL_GetError() << std::endl;
        return 1;
    }
    
    // Create window
    SDL_Window* window = SDL_CreateWindow(
        "AI Music Hardware UI Test",
        SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
        800, 600,
        SDL_WINDOW_SHOWN
    );
    
    if (!window) {
        std::cerr << "Window could not be created! SDL_Error: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return 1;
    }
    
    // Create renderer
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        std::cerr << "Renderer could not be created! SDL_Error: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }
    
    // Create custom SDL-based DisplayManager
    auto sdlDisplayManager = std::make_shared<SDLDisplayManager>(renderer);
    if (!sdlDisplayManager->initialize(800, 600)) {
        std::cerr << "Failed to initialize SDL DisplayManager" << std::endl;
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }
    
    // Create and initialize UI system
    UserInterface ui;
    if (!ui.initialize(800, 600)) {
        std::cerr << "Failed to initialize UI system" << std::endl;
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }
    
    // Replace the default DisplayManager with our SDL implementation
    // Note: This is a hack for testing - in a real application, we would modify
    // UserInterface and UIContext to accept a custom DisplayManager during initialization
    auto uiContext = reinterpret_cast<UIContext*>(ui.getUIContextForTesting());
    if (uiContext) {
        uiContext->setDisplayManager(sdlDisplayManager);
    }
    
    // Add some interactive components for testing
    auto mainScreen = ui.getScreen("main");
    if (mainScreen) {
        std::cout << "Adding interactive components to main screen" << std::endl;
        
        // Add title label
        auto titleLabel = ui.createLabel("main", "title_label", "AI Music Hardware UI Test",
                                        250, 50);
        
        // Add a test button with a callback
        auto testButton = ui.createButton("main", "test_button", "Click Me!",
                                           300, 400, 200, 40,
                                           []() {
                                               std::cout << "Button clicked!" << std::endl;
                                           });
        
        // Add multiple test knobs with callbacks for testing different controls
        
        // Filter cutoff knob
        auto filterCutoffKnob = ui.createKnob("main", "filter_cutoff", "Cutoff",
                                       250, 200, 80, 20.0f, 20000.0f, 1000.0f,
                                       [](float value) {
                                           std::cout << "Filter cutoff changed: " << value << " Hz" << std::endl;
                                       });
                                       
        // Filter resonance knob
        auto filterResKnob = ui.createKnob("main", "filter_resonance", "Resonance",
                                       350, 200, 80, 0.0f, 1.0f, 0.5f,
                                       [](float value) {
                                           std::cout << "Filter resonance changed: " << value << std::endl;
                                       });
                                       
        // Volume knob                               
        auto volumeKnob = ui.createKnob("main", "volume_knob", "Volume",
                                       450, 200, 80, 0.0f, 1.0f, 0.75f,
                                       [](float value) {
                                           std::cout << "Volume changed: " << value << std::endl;
                                       });
        
        // Standard value knob
        auto valueKnob = ui.createKnob("main", "test_knob", "Value",
                                       500, 350, 80, 0.0f, 1.0f, 0.5f,
                                       [](float value) {
                                           std::cout << "Knob value changed: " << value << std::endl;
                                       });
                                       
        // Add a description label
        auto infoLabel = ui.createLabel("main", "info_label", 
                                      "Click and drag knobs to change values. Press ESC to exit.",
                                      200, 500);
    }
    
    std::cout << "UI initialized. Starting main loop..." << std::endl;
    
    // Main loop
    bool quit = false;
    
    while (!quit) {
        // Process events
        SDL_Event sdlEvent;
        while (SDL_PollEvent(&sdlEvent)) {
            if (sdlEvent.type == SDL_QUIT) {
                quit = true;
            }
            else if (sdlEvent.type == SDL_KEYDOWN && sdlEvent.key.keysym.sym == SDLK_ESCAPE) {
                quit = true;
            }
            else {
                // Translate SDL event to InputEvent
                InputEvent inputEvent = translateSDLEvent(sdlEvent);
                
                // Pass the event to the UI system
                bool handled = ui.handleInput(inputEvent);
                if (handled) {
                    std::cout << "UI handled input event: type=" << static_cast<int>(inputEvent.type) 
                              << ", id=" << inputEvent.id 
                              << ", value=" << inputEvent.value 
                              << ", value2=" << inputEvent.value2 << std::endl;
                }
            }
        }
        
        // Update UI
        ui.update();
        
        // Clear the screen (handled by our SDL display manager through UI rendering)
        
        // Render UI (this will use our SDL display manager)
        ui.render();
        
        // Note: SDL_RenderPresent is called by our SDLDisplayManager::swapBuffers method
        
        // Cap frame rate
        SDL_Delay(16); // ~60fps
    }
    
    // Cleanup
    ui.shutdown();
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    
    return 0;
}