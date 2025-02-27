#include "../../include/effects/EffectProcessor.h"
#include "../../include/effects/EffectUtils.h"
#include <vector>
#include <cstring>

namespace AIMusicHardware {

// Reverb implementation
class Reverb::Impl {
public:
    Impl(int sampleRate) 
        : sampleRate(sampleRate),
          roomSize(0.5f),
          damping(0.5f),
          wetLevel(0.33f),
          dryLevel(0.7f),
          width(1.0f) {
        
        // Create a basic reverb using 8 parallel comb filters + allpass filters
        // This is a simplified Schroeder reverb algorithm
        
        // Initialize comb filters with different lengths
        const float combTunings[] = {
            0.0298f, 0.0333f, 0.0371f, 0.0411f,
            0.0447f, 0.0479f, 0.0509f, 0.0559f
        };
        
        // Initialize allpass filters
        const float allpassTunings[] = {
            0.005f, 0.0017f, 0.0013f, 0.0011f
        };
        
        // Create comb filter buffers
        for (int i = 0; i < 8; ++i) {
            int bufSize = static_cast<int>(sampleRate * combTunings[i]) * 2;
            combBuffers.push_back(std::vector<float>(bufSize, 0.0f));
            combWritePos.push_back(0);
        }
        
        // Create allpass filter buffers
        for (int i = 0; i < 4; ++i) {
            int bufSize = static_cast<int>(sampleRate * allpassTunings[i]) * 2;
            allpassBuffers.push_back(std::vector<float>(bufSize, 0.0f));
            allpassWritePos.push_back(0);
        }
    }
    
    void process(float* buffer, int numFrames) {
        // Simple reverb algorithm
        for (int i = 0; i < numFrames * 2; i += 2) {
            float inputL = buffer[i];
            float inputR = buffer[i + 1];
            
            float wetL = 0.0f;
            float wetR = 0.0f;
            
            // Comb filters in parallel
            for (size_t j = 0; j < combBuffers.size(); ++j) {
                // L channel
                int readPos = combWritePos[j] - 1;
                if (readPos < 0) readPos += combBuffers[j].size() / 2;
                
                float output = combBuffers[j][readPos * 2];
                combBuffers[j][combWritePos[j] * 2] = inputL + output * (roomSize * 0.9f);
                wetL += output;
                
                // R channel
                output = combBuffers[j][readPos * 2 + 1];
                combBuffers[j][combWritePos[j] * 2 + 1] = inputR + output * (roomSize * 0.9f);
                wetR += output;
                
                // Update position
                combWritePos[j] = (combWritePos[j] + 1) % (combBuffers[j].size() / 2);
            }
            
            // Allpass filters in series
            for (size_t j = 0; j < allpassBuffers.size(); ++j) {
                // L channel processing
                int apReadPos = allpassWritePos[j] - 1;
                if (apReadPos < 0) apReadPos += allpassBuffers[j].size() / 2;
                
                float allpassOut = allpassBuffers[j][apReadPos * 2];
                float temp = wetL + allpassOut * 0.5f;
                allpassBuffers[j][allpassWritePos[j] * 2] = temp;
                wetL = allpassOut - temp;
                
                // R channel processing
                allpassOut = allpassBuffers[j][apReadPos * 2 + 1];
                temp = wetR + allpassOut * 0.5f;
                allpassBuffers[j][allpassWritePos[j] * 2 + 1] = temp;
                wetR = allpassOut - temp;
                
                // Update position
                allpassWritePos[j] = (allpassWritePos[j] + 1) % (allpassBuffers[j].size() / 2);
            }
            
            // Apply width, wet and dry levels
            float outL = wetL * wetLevel + inputL * dryLevel;
            float outR = wetR * wetLevel + inputR * dryLevel;
            
            // Apply width by manipulating stereo image
            if (width != 1.0f) {
                float mono = (outL + outR) * 0.5f;
                float stereo = (outR - outL) * 0.5f * width;
                outL = mono - stereo;
                outR = mono + stereo;
            }
            
            // Store the result
            buffer[i] = outL;
            buffer[i + 1] = outR;
        }
    }
    
    int sampleRate;
    float roomSize;
    float damping;
    float wetLevel;
    float dryLevel;
    float width;
    
    std::vector<std::vector<float>> combBuffers;
    std::vector<int> combWritePos;
    
    std::vector<std::vector<float>> allpassBuffers;
    std::vector<int> allpassWritePos;
};

Reverb::Reverb(int sampleRate) 
    : Effect(sampleRate),
      roomSize_(0.5f),
      damping_(0.5f),
      wetLevel_(0.33f),
      dryLevel_(0.7f),
      width_(1.0f),
      pimpl_(new Impl(sampleRate)) {
}

Reverb::~Reverb() {
}

void Reverb::process(float* buffer, int numFrames) {
    pimpl_->process(buffer, numFrames);
}

void Reverb::setParameter(const std::string& name, float value) {
    if (name == "roomSize") {
        roomSize_ = clamp(value, 0.0f, 1.0f);
        pimpl_->roomSize = roomSize_;
    }
    else if (name == "damping") {
        damping_ = clamp(value, 0.0f, 1.0f);
        pimpl_->damping = damping_;
    }
    else if (name == "wetLevel") {
        wetLevel_ = clamp(value, 0.0f, 1.0f);
        pimpl_->wetLevel = wetLevel_;
    }
    else if (name == "dryLevel") {
        dryLevel_ = clamp(value, 0.0f, 1.0f);
        pimpl_->dryLevel = dryLevel_;
    }
    else if (name == "width") {
        width_ = clamp(value, 0.0f, 1.0f);
        pimpl_->width = width_;
    }
}

float Reverb::getParameter(const std::string& name) const {
    if (name == "roomSize") {
        return roomSize_;
    }
    else if (name == "damping") {
        return damping_;
    }
    else if (name == "wetLevel") {
        return wetLevel_;
    }
    else if (name == "dryLevel") {
        return dryLevel_;
    }
    else if (name == "width") {
        return width_;
    }
    
    return 0.0f;
}

} // namespace AIMusicHardware