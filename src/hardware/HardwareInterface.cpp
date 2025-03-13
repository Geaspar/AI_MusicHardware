#include "../../include/hardware/HardwareInterface.h"

namespace AIMusicHardware {

HardwareInterface::HardwareInterface() 
    : isInitialized_(false) {
}

HardwareInterface::~HardwareInterface() {
    shutdown();
}

bool HardwareInterface::initialize() {
    // Stub implementation - will be expanded with actual hardware interface
    isInitialized_ = true;
    return true;
}

void HardwareInterface::shutdown() {
    if (isInitialized_) {
        // Clean up hardware resources
        isInitialized_ = false;
    }
}

bool HardwareInterface::registerControlCallback(ControlType type, int controlId, ControlCallback callback) {
    if (!isInitialized_) {
        return false;
    }
    
    // Store callback
    controlCallbacks_[std::make_pair(type, controlId)] = callback;
    return true;
}

} // namespace AIMusicHardware