#pragma once

namespace AIMusicHardware {

// Minimal host sync bridge. In a plugin context, feed host BPM/PPQ to Transport.
class HostSync {
public:
    void updateFromHost(double hostBpm, double ppqPosition, double sampleRate);

    double getHostBpm() const { return hostBpm_; }
    double getHostPpq() const { return hostPpq_; }
    double getSampleRate() const { return sampleRate_; }

private:
    double hostBpm_ = 120.0;
    double hostPpq_ = 0.0; // quarter-note position
    double sampleRate_ = 48000.0;
};

} // namespace AIMusicHardware

