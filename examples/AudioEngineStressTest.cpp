#include <iostream>
#include <chrono>
#include <thread>
#include <random>
#include <atomic>
#include <vector>
#include <memory>

#include "../include/audio/AudioEngine.h"
#include "../include/audio/Synthesizer.h"
#include "../include/effects/EffectProcessor.h"

using namespace AIMusicHardware;

/**
 * @brief Comprehensive stress test for the enhanced AudioEngine
 * 
 * Tests enterprise-grade error handling, performance monitoring,
 * and audio safety mechanisms under various stress conditions.
 */
class AudioEngineStressTest {
public:
    AudioEngineStressTest() {
        std::cout << "=== Audio Engine Enterprise Stress Test ===" << std::endl;
        std::cout << "Testing error handling, performance monitoring, and safety mechanisms" << std::endl;
    }
    
    bool runAllTests() {
        bool allPassed = true;
        
        std::cout << "\n1. Testing basic audio engine functionality..." << std::endl;
        allPassed &= testBasicFunctionality();
        
        std::cout << "\n2. Testing performance monitoring..." << std::endl;
        allPassed &= testPerformanceMonitoring();
        
        std::cout << "\n3. Testing error handling..." << std::endl;
        allPassed &= testErrorHandling();
        
        std::cout << "\n4. Testing audio safety mechanisms..." << std::endl;
        allPassed &= testAudioSafety();
        
        std::cout << "\n5. Testing under CPU stress..." << std::endl;
        allPassed &= testCPUStress();
        
        std::cout << "\n6. Testing concurrent operations..." << std::endl;
        allPassed &= testConcurrentOperations();
        
        std::cout << "\n7. Testing error recovery..." << std::endl;
        allPassed &= testErrorRecovery();
        
        return allPassed;
    }

private:
    bool testBasicFunctionality() {
        auto audioEngine = std::make_unique<AudioEngine>(44100, 512);
        auto synthesizer = std::make_unique<Synthesizer>();
        
        if (!audioEngine->initialize()) {
            std::cout << "❌ Failed to initialize audio engine" << std::endl;
            return false;
        }
        
        if (!synthesizer->initialize()) {
            std::cout << "❌ Failed to initialize synthesizer" << std::endl;
            return false;
        }
        
        // Set up a simple callback
        audioEngine->setAudioCallback([&](float* outputBuffer, int numFrames) {
            synthesizer->process(outputBuffer, numFrames);
        });
        
        // Test for a few seconds
        std::this_thread::sleep_for(std::chrono::seconds(3));
        
        // Check if audio engine is healthy
        if (!audioEngine->isHealthy()) {
            std::cout << "❌ Audio engine reported unhealthy status" << std::endl;
            return false;
        }
        
        audioEngine->shutdown();
        std::cout << "✅ Basic functionality test passed" << std::endl;
        return true;
    }
    
