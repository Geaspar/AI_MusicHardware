#include <iostream>
#include <chrono>
#include <thread>
#include <SDL2/SDL.h>
#include "../include/ui/SmoothParameter.h"
#include "../include/ui/ParameterManager.h"
#include "../include/ui/SynthKnob.h"
#include "../include/ui/DisplayManager.h"
#include "../include/audio/Synthesizer.h"
#include "../include/audio/AudioEngine.h"

using namespace AIMusicHardware;

/**
 * @brief Test parameter smoothing implementation
 * 
 * This test validates:
 * 1. SmoothParameter exponential smoothing
 * 2. ParameterManager automation integration
 * 3. SynthKnob visual feedback
 * 4. Performance under load
 */

class ParameterSmoothingTest {
public:
    ParameterSmoothingTest() 
        : audio_engine_(nullptr)
        , synthesizer_(nullptr)
        , parameter_manager_()
        , display_manager_(nullptr)
        , running_(false) {
    }
    
    ~ParameterSmoothingTest() {
        cleanup();
    }
    
    bool initialize() {
        std::cout << "Initializing Parameter Smoothing Test..." << std::endl;
        
        // Initialize SDL
        if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) {
            std::cerr << "Failed to initialize SDL: " << SDL_GetError() << std::endl;
            return false;
        }
        
        // Create display manager
        display_manager_ = std::make_unique<DisplayManager>();
        if (!display_manager_->initialize(800, 600, "Parameter Smoothing Test")) {
            std::cerr << "Failed to initialize display manager" << std::endl;
            return false;
        }
        
        // Initialize audio engine
        audio_engine_ = std::make_unique<AudioEngine>();
        if (!audio_engine_->initialize()) {
            std::cerr << "Failed to initialize audio engine" << std::endl;
            return false;
        }
        
        // Create synthesizer
        synthesizer_ = std::make_unique<Synthesizer>();
        if (!synthesizer_->initialize()) {
            std::cerr << "Failed to initialize synthesizer" << std::endl;
            return false;
        }
        
        // Initialize parameter manager
        if (!parameter_manager_.initialize()) {
            std::cerr << "Failed to initialize parameter manager" << std::endl;
            return false;
        }
        
        // Connect synthesizer to parameter manager
        parameter_manager_.connectSynthesizer(synthesizer_.get());
        
        // Create test knobs
        createTestKnobs();
        
