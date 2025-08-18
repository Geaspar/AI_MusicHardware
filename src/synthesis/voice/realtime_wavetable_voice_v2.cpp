#include "synthesis/voice/realtime_wavetable_voice_v2.h"
#include <algorithm>
#include <cmath>
#include "synthesis/modulators/envelope.h"

namespace AIMusicHardware {

RealtimeWavetableVoiceV2::RealtimeWavetableVoiceV2(std::shared_ptr<SpectralWavetableCache> cache,
                                                   std::unique_ptr<SpectralRenderWorker>& worker,
                                                   int sampleRate)
: Voice(sampleRate), cache_(std::move(cache)), worker_(worker) {
}

void RealtimeWavetableVoiceV2::process(float* outputBuffer, int numSamples) {
    if (getState() == State::Inactive || table_ == nullptr) {
        // Inactive or not ready: do not modify buffer (let others accumulate)
        return;
    }

    // Update pitch modulation and frequency once per call (block-like)
    float targetPitch = pitchMod_.calculateTotalPitch();
    pitchMod_.updateSmoothedPitch(targetPitch);
    // base class helper computes frequency_ from smoothed pitch
    updateOscillatorFrequency();

    // Control-rate request (once per block): compute cache key
    CacheKey key{};
    key.tableIdHash = 0xABCDEF; // placeholder until we wire real IDs
    key.morphQ = quantizeMorph01(morph01_);
    // Approximate pitch band using current frequency in semitones relative to A4
    float semis = 12.0f * std::log2(std::max(1e-3f, frequency_) / 440.0f) + 69.0f;
    key.pitchBand = quantizePitchBand(semis);
    key.opsHash16 = quantizeOpsHash(ops_);
    key.fftSizeCode = 1; // 2048
    key.quality = 0; key.sampleRateQ = static_cast<uint16_t>(getSampleRate() / 100);

    SpectralJobSpec spec{ table_, morph01_, ops_, 2048, getSampleRate() };
    if (!current_) current_ = worker_.get()->requestRender(key, spec);
    // Prewarm neighbors to avoid stalls under fast morph
    worker_.get()->prewarmHints(table_, morph01_, ops_, 2048, getSampleRate());
    if (!pending_) {
        // Try to get fresh buffer; in real impl we'd compare hash/generation
        pending_ = worker_.get()->requestRender(key, spec);
        if (pending_ && pending_ != current_) {
            previous_ = current_;
            current_ = pending_;
            pending_.reset();
            // Set up a short equal-power crossfade of one cycle length
            crossfadeTotalSamples_ = static_cast<int>(previous_ ? previous_->samples.size() : 0);
            // Limit crossfade to max 2048 samples to keep snappy
            crossfadeTotalSamples_ = std::min(crossfadeTotalSamples_ > 0 ? crossfadeTotalSamples_ : 0, 2048);
            crossfadeSamplesRemaining_ = crossfadeTotalSamples_;
        }
    }

    const float* cur = current_ ? current_->samples.data() : nullptr;
    const float* pen = pending_ ? pending_->samples.data() : nullptr;
    size_t N = pending_ ? pending_->samples.size() : (current_ ? current_->samples.size() : 0);
    if (!cur && !pen) {
        // Nothing to render this block; leave buffer untouched
        return;
    }

    // For now, snap to pending if available; later we crossfade over one cycle
    if (pending_) current_ = pending_;
    pending_.reset();

    // Render using phase accumulator from current_ buffer
    const float* wt = current_->samples.data();
    const size_t wtLen = current_->samples.size();
    const float* wtPrev = (previous_ && crossfadeSamplesRemaining_ > 0) ? previous_->samples.data() : nullptr;
    const size_t wtPrevLen = wtPrev ? previous_->samples.size() : 0;
    const double phaseInc = static_cast<double>(frequency_) / static_cast<double>(getSampleRate());

    for (int i = 0; i < numSamples; ++i) {
        double idx = phase_ * wtLen;
        size_t i0 = static_cast<size_t>(idx);
        size_t i1 = (i0 + 1) % wtLen;
        float t = static_cast<float>(idx - i0);
        float s = wt[i0] * (1.0f - t) + wt[i1] * t;
        if (wtPrev && wtPrevLen == wtLen && crossfadeSamplesRemaining_ > 0) {
            double idxPrev = phase_ * wtPrevLen;
            size_t j0 = static_cast<size_t>(idxPrev);
            size_t j1 = (j0 + 1) % wtPrevLen;
            float u = static_cast<float>(idxPrev - j0);
            float sPrev = wtPrev[j0] * (1.0f - u) + wtPrev[j1] * u;
            float x = 1.0f - static_cast<float>(crossfadeTotalSamples_ - crossfadeSamplesRemaining_) / std::max(1, crossfadeTotalSamples_);
            // equal-power crossfade
            float a = std::sin(0.5f * 3.14159265f * (1.0f - x));
            float b = std::sin(0.5f * 3.14159265f * x);
            s = a * s + b * sPrev;
        }
        // Apply amplitude envelope and velocity
        float env = envelope_ ? envelope_->generateValue() : 1.0f;
        s *= env * velocity_;
        // mono to stereo (accumulate into shared output buffer)
        outputBuffer[2*i]   += s;
        outputBuffer[2*i+1] += s;
        phase_ += phaseInc;
        if (phase_ >= 1.0) phase_ -= 1.0;
    }

    // Update state transitions based on envelope
    if (state_ == State::Starting && envelope_ && envelope_->getCurrentValue() > 0.01f) {
        state_ = State::Playing;
    } else if (state_ == State::Released && envelope_ && !envelope_->isActive()) {
        state_ = State::Finished;
    }
    if (crossfadeSamplesRemaining_ > 0) {
        crossfadeSamplesRemaining_ = std::max(0, crossfadeSamplesRemaining_ - numSamples);
        if (crossfadeSamplesRemaining_ == 0) previous_.reset();
    }
}

} // namespace AIMusicHardware