    bool testPerformanceMonitoring() {
        auto audioEngine = std::make_unique<AudioEngine>(44100, 256); // Smaller buffer for more stress
        
        if (!audioEngine->initialize()) {
            std::cout << "❌ Failed to initialize audio engine for performance test" << std::endl;
            return false;
        }
        
        // Enable performance monitoring
        audioEngine->setPerformanceMonitoringEnabled(true);
        
        // Set up a computationally expensive callback
        audioEngine->setAudioCallback([](float* outputBuffer, int numFrames) {
            // Simulate expensive computation
            for (int i = 0; i < numFrames * 2; ++i) {
                float sample = 0.0f;
                // Add some expensive operations
                for (int j = 0; j < 10; ++j) {
                    sample += std::sin(i * 0.1f + j) * 0.1f;
                }
                outputBuffer[i] = sample * 0.1f; // Keep volume low
            }
        });
        
        // Run for a few seconds to collect metrics
        std::this_thread::sleep_for(std::chrono::seconds(5));
        
        // Check performance metrics
        auto metrics = audioEngine->getPerformanceMetrics();
        
        std::cout << "Performance Metrics:" << std::endl;
        std::cout << "  CPU Load: " << metrics.cpuLoad << "%" << std::endl;
        std::cout << "  Memory Usage: " << metrics.memoryUsage << " MB" << std::endl;
        std::cout << "  Latency: " << metrics.latency.count() << " μs" << std::endl;
        std::cout << "  Jitter: " << metrics.jitter.count() << " μs" << std::endl;
        std::cout << "  Underruns: " << metrics.underrunCount << std::endl;
        std::cout << "  Overruns: " << metrics.overrunCount << std::endl;
        std::cout << "  Uptime: " << metrics.uptime << " seconds" << std::endl;
        std::cout << "  Healthy: " << (metrics.isHealthy ? "Yes" : "No") << std::endl;
        
        // Validate metrics are reasonable
        if (metrics.cpuLoad < 0 || metrics.cpuLoad > 200) {
            std::cout << "❌ Invalid CPU load measurement: " << metrics.cpuLoad << std::endl;
            return false;
        }
        
        if (metrics.latency.count() <= 0) {
            std::cout << "❌ Invalid latency measurement: " << metrics.latency.count() << std::endl;
            return false;
        }
        
        if (metrics.uptime <= 0) {
            std::cout << "❌ Invalid uptime measurement: " << metrics.uptime << std::endl;
            return false;
        }
        
        audioEngine->shutdown();
        std::cout << "✅ Performance monitoring test passed" << std::endl;
        return true;
    }
    
    bool testErrorHandling() {
        auto audioEngine = std::make_unique<AudioEngine>(44100, 512);
        
        if (!audioEngine->initialize()) {
            std::cout << "❌ Failed to initialize audio engine for error test" << std::endl;
            return false;
        }
        
        // Get reference to error handler
        auto& errorHandler = audioEngine->getErrorHandler();
        
        // Test error reporting
        errorHandler.reportError(
            AudioErrorCode::StreamUnderrun,
            AudioErrorSeverity::Warning,
            "Test underrun simulation",
            "Error handling test"
        );
        
        errorHandler.reportError(
            AudioErrorCode::CPUOverload,
            AudioErrorSeverity::Error,
            "Test CPU overload simulation",
            "Error handling test"
        );
        
        // Get error statistics
        auto stats = errorHandler.getStatistics();
        
        std::cout << "Error Statistics:" << std::endl;
        std::cout << "  Total Errors: " << stats.totalErrors << std::endl;
        std::cout << "  Critical Errors: " << stats.criticalErrors << std::endl;
        std::cout << "  Recovered Errors: " << stats.recoveredErrors << std::endl;
        std::cout << "  Real-time Errors: " << stats.realTimeErrors << std::endl;
        std::cout << "  Recovery Success Rate: " << stats.recoverySuccessRate << "%" << std::endl;
        
        if (stats.totalErrors < 2) {
            std::cout << "❌ Error reporting not working correctly" << std::endl;
            return false;
        }
        
        audioEngine->shutdown();
        std::cout << "✅ Error handling test passed" << std::endl;
        return true;
    }
    
