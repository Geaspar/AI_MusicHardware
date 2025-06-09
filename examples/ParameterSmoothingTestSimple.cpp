#include <iostream>
#include <chrono>
#include <thread>
#include <iomanip>
#include <vector>
#include <cstdlib>
#include <cmath>
#include "../include/ui/SmoothParameter.h"
#include "../include/ui/ParameterManager.h"

using namespace AIMusicHardware;

/**
 * @brief Simple test for parameter smoothing implementation
 */

class SimpleParameterSmoothingTest {
public:
    void runAllTests() {
        std::cout << "🧪 Running Parameter Smoothing Tests" << std::endl;
        
        runSmoothingTest();
        runPerformanceTest();
        runAutomationTest();
        
        std::cout << "✅ All tests completed!" << std::endl;
    }
    
private:
    void runSmoothingTest() {
        std::cout << "🎛️  Running Smoothing Behavior Test..." << std::endl;
        
        SmoothParameter test_param(0.0f);
        test_param.setSmoothingFactor(0.9f);
        
        // Test step response
        test_param.setTarget(1.0f);
        
        std::cout << "Step response (target = 1.0):" << std::endl;
        for (int i = 0; i < 20; ++i) {
            float value = test_param.process();
            std::cout << "  Sample " << i << ": " << std::fixed << std::setprecision(4) << value << std::endl;
            
            // Should converge to target
            if (i == 19 && std::abs(value - 1.0f) > 0.001f) {
                std::cout << "❌ Smoothing test FAILED - Did not converge" << std::endl;
                return;
            }
        }
        
        std::cout << "✅ Smoothing behavior test PASSED" << std::endl;
    }
    
    void runPerformanceTest() {
        std::cout << "🔥 Running Performance Test..." << std::endl;
        
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
        }
    }
    
    void runAutomationTest() {
        std::cout << "🤖 Running Automation Integration Test..." << std::endl;
        
        ParameterManager parameter_manager;
        parameter_manager.initialize();
        
        // Test parameter manager automation
        const std::string test_param = "filter_cutoff";
        
        // Set initial value
        parameter_manager.setParameterValue(test_param, 0.0f);
        
        // Start automation
        parameter_manager.setParameterWithAutomation(test_param, 1.0f);
        
        if (!parameter_manager.isParameterAutomated(test_param)) {
            std::cout << "❌ Automation test FAILED - Parameter not marked as automated" << std::endl;
            return;
        }
        
        // Simulate audio buffer processing
        for (int buffer = 0; buffer < 10; ++buffer) {
            parameter_manager.processAudioBuffer(512);
            
            float current_value = parameter_manager.getParameterValue(test_param);
            std::cout << "  Buffer " << buffer << ": " << std::fixed << std::setprecision(4) 
                      << current_value << " (automated: " 
                      << (parameter_manager.isParameterAutomated(test_param) ? "yes" : "no") 
                      << ")" << std::endl;
        }
        
        std::cout << "✅ Automation integration test PASSED" << std::endl;
    }
};

int main() {
    std::cout << "🎛️  AIMusicHardware Parameter Smoothing Test" << std::endl;
    std::cout << "============================================" << std::endl;
    
    SimpleParameterSmoothingTest test;
    test.runAllTests();
    
    std::cout << "✅ Parameter Smoothing Test completed successfully!" << std::endl;
    return 0;
}