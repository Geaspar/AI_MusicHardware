#pragma once

#include <string>
#include <vector>
#include <complex>
#include "nlohmann/json.hpp"

class FrequencyDomainWavetable {
public:
    FrequencyDomainWavetable() = default;

    bool loadFromFile(const std::string& path);
    bool saveToFile(const std::string& path) const;

    const std::vector<std::complex<float>>& getHarmonicData(int frame) const;
    int getNumFrames() const { return num_frames_; }
    const std::string& getName() const { return name_; }

    void setName(const std::string& name) { name_ = name; }
    void setNumFrames(int num_frames) { num_frames_ = num_frames; }
    void setHarmonicData(int frame, const std::vector<std::complex<float>>& data);

private:
    std::string name_;
    int num_frames_ = 0;
    std::vector<std::vector<std::complex<float>>> harmonic_data_;
};