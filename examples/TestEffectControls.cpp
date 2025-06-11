#include <iostream>
#include <memory>
#include "../include/audio/AudioEngine.h"
#include "../include/effects/EffectProcessor.h"
#include "../include/effects/AllEffects.h"

using namespace AIMusicHardware;

int main() {
    std::cout << "Testing Effect Controls..." << std::endl;
    
    // Create effect processor
    auto effectProcessor = std::make_unique<EffectProcessor>();
    effectProcessor->initialize();
    
    // Add filter as first effect
    auto filter = std::make_unique<Filter>(44100, Filter::Type::LowPass);
    effectProcessor->addEffect(std::move(filter));
    std::cout << "Added filter. Total effects: " << effectProcessor->getNumEffects() << std::endl;
    
    // Test 1: Add distortion effect
    std::cout << "\nTest 1: Adding Distortion effect..." << std::endl;
    auto distortion = createEffectComplete("Distortion", 44100);
    if (distortion) {
        distortion->setParameter("drive", 5.0f);
        distortion->setParameter("mix", 0.8f);
        effectProcessor->addEffect(std::move(distortion));
        std::cout << "Added distortion. Total effects: " << effectProcessor->getNumEffects() << std::endl;
        
        // Test mix control
        if (effectProcessor->getNumEffects() > 1) {
            if (auto* effect = effectProcessor->getEffect(1)) {
                std::cout << "Effect name: " << effect->getName() << std::endl;
                
                // Set mix to 0.5
                effect->setParameter("mix", 0.5f);
                float mix = effect->getParameter("mix");
                std::cout << "Mix set to: " << mix << std::endl;
                
                // Test bypass (mix = 0)
                effect->setParameter("mix", 0.0f);
                mix = effect->getParameter("mix");
                std::cout << "Bypassed (mix = 0): " << mix << std::endl;
                
                // Re-enable
                effect->setParameter("mix", 0.8f);
                mix = effect->getParameter("mix");
                std::cout << "Re-enabled (mix = 0.8): " << mix << std::endl;
            }
        }
    }
    
    // Test 2: Replace with Reverb
    std::cout << "\nTest 2: Replacing with Reverb effect..." << std::endl;
    
    // Remove distortion
    while (effectProcessor->getNumEffects() > 1) {
        effectProcessor->removeEffect(effectProcessor->getNumEffects() - 1);
    }
    
    auto reverb = createEffectComplete("Reverb", 44100);
    if (reverb) {
        reverb->setParameter("wetLevel", 0.3f);
        reverb->setParameter("dryLevel", 0.7f);
        effectProcessor->addEffect(std::move(reverb));
        std::cout << "Added reverb. Total effects: " << effectProcessor->getNumEffects() << std::endl;
        
        // Test mix control for reverb
        if (effectProcessor->getNumEffects() > 1) {
            if (auto* effect = effectProcessor->getEffect(1)) {
                std::cout << "Effect name: " << effect->getName() << std::endl;
                
                // Set mix to 0.5 (wetLevel/dryLevel)
                effect->setParameter("wetLevel", 0.5f);
                effect->setParameter("dryLevel", 0.5f);
                float wet = effect->getParameter("wetLevel");
                float dry = effect->getParameter("dryLevel");
                std::cout << "Mix set to 50%: wet=" << wet << " dry=" << dry << std::endl;
                
                // Test bypass
                effect->setParameter("wetLevel", 0.0f);
                effect->setParameter("dryLevel", 1.0f);
                wet = effect->getParameter("wetLevel");
                dry = effect->getParameter("dryLevel");
                std::cout << "Bypassed: wet=" << wet << " dry=" << dry << std::endl;
            }
        }
    }
    
    std::cout << "\nTest completed successfully!" << std::endl;
    return 0;
}