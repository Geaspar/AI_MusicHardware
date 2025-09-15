#include "../../include/sequencer/HostSync.h"

namespace AIMusicHardware {

void HostSync::updateFromHost(double hostBpm, double ppqPosition, double sampleRate) {
    hostBpm_ = hostBpm;
    hostPpq_ = ppqPosition;
    sampleRate_ = sampleRate;
}

} // namespace AIMusicHardware

