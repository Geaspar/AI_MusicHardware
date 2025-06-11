#include <iostream>
#include <memory>
#include <thread>
#include <chrono>
#include <mutex>
#include <atomic>
#include <iomanip>
#include <sstream>
#include <cmath>
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
#include "../include/ui/DropdownMenu.h"
#include "../include/ui/parameters/ParameterManager.h"
#include "../include/ui/presets/PresetManager.h"
#include "../include/ui/presets/PresetDatabase.h"
#include "../include/midi/MidiCCLearning.h"

// Effects
#include "../include/effects/Filter.h"
#include "../include/effects/AllEffects.h"

// IoT support
// #include "../include/iot/DummyIoTInterface.h" // Disabled to avoid crash

using namespace AIMusicHardware;

// Custom SDL DisplayManager for rendering
class SDLDisplayManager : public DisplayManager {
public:
    SDLDisplayManager(SDL_Renderer* renderer) : renderer_(renderer), width_(1280), height_(800), 
                                             font_(nullptr), fontLarge_(nullptr), fontSmall_(nullptr) {
#ifdef HAVE_SDL_TTF
        // Initialize SDL_ttf
        if (TTF_Init() == -1) {
            std::cerr << "TTF_Init failed: " << TTF_GetError() << std::endl;
        } else {
            // Load fonts in different sizes
            font_ = TTF_OpenFont("/System/Library/Fonts/Helvetica.ttc", 14);          // Normal text
            fontLarge_ = TTF_OpenFont("/System/Library/Fonts/Helvetica.ttc", 18);     // Section headers
            fontSmall_ = TTF_OpenFont("/System/Library/Fonts/Helvetica.ttc", 12);     // Small labels
            
            if (!font_ || !fontLarge_ || !fontSmall_) {
                std::cerr << "Failed to load some fonts: " << TTF_GetError() << std::endl;
            } else {
                std::cout << "SDL_ttf initialized with multiple font sizes" << std::endl;
            }
        }
#endif
    }
    
