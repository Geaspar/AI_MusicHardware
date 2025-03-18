#pragma once

#include <string>
#include <memory>
#include <vector>

namespace AIMusicHardware {

// Forward declarations
class ValueInterface;
class ProcessorRouter;

/**
 * Base Processor class for all audio processing components.
 * Similar to Vital's processor class but simplified.
 */
class Processor {
public:
    Processor(int sampleRate = 44100);
    virtual ~Processor();
    
    // Core processing method
    virtual void process(float* buffer, int numFrames) = 0;
    
    // Processor state control
    virtual void reset();
    
    // Sample rate management
    virtual void setSampleRate(int sampleRate);
    int getSampleRate() const { return sampleRate_; }
    
    // Router management
    void setRouter(ProcessorRouter* router) { router_ = router; }
    ProcessorRouter* getRouter() const { return router_; }
    
    // Identification
    virtual std::string getName() const = 0;
    
    // Enabled state
    bool isEnabled() const { return enabled_; }
    virtual void setEnabled(bool enabled) { enabled_ = enabled; }
    
protected:
    int sampleRate_;
    bool enabled_;
    ProcessorRouter* router_;
};

/**
 * Router for connecting multiple processors in series or parallel.
 */
class ProcessorRouter : public Processor {
public:
    ProcessorRouter(int sampleRate = 44100);
    ~ProcessorRouter() override;
    
    // Add a processor to the chain
    void addProcessor(std::unique_ptr<Processor> processor);
    
    // Remove a processor
    void removeProcessor(Processor* processor);
    void removeProcessor(size_t index);
    
    // Access processors
    Processor* getProcessor(size_t index);
    size_t getNumProcessors() const;
    
    // Clear all processors
    void clearProcessors();
    
    // ProcessorRouter implementation
    void process(float* buffer, int numFrames) override;
    void reset() override;
    void setSampleRate(int sampleRate) override;
    std::string getName() const override { return "ProcessorRouter"; }
    
private:
    std::vector<std::unique_ptr<Processor>> processors_;
    std::vector<float> tempBuffer_;
};

} // namespace AIMusicHardware