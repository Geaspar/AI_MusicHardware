#include <iostream>
#include <memory>
#include <thread>
#include <chrono>
#include <mutex>
#include <atomic>
#include <iomanip>
#include <sstream>
#include <queue>
#include <SDL2/SDL.h>
#ifdef HAVE_SDL_TTF
#include <SDL_ttf.h>
#endif

// Core systems
#include "../include/audio/AudioEngine.h"
#include "../include/audio/Synthesizer.h"
#include "../include/effects/EffectProcessor.h"
#include "../include/sequencer/Sequencer.h"
#include "../include/midi/MidiInterface.h"
#include "../include/hardware/HardwareInterface.h"

// Enhanced UI system
#include "../include/ui/UIContext.h"
#include "../include/ui/SynthKnob.h"
#include "../include/ui/PresetBrowserUIComponent.h"
#include "../include/ui/VisualizationComponents.h"
#include "../include/ui/ParameterUpdateQueue.h"
#include "../include/ui/MidiKeyboard.h"
#include "../include/ui/GridLayout.h"
#include "../include/ui/parameters/ParameterManager.h"
#include "../include/ui/presets/PresetManager.h"
#include "../include/ui/presets/PresetDatabase.h"
#include "../include/midi/MidiCCLearning.h"

using namespace AIMusicHardware;

// Thread-safe parameter update system
class ParameterUpdateSystem {
public:
    struct Update {
        std::string parameterId;
        float value;
        enum Source { UI, Engine, MIDI, Automation } source;
    };
    
private:
    // Lock-free queues for thread-safe communication
    mutable std::mutex uiQueueMutex_;
    mutable std::mutex engineQueueMutex_;
    std::queue<Update> uiQueue_;
    std::queue<Update> engineQueue_;
    
public:
    void pushUIUpdate(const std::string& id, float value, Update::Source source = Update::UI) {
        std::lock_guard<std::mutex> lock(uiQueueMutex_);
        uiQueue_.push({id, value, source});
    }
    
    void pushEngineUpdate(const std::string& id, float value) {
        std::lock_guard<std::mutex> lock(engineQueueMutex_);
        engineQueue_.push({id, value, Update::Engine});
    }
    
    bool popUIUpdate(Update& update) {
        std::lock_guard<std::mutex> lock(uiQueueMutex_);
        if (uiQueue_.empty()) return false;
        update = uiQueue_.front();
        uiQueue_.pop();
        return true;
    }
    
    bool popEngineUpdate(Update& update) {
        std::lock_guard<std::mutex> lock(engineQueueMutex_);
        if (engineQueue_.empty()) return false;
        update = engineQueue_.front();
        engineQueue_.pop();
        return true;
    }
};

// Enhanced SDL DisplayManager with animations
class EnhancedSDLDisplayManager : public DisplayManager {
public:
    EnhancedSDLDisplayManager(SDL_Renderer* renderer) : renderer_(renderer), width_(1280), height_(800), 
                                             font_(nullptr), fontLarge_(nullptr), fontSmall_(nullptr) {
#ifdef HAVE_SDL_TTF
        if (TTF_Init() == -1) {
            std::cerr << "TTF_Init failed: " << TTF_GetError() << std::endl;
        } else {
            font_ = TTF_OpenFont("/System/Library/Fonts/Helvetica.ttc", 14);
            fontLarge_ = TTF_OpenFont("/System/Library/Fonts/Helvetica.ttc", 18);
            fontSmall_ = TTF_OpenFont("/System/Library/Fonts/Helvetica.ttc", 12);
            
            if (!font_ || !fontLarge_ || !fontSmall_) {
                std::cerr << "Failed to load some fonts: " << TTF_GetError() << std::endl;
            } else {
                std::cout << "SDL_ttf initialized with multiple font sizes" << std::endl;
            }
        }
#endif
    }
    
    ~EnhancedSDLDisplayManager() {
#ifdef HAVE_SDL_TTF
        if (font_) TTF_CloseFont(font_);
        if (fontLarge_) TTF_CloseFont(fontLarge_);
        if (fontSmall_) TTF_CloseFont(fontSmall_);
        TTF_Quit();
#endif
    }
    
    bool initialize(int width, int height) override {
        width_ = width;
        height_ = height;
        return renderer_ != nullptr;
    }
    
    void shutdown() override {
        renderer_ = nullptr;
    }
    
    void clear(const Color& color = Color(0, 0, 0)) override {
        if (!renderer_) return;
        SDL_SetRenderDrawColor(renderer_, color.r, color.g, color.b, color.a);
        SDL_RenderClear(renderer_);
    }
    
    void swapBuffers() override {
        if (!renderer_) return;
        SDL_RenderPresent(renderer_);
    }
    
    void drawLine(int x1, int y1, int x2, int y2, const Color& color) override {
        if (!renderer_) return;
        SDL_SetRenderDrawColor(renderer_, color.r, color.g, color.b, color.a);
        SDL_RenderDrawLine(renderer_, x1, y1, x2, y2);
    }
    
    void drawRect(int x, int y, int width, int height, const Color& color) override {
        if (!renderer_) return;
        SDL_Rect rect = { x, y, width, height };
        SDL_SetRenderDrawColor(renderer_, color.r, color.g, color.b, color.a);
        SDL_RenderDrawRect(renderer_, &rect);
    }
    
