/**
 * Grid Layout Enhancement Demo
 * 
 * This demo shows key UI improvements based on the documentation:
 * 1. Better visual feedback for knobs
 * 2. Multiple visualization modes
 * 3. Improved color scheme
 * 4. Modulation visualization hints
 */

#include <iostream>
#include <memory>
#include <thread>
#include <chrono>
#include <mutex>
#include <atomic>
#include <iomanip>
#include <sstream>
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

// Enhanced display manager with better visual features
class EnhancedDisplayManager : public DisplayManager {
public:
    EnhancedDisplayManager(SDL_Renderer* renderer) : renderer_(renderer), width_(1280), height_(800), 
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
    
    ~EnhancedDisplayManager() {
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

// Enhanced audio processing callback
void audioCallback(AudioEngine* audioEngine, Synthesizer* synthesizer, 
                  EffectProcessor* effectProcessor, Sequencer* sequencer,
                  WaveformVisualizer* waveform, LevelMeter* levelMeter,
                  float* outputBuffer, int numFrames) {
    
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
    
    // Calculate RMS for level meter
    if (levelMeter) {
        float rms = 0.0f;
        for (int i = 0; i < numFrames * 2; i += 2) {
            float sample = (outputBuffer[i] + outputBuffer[i + 1]) * 0.5f;
            rms += sample * sample;
        }
        rms = std::sqrt(rms / numFrames);
        levelMeter->setLevel(rms * 2.0f);
    }
}

int main(int argc, char* argv[]) {
    std::cout << "AI Music Hardware - Grid Layout Enhancements Demo" << std::endl;
    std::cout << "Demonstrating improved UI features..." << std::endl;
    
    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) {
        std::cerr << "SDL initialization failed: " << SDL_GetError() << std::endl;
        return 1;
    }
    
    // Create SDL window
    SDL_Window* window = SDL_CreateWindow(
        "AI Music Hardware - Enhanced Grid Demo",
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
    auto sdlDisplayManager = std::make_shared<EnhancedDisplayManager>(renderer);
    uiContext->setDisplayManager(sdlDisplayManager);
    uiContext->initialize(1280, 800);
    
    // Set enhanced theme colors
    uiContext->setThemeColor("background", Color(25, 25, 30));
    uiContext->setThemeColor("foreground", Color(240, 240, 240));
    uiContext->setThemeColor("highlight", Color(0, 180, 255));
    uiContext->setThemeColor("accent", Color(255, 120, 0));
    uiContext->setThemeColor("warning", Color(255, 60, 60));
    uiContext->setThemeColor("success", Color(60, 200, 60));
    
    // Initialize parameter manager
    auto& paramManager = EnhancedParameterManager::getInstance();
    paramManager.connectSynthesizer(synthesizer.get());
    
    // Initialize MIDI CC Learning system
    auto& ccLearning = MidiCCLearningManager::getInstance();
    ccLearning.initialize();
    
    // Create parameter mapping storage
    std::map<std::string, SynthKnob*> parameterKnobs;
    
    // Create main synthesizer screen
    auto mainScreen = std::make_unique<Screen>("main");
    mainScreen->setBackgroundColor(uiContext->getThemeColor("background"));
    mainScreen->setPosition(0, 0);
    mainScreen->setSize(1280, 800);
    
    // Create the main grid layout (8x6 grid)
    auto mainGrid = std::make_unique<GridLayout>("main_grid", 6, 8);
    mainGrid->setPosition(0, 0);
    mainGrid->setSize(1280, 800);
    mainGrid->setPadding(20);
    mainGrid->setSpacing(12, 12); // Slightly more spacing
    
    // Title with improved styling
    auto titleLabel = std::make_unique<Label>("title", "AI Music Hardware - Enhanced UI");
    titleLabel->setTextColor(uiContext->getThemeColor("foreground"));
    mainGrid->addComponent(std::move(titleLabel), 0, 0, 1, 8);
    
    // OSCILLATOR SECTION with improved colors
    auto oscContainer = std::make_unique<GridLayout>("osc_grid", 2, 2);
    
    auto oscSection = std::make_unique<Label>("osc_section", "OSCILLATOR");
    oscSection->setTextColor(Color(255, 200, 100)); // Warm orange
    oscContainer->addComponent(std::move(oscSection), 0, 0, 1, 2);
    
    // Frequency knob with better visual feedback
    auto freqKnob = SynthKnobFactory::createFrequencyKnob("Frequency", 0, 0, 85);
    freqKnob->setValueFormatter([](float value) {
        std::stringstream ss;
        if (value >= 1000.0f) {
            ss << std::fixed << std::setprecision(2) << value / 1000.0f << " kHz";
        } else {
            ss << std::fixed << std::setprecision(1) << value << " Hz";
        }
        return ss.str();
    });
    freqKnob->setColor(Color(255, 200, 100)); // Match section color
    freqKnob->setBackgroundColor(Color(40, 40, 45));
    SynthKnob* freqKnobPtr = freqKnob.get();
    oscContainer->addComponent(std::move(freqKnob), 1, 0);
    
    // Wave shape knob
    auto waveKnob = std::make_unique<SynthKnob>("Wave", 0, 0, 85, 0.0f, 4.0f, 0.0f);
    waveKnob->setValueFormatter([](float value) {
        const char* waveNames[] = {"Sine", "Saw", "Square", "Triangle", "Noise"};
        int index = static_cast<int>(value);
        if (index >= 0 && index < 5) {
            return std::string(waveNames[index]);
        }
        return std::string("Unknown");
    });
    waveKnob->setColor(Color(255, 200, 100));
    waveKnob->setBackgroundColor(Color(40, 40, 45));
    SynthKnob* waveKnobPtr = waveKnob.get();
    oscContainer->addComponent(std::move(waveKnob), 1, 1);
    
    mainGrid->addComponent(std::move(oscContainer), 1, 0, 1, 2);
    
    // FILTER SECTION with cool colors
    auto filterContainer = std::make_unique<GridLayout>("filter_grid", 2, 2);
    
    auto filterSection = std::make_unique<Label>("filter_section", "FILTER");
    filterSection->setTextColor(Color(100, 200, 255)); // Cool blue
    filterContainer->addComponent(std::move(filterSection), 0, 0, 1, 2);
    
    // Cutoff with enhanced visuals
    auto cutoffKnob = SynthKnobFactory::createFrequencyKnob("Cutoff", 0, 0, 85);
    cutoffKnob->setValueFormatter([](float value) {
        std::stringstream ss;
        if (value >= 1000.0f) {
            ss << std::fixed << std::setprecision(1) << value / 1000.0f << " kHz";
        } else {
            ss << std::fixed << std::setprecision(0) << value << " Hz";
        }
        return ss.str();
    });
    cutoffKnob->setColor(Color(100, 200, 255));
    cutoffKnob->setBackgroundColor(Color(35, 40, 50));
    cutoffKnob->setModulationColor(Color(0, 255, 200)); // Cyan modulation
    cutoffKnob->setModulationAmount(0.3f); // Show modulation demo
    SynthKnob* cutoffKnobPtr = cutoffKnob.get();
    filterContainer->addComponent(std::move(cutoffKnob), 1, 0);
    
    // Resonance
    auto resKnob = SynthKnobFactory::createResonanceKnob("Resonance", 0, 0, 85);
    resKnob->setValueFormatter([](float value) {
        std::stringstream ss;
        ss << std::fixed << std::setprecision(0) << value * 100.0f << "%";
        return ss.str();
    });
    resKnob->setColor(Color(100, 200, 255));
    resKnob->setBackgroundColor(Color(35, 40, 50));
    SynthKnob* resKnobPtr = resKnob.get();
    filterContainer->addComponent(std::move(resKnob), 1, 1);
    
    mainGrid->addComponent(std::move(filterContainer), 1, 2, 1, 2);
    
    // ENVELOPE SECTION with gradient colors
    auto envContainer = std::make_unique<GridLayout>("env_grid", 2, 4);
    
    auto envSection = std::make_unique<Label>("env_section", "ENVELOPE");
    envSection->setTextColor(Color(200, 150, 255)); // Purple
    envContainer->addComponent(std::move(envSection), 0, 0, 1, 4);
    
    // ADSR knobs with time formatting
    auto attackKnob = SynthKnobFactory::createTimeKnob("Attack", 0, 0, 65, 2.0f);
    attackKnob->setColor(Color(200, 150, 255));
    attackKnob->setBackgroundColor(Color(45, 40, 50));
    
    auto decayKnob = SynthKnobFactory::createTimeKnob("Decay", 0, 0, 65, 2.0f);
    decayKnob->setColor(Color(200, 150, 255));
    decayKnob->setBackgroundColor(Color(45, 40, 50));
    
    auto sustainKnob = SynthKnobFactory::createVolumeKnob("Sustain", 0, 0, 65);
    sustainKnob->setColor(Color(200, 150, 255));
    sustainKnob->setBackgroundColor(Color(45, 40, 50));
    
    auto releaseKnob = SynthKnobFactory::createTimeKnob("Release", 0, 0, 65, 4.0f);
    releaseKnob->setColor(Color(200, 150, 255));
    releaseKnob->setBackgroundColor(Color(45, 40, 50));
    
    envContainer->addComponent(std::move(attackKnob), 1, 0);
    envContainer->addComponent(std::move(decayKnob), 1, 1);
    envContainer->addComponent(std::move(sustainKnob), 1, 2);
    envContainer->addComponent(std::move(releaseKnob), 1, 3);
    
    mainGrid->addComponent(std::move(envContainer), 1, 4, 1, 3);
    
    // MASTER SECTION
    auto masterContainer = std::make_unique<GridLayout>("master_grid", 2, 1);
    
    auto masterSection = std::make_unique<Label>("master_section", "MASTER");
    masterSection->setTextColor(Color(150, 200, 255));
    masterContainer->addComponent(std::move(masterSection), 0, 0);
    
    auto volumeKnob = SynthKnobFactory::createVolumeKnob("Volume", 0, 0, 85);
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
    volumeKnob->setColor(Color(150, 200, 255));
    volumeKnob->setBackgroundColor(Color(40, 45, 50));
    SynthKnob* volumeKnobPtr = volumeKnob.get();
    masterContainer->addComponent(std::move(volumeKnob), 1, 0);
    
    mainGrid->addComponent(std::move(masterContainer), 1, 7, 1, 1);
    
    // ENHANCED VISUALIZATION SECTION
    auto vizSection = std::make_unique<Label>("viz_section", "VISUALIZATION");
    vizSection->setTextColor(Color(255, 200, 100));
    mainGrid->addComponent(std::move(vizSection), 2, 0, 1, 2);
    
    // Waveform display with better colors
    auto waveform = std::make_unique<WaveformVisualizer>("waveform", 512);
    waveform->setWaveformColor(Color(0, 255, 128));
    waveform->setBackgroundColor(Color(20, 20, 25));
    waveform->setGridColor(Color(40, 40, 50));
    WaveformVisualizer* waveformPtr = waveform.get();
    mainGrid->addComponent(std::move(waveform), 2, 0, 1, 2);
    
    // Spectrum analyzer
    auto spectrum = std::make_unique<SpectrumAnalyzer>("spectrum", 32);
    spectrum->setWaveformColor(Color(100, 200, 255));
    spectrum->setBackgroundColor(Color(20, 20, 25));
    mainGrid->addComponent(std::move(spectrum), 2, 2, 1, 2);
    
    // Interactive envelope visualizer
    auto envelope = std::make_unique<EnvelopeVisualizer>("envelope");
    envelope->setADSR(0.01f, 0.1f, 0.7f, 0.5f);
    envelope->setEditable(true);
    envelope->setEnvelopeColor(Color(200, 150, 255));
    envelope->setActiveColor(Color(255, 200, 100));
    envelope->setBackgroundColor(Color(20, 20, 25));
    mainGrid->addComponent(std::move(envelope), 2, 4, 1, 2);
    
    // Level meter with gradient
    auto levelMeter = std::make_unique<LevelMeter>("level", LevelMeter::Orientation::Vertical);
    levelMeter->setMeterColors(
        Color(0, 200, 0),    // Low (green)
        Color(200, 200, 0),  // Mid (yellow)
        Color(255, 60, 60)   // High (red)
    );
    LevelMeter* levelMeterPtr = levelMeter.get();
    mainGrid->addComponent(std::move(levelMeter), 2, 6, 1, 1);
    
    // Display mode info
    auto modeLabel = std::make_unique<Label>("mode_info", "Press SPACE to cycle modes");
    modeLabel->setTextColor(Color(150, 150, 150));
    mainGrid->addComponent(std::move(modeLabel), 2, 7, 1, 1);
    
    // MIDI KEYBOARD
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
    
    // Connect keyboard to synthesizer
    midiKeyboard->setNoteCallback([&synthesizer](int note, int velocity, bool isNoteOn) {
        if (isNoteOn) {
            float normalizedVelocity = velocity / 127.0f;
            synthesizer->noteOn(note, normalizedVelocity);
        } else {
            synthesizer->noteOff(note);
        }
    });
    
    MidiKeyboard* midiKeyboardPtr = midiKeyboard.get();
    mainGrid->addComponent(std::move(midiKeyboard), 4, 0, 1, 6);
    
    // Enhanced keyboard controls
    auto octaveDownButton = std::make_unique<Button>("octave_down", "OCT-");
    octaveDownButton->setBackgroundColor(Color(60, 60, 80));
    octaveDownButton->setTextColor(Color(255, 255, 255));
    octaveDownButton->setHighlightColor(Color(80, 80, 100));
    octaveDownButton->setClickCallback([midiKeyboardPtr]() {
        if (midiKeyboardPtr) {
            midiKeyboardPtr->transposeOctave(-1);
        }
    });
    mainGrid->addComponent(std::move(octaveDownButton), 5, 0);
    
    auto octaveUpButton = std::make_unique<Button>("octave_up", "OCT+");
    octaveUpButton->setBackgroundColor(Color(60, 60, 80));
    octaveUpButton->setTextColor(Color(255, 255, 255));
    octaveUpButton->setHighlightColor(Color(80, 80, 100));
    octaveUpButton->setClickCallback([midiKeyboardPtr]() {
        if (midiKeyboardPtr) {
            midiKeyboardPtr->transposeOctave(1);
        }
    });
    mainGrid->addComponent(std::move(octaveUpButton), 5, 1);
    
    // PRESET BROWSER
    auto presetSection = std::make_unique<Label>("preset_section", "PRESET BROWSER");
    presetSection->setTextColor(Color(150, 255, 200));
    mainGrid->addComponent(std::move(presetSection), 3, 6, 1, 2);
    
    // Initialize preset system
    auto presetManager = std::make_unique<PresetManager>(synthesizer.get());
    auto presetDatabase = std::make_unique<PresetDatabase>();
    
    // Add some test presets
    PresetInfo preset1;
    preset1.name = "Deep Bass";
    preset1.category = "Bass";
    preset1.author = "System";
    presetDatabase->addPreset(preset1);
    
    PresetInfo preset2;
    preset2.name = "Acid Lead";
    preset2.category = "Lead";
    preset2.author = "System";
    presetDatabase->addPreset(preset2);
    
    auto presetBrowser = std::make_unique<PresetBrowserUI>("preset_browser");
    presetBrowser->initialize(presetManager.get(), presetDatabase.get());
    presetBrowser->setParameterManager(&paramManager);
    mainGrid->addComponent(std::move(presetBrowser), 4, 6, 2, 2);
    
    // Connect knobs to synthesizer parameters
    auto connectKnobToParam = [&](SynthKnob* knob, const std::string& paramId) {
        if (knob) {
            parameterKnobs[paramId] = knob;
            knob->setValueChangeCallback([&synthesizer, paramId](float normalizedValue) {
                synthesizer->setParameter(paramId, normalizedValue);
            });
            float currentValue = synthesizer->getParameter(paramId);
            knob->setValue(currentValue);
        }
    };
    
    connectKnobToParam(waveKnobPtr, "oscillator_type");
    connectKnobToParam(resKnobPtr, "filter_resonance");
    connectKnobToParam(volumeKnobPtr, "master_volume");
    
    // Special handling for filter cutoff
    if (cutoffKnobPtr) {
        parameterKnobs["filter_cutoff"] = cutoffKnobPtr;
        cutoffKnobPtr->setValueChangeCallback([&synthesizer](float frequencyHz) {
            float normalized = (std::log(frequencyHz / 20.0f) / std::log(20000.0f / 20.0f));
            normalized = std::max(0.0f, std::min(1.0f, normalized));
            synthesizer->setParameter("filter_cutoff", normalized);
        });
        cutoffKnobPtr->setValue(1000.0f);
    }
    
    // Add grid to screen
    mainScreen->addChild(std::move(mainGrid));
    
    // Add screen to context
    uiContext->addScreen(std::move(mainScreen));
    uiContext->setActiveScreen("main");
    
    // Get pointers to visualization components
    WaveformVisualizer* waveformVisualizerPtr = waveformPtr;
    SpectrumAnalyzer* spectrumPtr = 
        static_cast<SpectrumAnalyzer*>(
            static_cast<GridLayout*>(uiContext->getScreen("main")->getChild("main_grid"))
                ->getChild("spectrum"));
    
    // Set up audio callback
    std::mutex audioMutex;
    audioEngine->setAudioCallback([&](float* outputBuffer, int numFrames) {
        std::lock_guard<std::mutex> lock(audioMutex);
        audioCallback(audioEngine.get(), synthesizer.get(), effectProcessor.get(),
                     sequencer.get(), waveformVisualizerPtr, levelMeterPtr, outputBuffer, numFrames);
    });
    
    // Main loop
    bool running = true;
    auto lastFrameTime = std::chrono::high_resolution_clock::now();
    int displayMode = 0; // Track current display mode
    
    std::cout << "Starting enhanced main loop..." << std::endl;
    std::cout << "Press SPACE to cycle visualization modes" << std::endl;
    std::cout << "Press ESC to exit" << std::endl;
    
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
                    // Cycle visualization modes
                    displayMode = (displayMode + 1) % 4;
                    if (waveformVisualizerPtr) {
                        switch (displayMode) {
                            case 0:
                                waveformVisualizerPtr->setDisplayMode(WaveformVisualizer::DisplayMode::Waveform);
                                std::cout << "Display mode: Waveform" << std::endl;
                                break;
                            case 1:
                                waveformVisualizerPtr->setDisplayMode(WaveformVisualizer::DisplayMode::Spectrum);
                                std::cout << "Display mode: Spectrum" << std::endl;
                                break;
                            case 2:
                                waveformVisualizerPtr->setDisplayMode(WaveformVisualizer::DisplayMode::Waterfall);
                                std::cout << "Display mode: Waterfall" << std::endl;
                                break;
                            case 3:
                                waveformVisualizerPtr->setDisplayMode(WaveformVisualizer::DisplayMode::Lissajous);
                                std::cout << "Display mode: Lissajous (X-Y)" << std::endl;
                                break;
                        }
                    }
                }
            } else {
                InputEvent inputEvent = translateSDLEvent(sdlEvent);
                uiContext->handleInput(inputEvent);
            }
        }
        
        // Update UI
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
        
        // Present frame
        SDL_RenderPresent(renderer);
        
        // Frame rate limiting
        auto frameEnd = std::chrono::high_resolution_clock::now();
        auto frameDuration = std::chrono::duration<float, std::milli>(frameEnd - frameStart).count();
        
        if (frameDuration < 16.67f) { // Target 60 FPS
            SDL_Delay(static_cast<Uint32>(16.67f - frameDuration));
        }
    }
    
    // Cleanup
    std::cout << "Shutting down..." << std::endl;
    
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
    
    std::cout << "Shutdown complete." << std::endl;
    
    return 0;
}