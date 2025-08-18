#pragma once

#include "synthesis/voice/voice.h"
#include "synthesis/wavetable/hybrid_wavetable.h"
#include "synthesis/wavetable/hybrid_wavetable_cache.h"
#include <memory>

namespace AIMusicHardware {

class RealtimeWavetableVoiceV2 : public Voice {
public:
    RealtimeWavetableVoiceV2(std::shared_ptr<SpectralWavetableCache> cache,
                              std::unique_ptr<SpectralRenderWorker>& worker,
                              int sampleRate);

    void setSpectralTable(const SpectralTable* table) { table_ = table; }
    void setSpectralOps(const SpectralOps& ops) { ops_ = ops; }
    void setMorph01(float t) { morph01_ = t; }

    void process(float* outputBuffer, int numSamples) override;

private:
    const SpectralTable* table_ = nullptr;
    SpectralOps ops_{};
    float morph01_ = 0.0f;

    std::shared_ptr<SpectralWavetableCache> cache_;
    std::unique_ptr<SpectralRenderWorker>& worker_;

    std::shared_ptr<WavetableBuffer> current_;
    std::shared_ptr<WavetableBuffer> pending_;
    std::shared_ptr<WavetableBuffer> previous_;
    int crossfadeSamplesRemaining_ = 0;
    int crossfadeTotalSamples_ = 0;

    double phase_ = 0.0;
};

} // namespace AIMusicHardware