    void fillRect(int x, int y, int width, int height, const Color& color) override {
        if (!renderer_) return;
        SDL_Rect rect = { x, y, width, height };
        SDL_SetRenderDrawColor(renderer_, color.r, color.g, color.b, color.a);
        SDL_RenderFillRect(renderer_, &rect);
    }
    
    void drawCircle(int centerX, int centerY, int radius, const Color& color) override {
        if (!renderer_) return;
        SDL_SetRenderDrawColor(renderer_, color.r, color.g, color.b, color.a);
        
        // Midpoint circle algorithm
        int x = radius;
        int y = 0;
        int p = 1 - radius;
        
        while (x >= y) {
            SDL_RenderDrawPoint(renderer_, centerX + x, centerY + y);
            SDL_RenderDrawPoint(renderer_, centerX - x, centerY + y);
            SDL_RenderDrawPoint(renderer_, centerX + x, centerY - y);
            SDL_RenderDrawPoint(renderer_, centerX - x, centerY - y);
            SDL_RenderDrawPoint(renderer_, centerX + y, centerY + x);
            SDL_RenderDrawPoint(renderer_, centerX - y, centerY + x);
            SDL_RenderDrawPoint(renderer_, centerX + y, centerY - x);
            SDL_RenderDrawPoint(renderer_, centerX - y, centerY - x);
            
            y++;
            if (p <= 0) {
                p = p + 2 * y + 1;
            } else {
                x--;
                p = p + 2 * y - 2 * x + 1;
            }
        }
    }
    
    void fillCircle(int centerX, int centerY, int radius, const Color& color) override {
        if (!renderer_) return;
        SDL_SetRenderDrawColor(renderer_, color.r, color.g, color.b, color.a);
        
        for (int y = -radius; y <= radius; y++) {
            for (int x = -radius; x <= radius; x++) {
                if (x * x + y * y <= radius * radius) {
                    SDL_RenderDrawPoint(renderer_, centerX + x, centerY + y);
                }
            }
        }
    }
    
    void drawText(int x, int y, const std::string& text, Font* font, const Color& color) override {
        drawTextWithSize(x, y, text, color, TextSize::Normal);
    }
    
    enum class TextSize {
        Small,
        Normal, 
        Large
    };
    
    void drawTextWithSize(int x, int y, const std::string& text, const Color& color, TextSize size) {
        if (!renderer_) return;
        
#ifdef HAVE_SDL_TTF
        TTF_Font* selectedFont = font_;
        switch (size) {
            case TextSize::Small: selectedFont = fontSmall_; break;
            case TextSize::Large: selectedFont = fontLarge_; break;
            default: selectedFont = font_; break;
        }
        
        if (selectedFont) {
            SDL_Color textColor = { color.r, color.g, color.b, 255 };
            SDL_Surface* textSurface = TTF_RenderText_Blended(selectedFont, text.c_str(), textColor);
            
            if (textSurface) {
                SDL_Texture* textTexture = SDL_CreateTextureFromSurface(renderer_, textSurface);
                if (textTexture) {
                    int textWidth = textSurface->w;
                    int textHeight = textSurface->h;
                    SDL_Rect destRect = { x, y, textWidth, textHeight };
                    
                    SDL_RenderCopy(renderer_, textTexture, nullptr, &destRect);
                    SDL_DestroyTexture(textTexture);
                }
                SDL_FreeSurface(textSurface);
            }
            return;
        }
#endif
    }
    
    // Draw a gradient rectangle (for modulation visualization)
    void drawGradientRect(int x, int y, int width, int height, 
                         const Color& topColor, const Color& bottomColor) {
        if (!renderer_) return;
        
        for (int i = 0; i < height; i++) {
            float t = static_cast<float>(i) / height;
            Color interpolated(
                topColor.r + t * (bottomColor.r - topColor.r),
                topColor.g + t * (bottomColor.g - topColor.g),
                topColor.b + t * (bottomColor.b - topColor.b),
                topColor.a + t * (bottomColor.a - topColor.a)
            );
            
            SDL_SetRenderDrawColor(renderer_, interpolated.r, interpolated.g, interpolated.b, interpolated.a);
            SDL_RenderDrawLine(renderer_, x, y + i, x + width, y + i);
        }
    }
    
    int getWidth() const override { return width_; }
    int getHeight() const override { return height_; }
    
private:
    SDL_Renderer* renderer_;
    int width_;
    int height_;
#ifdef HAVE_SDL_TTF
    TTF_Font* font_;
    TTF_Font* fontLarge_;
    TTF_Font* fontSmall_;
#endif
};

// Enhanced knob with modulation visualization
class EnhancedSynthKnob : public SynthKnob {
    float modulationAmount_ = 0.0f;
    Color modulationColor_ = Color(0, 255, 128);
    float animationPhase_ = 0.0f;
    bool showTooltip_ = false;
    std::string tooltipText_;
    
public:
    EnhancedSynthKnob(const std::string& label, int x, int y, int size, 
                      float min, float max, float defaultVal)
        : SynthKnob(label, x, y, size, min, max, defaultVal) {}
    
    void setModulationAmount(float amount) {
        modulationAmount_ = std::clamp(amount, -1.0f, 1.0f);
    }
    