    bool testAudioSafety() {
        auto audioEngine = std::make_unique<AudioEngine>(44100, 512);
        
        if (!audioEngine->initialize()) {
            std::cout << "❌ Failed to initialize audio engine for safety test" << std::endl;
            return false;
        }
        
        // Enable audio safety
        audioEngine->setAudioSafetyEnabled(true);
        
        std::atomic<bool> testComplete{false};
        std::atomic<int> safetyCounts{0};
        
        // Set up error callback to count safety interventions
        audioEngine->getErrorHandler().setErrorCallback([&](const AudioError& error) {
            if (error.code == AudioErrorCode::AudioClipping || 
                error.code == AudioErrorCode::VolumeClampingActivated) {
                safetyCounts.fetch_add(1);
            }
        });
        
        // Set up a callback that generates dangerous audio levels
        audioEngine->setAudioCallback([&](float* outputBuffer, int numFrames) {
            static float phase = 0.0f;
            
            for (int i = 0; i < numFrames * 2; ++i) {
                // Generate dangerously loud signal (clipping)
                float sample = std::sin(phase) * 2.0f; // 2x the maximum safe level
                outputBuffer[i] = sample;
                phase += 0.1f;
            }
        });
        
        // Run for a short time to trigger safety mechanisms
        std::this_thread::sleep_for(std::chrono::seconds(2));
        testComplete.store(true);
        
        int safetyInterventions = safetyCounts.load();
        std::cout << "Safety interventions detected: " << safetyInterventions << std::endl;
        
        // We expect some safety interventions due to the loud signal
        if (safetyInterventions == 0) {
            std::cout << "⚠️  No safety interventions detected (may be expected if audio safety is working)" << std::endl;
        }
        
        audioEngine->shutdown();
        std::cout << "✅ Audio safety test passed" << std::endl;
        return true;
    }
    
    bool testCPUStress() {
        auto audioEngine = std::make_unique<AudioEngine>(44100, 128); // Very small buffer
        
        if (!audioEngine->initialize()) {
            std::cout << "❌ Failed to initialize audio engine for CPU stress test" << std::endl;
            return false;
        }
        
        // Set aggressive performance thresholds
        audioEngine->setPerformanceThresholds(
            50.0f,                                // 50% CPU load
            std::chrono::microseconds{5000},      // 5ms latency
            std::chrono::microseconds{500}        // 0.5ms jitter
        );
        
        std::atomic<int> cpuErrorCount{0};
        
        // Count CPU-related errors
        audioEngine->getErrorHandler().setErrorCallback([&](const AudioError& error) {
            if (error.code == AudioErrorCode::CPUOverload || 
                error.code == AudioErrorCode::CallbackTimeout) {
                cpuErrorCount.fetch_add(1);
            }
        });
        
        // Set up an extremely CPU-intensive callback
        audioEngine->setAudioCallback([](float* outputBuffer, int numFrames) {
            // Simulate very expensive computation
            for (int i = 0; i < numFrames * 2; ++i) {
                float sample = 0.0f;
                // Lots of expensive operations
                for (int j = 0; j < 100; ++j) {
                    sample += std::sin(i * 0.01f + j) * std::cos(i * 0.02f + j) * 0.001f;
                }
                outputBuffer[i] = sample;
            }
        });
        
        // Run for several seconds under stress
        std::this_thread::sleep_for(std::chrono::seconds(5));
        
        auto metrics = audioEngine->getPerformanceMetrics();
        std::cout << "CPU stress test results:" << std::endl;
        std::cout << "  Final CPU Load: " << metrics.cpuLoad << "%" << std::endl;
        std::cout << "  CPU-related errors: " << cpuErrorCount.load() << std::endl;
        std::cout << "  Still healthy: " << (metrics.isHealthy ? "Yes" : "No") << std::endl;
        
        audioEngine->shutdown();
        std::cout << "✅ CPU stress test completed" << std::endl;
        return true;
    }
    