    ~SDLDisplayManager() {
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
        // Ensure we don't try to use the renderer after SDL cleanup
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
    
    void drawText(int x, int y, const std::string& text, Font* font, const Color& color) override {
        // Check if this looks like a section header based on content
        bool isHeader = (text.find("OSCILLATOR") != std::string::npos ||
                        text.find("FILTER") != std::string::npos ||
                        text.find("ENVELOPE") != std::string::npos ||
                        text.find("MASTER") != std::string::npos ||
                        text.find("VISUALIZATION") != std::string::npos ||
                        text.find("KEYBOARD") != std::string::npos ||
                        text.find("PRESET") != std::string::npos ||
                        text.find("MIDI CC") != std::string::npos ||
                        text.find("TRANSPORT") != std::string::npos ||
                        text.find("PERFORMANCE") != std::string::npos);
        
        // Check if this looks like a small knob label (knobs render their own labels)
        bool isSmallLabel = false;
        
        if (isHeader) {
            drawTextWithSize(x, y, text, color, TextSize::Large);
        } else if (isSmallLabel) {
            drawTextWithSize(x, y, text, color, TextSize::Small);
        } else {
            drawTextWithSize(x, y, text, color, TextSize::Normal);
        }
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
            // Use SDL_ttf for real text rendering
            SDL_Color textColor = { color.r, color.g, color.b, 255 };
            SDL_Surface* textSurface = TTF_RenderText_Solid(selectedFont, text.c_str(), textColor);
            
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

// Audio processing callback for real-time thread
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
    std::cout << "AI Music Hardware - Integrated UI Version" << std::endl;
    std::cout << "Starting production-ready synthesizer..." << std::endl;
    
    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) {
        std::cerr << "SDL initialization failed: " << SDL_GetError() << std::endl;
        return 1;
    }
    
    // Create SDL window
    SDL_Window* window = SDL_CreateWindow(
        "AI Music Hardware - Professional Synthesizer",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        1280, 800,
        SDL_WINDOW_SHOWN
    );
    
    if (!window) {
        std::cerr << "Window creation failed: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return 1;
    }
    
    // Create SDL renderer
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        std::cerr << "Renderer creation failed: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }
    
    // Create core audio components
    auto audioEngine = std::make_unique<AudioEngine>();
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
    
    // Add filter to the external effect processor
    auto filter = std::make_unique<Filter>(audioEngine->getSampleRate(), Filter::Type::LowPass);
    filter->setParameter("mix", 1.0f); // Full wet signal
    filter->setParameter("frequency", 20000.0f); // Start with filter wide open
    filter->setParameter("resonance", 0.7f); // Default resonance
    effectProcessor->addEffect(std::move(filter));
    std::cout << "Added low-pass filter to effect processor chain" << std::endl;
    
    // Initialize hardware (non-critical)
    if (!hardwareInterface->initialize()) {
        std::cerr << "Hardware interface unavailable, continuing without hardware..." << std::endl;
    }
    
    // Create UI context with SDL display manager
    auto uiContext = std::make_unique<UIContext>();
    auto sdlDisplayManager = std::make_shared<SDLDisplayManager>(renderer);
    uiContext->setDisplayManager(sdlDisplayManager);
    uiContext->initialize(1280, 800);
    
    // Initialize parameter manager
    auto& paramManager = EnhancedParameterManager::getInstance();
    // Skip IoT for now to avoid crash
    // paramManager.connectIoTInterface(dummyIoT.get());
    paramManager.connectSynthesizer(synthesizer.get());
    
    // Initialize MIDI CC Learning system
    auto& ccLearning = MidiCCLearningManager::getInstance();
    ccLearning.initialize();
    
    // Create parameter mapping storage for CC learning integration
    std::map<std::string, Slider*> parameterSliders; // Map parameter IDs to sliders
    
    // Set up CC learning to update synthesizer parameters and UI
    ccLearning.getLearning().setParameterChangeCallback([&synthesizer, &parameterSliders](const std::string& parameterId, float value) {
        // Update synthesizer
        synthesizer->setParameter(parameterId, value);
        
        // Update corresponding UI slider if it exists
        auto sliderIt = parameterSliders.find(parameterId);
        if (sliderIt != parameterSliders.end() && sliderIt->second) {
            sliderIt->second->setValue(value);
        }
        
        std::cout << "CC Learning -> " << parameterId << " = " << value << std::endl;
    });
    
    // Create main synthesizer screen first
    auto mainScreen = std::make_unique<Screen>("main");
    
    // Set up learning state callback for UI feedback
    ccLearning.getLearning().setLearningStateCallback([](MidiCCLearning::LearningState state, const std::string& message) {
        std::cout << "Learning State: " << message << std::endl;
        // Status could be displayed in a different tab later
    });
    
    auto connectSliderToParam = [&](Slider* slider, const std::string& paramId) {
        if (slider) {
            // Store slider reference for CC learning updates
            parameterSliders[paramId] = slider;
            
            // Set up value change callback to update synthesizer
            slider->setValueChangeCallback([&synthesizer, paramId](float normalizedValue) {
                // Convert normalized value (0-1) to parameter range and update synthesizer
                synthesizer->setParameter(paramId, normalizedValue);
                std::cout << "Updated " << paramId << " to " << normalizedValue << std::endl;
            });
            
            // Initialize slider with current parameter value
            float currentValue = synthesizer->getParameter(paramId);
            slider->setValue(currentValue);
        }
    };
    
    // Helper to add parameter-specific learning functionality
    auto addParameterLearning = [&](Slider* slider, const std::string& paramId, int x, int y) {
        if (slider) {
            // Add a small learn button next to the knob
            auto learnButton = std::make_unique<Button>("learn_" + paramId, "L");
            learnButton->setPosition(x + 85, y + 30); // Position next to knob
            learnButton->setSize(20, 20);
            learnButton->setBackgroundColor(Color(80, 80, 120));
            learnButton->setTextColor(Color(255, 255, 255));
            learnButton->setClickCallback([&ccLearning, paramId]() {
                auto& learning = ccLearning.getLearning();
                if (learning.getLearningState() == MidiCCLearning::LearningState::Idle) {
                    learning.startLearning(paramId, std::chrono::milliseconds{5000});
                    std::cout << "Started learning for parameter: " << paramId << std::endl;
                } else {
                    learning.stopLearning();
                }
            });
            mainScreen->addChild(std::move(learnButton));
        }
    };
    mainScreen->setBackgroundColor(Color(40, 40, 50)); // Lighter background
    mainScreen->setPosition(0, 0);
    mainScreen->setSize(1280, 800);
    std::cout << "Created main screen" << std::endl;
    
    // Title
    auto titleLabel = std::make_unique<Label>("title", "AI Music Hardware - Professional Synthesizer");
    titleLabel->setPosition(400, 10);
    titleLabel->setSize(400, 30); // Set explicit size
    titleLabel->setTextColor(Color(200, 220, 255));
    mainScreen->addChild(std::move(titleLabel));
    
    // Create oscillator section with bright colors
    auto oscSection = std::make_unique<Label>("osc_section", "OSCILLATOR");
    oscSection->setPosition(50, 40);
    oscSection->setSize(200, 25);
    oscSection->setTextColor(Color(255, 255, 100)); // Bright yellow for visibility
    mainScreen->addChild(std::move(oscSection));
    
    // Create oscillator sliders connected to synthesizer parameters
    auto freqSlider = std::make_unique<Slider>("freq_slider", "Frequency", 50, 85, 40, 100);
    freqSlider->setRange(20.0f, 20000.0f);
    freqSlider->setValue(440.0f);
    freqSlider->setValueFormatter([](float value) {
        std::stringstream ss;
        ss << std::fixed << std::setprecision(1) << value << " Hz";
        return ss.str();
    });
    freqSlider->setColor(Color(255, 255, 100));
    freqSlider->setThumbColor(Color(255, 200, 100));
    Slider* freqSliderPtr = freqSlider.get();
    mainScreen->addChild(std::move(freqSlider));
    
    auto waveSlider = std::make_unique<Slider>("wave_slider", "Wave", 170, 85, 40, 100);
    waveSlider->setRange(0.0f, 4.0f);
    waveSlider->setValue(0.0f);
    waveSlider->setStep(1.0f);
    waveSlider->setValueFormatter([](float value) {
        const char* waveNames[] = {"Sine", "Saw", "Square", "Triangle", "Noise"};
        int index = static_cast<int>(value);
        if (index >= 0 && index < 5) {
            return std::string(waveNames[index]);
        }
        return std::string("Unknown");
    });
    waveSlider->setColor(Color(255, 255, 100));
    waveSlider->setThumbColor(Color(255, 200, 100));
    Slider* waveSliderPtr = waveSlider.get();
    mainScreen->addChild(std::move(waveSlider));
    
    // Create filter section
    auto filterSection = std::make_unique<Label>("filter_section", "FILTER");
    filterSection->setPosition(350, 40);
    filterSection->setSize(180, 25);
    filterSection->setTextColor(Color(100, 255, 100)); // Bright green for visibility
    mainScreen->addChild(std::move(filterSection));
    
    auto cutoffSlider = std::make_unique<Slider>("cutoff_slider", "Cutoff", 350, 85, 40, 100);
    // Use normalized 0-1 range for internal value, will convert to frequency
    cutoffSlider->setRange(0.0f, 1.0f);
    // Set to 0.5 which will map to ~500 Hz
    cutoffSlider->setValue(0.5f);
    cutoffSlider->setValueFormatter([](float normalizedValue) {
        // Convert normalized value to frequency using logarithmic scale
        // 0.0 = 20 Hz, 0.5 = 500 Hz, 1.0 = 20000 Hz
        float minFreq = 20.0f;
        float maxFreq = 20000.0f;
        float logMin = std::log10(minFreq);
        float logMax = std::log10(maxFreq);
        
        // Map so that 0.5 = 500 Hz
        // We need to solve for the curve that passes through (0,20), (0.5,500), (1,20000)
        // Using exponential mapping: freq = 20 * (1000)^normalizedValue
        float freq = minFreq * std::pow(1000.0f, normalizedValue);
        
        std::stringstream ss;
        if (freq >= 1000.0f) {
            ss << std::fixed << std::setprecision(1) << freq / 1000.0f << " kHz";
        } else {
            ss << std::fixed << std::setprecision(0) << freq << " Hz";
        }
        return ss.str();
    });
    cutoffSlider->setColor(Color(100, 255, 100));
    cutoffSlider->setThumbColor(Color(100, 200, 255));
    Slider* cutoffSliderPtr = cutoffSlider.get();
    mainScreen->addChild(std::move(cutoffSlider));
    
    auto resSlider = std::make_unique<Slider>("res_slider", "Resonance", 460, 85, 40, 100);
    resSlider->setRange(0.0f, 1.0f);
    resSlider->setValue(0.5f);
    resSlider->setValueFormatter([](float value) {
        std::stringstream ss;
        ss << std::fixed << std::setprecision(0) << value * 100.0f << "%";
        return ss.str();
    });
    resSlider->setColor(Color(100, 255, 100));
    resSlider->setThumbColor(Color(100, 200, 255));
    Slider* resSliderPtr = resSlider.get();
    mainScreen->addChild(std::move(resSlider));
    
    // Create envelope section  
    auto envSection = std::make_unique<Label>("env_section", "ENVELOPE");
    envSection->setPosition(590, 40);
    envSection->setSize(200, 25);
    envSection->setTextColor(Color(255, 100, 255)); // Bright magenta for visibility
    mainScreen->addChild(std::move(envSection));
    
    auto attackSlider = std::make_unique<Slider>("attack_slider", "Attack", 590, 85, 40, 100);
    attackSlider->setRange(0.0f, 2.0f);
    attackSlider->setValue(0.01f);
    attackSlider->setValueFormatter([](float value) {
        std::stringstream ss;
        if (value < 0.1f) {
            ss << std::fixed << std::setprecision(0) << value * 1000.0f << " ms";
        } else {
            ss << std::fixed << std::setprecision(2) << value << " s";
        }
        return ss.str();
    });
    attackSlider->setColor(Color(255, 100, 255));
    attackSlider->setThumbColor(Color(255, 150, 255));
    Slider* attackSliderPtr = attackSlider.get();
    mainScreen->addChild(std::move(attackSlider));
    
    auto decaySlider = std::make_unique<Slider>("decay_slider", "Decay", 680, 85, 40, 100);
    decaySlider->setRange(0.0f, 2.0f);
    decaySlider->setValue(0.1f);
    decaySlider->setValueFormatter([](float value) {
        std::stringstream ss;
        if (value < 0.1f) {
            ss << std::fixed << std::setprecision(0) << value * 1000.0f << " ms";
        } else {
            ss << std::fixed << std::setprecision(2) << value << " s";
        }
        return ss.str();
    });
    decaySlider->setColor(Color(255, 100, 255));
    decaySlider->setThumbColor(Color(255, 150, 255));
    Slider* decaySliderPtr = decaySlider.get();
    mainScreen->addChild(std::move(decaySlider));
    
    auto sustainSlider = std::make_unique<Slider>("sustain_slider", "Sustain", 770, 85, 40, 100);
    sustainSlider->setRange(0.0f, 1.0f);
    sustainSlider->setValue(0.7f);
    sustainSlider->setValueFormatter([](float value) {
        std::stringstream ss;
        ss << std::fixed << std::setprecision(0) << value * 100.0f << "%";
        return ss.str();
    });
    sustainSlider->setColor(Color(255, 100, 255));
    sustainSlider->setThumbColor(Color(255, 150, 255));
    Slider* sustainSliderPtr = sustainSlider.get();
    mainScreen->addChild(std::move(sustainSlider));
    
    auto releaseSlider = std::make_unique<Slider>("release_slider", "Release", 860, 85, 40, 100);
    releaseSlider->setRange(0.0f, 4.0f);
    releaseSlider->setValue(0.5f);
    releaseSlider->setValueFormatter([](float value) {
        std::stringstream ss;
        if (value < 0.1f) {
            ss << std::fixed << std::setprecision(0) << value * 1000.0f << " ms";
        } else {
            ss << std::fixed << std::setprecision(2) << value << " s";
        }
        return ss.str();
    });
    releaseSlider->setColor(Color(255, 100, 255));
    releaseSlider->setThumbColor(Color(255, 150, 255));
    Slider* releaseSliderPtr = releaseSlider.get();
    mainScreen->addChild(std::move(releaseSlider));
    
    // Create master section
    auto masterSection = std::make_unique<Label>("master_section", "MASTER");
    masterSection->setPosition(980, 40);
    masterSection->setSize(130, 25);
    masterSection->setTextColor(Color(100, 200, 255)); // Bright cyan for visibility
    mainScreen->addChild(std::move(masterSection));
    
    auto volumeSlider = std::make_unique<Slider>("volume_slider", "Volume", 980, 85, 40, 100);
    volumeSlider->setRange(0.0f, 1.0f);
    volumeSlider->setValue(0.75f);
    volumeSlider->setValueFormatter([](float value) {
        std::stringstream ss;
        if (value == 0.0f) {
            ss << "-∞ dB";
        } else {
            float db = 20.0f * std::log10(value);
            ss << std::fixed << std::setprecision(1) << db << " dB";
        }
        return ss.str();
    });
    volumeSlider->setColor(Color(100, 200, 255));
    volumeSlider->setThumbColor(Color(150, 200, 255));
    Slider* volumeSliderPtr = volumeSlider.get();
    mainScreen->addChild(std::move(volumeSlider));
    
    // Create visualization section
    auto vizSection = std::make_unique<Label>("viz_section", "VISUALIZATION");
    vizSection->setPosition(50, 220);
    vizSection->setSize(200, 25);
    vizSection->setTextColor(Color(255, 200, 100)); // Bright orange for visibility
    mainScreen->addChild(std::move(vizSection));
    
    auto waveform = std::make_unique<WaveformVisualizer>("waveform", 512);
    waveform->setPosition(50, 250);
    waveform->setSize(220, 150);
    waveform->setWaveformColor(Color(0, 255, 128));
    mainScreen->addChild(std::move(waveform));
    
    // Create filter visualizer (Vital-style) - between waveform and envelope
    auto filterViz = std::make_unique<FilterVisualizer>("filter_viz");
    filterViz->setPosition(280, 250);
    filterViz->setSize(300, 150);
    filterViz->setCurveColor(Color(100, 255, 100));
    filterViz->setFillColor(Color(100, 255, 100, 50));
    filterViz->setBackgroundColor(Color(30, 30, 35));
    filterViz->setGridColor(Color(50, 50, 55));
    filterViz->showGrid(true);
    filterViz->showFill(true);
    filterViz->setEditable(true);
    filterViz->setFilterType(FilterVisualizer::FilterType::LowPass);
    filterViz->setCutoffFrequency(1000.0f);
    filterViz->setResonance(0.7f);
    filterViz->setSampleRate(audioEngine->getSampleRate());
    FilterVisualizer* filterVizPtr = filterViz.get();
    mainScreen->addChild(std::move(filterViz));
    
    auto envelope = std::make_unique<EnvelopeVisualizer>("envelope");
    envelope->setPosition(590, 250);
    envelope->setSize(250, 150);
    // Match initial values with slider defaults
    envelope->setADSR(0.01f, 0.1f, 0.7f, 0.5f);
    envelope->setEditable(true);
    EnvelopeVisualizer* envelopePtr = envelope.get();
    mainScreen->addChild(std::move(envelope));
    
    auto levelMeter = std::make_unique<LevelMeter>("level", LevelMeter::Orientation::Vertical);
    levelMeter->setPosition(850, 250);
    levelMeter->setSize(30, 150);
    mainScreen->addChild(std::move(levelMeter));
    
    // Create MIDI keyboard section
    auto keyboardSection = std::make_unique<Label>("keyboard_section", "MIDI KEYBOARD");
    keyboardSection->setPosition(50, 430);
    keyboardSection->setTextColor(Color(255, 150, 255)); // Bright pink for visibility
    mainScreen->addChild(std::move(keyboardSection));
    
    // Create the MIDI keyboard
    auto midiKeyboard = std::make_unique<MidiKeyboard>("midi_keyboard", 50, 460);
    
    // Configure keyboard for 3 octaves starting from C3
    MidiKeyboard::KeyboardConfig keyboardConfig;
    keyboardConfig.startOctave = 3;     // C3 - C6 range
    keyboardConfig.numOctaves = 3;
    keyboardConfig.whiteKeyWidth = 28;   // Slightly larger keys
    keyboardConfig.whiteKeyHeight = 140;
    keyboardConfig.blackKeyWidth = 20;
    keyboardConfig.blackKeyHeight = 90;
    keyboardConfig.whiteKeyColor = Color(250, 250, 250);
    keyboardConfig.blackKeyColor = Color(30, 30, 30);
    keyboardConfig.pressedWhiteColor = Color(100, 150, 255);
    keyboardConfig.pressedBlackColor = Color(80, 120, 200);
    keyboardConfig.keyBorderColor = Color(120, 120, 120);
    
    midiKeyboard->setConfig(keyboardConfig);
    midiKeyboard->setVelocityRange(30, 127);  // More expressive velocity range
    
    // Connect keyboard to synthesizer
    midiKeyboard->setNoteCallback([&synthesizer, &audioEngine](int note, int velocity, bool isNoteOn) {
        if (isNoteOn) {
            float normalizedVelocity = velocity / 127.0f;
            std::cout << "Keyboard Note On: " << MidiKeyboard::getNoteName(note) 
                      << " (note " << note << ") velocity " << velocity 
                      << " normalized: " << normalizedVelocity << std::endl;
            
            // Check audio engine status
            std::cout << "Audio Engine - Sample Rate: " << audioEngine->getSampleRate() 
                      << ", Buffer Size: " << audioEngine->getBufferSize() 
                      << ", Stream Time: " << audioEngine->getStreamTime() << std::endl;
            
            synthesizer->noteOn(note, normalizedVelocity);
            
            // Check synthesizer state
            std::cout << "Master Volume: " << synthesizer->getParameter("master_volume") << std::endl;
            std::cout << "Filter Cutoff: " << synthesizer->getParameter("filter_cutoff") << std::endl;
            std::cout << "Oscillator Type: " << synthesizer->getParameter("oscillator_type") << std::endl;
            
            // Try setting reasonable filter cutoff if it's too low
            if (synthesizer->getParameter("filter_cutoff") < 0.3f) {
                std::cout << "Filter cutoff too low, setting to 0.5 (mid-range)" << std::endl;
                synthesizer->setParameter("filter_cutoff", 0.5f);
            }
        } else {
            synthesizer->noteOff(note);
            std::cout << "Keyboard Note Off: " << MidiKeyboard::getNoteName(note) 
                      << " (note " << note << ")" << std::endl;
        }
    });
    
    // Store pointer for potential external MIDI input display
    MidiKeyboard* midiKeyboardPtr = midiKeyboard.get();
    mainScreen->addChild(std::move(midiKeyboard));
    
    // Add octave control buttons (positioned after keyboard ends)
    auto octaveDownButton = std::make_unique<Button>("octave_down", "OCT-");
    octaveDownButton->setPosition(50, 610);
    octaveDownButton->setSize(60, 30);
    octaveDownButton->setBackgroundColor(Color(80, 80, 100));
    octaveDownButton->setTextColor(Color(255, 255, 255));
    octaveDownButton->setClickCallback([midiKeyboardPtr]() {
        if (midiKeyboardPtr) {
            midiKeyboardPtr->transposeOctave(-1);
            std::cout << "Keyboard transposed down one octave" << std::endl;
        }
    });
    mainScreen->addChild(std::move(octaveDownButton));
    
    auto octaveUpButton = std::make_unique<Button>("octave_up", "OCT+");
    octaveUpButton->setPosition(120, 610);
    octaveUpButton->setSize(60, 30);
    octaveUpButton->setBackgroundColor(Color(80, 80, 100));
    octaveUpButton->setTextColor(Color(255, 255, 255));
    octaveUpButton->setClickCallback([midiKeyboardPtr]() {
        if (midiKeyboardPtr) {
            midiKeyboardPtr->transposeOctave(1);
            std::cout << "Keyboard transposed up one octave" << std::endl;
        }
    });
    mainScreen->addChild(std::move(octaveUpButton));
    
    // Add velocity mode button
    auto velocityModeButton = std::make_unique<Button>("velocity_mode", "VEL: VAR");
    velocityModeButton->setPosition(190, 610);
    velocityModeButton->setSize(130, 30);
    velocityModeButton->setBackgroundColor(Color(60, 100, 60));
    velocityModeButton->setTextColor(Color(255, 255, 255));
    velocityModeButton->setToggleMode(true);
    
    // Store raw pointer for callback
    Button* velocityModeButtonPtr = velocityModeButton.get();
    
    bool isFixedVelocity = false;
    velocityModeButton->setClickCallback([midiKeyboardPtr, &isFixedVelocity, velocityModeButtonPtr]() {
        if (midiKeyboardPtr) {
            isFixedVelocity = !isFixedVelocity;
            if (isFixedVelocity) {
                midiKeyboardPtr->setFixedVelocity(100);  // Fixed velocity
                velocityModeButtonPtr->setText("VEL: FIX");
                velocityModeButtonPtr->setBackgroundColor(Color(100, 60, 60));
                std::cout << "Keyboard set to fixed velocity mode" << std::endl;
            } else {
                midiKeyboardPtr->setFixedVelocity(0);    // Variable velocity
                velocityModeButtonPtr->setText("VEL: VAR");
                velocityModeButtonPtr->setBackgroundColor(Color(60, 100, 60));
                std::cout << "Keyboard set to variable velocity mode" << std::endl;
            }
        }
    });
    mainScreen->addChild(std::move(velocityModeButton));
    
    // Create preset selection section (moved to after performance info)
    // Will be added later at bottom right
    
    // Initialize preset system
    auto presetManager = std::make_unique<PresetManager>(synthesizer.get());
    auto presetDatabase = std::make_unique<PresetDatabase>();
    
    // Load real presets from test_presets directory
    std::string presetBaseDir = "test_presets";
    std::vector<std::string> categories = {"Bass", "Lead", "Pad"};
    
    for (const auto& category : categories) {
        std::string categoryDir = presetBaseDir + "/" + category;
        
        // Add presets from each category directory
        // In a real implementation, we'd scan the directory for .json files
        // For now, add the known presets manually
        if (category == "Bass") {
            PresetInfo preset;
            preset.name = "Deep Bass";
            preset.category = "Bass";
            preset.author = "System";
            preset.description = "Deep sub bass sound";
            preset.filePath = categoryDir + "/Deep Bass.json";
            presetDatabase->addPreset(preset);
            
            preset.name = "Pluck Bass";
            preset.description = "Percussive pluck bass";
            preset.filePath = categoryDir + "/Pluck Bass.json";
            presetDatabase->addPreset(preset);
            
            preset.name = "Sub Bass";
            preset.description = "Powerful sub-bass sound";
            preset.filePath = categoryDir + "/Sub Bass.json";
            presetDatabase->addPreset(preset);
        }
        else if (category == "Lead") {
            PresetInfo preset;
            preset.name = "Acid Lead";
            preset.category = "Lead";
            preset.author = "Alex Johnson";
            preset.description = "Classic acid lead synthesizer";
            preset.filePath = categoryDir + "/Acid Lead.json";
            presetDatabase->addPreset(preset);
            
            preset.name = "Bright Lead";
            preset.description = "Cutting lead synthesizer";
            preset.filePath = categoryDir + "/Bright Lead.json";
            presetDatabase->addPreset(preset);
            
            preset.name = "Warm Lead";
            preset.description = "Warm analog lead sound";
            preset.filePath = categoryDir + "/Warm Lead.json";
            presetDatabase->addPreset(preset);
        }
        else if (category == "Pad") {
            PresetInfo preset;
            preset.name = "Ambient Pad";
            preset.category = "Pad";
            preset.author = "System";
            preset.description = "Atmospheric pad sound";
            preset.filePath = categoryDir + "/Ambient Pad.json";
            presetDatabase->addPreset(preset);
            
            preset.name = "Lush Pad";
            preset.description = "Rich, lush pad sound";
            preset.filePath = categoryDir + "/Lush Pad.json";
            presetDatabase->addPreset(preset);
            
            preset.name = "String Pad";
            preset.description = "String-like pad sound";
            preset.filePath = categoryDir + "/String Pad.json";
            presetDatabase->addPreset(preset);
        }
    }
    
    // Preset dropdown will be created later at bottom right
    
    // Create MODULATION section
    auto modSection = std::make_unique<Label>("mod_section", "MODULATION ROUTING");
    modSection->setPosition(850, 430);
    modSection->setTextColor(Color(200, 150, 255)); // Purple for modulation
    mainScreen->addChild(std::move(modSection));
    
    // Create modulation routing rows
    const int modRowStartY = 460;
    const int modRowHeight = 35;
    const int modRowCount = 3;
    
    // Modulation sources and destinations
    std::vector<std::string> modSources = {
        "None", "LFO 1", "LFO 2", "Envelope", "Velocity", "Aftertouch", "Mod Wheel"
    };
    
    std::vector<std::string> modDestinations = {
        "None", "Pitch", "Filter Cutoff", "Filter Res", "Volume", "Pan", "Attack", "Release"
    };
    
    // Store dropdown references to add them last (for proper z-order)
    std::vector<std::unique_ptr<DropdownMenu>> modSourceDropdowns;
    std::vector<std::unique_ptr<DropdownMenu>> modDestDropdowns;
    
    for (int i = 0; i < modRowCount; ++i) {
        int yPos = modRowStartY + (i * modRowHeight);
        
        // Create source dropdown but don't add yet
        auto sourceDropdown = std::make_unique<DropdownMenu>("mod_source_" + std::to_string(i), "Source");
        sourceDropdown->setPosition(850, yPos);
        sourceDropdown->setSize(120, 25);
        sourceDropdown->addItems(modSources);
        sourceDropdown->setSelectionCallback([i](int index, const std::string& item) {
            std::cout << "Mod " << i << " source: " << item << std::endl;
        });
        modSourceDropdowns.push_back(std::move(sourceDropdown));
        
        // Amount slider
        auto amountSlider = std::make_unique<Slider>("mod_amount_" + std::to_string(i), "", 980, yPos, 80, 25);
        amountSlider->setOrientation(Slider::Orientation::Horizontal);
        amountSlider->setRange(-1.0f, 1.0f);
        amountSlider->setValue(0.0f);
        amountSlider->setValueFormatter([](float value) {
            std::stringstream ss;
            ss << std::fixed << std::setprecision(0) << (value * 100.0f) << "%";
            return ss.str();
        });
        amountSlider->setColor(Color(200, 150, 255));
        amountSlider->setThumbColor(Color(220, 170, 255));
        mainScreen->addChild(std::move(amountSlider));
        
        // Create destination dropdown but don't add yet
        auto destDropdown = std::make_unique<DropdownMenu>("mod_dest_" + std::to_string(i), "Destination");
        destDropdown->setPosition(1070, yPos);
        destDropdown->setSize(130, 25);
        destDropdown->addItems(modDestinations);
        destDropdown->setSelectionCallback([i](int index, const std::string& item) {
            std::cout << "Mod " << i << " destination: " << item << std::endl;
        });
        modDestDropdowns.push_back(std::move(destDropdown));
    }
    
    // Create EFFECTS CHAIN section
    auto effectsSection = std::make_unique<Label>("effects_section", "EFFECTS CHAIN");
    effectsSection->setPosition(850, 570);
    effectsSection->setTextColor(Color(100, 200, 200)); // Cyan for effects
    mainScreen->addChild(std::move(effectsSection));
    
    // Effects chain slots
    const int effectsStartY = 600;
    const int effectSlotHeight = 35;
    const int effectSlotCount = 3;
    
    std::vector<std::string> effectTypes = {
        "None", "Reverb", "Delay", "Chorus", "Phaser", "Distortion", "Compressor", "EQ"
    };
    
    // Store effect dropdowns to add them last
    std::vector<std::unique_ptr<DropdownMenu>> effectDropdowns;
    
    // Store mix slider pointers for bypass button callbacks
    std::vector<Slider*> effectMixSliders;
    
    for (int i = 0; i < effectSlotCount; ++i) {
        int yPos = effectsStartY + (i * effectSlotHeight);
        
        // Create effect type dropdown but don't add yet
        auto effectDropdown = std::make_unique<DropdownMenu>("effect_type_" + std::to_string(i), "Effect " + std::to_string(i + 1));
        effectDropdown->setPosition(850, yPos);
        effectDropdown->setSize(150, 25);
        effectDropdown->addItems(effectTypes);
        effectDropdown->setSelectionCallback([i, &effectProcessor, &audioEngine](int index, const std::string& item) {
            std::cout << "Effect slot " << i << ": " << item << std::endl;
            
            if (item != "None" && audioEngine) {
                // Map UI names to effect types
                std::string effectType;
                if (item == "Reverb") effectType = "Reverb";
                else if (item == "Delay") effectType = "Delay";
                else if (item == "Chorus") effectType = "Modulation";
                else if (item == "Phaser") effectType = "Phaser";
                else if (item == "Distortion") effectType = "Distortion";
                else if (item == "Compressor") effectType = "Compressor";
                else if (item == "EQ") effectType = "EQ";
                
                if (!effectType.empty()) {
                    try {
                        // Create the effect
                        auto effect = createEffectComplete(effectType, audioEngine->getSampleRate());
                        if (effect) {
                            // Set safe default parameters based on effect type
                            if (effectType == "Reverb") {
                                // Reverb uses wetLevel/dryLevel instead of mix
                                // Larger room size and less damping for longer decay
                                effect->setParameter("roomSize", 0.85f);    // Larger room for longer decay
                                effect->setParameter("damping", 0.2f);      // Less damping = longer decay
                                effect->setParameter("wetLevel", 0.3f);     // 30% wet signal
                                effect->setParameter("dryLevel", 0.7f);     // 70% dry signal
                                effect->setParameter("width", 1.0f);        // Full stereo width
                            } else if (effectType == "Phaser") {
                                // Set safe phaser parameters
                                effect->setParameter("rate", 0.5f);
                                effect->setParameter("depth", 0.5f);
                                effect->setParameter("feedback", 0.2f);  // Lower feedback to prevent instability
                                effect->setParameter("mix", 0.5f);
                                effect->setParameter("stages", 4.0f);
                            } else if (effectType == "Distortion") {
                                // Distortion needs specific parameters
                                effect->setParameter("drive", 5.0f);    // More noticeable drive
                                effect->setParameter("level", 0.5f);    // Moderate output level
                                effect->setParameter("tone", 0.5f);     // Neutral tone
                                effect->setParameter("mix", 0.8f);      // 80% wet for noticeable effect
                                effect->setParameter("type", 0.0f);     // Soft clipping
                            } else {
                                // Most effects use "mix" parameter
                                effect->setParameter("mix", 0.5f);
                            }
                            
                            // For now, we'll just support one effect at a time
                            // Remove all effects after the filter
                            while (effectProcessor->getNumEffects() > 1) {
                                effectProcessor->removeEffect(effectProcessor->getNumEffects() - 1);
                            }
                            
                            // Add the new effect
                            effectProcessor->addEffect(std::move(effect));
                            std::cout << "Added " << effectType << " (total effects: " << effectProcessor->getNumEffects() << ")" << std::endl;
                            
                            // Note: Mix slider will have its default value of 0.5
                            // We don't need to update it here as it could cause crashes
                        } else {
                            std::cerr << "Failed to create effect: " << effectType << std::endl;
                        }
                    } catch (const std::exception& e) {
                        std::cerr << "Error creating effect " << effectType << ": " << e.what() << std::endl;
                    } catch (...) {
                        std::cerr << "Unknown error creating effect " << effectType << std::endl;
                    }
                }
            } else if (item == "None") {
                // Remove effects from this slot onwards
                while (effectProcessor->getNumEffects() > i + 1) {
                    effectProcessor->removeEffect(effectProcessor->getNumEffects() - 1);
                }
                std::cout << "Removed effect from slot " << i << std::endl;
            }
        });
        effectDropdowns.push_back(std::move(effectDropdown));
        
        // Mix/Dry-Wet slider
        auto mixSlider = std::make_unique<Slider>("effect_mix_" + std::to_string(i), "", 1010, yPos, 80, 25);
        mixSlider->setOrientation(Slider::Orientation::Horizontal);
        mixSlider->setRange(0.0f, 1.0f);
        mixSlider->setValue(0.5f);
        mixSlider->setValueFormatter([](float value) {
            std::stringstream ss;
            ss << "Mix " << std::fixed << std::setprecision(0) << (value * 100.0f) << "%";
            return ss.str();
        });
        mixSlider->setColor(Color(100, 200, 200));
        mixSlider->setThumbColor(Color(120, 220, 220));
        mixSlider->setValueChangeCallback([i, &effectProcessor](float value) {
            // Since we only support one effect at a time, it's always at index 1
            if (effectProcessor->getNumEffects() > 1) {
                if (auto* effect = effectProcessor->getEffect(1)) {
                    std::cout << "Setting mix for " << effect->getName() << " to " << (value * 100.0f) << "%" << std::endl;
                    
                    // Handle different parameter names for different effects
                    if (effect->getName() == "Reverb") {
                        // Reverb uses wetLevel/dryLevel
                        effect->setParameter("wetLevel", value);
                        effect->setParameter("dryLevel", 1.0f - value);
                    } else {
                        // Most effects use "mix"
                        effect->setParameter("mix", value);
                    }
                } else {
                    std::cout << "No effect at index 1 to set mix for" << std::endl;
                }
            } else {
                std::cout << "Only " << effectProcessor->getNumEffects() << " effects in chain (need > 1)" << std::endl;
            }
        });
        
        // Store pointer before moving unique_ptr
        Slider* mixSliderPtr = mixSlider.get();
        effectMixSliders.push_back(mixSliderPtr);
        mainScreen->addChild(std::move(mixSlider));
        
        // Bypass button
        auto bypassButton = std::make_unique<Button>("effect_bypass_" + std::to_string(i), "ON");
        bypassButton->setPosition(1100, yPos);
        bypassButton->setSize(40, 25);
        bypassButton->setToggleMode(true);
        bypassButton->setBackgroundColor(Color(50, 100, 50));
        bypassButton->setTextColor(Color(255, 255, 255));
        bypassButton->setClickCallback([i, &effectProcessor, bypassButton = bypassButton.get(), mixSliderPtr]() {
            bool bypassed = bypassButton->getText() == "OFF";
            bypassButton->setText(bypassed ? "ON" : "OFF");
            bypassButton->setBackgroundColor(bypassed ? Color(50, 100, 50) : Color(100, 50, 50));
            
            std::cout << "Bypass button clicked for slot " << i << ", bypassed=" << !bypassed << std::endl;
            
            // Since we only support one effect at a time, it's always at index 1
            if (effectProcessor->getNumEffects() > 1) {
                if (auto* effect = effectProcessor->getEffect(1)) {
                    float mixValue = bypassed ? (mixSliderPtr ? mixSliderPtr->getValue() : 0.5f) : 0.0f;
                    
                    std::cout << "Bypassing " << effect->getName() << ", setting mix to " << mixValue << std::endl;
                    
                    // Handle different parameter names for different effects
                    if (effect->getName() == "Reverb") {
                        if (!bypassed) {
                            // Bypass: all dry
                            effect->setParameter("wetLevel", 0.0f);
                            effect->setParameter("dryLevel", 1.0f);
                        } else {
                            // Restore: use mix value
                            effect->setParameter("wetLevel", mixValue);
                            effect->setParameter("dryLevel", 1.0f - mixValue);
                        }
                    } else {
                        // Most effects use "mix"
                        effect->setParameter("mix", mixValue);
                    }
                    std::cout << "Effect " << effect->getName() << " " << (bypassed ? "enabled" : "bypassed") << std::endl;
                } else {
                    std::cout << "No effect at index 1 to bypass" << std::endl;
                }
            } else {
                std::cout << "Only " << effectProcessor->getNumEffects() << " effects in chain (need > 1)" << std::endl;
            }
        });
        mainScreen->addChild(std::move(bypassButton));
    }
    
    // Get pointers to visualization components for audio thread
    WaveformVisualizer* waveformPtr = 
        static_cast<WaveformVisualizer*>(mainScreen->getChild("waveform"));
    LevelMeter* levelPtr = 
        static_cast<LevelMeter*>(mainScreen->getChild("level"));
    
    // Connect UI controls to synthesizer parameters
    std::cout << "Connecting UI controls to synthesizer parameters..." << std::endl;
    
    // Note: For oscillator frame parameter, we need special handling since it uses normalized values
    connectSliderToParam(waveSliderPtr, "oscillator_type");
    
    // Add flags to prevent feedback loops
    std::atomic<bool> updatingFromSlider{false};
    std::atomic<bool> updatingFromVisualizer{false};
    
    // Special handling for filter cutoff - connect directly to effect processor
    if (cutoffSliderPtr) {
        parameterSliders["filter_cutoff"] = cutoffSliderPtr;
        cutoffSliderPtr->setValueChangeCallback([&effectProcessor, filterVizPtr, &updatingFromSlider, &updatingFromVisualizer](float normalizedValue) {
            if (updatingFromVisualizer) {
                std::cout << "CUTOFF SLIDER: Ignoring update from visualizer" << std::endl;
                return; // Prevent feedback loop
            }
            
            updatingFromSlider = true;
            
            // Convert normalized value to frequency
            // freq = 20 * (1000)^normalizedValue gives us 20Hz at 0, 500Hz at 0.5, 20kHz at 1
            float frequencyHz = 20.0f * std::pow(1000.0f, normalizedValue);
            
            std::cout << "CUTOFF SLIDER: Normalized " << normalizedValue << " -> " << frequencyHz << " Hz" << std::endl;
            
            // Update filter in effect processor (assume it's the first effect)
            if (effectProcessor->getNumEffects() > 0) {
                if (auto* filter = effectProcessor->getEffect(0)) {
                    filter->setParameter("frequency", frequencyHz);
                    std::cout << "CUTOFF SLIDER: Updated filter cutoff parameter to " << frequencyHz << " Hz" << std::endl;
                }
            }
            // Update visualizer
            if (filterVizPtr) {
                filterVizPtr->setCutoffFrequency(frequencyHz);
                std::cout << "CUTOFF SLIDER: Updated visualizer cutoff to " << frequencyHz << " Hz" << std::endl;
            }
            
            updatingFromSlider = false;
        });
        // Initialize with 500 Hz (normalized 0.5)
        cutoffSliderPtr->setValue(0.5f);
    }
    
    // Special handling for filter resonance - connect directly to effect processor
    if (resSliderPtr) {
        parameterSliders["filter_resonance"] = resSliderPtr;
        resSliderPtr->setValueChangeCallback([&effectProcessor, filterVizPtr, &updatingFromSlider, &updatingFromVisualizer](float resonanceValue) {
            if (updatingFromVisualizer) {
                std::cout << "RESONANCE SLIDER: Ignoring update from visualizer" << std::endl;
                return; // Prevent feedback loop
            }
            
            updatingFromSlider = true;
            
            std::cout << "RESONANCE SLIDER: Value changed to " << resonanceValue << " (normalized)" << std::endl;
            
            // Update filter in effect processor (assume it's the first effect)
            if (effectProcessor->getNumEffects() > 0) {
                if (auto* filter = effectProcessor->getEffect(0)) {
                    // Map 0-1 to reasonable resonance range (0.7-10)
                    float resonance = 0.7f + resonanceValue * 9.3f;
                    filter->setParameter("resonance", resonance);
                    std::cout << "RESONANCE SLIDER: Updated filter resonance parameter to " << resonance << std::endl;
                    
                    // Update visualizer with actual resonance value
                    if (filterVizPtr) {
                        filterVizPtr->setResonance(resonance);
                        std::cout << "RESONANCE SLIDER: Updated visualizer resonance to " << resonance << std::endl;
                    }
                }
            }
            
            updatingFromSlider = false;
        });
        // Initialize with default resonance
        resSliderPtr->setValue(0.5f);
    }
    
    // Connect filter visualizer to update both effect processor and sliders
    if (filterVizPtr) {
        filterVizPtr->setParameterChangeCallback([&effectProcessor, cutoffSliderPtr, resSliderPtr, &updatingFromSlider, &updatingFromVisualizer](float cutoff, float resonance) {
            if (updatingFromSlider) {
                std::cout << "FILTER VIZ: Ignoring callback - update came from slider" << std::endl;
                return; // Prevent feedback loop
            }
            
            updatingFromVisualizer = true;
            
            std::cout << "FILTER VIZ: Dragged to cutoff=" << cutoff << " Hz, resonance=" << resonance << std::endl;
            
            // Update filter in effect processor
            if (effectProcessor->getNumEffects() > 0) {
                if (auto* filter = effectProcessor->getEffect(0)) {
                    filter->setParameter("frequency", cutoff);
                    filter->setParameter("resonance", resonance);
                    std::cout << "FILTER VIZ: Updated effect processor" << std::endl;
                }
            }
            
            // Update sliders
            if (cutoffSliderPtr) {
                // Convert frequency back to normalized value
                // freq = 20 * (1000)^normalizedValue
                // normalizedValue = log(freq/20) / log(1000)
                float normalizedValue = std::log(cutoff / 20.0f) / std::log(1000.0f);
                normalizedValue = std::max(0.0f, std::min(1.0f, normalizedValue));
                cutoffSliderPtr->setValue(normalizedValue);
                std::cout << "FILTER VIZ: Updated cutoff slider to " << normalizedValue << " (normalized)" << std::endl;
            }
            if (resSliderPtr) {
                // Convert resonance back to 0-1 range for slider
                float sliderValue = (resonance - 0.7f) / 9.3f;
                resSliderPtr->setValue(sliderValue);
                std::cout << "FILTER VIZ: Updated resonance slider to " << sliderValue << " (normalized)" << std::endl;
            }
            
            updatingFromVisualizer = false;
        });
    }
    
    connectSliderToParam(volumeSliderPtr, "master_volume");
    
    // Add CC learning buttons for each parameter
    addParameterLearning(waveSliderPtr, "oscillator_type", 170, 85);
    addParameterLearning(cutoffSliderPtr, "filter_cutoff", 350, 85);
    addParameterLearning(resSliderPtr, "filter_resonance", 460, 85);
    addParameterLearning(volumeSliderPtr, "master_volume", 980, 85);
    
    // Connect envelope parameters
    connectSliderToParam(attackSliderPtr, "envelope_attack");
    connectSliderToParam(decaySliderPtr, "envelope_decay");
    connectSliderToParam(sustainSliderPtr, "envelope_sustain");
    connectSliderToParam(releaseSliderPtr, "envelope_release");
    
    // Connect envelope visualizer to update synthesizer and sliders
    if (envelopePtr) {
        envelopePtr->setParameterChangeCallback([&synthesizer, attackSliderPtr, decaySliderPtr, 
                                                 sustainSliderPtr, releaseSliderPtr]
                                                (float attack, float decay, float sustain, float release) {
            // Update synthesizer parameters
            synthesizer->setParameter("envelope_attack", attack);
            synthesizer->setParameter("envelope_decay", decay);
            synthesizer->setParameter("envelope_sustain", sustain);
            synthesizer->setParameter("envelope_release", release);
            
            // Update sliders to reflect the new values
            if (attackSliderPtr) attackSliderPtr->setValue(attack);
            if (decaySliderPtr) decaySliderPtr->setValue(decay);
            if (sustainSliderPtr) sustainSliderPtr->setValue(sustain);
            if (releaseSliderPtr) releaseSliderPtr->setValue(release);
            
            std::cout << "Envelope updated from visualizer - A:" << attack 
                      << " D:" << decay << " S:" << sustain << " R:" << release << std::endl;
        });
    }
    
    // Also update visualizer when sliders change
    // We need to modify the connectSliderToParam lambda to also update the visualizer
    auto connectSliderToEnvelope = [&](Slider* slider, const std::string& paramId, 
                                      EnvelopeVisualizer* envViz,
                                      Slider* attackSlider, Slider* decaySlider, 
                                      Slider* sustainSlider, Slider* releaseSlider) {
        if (slider) {
            // Store slider reference for CC learning updates
            parameterSliders[paramId] = slider;
            
            // Set up value change callback to update both synthesizer and visualizer
            slider->setValueChangeCallback([&synthesizer, paramId, envViz, 
                                          attackSlider, decaySlider, sustainSlider, releaseSlider](float value) {
                // Update synthesizer
                synthesizer->setParameter(paramId, value);
                std::cout << "Updated " << paramId << " to " << value << std::endl;
                
                // Update visualizer with all current values
                if (envViz && attackSlider && decaySlider && sustainSlider && releaseSlider) {
                    envViz->setADSR(attackSlider->getValue(), decaySlider->getValue(),
                                   sustainSlider->getValue(), releaseSlider->getValue());
                }
            });
            
            // Initialize slider with current parameter value
            float currentValue = synthesizer->getParameter(paramId);
            slider->setValue(currentValue);
        }
    };
    
    // Re-connect envelope sliders with visualizer updates
    connectSliderToEnvelope(attackSliderPtr, "envelope_attack", envelopePtr,
                           attackSliderPtr, decaySliderPtr, sustainSliderPtr, releaseSliderPtr);
    connectSliderToEnvelope(decaySliderPtr, "envelope_decay", envelopePtr,
                           attackSliderPtr, decaySliderPtr, sustainSliderPtr, releaseSliderPtr);
    connectSliderToEnvelope(sustainSliderPtr, "envelope_sustain", envelopePtr,
                           attackSliderPtr, decaySliderPtr, sustainSliderPtr, releaseSliderPtr);
    connectSliderToEnvelope(releaseSliderPtr, "envelope_release", envelopePtr,
                           attackSliderPtr, decaySliderPtr, sustainSliderPtr, releaseSliderPtr);
    
    std::cout << "Parameter connections and CC learning established" << std::endl;

    // Create preset section at bottom right
    auto presetSection = std::make_unique<Label>("preset_section", "PRESETS");
    presetSection->setPosition(850, 720);  // Near bottom of 800px window
    presetSection->setTextColor(Color(150, 255, 150)); // Bright light green
    mainScreen->addChild(std::move(presetSection));
    
    // Create preset dropdown menu
    auto presetDropdown = std::make_unique<PresetDropdown>("preset_dropdown");
    presetDropdown->setPosition(850, 750);  // Bottom right
    presetDropdown->setSize(250, 30);
    
    // Add all presets to dropdown
    auto allPresets = presetDatabase->getAllPresets();
    for (const auto& preset : allPresets) {
        presetDropdown->addPreset(preset.name, preset.category, preset.filePath);
    }
    
    std::cout << "Added " << allPresets.size() << " presets to dropdown" << std::endl;
    
    // Store pointer to dropdown for button callback
    PresetDropdown* presetDropdownPtr = presetDropdown.get();
    
    // Create Load button
    auto loadPresetButton = std::make_unique<Button>("load_preset", "Load");
    loadPresetButton->setPosition(1110, 750);
    loadPresetButton->setSize(60, 30);
    loadPresetButton->setBackgroundColor(Color(60, 100, 60));
    loadPresetButton->setTextColor(Color(255, 255, 255));
    
    loadPresetButton->setClickCallback([&, presetDropdownPtr, waveSliderPtr, cutoffSliderPtr, resSliderPtr, volumeSliderPtr]() {
        auto selectedPreset = presetDropdownPtr->getSelectedPreset();
        if (!selectedPreset.fullPath.empty()) {
            std::cout << "Loading preset: " << selectedPreset.name << " from " << selectedPreset.fullPath << std::endl;
            
            // Load the preset file and apply parameters to synthesizer
            if (presetManager->loadPreset(selectedPreset.fullPath)) {
                std::cout << "Successfully loaded preset: " << selectedPreset.name << std::endl;
                
                // Update UI controls to reflect loaded preset values
                if (waveSliderPtr) {
                    float oscType = synthesizer->getParameter("oscillator_type");
                    waveSliderPtr->setValue(oscType); // Already normalized 0-4
                }
                
                if (cutoffSliderPtr) {
                    float cutoff = synthesizer->getParameter("filter_cutoff");
                    // Convert normalized value back to Hz
                    float frequencyHz = 20.0f * std::pow(1000.0f, cutoff);
                    cutoffSliderPtr->setValue(frequencyHz);
                }
                
                if (resSliderPtr) {
                    float resonance = synthesizer->getParameter("filter_resonance");
                    resSliderPtr->setValue(resonance); // Already normalized
                }
                
                if (volumeSliderPtr) {
                    float volume = synthesizer->getParameter("master_volume");
                    volumeSliderPtr->setValue(volume); // Already normalized
                }
                
                std::cout << "Preset loaded and UI updated: " << selectedPreset.name << std::endl;
            } else {
                std::cerr << "Failed to load preset: " << selectedPreset.name << std::endl;
            }
        }
    });
    
    // Create Save button (placeholder for now)
    auto savePresetButton = std::make_unique<Button>("save_preset", "Save");
    savePresetButton->setPosition(1180, 750);
    savePresetButton->setSize(60, 30);
    savePresetButton->setBackgroundColor(Color(60, 60, 100));
    savePresetButton->setTextColor(Color(255, 255, 255));
    savePresetButton->setClickCallback([]() {
        std::cout << "Save preset functionality not yet implemented" << std::endl;
    });
    
    mainScreen->addChild(std::move(presetDropdown));
    mainScreen->addChild(std::move(loadPresetButton));
    mainScreen->addChild(std::move(savePresetButton));

    // Add all dropdowns last for proper z-order (they render on top)
    // Add modulation dropdowns
    for (auto& dropdown : modSourceDropdowns) {
        mainScreen->addChild(std::move(dropdown));
    }
    for (auto& dropdown : modDestDropdowns) {
        mainScreen->addChild(std::move(dropdown));
    }
    
    // Add effect dropdowns
    for (auto& dropdown : effectDropdowns) {
        mainScreen->addChild(std::move(dropdown));
    }

    // Add screen to context
    uiContext->addScreen(std::move(mainScreen));
    uiContext->setActiveScreen("main");
    std::cout << "Added screen to UI context" << std::endl;
    
    // Set up MIDI handling
    midiInput->setCallback(midiHandler.get());
    
    midiHandler->setNoteOnCallback([&, midiKeyboardPtr](int channel, int note, int velocity) {
        float normalizedVelocity = velocity / 127.0f;
        synthesizer->noteOn(note, normalizedVelocity);
        
        // Display external MIDI input on keyboard
        if (midiKeyboardPtr) {
            midiKeyboardPtr->setNotePressed(note, true, velocity);
        }
        
        std::cout << "MIDI Note On: " << MidiKeyboard::getNoteName(note) 
                  << " (note " << note << ") velocity " << velocity << std::endl;
    });
    
    midiHandler->setNoteOffCallback([&, midiKeyboardPtr](int channel, int note) {
        synthesizer->noteOff(note);
        
        // Update keyboard display
        if (midiKeyboardPtr) {
            midiKeyboardPtr->setNotePressed(note, false, 0);
        }
        
        std::cout << "MIDI Note Off: " << MidiKeyboard::getNoteName(note) 
                  << " (note " << note << ")" << std::endl;
    });
    
    // Set up MIDI CC processing for CC learning
    midiHandler->setControlChangeCallback([&](int channel, int ccNumber, int value) {
        ccLearning.getLearning().processMidiCC(channel, ccNumber, value, "MIDI Input");
    });
    
    // Set up sequencer callbacks
    sequencer->setNoteCallbacks(
        [&](int pitch, float velocity, int channel, const Envelope& env) {
            synthesizer->noteOn(pitch, velocity);
            midiOutput->sendNoteOn(channel, pitch, static_cast<int>(velocity * 127.0f));
        },
        [&](int pitch, int channel) {
            synthesizer->noteOff(pitch);
            midiOutput->sendNoteOff(channel, pitch);
        }
    );
    
    // Set up audio callback
    std::mutex audioMutex;
    audioEngine->setAudioCallback([&](float* outputBuffer, int numFrames) {
        std::lock_guard<std::mutex> lock(audioMutex);
        audioCallback(audioEngine.get(), synthesizer.get(), effectProcessor.get(),
                     sequencer.get(), waveformPtr, levelPtr, outputBuffer, numFrames);
    });
    
    // Main loop
    bool running = true;
    auto lastFrameTime = std::chrono::high_resolution_clock::now();
    auto lastPerfUpdate = std::chrono::high_resolution_clock::now();
    int frameCount = 0;
    float cpuUsage = 0.0f;
    
    std::cout << "Starting main loop..." << std::endl;
    
    while (running) {
        auto frameStart = std::chrono::high_resolution_clock::now();
        
        // Process SDL events
        SDL_Event sdlEvent;
        while (SDL_PollEvent(&sdlEvent)) {
            if (sdlEvent.type == SDL_QUIT) {
                std::cout << "Got SDL_QUIT event" << std::endl;
                running = false;
            } else if (sdlEvent.type == SDL_KEYDOWN && sdlEvent.key.keysym.sym == SDLK_ESCAPE) {
                std::cout << "Got ESC key" << std::endl;
                running = false;
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
        
        // Update performance info every second
        frameCount++;
        if (std::chrono::duration<float>(currentTime - lastPerfUpdate).count() > 1.0f) {
            float fps = frameCount / std::chrono::duration<float>(currentTime - lastPerfUpdate).count();
            
            // Performance tracking is now just for internal use
            // Could be displayed in a different tab later
            std::stringstream perfText;
            perfText << "CPU: " << std::fixed << std::setprecision(1) 
                    << cpuUsage << "% | FPS: " 
                    << static_cast<int>(fps) << " | Audio: OK";
            
            frameCount = 0;
            lastPerfUpdate = currentTime;
        }
        
        // Let UIContext render, but don't let it swap buffers
        // First, save the current render state
        Screen* activeScreen = uiContext->getScreen("main");
        if (activeScreen) {
            sdlDisplayManager->clear(activeScreen->getBackgroundColor());
            activeScreen->render(sdlDisplayManager.get());
            
            // Render dropdown lists on top of everything else
            // This ensures dropdowns appear above all other components
            
            // Check modulation source dropdowns
            for (int i = 0; i < 3; ++i) {
                if (auto* dropdown = dynamic_cast<DropdownMenu*>(activeScreen->getChild("mod_source_" + std::to_string(i)))) {
                    if (dropdown->isDropdownOpen()) {
                        dropdown->renderDropdownList(sdlDisplayManager.get());
                    }
                }
                if (auto* dropdown = dynamic_cast<DropdownMenu*>(activeScreen->getChild("mod_dest_" + std::to_string(i)))) {
                    if (dropdown->isDropdownOpen()) {
                        dropdown->renderDropdownList(sdlDisplayManager.get());
                    }
                }
            }
            
            // Check effect dropdowns
            for (int i = 0; i < 3; ++i) {
                if (auto* dropdown = dynamic_cast<DropdownMenu*>(activeScreen->getChild("effect_type_" + std::to_string(i)))) {
                    if (dropdown->isDropdownOpen()) {
                        dropdown->renderDropdownList(sdlDisplayManager.get());
                    }
                }
            }
            
            // Check preset dropdown
            if (auto* dropdown = dynamic_cast<PresetDropdown*>(activeScreen->getChild("preset_dropdown"))) {
                if (dropdown->isDropdownOpen()) {
                    dropdown->renderDropdownList(sdlDisplayManager.get());
                }
            }
        }
        
        // Present the frame
        SDL_RenderPresent(renderer);
        
        // Frame rate limiting and CPU usage calculation
        auto frameEnd = std::chrono::high_resolution_clock::now();
        auto frameDuration = std::chrono::duration<float, std::milli>(frameEnd - frameStart).count();
        
        cpuUsage = (frameDuration / 16.67f) * 100.0f; // Percentage of 60 FPS frame time
        
        if (frameDuration < 16.67f) { // Target 60 FPS
            SDL_Delay(static_cast<Uint32>(16.67f - frameDuration));
        }
    }
    
    // Cleanup - Order is critical to prevent crashes
    std::cout << "AI Music Hardware - Shutting down..." << std::endl;
    
    // 1. First stop audio engine to prevent callbacks during shutdown
    std::cout << "Stopping audio engine..." << std::endl;
    if (audioEngine) {
        audioEngine->shutdown();
    }
    
    // 2. Send all notes off (only if MIDI output is open)
    if (midiOutput && midiOutput->isDeviceOpen()) {
        std::cout << "Sending all notes off..." << std::endl;
        try {
            for (int ch = 0; ch < 16; ++ch) {
                for (int note = 0; note < 128; ++note) {
                    midiOutput->sendNoteOff(ch, note);
                }
            }
        } catch (const std::exception& e) {
            std::cerr << "Error sending notes off: " << e.what() << std::endl;
        }
    }
    
    // 3. Clear all UI connections before shutting down UI
    std::cout << "Clearing UI connections..." << std::endl;
    if (uiContext) {
        uiContext->connectSynthesizer(nullptr);
        uiContext->connectEffectProcessor(nullptr);
        uiContext->connectSequencer(nullptr);
        uiContext->connectHardwareInterface(nullptr);
        uiContext->connectAdaptiveSequencer(nullptr);
        uiContext->connectLLMInterface(nullptr);
    }
    
    // 4. Shutdown UI context first (before destroying display manager)
    std::cout << "Shutting down UI..." << std::endl;
    if (uiContext) {
        uiContext->shutdown();
        uiContext.reset();
    }
    
    // 5. Reset display manager before destroying SDL renderer
    std::cout << "Resetting display manager..." << std::endl;
    sdlDisplayManager.reset();
    
    // 6. Stop hardware interface
    std::cout << "Stopping hardware interface..." << std::endl;
    if (hardwareInterface) {
        hardwareInterface->shutdown();
    }
    
    // 7. Cleanup SDL in correct order: renderer first, then window, then SDL
    std::cout << "Cleaning up SDL..." << std::endl;
    if (renderer) {
        SDL_DestroyRenderer(renderer);
        renderer = nullptr;
    }
    if (window) {
        SDL_DestroyWindow(window);
        window = nullptr;
    }
    SDL_Quit();
    
    // 8. Force destruction of remaining components in safe order
    std::cout << "Destroying audio components..." << std::endl;
    midiHandler.reset();
    midiOutput.reset();
    midiInput.reset();
    sequencer.reset();
    effectProcessor.reset();
    synthesizer.reset();
    audioEngine.reset();
    hardwareInterface.reset();
    
    std::cout << "Shutdown complete." << std::endl;
    
    return 0;
}