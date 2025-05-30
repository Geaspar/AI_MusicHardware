#include <SDL2/SDL.h>
#include <iostream>
#include <memory>
#include <thread>
#include <chrono>
#include <iomanip>
#include <sstream>

#include "../include/ui/UIContext.h"
#include "../include/ui/SynthKnob.h"
#include "../include/ui/ParameterUpdateQueue.h"
#include "../include/ui/parameters/ParameterManager.h"
#include "../include/audio/Synthesizer.h"
#include "../include/audio/AudioEngine.h"

using namespace AIMusicHardware;

// Custom SDL DisplayManager (same as TestUI)
class SDLDisplayManager : public DisplayManager {
public:
    SDLDisplayManager(SDL_Renderer* renderer) : renderer_(renderer), width_(800), height_(600) {}
    
    bool initialize(int width, int height) override {
        width_ = width;
        height_ = height;
        return renderer_ != nullptr;
    }
    
    void shutdown() override {}
    
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
    
    void drawText(int x, int y, const std::string& text, Font* font, const Color& color) override {
        SDL_SetRenderDrawColor(renderer_, color.r, color.g, color.b, color.a);
        
        int charWidth = 8;
        int charHeight = 16;
        
        for (size_t i = 0; i < text.length(); i++) {
            int charX = x + i * charWidth;
            
            if (text[i] != ' ') {
                if (isalpha(text[i])) {
                    SDL_Rect charRect = { charX, y, charWidth - 1, charHeight };
                    SDL_RenderDrawRect(renderer_, &charRect);
                    
                    if (islower(text[i])) {
                        SDL_RenderDrawLine(renderer_, charX, y + charHeight/2, 
                                         charX + charWidth - 2, y + charHeight/2);
                    }
                } else if (isdigit(text[i])) {
                    SDL_Rect digitRect = { charX + 1, y + 2, charWidth - 3, charHeight - 4 };
                    SDL_RenderDrawRect(renderer_, &digitRect);
                } else {
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

// Helper to translate SDL events
InputEvent translateSDLEvent(const SDL_Event& sdlEvent) {
    InputEvent event;
    
    switch (sdlEvent.type) {
        case SDL_MOUSEBUTTONDOWN:
            event.type = InputEventType::TouchPress;
            event.id = 0;
            event.value = sdlEvent.button.x;
            event.value2 = sdlEvent.button.y;
            break;
            
        case SDL_MOUSEBUTTONUP:
            event.type = InputEventType::TouchRelease;
            event.id = 0;
            event.value = sdlEvent.button.x;
            event.value2 = sdlEvent.button.y;
            break;
            
        case SDL_MOUSEMOTION:
            if (sdlEvent.motion.state & SDL_BUTTON_LMASK) {
                event.type = InputEventType::TouchMove;
                event.id = 0;
                event.value = sdlEvent.motion.x;
                event.value2 = sdlEvent.motion.y;
            }
            break;
            
        case SDL_KEYDOWN:
            event.type = InputEventType::ButtonPress;
            event.id = sdlEvent.key.keysym.sym;
            break;
            
        case SDL_KEYUP:
            event.type = InputEventType::ButtonRelease;
            event.id = sdlEvent.key.keysym.sym;
            break;
    }
    
    return event;
}

// Audio thread simulation
void audioThreadSimulation(bool& running) {
    auto& updateSystem = ParameterUpdateSystem::getInstance();
    
    while (running) {
        // Process parameter updates from UI
        updateSystem.processAudioUpdates(
            [](const ParameterUpdateQueue<>::ParameterChange& change) {
                std::cout << "[Audio Thread] Parameter " << change.id 
                         << " = " << change.value << std::endl;
            }
        );
        
        // Simulate some audio processing time
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        
        // Occasionally send updates back to UI (e.g., from automation)
        static int counter = 0;
        if (++counter % 100 == 0) {
            float automationValue = 0.5f + 0.4f * std::sin(counter * 0.01f);
            updateSystem.pushToUI("filter_resonance", automationValue,
                                 ParameterUpdateQueue<>::ChangeSource::Automation);
        }
    }
}

int main(int argc, char* argv[]) {
    std::cout << "Enhanced UI Integration Test - Demonstrating parameter binding and thread-safe updates" << std::endl;
    
    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cerr << "SDL initialization failed: " << SDL_GetError() << std::endl;
        return 1;
    }
    
    // Create window
    SDL_Window* window = SDL_CreateWindow(
        "Enhanced UI Integration Test",
        SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
        1024, 768,
        SDL_WINDOW_SHOWN
    );
    
    if (!window) {
        std::cerr << "Window creation failed: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return 1;
    }
    
    // Create renderer
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        std::cerr << "Renderer creation failed: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }
    
    // Create UI context with custom display manager
    auto uiContext = std::make_unique<UIContext>();
    auto sdlDisplayManager = std::make_shared<SDLDisplayManager>(renderer);
    uiContext->setDisplayManager(sdlDisplayManager);
    uiContext->initialize(1024, 768);
    
    // Create main screen
    auto mainScreen = std::make_unique<Screen>("main");
    
    // Title
    auto titleLabel = std::make_unique<Label>("title_label", "Enhanced UI Integration Demo");
    titleLabel->setPosition(350, 20);
    mainScreen->addChild(std::move(titleLabel));
    
    // Create parameter manager and register some test parameters
    auto& paramManager = EnhancedParameterManager::getInstance();
    auto* rootGroup = paramManager.getRootGroup();
    
    // Create synthesizer parameter group
    auto synthGroup = std::make_unique<ParameterGroup>("synth", "Synthesizer");
    
    // Create filter parameters
    auto filterCutoff = std::make_unique<FloatParameter>(
        "filter_cutoff", "Filter Cutoff", 1000.0f);
    filterCutoff->setRange(20.0f, 20000.0f);
    
    auto filterResonance = std::make_unique<FloatParameter>(
        "filter_resonance", "Filter Resonance", 0.5f);
    filterResonance->setRange(0.0f, 1.0f);
    
    // Create oscillator parameters
    auto oscDetune = std::make_unique<FloatParameter>(
        "osc_detune", "Oscillator Detune", 0.0f);
    oscDetune->setRange(-50.0f, 50.0f);
    
    auto oscVolume = std::make_unique<FloatParameter>(
        "osc_volume", "Oscillator Volume", 0.75f);
    oscVolume->setRange(0.0f, 1.0f);
    
    // Register parameters
    paramManager.registerParameter(filterCutoff.get());
    paramManager.registerParameter(filterResonance.get());
    paramManager.registerParameter(oscDetune.get());
    paramManager.registerParameter(oscVolume.get());
    
    // Add parameters to group (transfers ownership)
    synthGroup->addParameter(std::move(filterCutoff));
    synthGroup->addParameter(std::move(filterResonance));
    synthGroup->addParameter(std::move(oscDetune));
    synthGroup->addParameter(std::move(oscVolume));
    
    rootGroup->addGroup(std::move(synthGroup));
    
    // Create enhanced knobs with parameter binding
    auto cutoffKnob = SynthKnobFactory::createFrequencyKnob("Cutoff", 100, 100);
    cutoffKnob->bindToParameter(paramManager.findParameter("filter_cutoff"),
                               ParameterBridge::ScaleType::Exponential);
    cutoffKnob->setModulationColor(Color(0, 255, 128)); // Green modulation
    
    auto resonanceKnob = SynthKnobFactory::createResonanceKnob("Resonance", 250, 100);
    resonanceKnob->bindToParameter(paramManager.findParameter("filter_resonance"),
                                  ParameterBridge::ScaleType::Quadratic);
    resonanceKnob->setModulationColor(Color(255, 128, 0)); // Orange modulation
    
    auto detuneKnob = std::make_unique<SynthKnob>("Detune", 400, 100, 80, -50.0f, 50.0f, 0.0f);
    detuneKnob->bindToParameter(paramManager.findParameter("osc_detune"));
    detuneKnob->setValueFormatter([](float value) {
        std::stringstream ss;
        ss << std::fixed << std::setprecision(1) << value << " cents";
        return ss.str();
    });
    
    auto volumeKnob = SynthKnobFactory::createVolumeKnob("Volume", 550, 100);
    volumeKnob->bindToParameter(paramManager.findParameter("osc_volume"),
                               ParameterBridge::ScaleType::Logarithmic);
    
    // Add info labels
    auto infoLabel1 = std::make_unique<Label>("info1", "Click and drag knobs to adjust parameters");
    infoLabel1->setPosition(300, 250);
    auto infoLabel2 = std::make_unique<Label>("info2", "Hold SHIFT for fine control");
    infoLabel2->setPosition(350, 280);
    auto infoLabel3 = std::make_unique<Label>("info3", "Double-click to reset to default");
    infoLabel3->setPosition(330, 310);
    auto infoLabel4 = std::make_unique<Label>("info4", "Watch for automation on Resonance knob");
    infoLabel4->setPosition(310, 340);
    
    // Add components to screen
    mainScreen->addChild(std::move(cutoffKnob));
    mainScreen->addChild(std::move(resonanceKnob));
    mainScreen->addChild(std::move(detuneKnob));
    mainScreen->addChild(std::move(volumeKnob));
    mainScreen->addChild(std::move(infoLabel1));
    mainScreen->addChild(std::move(infoLabel2));
    mainScreen->addChild(std::move(infoLabel3));
    mainScreen->addChild(std::move(infoLabel4));
    
    // Add screen to context
    uiContext->addScreen(std::move(mainScreen));
    uiContext->setActiveScreen("main");
    
    // Start simulated audio thread
    bool audioThreadRunning = true;
    std::thread audioThread(audioThreadSimulation, std::ref(audioThreadRunning));
    
    // Enable logging for debugging
    ParameterUpdateSystem::getInstance().setLoggingEnabled(true);
    
    // Main loop
    bool quit = false;
    auto lastTime = std::chrono::steady_clock::now();
    
    while (!quit) {
        // Calculate delta time
        auto currentTime = std::chrono::steady_clock::now();
        float deltaTime = std::chrono::duration<float>(currentTime - lastTime).count();
        lastTime = currentTime;
        
        // Process events
        SDL_Event sdlEvent;
        while (SDL_PollEvent(&sdlEvent)) {
            if (sdlEvent.type == SDL_QUIT) {
                quit = true;
            } else if (sdlEvent.type == SDL_KEYDOWN && sdlEvent.key.keysym.sym == SDLK_ESCAPE) {
                quit = true;
            } else {
                InputEvent inputEvent = translateSDLEvent(sdlEvent);
                uiContext->handleInput(inputEvent);
            }
        }
        
        // Process UI updates from audio thread
        ParameterUpdateSystem::getInstance().processUIUpdates(
            [&uiContext, &paramManager](const ParameterUpdateQueue<>::ParameterChange& change) {
                // Update parameter value
                if (auto* param = paramManager.findParameter(change.id)) {
                    if (auto* floatParam = dynamic_cast<FloatParameter*>(param)) {
                        floatParam->setValue(change.value);
                    }
                }
                
                // The parameter bridge will automatically update the UI control
            }
        );
        
        // Process parameter smoothing
        ParameterBridgeManager::getInstance().processAllSmoothing(deltaTime);
        
        // Update UI
        uiContext->update(deltaTime);
        
        // Render
        sdlDisplayManager->clear(Color(20, 20, 30)); // Dark blue background
        uiContext->render();
        
        // Cap frame rate
        SDL_Delay(16); // ~60 FPS
    }
    
    // Cleanup
    audioThreadRunning = false;
    audioThread.join();
    
    // Show statistics
    auto stats = ParameterUpdateSystem::getInstance().getStatistics();
    std::cout << "\nParameter Update Statistics:" << std::endl;
    std::cout << "  Total UI->Audio updates: " << stats.totalAudioUpdates << std::endl;
    std::cout << "  Total Audio->UI updates: " << stats.totalUIUpdates << std::endl;
    std::cout << "  Dropped updates: " << stats.droppedAudioUpdates + stats.droppedUIUpdates << std::endl;
    
    uiContext->shutdown();
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    
    return 0;
}