    void setModulationColor(const Color& color) {
        modulationColor_ = color;
    }
    
    void update(float deltaTime) override {
        SynthKnob::update(deltaTime);
        
        // Update animation phase for modulation visualization
        if (modulationAmount_ != 0.0f) {
            animationPhase_ += deltaTime * 2.0f; // 2Hz animation
            if (animationPhase_ > 6.28318f) animationPhase_ -= 6.28318f;
        }
    }
    
    void render(DisplayManager* display) override {
        // Render base knob
        SynthKnob::render(display);
        
        // Render modulation indicator
        if (modulationAmount_ != 0.0f) {
            auto* enhancedDisplay = static_cast<EnhancedSDLDisplayManager*>(display);
            
            int centerX = x_ + width_ / 2;
            int centerY = y_ + height_ / 2;
            int radius = std::min(width_, height_) / 2 - 4;
            
            // Draw modulation arc
            float modAngle = animationPhase_ + getValue() * 3.14159f;
            int modRadius = radius + 5 + std::sin(animationPhase_) * 2;
            
            Color modColor = modulationColor_;
            modColor.a = 128 + std::sin(animationPhase_) * 64;
            
            // Draw pulsing outer ring for modulation
            for (int i = 0; i < 3; i++) {
                int r = modRadius + i;
                modColor.a = modColor.a / (i + 1);
                display->drawCircle(centerX, centerY, r, modColor);
            }
        }
        
        // Render tooltip on hover
        if (showTooltip_ && !tooltipText_.empty()) {
            auto* enhancedDisplay = static_cast<EnhancedSDLDisplayManager*>(display);
            int tooltipX = x_ + width_ + 10;
            int tooltipY = y_ + height_ / 2 - 10;
            
            // Draw tooltip background
            display->fillRect(tooltipX, tooltipY, tooltipText_.length() * 8 + 10, 20, Color(40, 40, 40, 220));
            display->drawRect(tooltipX, tooltipY, tooltipText_.length() * 8 + 10, 20, Color(80, 80, 80));
            
            // Draw tooltip text
            enhancedDisplay->drawTextWithSize(tooltipX + 5, tooltipY + 3, tooltipText_, 
                                            Color(220, 220, 220), EnhancedSDLDisplayManager::TextSize::Small);
        }
    }
    
    bool handleInput(const InputEvent& event) override {
        bool handled = SynthKnob::handleInput(event);
        
        // Update tooltip on hover
        if (event.type == InputEventType::TouchMove) {
            int dx = event.value - (x_ + width_ / 2);
            int dy = event.value2 - (y_ + height_ / 2);
            float distance = std::sqrt(dx*dx + dy*dy);
            int radius = std::min(width_, height_) / 2;
            
            showTooltip_ = (distance <= radius + 10);
            if (showTooltip_ && valueFormatter_) {
                tooltipText_ = valueFormatter_(getValue());
            }
        }
        
        return handled;
    }
};

