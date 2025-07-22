#include "synthesis/FrequencyDomainWavetable.h"
#include "nlohmann/json.hpp"
#include <fstream>

using json = nlohmann::json;

bool FrequencyDomainWavetable::loadFromFile(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        return false;
    }

    json j;
    file >> j;

    name_ = j["name"];
    num_frames_ = j["num_frames"];

    harmonic_data_.resize(num_frames_);
    for (int i = 0; i < num_frames_; ++i) {
        const auto& frame_data = j["harmonic_data"][i];
        harmonic_data_[i].resize(frame_data.size());
        for (size_t k = 0; k < frame_data.size(); ++k) {
            harmonic_data_[i][k] = std::complex<float>(
                frame_data[k][0].get<float>(),
                frame_data[k][1].get<float>()
            );
        }
    }

    return true;
}

bool FrequencyDomainWavetable::saveToFile(const std::string& path) const {
    json j;
    j["name"] = name_;
    j["num_frames"] = num_frames_;

    for (int i = 0; i < num_frames_; ++i) {
        for (const auto& harmonic : harmonic_data_[i]) {
            j["harmonic_data"][i].push_back({harmonic.real(), harmonic.imag()});
        }
    }

    std::ofstream file(path);
    if (!file.is_open()) {
        return false;
    }

    file << j.dump(4);
    return true;
}

const std::vector<std::complex<float>>& FrequencyDomainWavetable::getHarmonicData(int frame) const {
    return harmonic_data_[frame];
}

void FrequencyDomainWavetable::setHarmonicData(int frame, const std::vector<std::complex<float>>& data) {
    if (frame >= harmonic_data_.size()) {
        harmonic_data_.resize(frame + 1);
    }
    harmonic_data_[frame] = data;
}