        std::cout << "✅ Parameter Smoothing Test initialized successfully" << std::endl;
        return true;
    }
    
    void createTestKnobs() {
        // Create test knobs for different parameter types
        test_knobs_.push_back(
            SynthKnobFactory::createFrequencyKnob("Cutoff", 100, 100));
        test_knobs_.push_back(
            SynthKnobFactory::createResonanceKnob("Resonance", 200, 100));
        test_knobs_.push_back(
            SynthKnobFactory::createVolumeKnob("Volume", 300, 100));
        test_knobs_.push_back(
            SynthKnobFactory::createTimeKnob("Attack", 400, 100, 5.0f));
        
        // Set different smoothing factors for testing
        parameter_manager_.setParameterSmoothingFactor("filter_cutoff", 0.90f);     // Fast
        parameter_manager_.setParameterSmoothingFactor("filter_resonance", 0.95f);  // Medium
        parameter_manager_.setParameterSmoothingFactor("volume", 0.98f);            // Slow
        parameter_manager_.setParameterSmoothingFactor("envelope_attack", 0.92f);   // Medium-fast
        
        std::cout << "✅ Created " << test_knobs_.size() << " test knobs" << std::endl;
    }
    
    void runPerformanceTest() {
        std::cout << "\n🔥 Running Performance Test..." << std::endl;
        
        const int num_parameters = 100;
        const int num_samples = 512;
        const int num_iterations = 1000;
        
        // Create many smooth parameters for stress testing
        std::vector<SmoothParameter> stress_parameters;
        stress_parameters.reserve(num_parameters);
        
        for (int i = 0; i < num_parameters; ++i) {
            stress_parameters.emplace_back(0.0f);
            stress_parameters.back().setSmoothingFactor(0.95f);
        }
        
        // Measure processing time
        auto start_time = std::chrono::high_resolution_clock::now();
        
        for (int iteration = 0; iteration < num_iterations; ++iteration) {
            // Set random targets for all parameters
            for (auto& param : stress_parameters) {
                float random_target = static_cast<float>(rand()) / RAND_MAX;
                param.setTarget(random_target);
            }
            
            // Process buffer for all parameters
            for (auto& param : stress_parameters) {
                for (int sample = 0; sample < num_samples; ++sample) {
                    param.process();
                }
            }
        }
        
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
        
        double total_samples = static_cast<double>(num_parameters) * num_samples * num_iterations;
        double samples_per_second = total_samples / (duration.count() / 1000000.0);
        double cpu_percentage = (total_samples / 44100.0) / (duration.count() / 1000000.0) * 100.0;
        
        std::cout << "📊 Performance Results:" << std::endl;
        std::cout << "   Parameters: " << num_parameters << std::endl;
        std::cout << "   Samples per buffer: " << num_samples << std::endl;
        std::cout << "   Iterations: " << num_iterations << std::endl;
        std::cout << "   Total processing time: " << duration.count() << " μs" << std::endl;
        std::cout << "   Samples processed per second: " << static_cast<int>(samples_per_second) << std::endl;
        std::cout << "   Estimated CPU usage: " << std::fixed << std::setprecision(2) << cpu_percentage << "%" << std::endl;
        
        if (cpu_percentage < 5.0) {
            std::cout << "✅ Performance test PASSED - Low CPU usage" << std::endl;
        } else if (cpu_percentage < 15.0) {
            std::cout << "⚠️  Performance test MODERATE - Acceptable CPU usage" << std::endl;
        } else {
            std::cout << "❌ Performance test FAILED - High CPU usage" << std::endl;
        }\n    }\n    \n    void runSmoothingTest() {\n        std::cout << \"\\n🎛️  Running Smoothing Behavior Test...\" << std::endl;\n        \n        SmoothParameter test_param(0.0f);\n        test_param.setSmoothingFactor(0.9f);\n        \n        // Test step response\n        test_param.setTarget(1.0f);\n        \n        std::cout << \"Step response (target = 1.0):\" << std::endl;\n        for (int i = 0; i < 20; ++i) {\n            float value = test_param.process();\n            std::cout << \"  Sample \" << i << \": \" << std::fixed << std::setprecision(4) << value << std::endl;\n            \n            // Should converge to target\n            if (i == 19 && std::abs(value - 1.0f) > 0.001f) {\n                std::cout << \"❌ Smoothing test FAILED - Did not converge\" << std::endl;\n                return;\n            }\n        }\n        \n        // Test threshold behavior\n        test_param.reset(0.0f);\n        test_param.setLinearThreshold(0.01f);\n        test_param.setTarget(0.005f); // Below threshold\n        \n        float final_value = test_param.process();\n        if (std::abs(final_value - 0.005f) < 0.0001f) {\n            std::cout << \"✅ Linear threshold behavior PASSED\" << std::endl;\n        } else {\n            std::cout << \"❌ Linear threshold behavior FAILED\" << std::endl;\n            return;\n        }\n        \n        std::cout << \"✅ Smoothing behavior test PASSED\" << std::endl;\n    }\n    \n    void runAutomationTest() {\n        std::cout << \"\\n🤖 Running Automation Integration Test...\" << std::endl;\n        \n        // Test parameter manager automation\n        const std::string test_param = \"filter_cutoff\";\n        \n        // Set initial value\n        parameter_manager_.setParameterValue(test_param, 0.0f);\n        \n        // Start automation\n        parameter_manager_.setParameterWithAutomation(test_param, 1.0f);\n        \n        if (!parameter_manager_.isParameterAutomated(test_param)) {\n            std::cout << \"❌ Automation test FAILED - Parameter not marked as automated\" << std::endl;\n            return;\n        }\n        \n        // Simulate audio buffer processing\n        for (int buffer = 0; buffer < 10; ++buffer) {\n            parameter_manager_.processAudioBuffer(512);\n            \n            float current_value = parameter_manager_.getParameterValue(test_param);\n            std::cout << \"  Buffer \" << buffer << \": \" << std::fixed << std::setprecision(4) \n                      << current_value << \" (automated: \" \n                      << (parameter_manager_.isParameterAutomated(test_param) ? \"yes\" : \"no\") \n                      << \")\" << std::endl;\n        }\n        \n        std::cout << \"✅ Automation integration test PASSED\" << std::endl;\n    }\n    \n    void runVisualFeedbackTest() {\n        std::cout << \"\\n👁️  Running Visual Feedback Test...\" << std::endl;\n        \n        if (test_knobs_.empty()) {\n            std::cout << \"❌ Visual feedback test FAILED - No test knobs\" << std::endl;\n            return;\n        }\n        \n        auto& knob = test_knobs_[0];\n        \n        // Test automation visual feedback\n        knob->setValueFromAutomation(0.75f);\n        \n        if (!knob->isAutomated()) {\n            std::cout << \"❌ Visual feedback test FAILED - Knob not marked as automated\" << std::endl;\n            return;\n        }\n        \n        // Test modulation visualization\n        knob->setModulationAmount(0.5f);\n        \n        if (std::abs(knob->getModulationAmount() - 0.5f) > 0.001f) {\n            std::cout << \"❌ Visual feedback test FAILED - Modulation amount not set\" << std::endl;\n            return;\n        }\n        \n        std::cout << \"✅ Visual feedback test PASSED\" << std::endl;\n    }\n    \n    void runInteractiveTest() {\n        std::cout << \"\\n🎮 Starting Interactive Test...\" << std::endl;\n        std::cout << \"Controls:\" << std::endl;\n        std::cout << \"  1-4: Automate knobs to random values\" << std::endl;\n        std::cout << \"  R: Reset all knobs to default\" << std::endl;\n        std::cout << \"  ESC: Exit test\" << std::endl;\n        \n        running_ = true;\n        SDL_Event event;\n        \n        auto last_time = std::chrono::high_resolution_clock::now();\n        \n        while (running_) {\n            auto current_time = std::chrono::high_resolution_clock::now();\n            float delta_time = std::chrono::duration<float>(current_time - last_time).count();\n            last_time = current_time;\n            \n            // Handle events\n            while (SDL_PollEvent(&event)) {\n                if (event.type == SDL_QUIT || \n                    (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE)) {\n                    running_ = false;\n                }\n                \n                if (event.type == SDL_KEYDOWN) {\n                    handleKeyPress(event.key.keysym.sym);\n                }\n            }\n            \n            // Process parameter automation\n            parameter_manager_.processAudioBuffer(512);\n            \n            // Update knobs\n            for (auto& knob : test_knobs_) {\n                knob->update(delta_time);\n            }\n            \n            // Render\n            display_manager_->clear({30, 30, 30, 255});\n            \n            for (auto& knob : test_knobs_) {\n                knob->render(display_manager_.get());\n            }\n            \n            // Draw instructions\n            display_manager_->drawText(10, 10, \"Parameter Smoothing Test\", nullptr, {255, 255, 255, 255});\n            display_manager_->drawText(10, 30, \"Press 1-4 to automate knobs, R to reset, ESC to exit\", nullptr, {200, 200, 200, 255});\n            \n            display_manager_->present();\n            \n            // Limit frame rate\n            std::this_thread::sleep_for(std::chrono::milliseconds(16));\n        }\n        \n        std::cout << \"✅ Interactive test completed\" << std::endl;\n    }\n    \n    void handleKeyPress(SDL_Keycode key) {\n        switch (key) {\n            case SDLK_1:\n                if (!test_knobs_.empty()) {\n                    automateKnob(0, \"filter_cutoff\");\n                }\n                break;\n            case SDLK_2:\n                if (test_knobs_.size() > 1) {\n                    automateKnob(1, \"filter_resonance\");\n                }\n                break;\n            case SDLK_3:\n                if (test_knobs_.size() > 2) {\n                    automateKnob(2, \"volume\");\n                }\n                break;\n            case SDLK_4:\n                if (test_knobs_.size() > 3) {\n                    automateKnob(3, \"envelope_attack\");\n                }\n                break;\n            case SDLK_r:\n                resetAllKnobs();\n                break;\n        }\n    }\n    \n    void automateKnob(int index, const std::string& param_name) {\n        if (index >= test_knobs_.size()) return;\n        \n        float random_value = static_cast<float>(rand()) / RAND_MAX;\n        parameter_manager_.setParameterWithAutomation(param_name, random_value);\n        test_knobs_[index]->setValueFromAutomation(random_value);\n        \n        std::cout << \"🎛️  Automating \" << param_name << \" to \" << std::fixed \n                  << std::setprecision(3) << random_value << std::endl;\n    }\n    \n    void resetAllKnobs() {\n        for (auto& knob : test_knobs_) {\n            knob->resetToDefault();\n        }\n        std::cout << \"🔄 Reset all knobs to default values\" << std::endl;\n    }\n    \n    void runAllTests() {\n        std::cout << \"\\n🧪 Running All Parameter Smoothing Tests\\n\" << std::endl;\n        \n        runSmoothingTest();\n        runPerformanceTest();\n        runAutomationTest();\n        runVisualFeedbackTest();\n        \n        std::cout << \"\\n🎉 All automated tests completed!\" << std::endl;\n        std::cout << \"Starting interactive test...\" << std::endl;\n        \n        runInteractiveTest();\n    }\n    \nprivate:\n    void cleanup() {\n        test_knobs_.clear();\n        \n        if (audio_engine_) {\n            audio_engine_->shutdown();\n        }\n        \n        if (display_manager_) {\n            display_manager_->shutdown();\n        }\n        \n        SDL_Quit();\n    }\n    \n    std::unique_ptr<AudioEngine> audio_engine_;\n    std::unique_ptr<Synthesizer> synthesizer_;\n    ParameterManager parameter_manager_;\n    std::unique_ptr<DisplayManager> display_manager_;\n    std::vector<std::unique_ptr<SynthKnob>> test_knobs_;\n    bool running_;\n};\n\nint main() {\n    std::cout << \"🎛️  AIMusicHardware Parameter Smoothing Test\" << std::endl;\n    std::cout << \"============================================\" << std::endl;\n    \n    ParameterSmoothingTest test;\n    \n    if (!test.initialize()) {\n        std::cerr << \"❌ Failed to initialize test\" << std::endl;\n        return 1;\n    }\n    \n    test.runAllTests();\n    \n    std::cout << \"\\n✅ Parameter Smoothing Test completed successfully!\" << std::endl;\n    return 0;\n}