// Helper to translate SDL events to our InputEvent format
InputEvent translateSDLEvent(const SDL_Event& sdlEvent) {
    InputEvent event;
    
    switch (sdlEvent.type) {
        case SDL_MOUSEBUTTONDOWN:
            event.type = InputEventType::TouchPress;
            event.id = 0;
            event.value = static_cast<float>(sdlEvent.button.x);
            event.value2 = static_cast<float>(sdlEvent.button.y);
            break;
            
        case SDL_MOUSEBUTTONUP:
            event.type = InputEventType::TouchRelease;
            event.id = 0;
            event.value = static_cast<float>(sdlEvent.button.x);
            event.value2 = static_cast<float>(sdlEvent.button.y);
            break;
            
        case SDL_MOUSEMOTION:
            if (sdlEvent.motion.state & SDL_BUTTON_LMASK) {
                event.type = InputEventType::TouchMove;
                event.id = 0;
                event.value = static_cast<float>(sdlEvent.motion.x);
                event.value2 = static_cast<float>(sdlEvent.motion.y);
            } else {
                // Hover event
                event.type = InputEventType::TouchMove;
                event.id = 0;
                event.value = static_cast<float>(sdlEvent.motion.x);
                event.value2 = static_cast<float>(sdlEvent.motion.y);
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
            
        case SDL_MOUSEWHEEL:
            event.type = InputEventType::EncoderRotate;
            event.id = 0;
            event.value = static_cast<float>(sdlEvent.wheel.y);
            break;
    }
    
    return event;
}

// Enhanced audio processing callback with parameter updates
void audioCallback(AudioEngine* audioEngine, Synthesizer* synthesizer, 
                  EffectProcessor* effectProcessor, Sequencer* sequencer,
                  WaveformVisualizer* waveform, LevelMeter* levelMeter,
                  SpectrumAnalyzer* spectrum, PhaseMeter* phaseMeter,
                  ParameterUpdateSystem* paramSystem,
                  float* outputBuffer, int numFrames) {
    
    // Process parameter updates from UI
    ParameterUpdateSystem::Update update;
    while (paramSystem->popUIUpdate(update)) {
        synthesizer->setParameter(update.parameterId, update.value);
    }
    
    // Process sequencer
    sequencer->process(static_cast<float>(numFrames) / audioEngine->getSampleRate());
    
    // Process synthesizer
    synthesizer->process(outputBuffer, numFrames);
    
    // Process effects
    effectProcessor->process(outputBuffer, numFrames);
    
    // Update visualizers (thread-safe)
    if (waveform) {
        waveform->pushSamples(outputBuffer, numFrames, 2);
    }
    
    if (spectrum) {
        // Spectrum analyzer expects mono input
        std::vector<float> monoBuffer(numFrames);
        for (int i = 0; i < numFrames; i++) {
            monoBuffer[i] = (outputBuffer[i * 2] + outputBuffer[i * 2 + 1]) * 0.5f;
        }
        spectrum->pushSamples(monoBuffer.data(), numFrames, 1);
    }
    
    if (phaseMeter) {
        // Phase meter needs separate L/R buffers
        std::vector<float> leftBuffer(numFrames);
        std::vector<float> rightBuffer(numFrames);
        for (int i = 0; i < numFrames; i++) {
            leftBuffer[i] = outputBuffer[i * 2];
            rightBuffer[i] = outputBuffer[i * 2 + 1];
        }
        phaseMeter->pushSamples(leftBuffer.data(), rightBuffer.data(), numFrames);
    }
    
    // Calculate RMS for level meter
    if (levelMeter) {
        float leftRms = 0.0f, rightRms = 0.0f;
        for (int i = 0; i < numFrames; i++) {
            leftRms += outputBuffer[i * 2] * outputBuffer[i * 2];
            rightRms += outputBuffer[i * 2 + 1] * outputBuffer[i * 2 + 1];
        }
        leftRms = std::sqrt(leftRms / numFrames);
        rightRms = std::sqrt(rightRms / numFrames);
        levelMeter->setLevels(leftRms * 2.0f, rightRms * 2.0f);
    }
}

int main(int argc, char* argv[]) {
    std::cout << "AI Music Hardware - Enhanced Grid Layout Version" << std::endl;
    std::cout << "Starting synthesizer with advanced UI features..." << std::endl;
    
    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) {
        std::cerr << "SDL initialization failed: " << SDL_GetError() << std::endl;
        return 1;
    }
    
    // Create SDL window
    SDL_Window* window = SDL_CreateWindow(
        "AI Music Hardware - Enhanced Grid Layout",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        1280, 800,
        SDL_WINDOW_SHOWN
    );
    
    if (!window) {
        std::cerr << "Window creation failed: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return 1;
    }
    
    // Create SDL renderer with VSync
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, 
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer) {
        std::cerr << "Renderer creation failed: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }
    
    // Create parameter update system
    auto paramUpdateSystem = std::make_unique<ParameterUpdateSystem>();
    
    // Create core audio components
    auto audioEngine = std::make_unique<AudioEngine>(44100, 128); // Low latency
    auto synthesizer = std::make_unique<Synthesizer>();
    auto effectProcessor = std::make_unique<EffectProcessor>();
    auto sequencer = std::make_unique<Sequencer>();
    auto midiInput = std::make_unique<MidiInput>();
    auto midiOutput = std::make_unique<MidiOutput>();
    auto midiHandler = std::make_unique<MidiHandler>();
    auto hardwareInterface = std::make_unique<HardwareInterface>();
    
    // Initialize core components
    if (!synthesizer->initialize()) {
        std::cerr << "Failed to initialize synthesizer!" << std::endl;
        return 1;
    }
    
    if (!effectProcessor->initialize()) {
        std::cerr << "Failed to initialize effect processor!" << std::endl;
        return 1;
    }
    
    if (!sequencer->initialize()) {
        std::cerr << "Failed to initialize sequencer!" << std::endl;
        return 1;
    }
    
    if (!audioEngine->initialize()) {
        std::cerr << "Failed to initialize audio engine!" << std::endl;
        return 1;
    }
    
    // Create UI context with enhanced display manager
    auto uiContext = std::make_unique<UIContext>();
    auto sdlDisplayManager = std::make_shared<EnhancedSDLDisplayManager>(renderer);
    uiContext->setDisplayManager(sdlDisplayManager);
    uiContext->initialize(1280, 800);
    
    // Initialize parameter manager
    auto& paramManager = EnhancedParameterManager::getInstance();
    paramManager.connectSynthesizer(synthesizer.get());
    
    // Initialize MIDI CC Learning system
    auto& ccLearning = MidiCCLearningManager::getInstance();
    ccLearning.initialize();
    
    // Create parameter mapping storage
    std::map<std::string, SynthKnob*> parameterKnobs;
    
    // Create main synthesizer screen with enhanced grid layout
    auto mainScreen = std::make_unique<Screen>("main");
    mainScreen->setBackgroundColor(Color(30, 30, 35)); // Darker, more professional background
    mainScreen->setPosition(0, 0);
    mainScreen->setSize(1280, 800);
    
    // Create the main grid layout (8x6 grid)
    auto mainGrid = std::make_unique<GridLayout>("main_grid", 6, 8);
    mainGrid->setPosition(0, 0);
    mainGrid->setSize(1280, 800);
    mainGrid->setPadding(20);
    mainGrid->setSpacing(10, 10);
    
    // Title with gradient effect
    auto titleLabel = std::make_unique<Label>("title", "AI Music Hardware - Enhanced");
    titleLabel->setTextColor(Color(220, 240, 255));
    mainGrid->addComponent(std::move(titleLabel), 0, 0, 1, 8);
    
    // OSCILLATOR SECTION with enhanced knobs
    auto oscContainer = std::make_unique<GridLayout>("osc_grid", 2, 3);
    
    auto oscSection = std::make_unique<Label>("osc_section", "OSCILLATOR");
    oscSection->setTextColor(Color(255, 200, 100)); // Warmer color
    oscContainer->addComponent(std::move(oscSection), 0, 0, 1, 3);
    
    // Enhanced frequency knob with modulation
    auto freqKnob = std::make_unique<EnhancedSynthKnob>("Frequency", 0, 0, 80, 20.0f, 20000.0f, 440.0f);
    freqKnob->setValueFormatter([](float value) {
        std::stringstream ss;
        if (value >= 1000.0f) {
            ss << std::fixed << std::setprecision(2) << value / 1000.0f << " kHz";
        } else {
            ss << std::fixed << std::setprecision(1) << value << " Hz";
        }
        return ss.str();
    });
    freqKnob->setModulationAmount(0.3f); // Demo modulation
    EnhancedSynthKnob* freqKnobPtr = freqKnob.get();
    oscContainer->addComponent(std::move(freqKnob), 1, 0);
    
    // Wave shape knob
    auto waveKnob = std::make_unique<EnhancedSynthKnob>("Wave", 0, 0, 80, 0.0f, 4.0f, 0.0f);
    waveKnob->setValueFormatter([](float value) {
        const char* waveNames[] = {"Sine", "Saw", "Square", "Triangle", "Noise"};
        int index = static_cast<int>(value);
        if (index >= 0 && index < 5) {
            return std::string(waveNames[index]);
        }
        return std::string("Unknown");
    });
    SynthKnob* waveKnobPtr = waveKnob.get();
    oscContainer->addComponent(std::move(waveKnob), 1, 1);
    
    // Detune knob
    auto detuneKnob = std::make_unique<EnhancedSynthKnob>("Detune", 0, 0, 80, -50.0f, 50.0f, 0.0f);
    detuneKnob->setValueFormatter([](float value) {
        std::stringstream ss;
        ss << std::fixed << std::setprecision(1) << value << " cents";
        return ss.str();
    });
    oscContainer->addComponent(std::move(detuneKnob), 1, 2);
    
    mainGrid->addComponent(std::move(oscContainer), 1, 0, 1, 2);
    
    // FILTER SECTION with resonance visualization
    auto filterContainer = std::make_unique<GridLayout>("filter_grid", 2, 3);
    
    auto filterSection = std::make_unique<Label>("filter_section", "FILTER");
    filterSection->setTextColor(Color(100, 255, 150)); // Cooler color
    filterContainer->addComponent(std::move(filterSection), 0, 0, 1, 3);
    
    // Enhanced cutoff with logarithmic scaling
    auto cutoffKnob = std::make_unique<EnhancedSynthKnob>("Cutoff", 0, 0, 80, 20.0f, 20000.0f, 1000.0f);
    cutoffKnob->setValueFormatter([](float value) {
        std::stringstream ss;
        if (value >= 1000.0f) {
            ss << std::fixed << std::setprecision(1) << value / 1000.0f << " kHz";
        } else {
            ss << std::fixed << std::setprecision(0) << value << " Hz";
        }
        return ss.str();
    });
    cutoffKnob->setModulationAmount(0.5f); // Demo modulation
    cutoffKnob->setModulationColor(Color(100, 200, 255)); // Blue modulation
    SynthKnob* cutoffKnobPtr = cutoffKnob.get();
    filterContainer->addComponent(std::move(cutoffKnob), 1, 0);
    
    // Resonance with visual feedback
    auto resKnob = std::make_unique<EnhancedSynthKnob>("Resonance", 0, 0, 80, 0.0f, 1.0f, 0.5f);
    resKnob->setValueFormatter([](float value) {
        std::stringstream ss;
        ss << std::fixed << std::setprecision(0) << value * 100.0f << "%";
        return ss.str();
    });
    SynthKnob* resKnobPtr = resKnob.get();
    filterContainer->addComponent(std::move(resKnob), 1, 1);
    
    // Filter type selector
    auto filterTypeKnob = std::make_unique<EnhancedSynthKnob>("Type", 0, 0, 80, 0.0f, 3.0f, 0.0f);
    filterTypeKnob->setValueFormatter([](float value) {
        const char* filterTypes[] = {"LP", "HP", "BP", "Notch"};
        int index = static_cast<int>(value);
        if (index >= 0 && index < 4) {
            return std::string(filterTypes[index]);
        }
        return std::string("LP");
    });
    filterContainer->addComponent(std::move(filterTypeKnob), 1, 2);
    
    mainGrid->addComponent(std::move(filterContainer), 1, 2, 1, 2);
    
    // ENVELOPE SECTION with interactive visualization
    auto envContainer = std::make_unique<GridLayout>("env_grid", 2, 4);
    
    auto envSection = std::make_unique<Label>("env_section", "ENVELOPE");
    envSection->setTextColor(Color(255, 150, 255)); // Purple
    envContainer->addComponent(std::move(envSection), 0, 0, 1, 4);
    
    // ADSR knobs with time formatting
    auto attackKnob = std::make_unique<EnhancedSynthKnob>("Attack", 0, 0, 60, 0.001f, 2.0f, 0.01f);
    attackKnob->setValueFormatter([](float value) {
        std::stringstream ss;
        if (value < 0.1f) {
            ss << std::fixed << std::setprecision(0) << value * 1000.0f << " ms";
        } else {
            ss << std::fixed << std::setprecision(2) << value << " s";
        }
        return ss.str();
    });
    
    auto decayKnob = std::make_unique<EnhancedSynthKnob>("Decay", 0, 0, 60, 0.001f, 2.0f, 0.1f);
    decayKnob->setValueFormatter([](float value) {
        std::stringstream ss;
        if (value < 0.1f) {
            ss << std::fixed << std::setprecision(0) << value * 1000.0f << " ms";
        } else {
            ss << std::fixed << std::setprecision(2) << value << " s";
        }
        return ss.str();
    });
    
    auto sustainKnob = std::make_unique<EnhancedSynthKnob>("Sustain", 0, 0, 60, 0.0f, 1.0f, 0.7f);
    sustainKnob->setValueFormatter([](float value) {
        std::stringstream ss;
        ss << std::fixed << std::setprecision(0) << value * 100.0f << "%";
        return ss.str();
    });
    
    auto releaseKnob = std::make_unique<EnhancedSynthKnob>("Release", 0, 0, 60, 0.001f, 4.0f, 0.5f);
    releaseKnob->setValueFormatter([](float value) {
        std::stringstream ss;
        if (value < 0.1f) {
            ss << std::fixed << std::setprecision(0) << value * 1000.0f << " ms";
        } else {
            ss << std::fixed << std::setprecision(2) << value << " s";
        }
        return ss.str();
    });
    
    envContainer->addComponent(std::move(attackKnob), 1, 0);
    envContainer->addComponent(std::move(decayKnob), 1, 1);
    envContainer->addComponent(std::move(sustainKnob), 1, 2);
    envContainer->addComponent(std::move(releaseKnob), 1, 3);
    
    mainGrid->addComponent(std::move(envContainer), 1, 4, 1, 3);
    
    // MASTER SECTION
    auto masterContainer = std::make_unique<GridLayout>("master_grid", 2, 1);
    
    auto masterSection = std::make_unique<Label>("master_section", "MASTER");
    masterSection->setTextColor(Color(150, 200, 255)); // Light blue
    masterContainer->addComponent(std::move(masterSection), 0, 0);
    
    // Master volume with dB display
    auto volumeKnob = std::make_unique<EnhancedSynthKnob>("Volume", 0, 0, 80, 0.0f, 1.0f, 0.7f);
    volumeKnob->setValueFormatter([](float value) {
        std::stringstream ss;
        if (value == 0.0f) {
            ss << "-∞ dB";
        } else {
            float db = 20.0f * std::log10(value);
            ss << std::fixed << std::setprecision(1) << db << " dB";
        }
        return ss.str();
    });
    SynthKnob* volumeKnobPtr = volumeKnob.get();
    masterContainer->addComponent(std::move(volumeKnob), 1, 0);
    
    mainGrid->addComponent(std::move(masterContainer), 1, 7, 1, 1);
    
    // ENHANCED VISUALIZATION SECTION
    auto vizSection = std::make_unique<Label>("viz_section", "VISUALIZATION");
    vizSection->setTextColor(Color(255, 200, 100));
    mainGrid->addComponent(std::move(vizSection), 2, 0, 1, 2);
    
    // Multi-mode waveform display
    auto waveform = std::make_unique<WaveformVisualizer>("waveform", 512);
    waveform->setWaveformColor(Color(0, 255, 128));
    waveform->setDisplayMode(WaveformVisualizer::DisplayMode::Waveform);
    WaveformVisualizer* waveformPtr = waveform.get();
    mainGrid->addComponent(std::move(waveform), 2, 0, 1, 2);
    
    // Spectrum analyzer
    auto spectrum = std::make_unique<SpectrumAnalyzer>("spectrum", 64);
    spectrum->setDisplayMode(WaveformVisualizer::DisplayMode::Spectrum);
    SpectrumAnalyzer* spectrumPtr = spectrum.get();
    mainGrid->addComponent(std::move(spectrum), 2, 2, 1, 2);
    
    // Interactive envelope visualizer
    auto envelope = std::make_unique<EnvelopeVisualizer>("envelope");
    envelope->setADSR(0.01f, 0.1f, 0.7f, 0.5f);
    envelope->setEditable(true);
    envelope->setEnvelopeColor(Color(255, 150, 50));
    envelope->setActiveColor(Color(255, 200, 100));
    mainGrid->addComponent(std::move(envelope), 2, 4, 1, 2);
    
    // Stereo level meter with peak hold
    auto levelMeter = std::make_unique<LevelMeter>("level", LevelMeter::Orientation::Vertical);
    levelMeter->setStereo(true);
    levelMeter->setPeakHoldTime(2.0f);
    levelMeter->setMeterColors(
        Color(0, 200, 0),    // Low (green)
        Color(200, 200, 0),  // Mid (yellow)
        Color(200, 0, 0)     // High (red)
    );
    LevelMeter* levelMeterPtr = levelMeter.get();
    mainGrid->addComponent(std::move(levelMeter), 2, 6, 1, 1);
    
    // Phase meter for stereo analysis
    auto phaseMeter = std::make_unique<PhaseMeter>("phase");
    phaseMeter->setTraceColor(Color(100, 200, 255));
    PhaseMeter* phaseMeterPtr = phaseMeter.get();
    mainGrid->addComponent(std::move(phaseMeter), 2, 7, 1, 1);
    
    // MIDI KEYBOARD with enhanced features
    auto keyboardSection = std::make_unique<Label>("keyboard_section", "MIDI KEYBOARD");
    keyboardSection->setTextColor(Color(200, 150, 255));
    mainGrid->addComponent(std::move(keyboardSection), 3, 0, 1, 2);
    
    auto midiKeyboard = std::make_unique<MidiKeyboard>("midi_keyboard", 0, 0);
    
    // Configure keyboard
    MidiKeyboard::KeyboardConfig keyboardConfig;
    keyboardConfig.startOctave = 3;
    keyboardConfig.numOctaves = 3;
    keyboardConfig.whiteKeyWidth = 28;
    keyboardConfig.whiteKeyHeight = 120;
    keyboardConfig.blackKeyWidth = 20;
    keyboardConfig.blackKeyHeight = 80;
    
    midiKeyboard->setConfig(keyboardConfig);
    midiKeyboard->setVelocityRange(30, 127);
    
    // Connect keyboard to synthesizer with parameter updates
    midiKeyboard->setNoteCallback([&synthesizer, &paramUpdateSystem](int note, int velocity, bool isNoteOn) {
        if (isNoteOn) {
            float normalizedVelocity = velocity / 127.0f;
            synthesizer->noteOn(note, normalizedVelocity);
            // Push velocity to parameter system for visualization
            paramUpdateSystem->pushEngineUpdate("last_velocity", normalizedVelocity);
        } else {
            synthesizer->noteOff(note);
        }
    });
    
    MidiKeyboard* midiKeyboardPtr = midiKeyboard.get();
    mainGrid->addComponent(std::move(midiKeyboard), 4, 0, 1, 6);
    
    // Enhanced keyboard controls
    auto octaveDownButton = std::make_unique<Button>("octave_down", "OCT-");
    octaveDownButton->setBackgroundColor(Color(70, 70, 90));
    octaveDownButton->setTextColor(Color(255, 255, 255));
    octaveDownButton->setHighlightColor(Color(100, 100, 120));
    octaveDownButton->setClickCallback([midiKeyboardPtr]() {
        if (midiKeyboardPtr) {
            midiKeyboardPtr->transposeOctave(-1);
        }
    });
    mainGrid->addComponent(std::move(octaveDownButton), 5, 0);
    
    auto octaveUpButton = std::make_unique<Button>("octave_up", "OCT+");
    octaveUpButton->setBackgroundColor(Color(70, 70, 90));
    octaveUpButton->setTextColor(Color(255, 255, 255));
    octaveUpButton->setHighlightColor(Color(100, 100, 120));
    octaveUpButton->setClickCallback([midiKeyboardPtr]() {
        if (midiKeyboardPtr) {
            midiKeyboardPtr->transposeOctave(1);
        }
    });
    mainGrid->addComponent(std::move(octaveUpButton), 5, 1);
    
    // Mode switching buttons
    auto waveformModeButton = std::make_unique<Button>("wave_mode", "Wave");
    waveformModeButton->setBackgroundColor(Color(60, 80, 60));
    waveformModeButton->setClickCallback([waveformPtr]() {
        if (waveformPtr) {
            waveformPtr->setDisplayMode(WaveformVisualizer::DisplayMode::Waveform);
        }
    });
    mainGrid->addComponent(std::move(waveformModeButton), 5, 2);
    
    auto spectrumModeButton = std::make_unique<Button>("lissajous_mode", "X-Y");
    spectrumModeButton->setBackgroundColor(Color(60, 60, 80));
    spectrumModeButton->setClickCallback([waveformPtr]() {
        if (waveformPtr) {
            waveformPtr->setDisplayMode(WaveformVisualizer::DisplayMode::Lissajous);
        }
    });
    mainGrid->addComponent(std::move(spectrumModeButton), 5, 3);
    
    // PRESET BROWSER
    auto presetSection = std::make_unique<Label>("preset_section", "PRESET BROWSER");
    presetSection->setTextColor(Color(150, 255, 200));
    mainGrid->addComponent(std::move(presetSection), 3, 6, 1, 2);
    
    // Initialize preset system
    auto presetManager = std::make_unique<PresetManager>(synthesizer.get());
    auto presetDatabase = std::make_unique<PresetDatabase>();
    
    // Add some enhanced presets
    PresetInfo preset1;
    preset1.name = "Deep Bass";
    preset1.category = "Bass";
    preset1.author = "System";
    preset1.description = "Rich, deep bass with warm filter";
    presetDatabase->addPreset(preset1);
    
    PresetInfo preset2;
    preset2.name = "Acid Lead";
    preset2.category = "Lead";
    preset2.author = "System";
    preset2.description = "Classic acid lead with resonant filter sweep";
    presetDatabase->addPreset(preset2);
    
    PresetInfo preset3;
    preset3.name = "Ambient Pad";
    preset3.category = "Pad";
    preset3.author = "System";
    preset3.description = "Lush ambient pad with slow attack";
    presetDatabase->addPreset(preset3);
    
    auto presetBrowser = std::make_unique<PresetBrowserUI>("preset_browser");
    presetBrowser->initialize(presetManager.get(), presetDatabase.get());
    presetBrowser->setParameterManager(&paramManager);
    mainGrid->addComponent(std::move(presetBrowser), 4, 6, 2, 2);
    
    // Connect knobs to synthesizer parameters with thread-safe updates
    auto connectKnobToParam = [&](SynthKnob* knob, const std::string& paramId) {
        if (knob) {
            parameterKnobs[paramId] = knob;
            knob->setValueChangeCallback([&paramUpdateSystem, paramId](float value) {
                paramUpdateSystem->pushUIUpdate(paramId, value);
            });
            float currentValue = synthesizer->getParameter(paramId);
            knob->setValue(currentValue);
        }
    };
    
    connectKnobToParam(waveKnobPtr, "oscillator_type");
    connectKnobToParam(resKnobPtr, "filter_resonance");
    connectKnobToParam(volumeKnobPtr, "master_volume");
    
    // Special handling for filter cutoff with logarithmic scaling
    if (cutoffKnobPtr) {
        parameterKnobs["filter_cutoff"] = cutoffKnobPtr;
        cutoffKnobPtr->setValueChangeCallback([&paramUpdateSystem](float frequencyHz) {
            float normalized = (std::log(frequencyHz / 20.0f) / std::log(20000.0f / 20.0f));
            normalized = std::max(0.0f, std::min(1.0f, normalized));
            paramUpdateSystem->pushUIUpdate("filter_cutoff", normalized);
        });
        cutoffKnobPtr->setValue(1000.0f);
    }
    
    // Add grid to screen
    mainScreen->addChild(std::move(mainGrid));
    
    // Add screen to context
    uiContext->addScreen(std::move(mainScreen));
    uiContext->setActiveScreen("main");
    
    // Set up enhanced audio callback
    std::mutex audioMutex;
    audioEngine->setAudioCallback([&](float* outputBuffer, int numFrames) {
        std::lock_guard<std::mutex> lock(audioMutex);
        audioCallback(audioEngine.get(), synthesizer.get(), effectProcessor.get(),
                     sequencer.get(), waveformPtr, levelMeterPtr, spectrumPtr, phaseMeterPtr,
                     paramUpdateSystem.get(), outputBuffer, numFrames);
    });
    
    // Main loop with enhanced features
    bool running = true;
    auto lastFrameTime = std::chrono::high_resolution_clock::now();
    
    std::cout << "Starting enhanced main loop..." << std::endl;
    std::cout << "Features: Parameter binding, modulation visualization, multi-mode displays" << std::endl;
    
    while (running) {
        auto frameStart = std::chrono::high_resolution_clock::now();
        
        // Process SDL events
        SDL_Event sdlEvent;
        while (SDL_PollEvent(&sdlEvent)) {
            if (sdlEvent.type == SDL_QUIT) {
                running = false;
            } else if (sdlEvent.type == SDL_KEYDOWN) {
                if (sdlEvent.key.keysym.sym == SDLK_ESCAPE) {
                    running = false;
                } else if (sdlEvent.key.keysym.sym == SDLK_SPACE) {
                    // Toggle spectrum analyzer mode
                    if (spectrumPtr) {
                        auto currentMode = spectrumPtr->getDisplayMode();
                        if (currentMode == WaveformVisualizer::DisplayMode::Spectrum) {
                            spectrumPtr->setDisplayMode(WaveformVisualizer::DisplayMode::Waterfall);
                        } else {
                            spectrumPtr->setDisplayMode(WaveformVisualizer::DisplayMode::Spectrum);
                        }
                    }
                }
            } else {
                InputEvent inputEvent = translateSDLEvent(sdlEvent);
                uiContext->handleInput(inputEvent);
            }
        }
        
        // Process parameter updates from engine
        ParameterUpdateSystem::Update update;
        while (paramUpdateSystem->popEngineUpdate(update)) {
            // Update UI knobs from engine changes
            auto it = parameterKnobs.find(update.parameterId);
            if (it != parameterKnobs.end() && it->second) {
                it->second->setValue(update.value);
            }
        }
        
        // Update UI with delta time
        auto currentTime = std::chrono::high_resolution_clock::now();
        float deltaTime = std::chrono::duration<float>(currentTime - lastFrameTime).count();
        lastFrameTime = currentTime;
        
        uiContext->update(deltaTime);
        
        // Render
        Screen* activeScreen = uiContext->getScreen("main");
        if (activeScreen) {
            sdlDisplayManager->clear(activeScreen->getBackgroundColor());
            activeScreen->render(sdlDisplayManager.get());
        }
        
        // Present frame (VSync will handle timing)
        SDL_RenderPresent(renderer);
    }
    
    // Cleanup
    std::cout << "Shutting down enhanced synthesizer..." << std::endl;
    
    if (audioEngine) {
        audioEngine->shutdown();
    }
    
    if (uiContext) {
        uiContext->shutdown();
    }
    
    sdlDisplayManager.reset();
    
    if (renderer) {
        SDL_DestroyRenderer(renderer);
    }
    if (window) {
        SDL_DestroyWindow(window);
    }
    SDL_Quit();
    
    std::cout << "Enhanced shutdown complete." << std::endl;
    
    return 0;
}