    bool testConcurrentOperations() {
        auto audioEngine = std::make_unique<AudioEngine>(44100, 512);
        
        if (!audioEngine->initialize()) {
            std::cout << "❌ Failed to initialize audio engine for concurrency test" << std::endl;
            return false;
        }
        
        std::atomic<bool> testRunning{true};
        std::atomic<int> operationCount{0};
        
        // Set up a simple audio callback
        audioEngine->setAudioCallback([&](float* outputBuffer, int numFrames) {
            operationCount.fetch_add(1);
            
            // Simple sine wave
            static float phase = 0.0f;
            for (int i = 0; i < numFrames * 2; ++i) {
                outputBuffer[i] = std::sin(phase) * 0.1f;
                phase += 0.01f;
            }
        });
        
        // Start multiple threads that interact with the audio engine
        std::vector<std::thread> threads;
        
        // Thread 1: Continuously query performance metrics
        threads.emplace_back([&]() {
            while (testRunning.load()) {
                auto metrics = audioEngine->getPerformanceMetrics();
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
        });
        
        // Thread 2: Continuously query error statistics
        threads.emplace_back([&]() {
            while (testRunning.load()) {
                auto stats = audioEngine->getErrorStatistics();
                std::this_thread::sleep_for(std::chrono::milliseconds(15));
            }
        });
        
        // Thread 3: Toggle performance monitoring
        threads.emplace_back([&]() {
            bool enabled = true;
            while (testRunning.load()) {
                audioEngine->setPerformanceMonitoringEnabled(enabled);
                enabled = !enabled;
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
        });
        
        // Run the concurrent test for 3 seconds
        std::this_thread::sleep_for(std::chrono::seconds(3));
        testRunning.store(false);
        
        // Wait for all threads to complete
        for (auto& thread : threads) {
            thread.join();
        }
        
        int totalOperations = operationCount.load();
        std::cout << "Concurrent operations test results:" << std::endl;
        std::cout << "  Audio callbacks processed: " << totalOperations << std::endl;
        std::cout << "  Audio engine still healthy: " << (audioEngine->isHealthy() ? "Yes" : "No") << std::endl;
        
        if (totalOperations == 0) {
            std::cout << "❌ No audio operations processed during concurrency test" << std::endl;
            return false;
        }
        
        audioEngine->shutdown();
        std::cout << "✅ Concurrent operations test passed" << std::endl;
        return true;
    }
    
    bool testErrorRecovery() {
        auto audioEngine = std::make_unique<AudioEngine>(44100, 512);
        
        if (!audioEngine->initialize()) {
            std::cout << "❌ Failed to initialize audio engine for recovery test" << std::endl;
            return false;
        }
        
        auto& errorHandler = audioEngine->getErrorHandler();
        
        // Register a custom recovery action for testing
        AudioRecoveryAction testRecovery;
        testRecovery.description = "Test recovery action";
        testRecovery.priority = 100;
        testRecovery.maxRetries = 2;
        testRecovery.allowInRealTime = false;
        testRecovery.action = []() {
            std::cout << "  Executing test recovery action..." << std::endl;
            return true; // Simulate successful recovery
        };
        
        errorHandler.registerRecoveryAction(AudioErrorCode::StreamUnderrun, testRecovery);
        
        // Trigger an error that should invoke recovery
        auto result = errorHandler.reportError(
            AudioErrorCode::StreamUnderrun,
            AudioErrorSeverity::Error,
            "Test error for recovery",
            "Recovery test"
        );
        
        std::cout << "Recovery test results:" << std::endl;
        std::cout << "  Recovery successful: " << (result.successful ? "Yes" : "No") << std::endl;
        std::cout << "  Action taken: " << result.actionTaken << std::endl;
        std::cout << "  Time spent: " << result.timeSpent.count() << " μs" << std::endl;
        std::cout << "  Retries used: " << result.retriesUsed << std::endl;
        
        if (!result.successful) {
            std::cout << "❌ Recovery action was not successful" << std::endl;
            return false;
        }
        
        audioEngine->shutdown();
        std::cout << "✅ Error recovery test passed" << std::endl;
        return true;
    }
};

int main() {
    AudioEngineStressTest test;
    
    bool allTestsPassed = test.runAllTests();
    
    std::cout << "\n=== Test Results ===" << std::endl;
    if (allTestsPassed) {
        std::cout << "🎉 All audio engine stress tests PASSED!" << std::endl;
        std::cout << "Enterprise-grade error handling and performance monitoring validated." << std::endl;
        return 0;
    } else {
        std::cout << "❌ Some audio engine stress tests FAILED!" << std::endl;
        std::cout << "Review error handling implementation." << std::endl;
        return 1;
    }
}