#include "../../../include/synthesis/framework/processor.h"

namespace AIMusicHardware {

// Processor implementation
Processor::Processor(int sampleRate)
    : sampleRate_(sampleRate),
      enabled_(true),
      router_(nullptr) {
}

Processor::~Processor() {
}

void Processor::reset() {
    // Base implementation does nothing
}

void Processor::setSampleRate(int sampleRate) {
    sampleRate_ = sampleRate;
}

// ProcessorRouter implementation
ProcessorRouter::ProcessorRouter(int sampleRate)
    : Processor(sampleRate) {
    // Initialize temp buffer with a reasonable size
    tempBuffer_.resize(8192);
}

ProcessorRouter::~ProcessorRouter() {
    clearProcessors();
}

void ProcessorRouter::addProcessor(std::unique_ptr<Processor> processor) {
    if (processor) {
        processor->setRouter(this);
        processor->setSampleRate(sampleRate_);
        processors_.push_back(std::move(processor));
    }
}

void ProcessorRouter::removeProcessor(Processor* processor) {
    for (auto it = processors_.begin(); it != processors_.end(); ++it) {
        if (it->get() == processor) {
            processors_.erase(it);
            return;
        }
    }
}

void ProcessorRouter::removeProcessor(size_t index) {
    if (index < processors_.size()) {
        processors_.erase(processors_.begin() + index);
    }
}

Processor* ProcessorRouter::getProcessor(size_t index) {
    if (index < processors_.size()) {
        return processors_[index].get();
    }
    return nullptr;
}

size_t ProcessorRouter::getNumProcessors() const {
    return processors_.size();
}

void ProcessorRouter::clearProcessors() {
    processors_.clear();
}

void ProcessorRouter::process(float* buffer, int numFrames) {
    if (!enabled_ || processors_.empty()) {
        return;
    }
    
    // Make sure our temp buffer is large enough (stereo buffer)
    if (tempBuffer_.size() < static_cast<size_t>(numFrames * 2)) {
        tempBuffer_.resize(numFrames * 2);
    }
    
    // Process each processor in series
    for (auto& processor : processors_) {
        if (processor->isEnabled()) {
            // Copy input buffer to temp buffer
            std::copy(buffer, buffer + numFrames * 2, tempBuffer_.data());
            
            // Process audio in temp buffer
            processor->process(tempBuffer_.data(), numFrames);
            
            // Copy back to input buffer
            std::copy(tempBuffer_.data(), tempBuffer_.data() + numFrames * 2, buffer);
        }
    }
}

void ProcessorRouter::reset() {
    Processor::reset();
    
    // Reset all child processors
    for (auto& processor : processors_) {
        processor->reset();
    }
}

void ProcessorRouter::setSampleRate(int sampleRate) {
    Processor::setSampleRate(sampleRate);
    
    // Update child processors
    for (auto& processor : processors_) {
        processor->setSampleRate(sampleRate);
    }
}

} // namespace AIMusicHardware