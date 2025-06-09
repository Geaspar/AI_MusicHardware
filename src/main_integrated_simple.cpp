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
#include "../include/ui/parameters/ParameterManager.h"
#include "../include/ui/presets/PresetManager.h"
#include "../include/ui/presets/PresetDatabase.h"
#include "../include/midi/MidiCCLearning.h"

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
    std::map<std::string, SynthKnob*> parameterKnobs; // Map parameter IDs to knobs
    
    // Set up CC learning to update synthesizer parameters and UI
    ccLearning.getLearning().setParameterChangeCallback([&synthesizer, &parameterKnobs](const std::string& parameterId, float value) {
        // Update synthesizer
        synthesizer->setParameter(parameterId, value);
        
        // Update corresponding UI knob if it exists
        auto knobIt = parameterKnobs.find(parameterId);
        if (knobIt != parameterKnobs.end() && knobIt->second) {
            knobIt->second->setValue(value);
        }
        
        std::cout << "CC Learning -> " << parameterId << " = " << value << std::endl;
    });
    
    // Create main synthesizer screen first
    auto mainScreen = std::make_unique<Screen>("main");
    
    // Forward declare status label pointer for callback
    Label* ccStatusPtr = nullptr;
    
    // Set up learning state callback for UI feedback
    ccLearning.getLearning().setLearningStateCallback([&ccStatusPtr](MidiCCLearning::LearningState state, const std::string& message) {
        std::cout << "Learning State: " << message << std::endl;
        
        // Update status label if available
        if (ccStatusPtr) {
            std::string statusText = "CC Status: " + message;
            ccStatusPtr->setText(statusText);
        }
    });
    
    auto connectKnobToParam = [&](SynthKnob* knob, const std::string& paramId) {
        if (knob) {
            // Store knob reference for CC learning updates
            parameterKnobs[paramId] = knob;
            
            // Set up value change callback to update synthesizer
            knob->setValueChangeCallback([&synthesizer, paramId](float normalizedValue) {
                // Convert normalized value (0-1) to parameter range and update synthesizer
                synthesizer->setParameter(paramId, normalizedValue);
                std::cout << "Updated " << paramId << " to " << normalizedValue << std::endl;
            });
            
            // Initialize knob with current parameter value
            float currentValue = synthesizer->getParameter(paramId);
            knob->setValue(currentValue);
        }
    };
    
    // Helper to add parameter-specific learning functionality
    auto addParameterLearning = [&](SynthKnob* knob, const std::string& paramId, int x, int y) {
        if (knob) {
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
    oscSection->setPosition(50, 50);
    oscSection->setSize(200, 25);
    oscSection->setTextColor(Color(255, 255, 100)); // Bright yellow for visibility
    mainScreen->addChild(std::move(oscSection));
    
    // Create oscillator knobs connected to synthesizer parameters with better spacing
    auto freqKnob = SynthKnobFactory::createFrequencyKnob("Frequency", 50, 85, 80);
    freqKnob->setValueFormatter([](float value) {
        std::stringstream ss;
        ss << std::fixed << std::setprecision(1) << value << " Hz";
        return ss.str();
    });
    SynthKnob* freqKnobPtr = freqKnob.get();
    mainScreen->addChild(std::move(freqKnob));
    
    auto waveKnob = std::make_unique<SynthKnob>("Wave", 170, 85, 80, 0.0f, 4.0f, 0.0f);
    waveKnob->setValueFormatter([](float value) {
        const char* waveNames[] = {"Sine", "Saw", "Square", "Triangle", "Noise"};
        int index = static_cast<int>(value);
        if (index >= 0 && index < 5) {
            return std::string(waveNames[index]);
        }
        return std::string("Unknown");
    });
    SynthKnob* waveKnobPtr = waveKnob.get();
    mainScreen->addChild(std::move(waveKnob));
    
    // Create filter section
    auto filterSection = std::make_unique<Label>("filter_section", "FILTER");
    filterSection->setPosition(350, 50);
    filterSection->setSize(180, 25);
    filterSection->setTextColor(Color(100, 255, 100)); // Bright green for visibility
    mainScreen->addChild(std::move(filterSection));
    
    auto cutoffKnob = SynthKnobFactory::createFrequencyKnob("Cutoff", 350, 85, 80);
    cutoffKnob->setValueFormatter([](float value) {
        std::stringstream ss;
        if (value >= 1000.0f) {
            ss << std::fixed << std::setprecision(1) << value / 1000.0f << " kHz";
        } else {
            ss << std::fixed << std::setprecision(0) << value << " Hz";
        }
        return ss.str();
    });
    SynthKnob* cutoffKnobPtr = cutoffKnob.get();
    mainScreen->addChild(std::move(cutoffKnob));
    
    auto resKnob = SynthKnobFactory::createResonanceKnob("Resonance", 460, 85, 80);
    resKnob->setValueFormatter([](float value) {
        std::stringstream ss;
        ss << std::fixed << std::setprecision(0) << value * 100.0f << "%";
        return ss.str();
    });
    SynthKnob* resKnobPtr = resKnob.get();
    mainScreen->addChild(std::move(resKnob));
    
    // Create envelope section  
    auto envSection = std::make_unique<Label>("env_section", "ENVELOPE");
    envSection->setPosition(590, 50);
    envSection->setSize(200, 25);
    envSection->setTextColor(Color(255, 100, 255)); // Bright magenta for visibility
    mainScreen->addChild(std::move(envSection));
    
    auto attackKnob = SynthKnobFactory::createTimeKnob("Attack", 590, 85, 80, 2.0f);
    attackKnob->setValueFormatter([](float value) {
        std::stringstream ss;
        if (value < 0.1f) {
            ss << std::fixed << std::setprecision(0) << value * 1000.0f << " ms";
        } else {
            ss << std::fixed << std::setprecision(2) << value << " s";
        }
        return ss.str();
    });
    SynthKnob* attackKnobPtr = attackKnob.get();
    
    auto decayKnob = SynthKnobFactory::createTimeKnob("Decay", 680, 85, 80, 2.0f);
    decayKnob->setValueFormatter([](float value) {
        std::stringstream ss;
        if (value < 0.1f) {
            ss << std::fixed << std::setprecision(0) << value * 1000.0f << " ms";
        } else {
            ss << std::fixed << std::setprecision(2) << value << " s";
        }
        return ss.str();
    });
    SynthKnob* decayKnobPtr = decayKnob.get();
    
    auto sustainKnob = SynthKnobFactory::createVolumeKnob("Sustain", 770, 85, 80);
    sustainKnob->setValueFormatter([](float value) {
        std::stringstream ss;
        ss << std::fixed << std::setprecision(0) << value * 100.0f << "%";
        return ss.str();
    });
    SynthKnob* sustainKnobPtr = sustainKnob.get();
    
    auto releaseKnob = SynthKnobFactory::createTimeKnob("Release", 860, 85, 80, 4.0f);
    releaseKnob->setValueFormatter([](float value) {
        std::stringstream ss;
        if (value < 0.1f) {
            ss << std::fixed << std::setprecision(0) << value * 1000.0f << " ms";
        } else {
            ss << std::fixed << std::setprecision(2) << value << " s";
        }
        return ss.str();
    });
    SynthKnob* releaseKnobPtr = releaseKnob.get();
    
    mainScreen->addChild(std::move(attackKnob));
    mainScreen->addChild(std::move(decayKnob));
    mainScreen->addChild(std::move(sustainKnob));
    mainScreen->addChild(std::move(releaseKnob));
    
    // Create master section
    auto masterSection = std::make_unique<Label>("master_section", "MASTER");
    masterSection->setPosition(980, 50);
    masterSection->setSize(130, 25);
    masterSection->setTextColor(Color(100, 200, 255)); // Bright cyan for visibility
    mainScreen->addChild(std::move(masterSection));
    
    auto volumeKnob = SynthKnobFactory::createVolumeKnob("Volume", 980, 85, 80);
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
    mainScreen->addChild(std::move(volumeKnob));
    
    // Create visualization section
    auto vizSection = std::make_unique<Label>("viz_section", "VISUALIZATION");
    vizSection->setPosition(50, 220);
    vizSection->setSize(200, 25);
    vizSection->setTextColor(Color(255, 200, 100)); // Bright orange for visibility
    mainScreen->addChild(std::move(vizSection));
    
    auto waveform = std::make_unique<WaveformVisualizer>("waveform", 512);
    waveform->setPosition(50, 250);
    waveform->setSize(300, 150);
    waveform->setWaveformColor(Color(0, 255, 128));
    mainScreen->addChild(std::move(waveform));
    
    auto spectrum = std::make_unique<SpectrumAnalyzer>("spectrum", 32);
    spectrum->setPosition(370, 250);
    spectrum->setSize(300, 150);
    mainScreen->addChild(std::move(spectrum));
    
    auto envelope = std::make_unique<EnvelopeVisualizer>("envelope");
    envelope->setPosition(690, 250);
    envelope->setSize(250, 150);
    envelope->setADSR(0.01f, 0.1f, 0.7f, 0.5f);
    envelope->setEditable(true);
    mainScreen->addChild(std::move(envelope));
    
    auto levelMeter = std::make_unique<LevelMeter>("level", LevelMeter::Orientation::Vertical);
    levelMeter->setPosition(960, 250);
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
    
    // Create preset browser section 
    auto presetSection = std::make_unique<Label>("preset_section", "PRESET BROWSER");
    presetSection->setPosition(600, 170);  // Above the preset browser
    presetSection->setTextColor(Color(150, 255, 150)); // Bright light green for visibility
    mainScreen->addChild(std::move(presetSection));
    
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
    
    auto presetBrowser = std::make_unique<PresetBrowserUI>("preset_browser");
    presetBrowser->setPosition(600, 200);  // Move to right side 
    presetBrowser->setSize(600, 350);      // Make it much bigger
    
    // Debug: Check preset database contents before initialization
    std::cout << "Preset database has " << presetDatabase->getAllPresets().size() << " presets before initialization" << std::endl;
    
    presetBrowser->initialize(presetManager.get(), presetDatabase.get());
    
    // Debug: Check preset database contents after initialization
    std::cout << "Preset database has " << presetDatabase->getAllPresets().size() << " presets after initialization" << std::endl;
    presetBrowser->setParameterManager(&paramManager);
    
    // Set up preset loading callback with UI updates
    presetBrowser->setPresetLoadCallback([&, waveKnobPtr, cutoffKnobPtr, resKnobPtr, volumeKnobPtr](const PresetInfo& preset) {
        std::cout << "Loading preset: " << preset.name << std::endl;
        
        // Load the preset file and apply parameters to synthesizer
        if (presetManager->loadPreset(preset.filePath)) {
            std::cout << "Successfully loaded preset: " << preset.name << std::endl;
            
            // Update UI controls to reflect loaded preset values
            std::cout << "Updating UI knobs to reflect preset parameters..." << std::endl;
            
            // Get current parameter values from synthesizer and update UI
            if (waveKnobPtr) {
                float oscType = synthesizer->getParameter("oscillator_type");
                waveKnobPtr->setValue(oscType / 4.0f); // Normalize to 0-1 range
            }
            
            if (cutoffKnobPtr) {
                float cutoff = synthesizer->getParameter("filter_cutoff");
                cutoffKnobPtr->setValue(cutoff); // Already normalized
            }
            
            if (resKnobPtr) {
                float resonance = synthesizer->getParameter("filter_resonance");
                resKnobPtr->setValue(resonance); // Already normalized
            }
            
            if (volumeKnobPtr) {
                float volume = synthesizer->getParameter("master_volume");
                volumeKnobPtr->setValue(volume); // Already normalized
            }
            
            std::cout << "Preset loaded and UI updated: " << preset.name << std::endl;
        } else {
            std::cerr << "Failed to load preset: " << preset.name << std::endl;
        }
    });
    
    mainScreen->addChild(std::move(presetBrowser));
    
    // Create MIDI CC learning controls
    auto ccLearningSection = std::make_unique<Label>("cc_section", "MIDI CC LEARNING");
    ccLearningSection->setPosition(600, 430);
    ccLearningSection->setTextColor(Color(150, 150, 180));
    mainScreen->addChild(std::move(ccLearningSection));
    
    auto learnButton = std::make_unique<Button>("learn_cc", "LEARN CC");
    learnButton->setPosition(600, 460);
    learnButton->setSize(120, 40);
    learnButton->setToggleMode(true);
    learnButton->setClickCallback([&ccLearning]() {
        auto& learning = ccLearning.getLearning();
        if (learning.getLearningState() == MidiCCLearning::LearningState::Idle) {
            learning.startAutoLearning(std::chrono::milliseconds{10000});
        } else {
            learning.stopLearning();
        }
    });
    mainScreen->addChild(std::move(learnButton));
    
    auto clearCCButton = std::make_unique<Button>("clear_cc", "CLEAR CC");
    clearCCButton->setPosition(730, 460);
    clearCCButton->setSize(100, 40);
    clearCCButton->setClickCallback([&ccLearning]() {
        ccLearning.getLearning().clearAllMappings();
        std::cout << "Cleared all CC mappings" << std::endl;
    });
    mainScreen->addChild(std::move(clearCCButton));
    
    // CC Learning status display
    auto ccStatusLabel = std::make_unique<Label>("cc_status", "CC Status: Ready");
    ccStatusLabel->setPosition(600, 510);
    ccStatusLabel->setSize(250, 20);
    ccStatusLabel->setTextColor(Color(150, 200, 150));
    ccStatusPtr = ccStatusLabel.get(); // Store pointer for callback updates
    mainScreen->addChild(std::move(ccStatusLabel));
    
    // Create transport controls  
    auto transportSection = std::make_unique<Label>("transport_section", "TRANSPORT");
    transportSection->setPosition(600, 540);
    transportSection->setTextColor(Color(150, 150, 180));
    mainScreen->addChild(std::move(transportSection));
    
    auto playButton = std::make_unique<Button>("play", "PLAY");
    playButton->setPosition(600, 570);
    playButton->setSize(80, 40);
    playButton->setToggleMode(true);
    playButton->setClickCallback([&sequencer]() {
        if (sequencer->isPlaying()) {
            sequencer->stop();
        } else {
            sequencer->start();
        }
    });
    mainScreen->addChild(std::move(playButton));
    
    auto stopButton = std::make_unique<Button>("stop", "STOP");
    stopButton->setPosition(690, 570);
    stopButton->setSize(80, 40);
    stopButton->setClickCallback([&sequencer]() {
        sequencer->stop();
    });
    mainScreen->addChild(std::move(stopButton));
    
    // Create performance info
    auto perfSection = std::make_unique<Label>("perf_section", "PERFORMANCE");
    perfSection->setPosition(600, 650);
    perfSection->setTextColor(Color(150, 150, 180));
    mainScreen->addChild(std::move(perfSection));
    
    auto perfInfo = std::make_unique<Label>("perf_info", "CPU: 0.0% | FPS: 60 | Audio: OK");
    perfInfo->setPosition(600, 680);
    mainScreen->addChild(std::move(perfInfo));
    
    // Get pointers to visualization components for audio thread
    WaveformVisualizer* waveformPtr = 
        static_cast<WaveformVisualizer*>(mainScreen->getChild("waveform"));
    LevelMeter* levelPtr = 
        static_cast<LevelMeter*>(mainScreen->getChild("level"));
    Label* perfInfoPtr = 
        static_cast<Label*>(mainScreen->getChild("perf_info"));
    
    // Connect UI controls to synthesizer parameters
    std::cout << "Connecting UI controls to synthesizer parameters..." << std::endl;
    
    // Note: For oscillator frame parameter, we need special handling since it uses normalized values
    connectKnobToParam(waveKnobPtr, "oscillator_type");
    
    // Special handling for filter cutoff - need to normalize from Hz to 0-1 range
    if (cutoffKnobPtr) {
        parameterKnobs["filter_cutoff"] = cutoffKnobPtr;
        cutoffKnobPtr->setValueChangeCallback([&synthesizer](float frequencyHz) {
            // Normalize frequency from 20-20000 Hz to 0-1 range
            float normalized = (std::log(frequencyHz / 20.0f) / std::log(20000.0f / 20.0f));
            normalized = std::max(0.0f, std::min(1.0f, normalized));
            synthesizer->setParameter("filter_cutoff", normalized);
            std::cout << "Updated filter_cutoff: " << frequencyHz << " Hz -> normalized " << normalized << std::endl;
        });
        // Initialize with a reasonable cutoff frequency (1000 Hz)
        cutoffKnobPtr->setValue(1000.0f);
    }
    
    connectKnobToParam(resKnobPtr, "filter_resonance");
    connectKnobToParam(volumeKnobPtr, "master_volume");
    
    // Add CC learning buttons for each parameter
    addParameterLearning(waveKnobPtr, "oscillator_type", 170, 85);
    addParameterLearning(cutoffKnobPtr, "filter_cutoff", 350, 85);
    addParameterLearning(resKnobPtr, "filter_resonance", 460, 85);
    addParameterLearning(volumeKnobPtr, "master_volume", 980, 85);
    
    // Envelope parameters will need to be added to the synthesizer parameter system
    // connectKnobToParam(attackKnobPtr, "envelope_attack");
    // connectKnobToParam(decayKnobPtr, "envelope_decay");
    // connectKnobToParam(sustainKnobPtr, "envelope_sustain");
    // connectKnobToParam(releaseKnobPtr, "envelope_release");
    
    std::cout << "Parameter connections and CC learning established" << std::endl;

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
            
            std::stringstream perfText;
            perfText << "CPU: " << std::fixed << std::setprecision(1) 
                    << cpuUsage << "% | FPS: " 
                    << static_cast<int>(fps) << " | Audio: OK";
            
            if (perfInfoPtr) {
                perfInfoPtr->setText(perfText.str());
            }
            
            frameCount = 0;
            lastPerfUpdate = currentTime;
        }
        
        // Let UIContext render, but don't let it swap buffers
        // First, save the current render state
        Screen* activeScreen = uiContext->getScreen("main");
        if (activeScreen) {
            sdlDisplayManager->clear(activeScreen->getBackgroundColor());
            activeScreen->render(sdlDisplayManager.get());
            
            // Debug: Draw a small marker to show we're rendering
            SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255);
            SDL_Rect marker = {10, 580, 20, 10};
            SDL_RenderFillRect(renderer, &marker);
            
            // Debug: Draw where the button should be
            SDL_SetRenderDrawColor(renderer, 255, 0, 255, 255); // Magenta
            SDL_Rect buttonOutline = {10, 10, 120, 40};
            SDL_RenderDrawRect(renderer, &buttonOutline);
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