#include "synthesis/wavetable/hybrid_wavetable_ops.h"
#include "audio/FourierTransform.h"
#include "synthesis/wavetable/wavetable.h"
#include <algorithm>
#include <numeric>
#include <complex>

namespace AIMusicHardware {

static inline float dbToLin(float db) { return std::pow(10.0f, db / 20.0f); }

static void applyTilt(std::vector<SpectralBin>& bins, float tiltDbPerOct, int sampleRate, int fftSize) {
    if (std::abs(tiltDbPerOct) < 1e-6f) return;
    float binHz = static_cast<float>(sampleRate) / static_cast<float>(fftSize);
    for (size_t i = 1; i < bins.size(); ++i) { // skip DC
        float f = binHz * static_cast<float>(i);
        if (f <= 0.0f) continue;
        float oct = std::log2(f / 1000.0f); // pivot 1 kHz
        float gain = dbToLin(tiltDbPerOct * oct);
        bins[i].magnitude *= gain;
    }
}

static void applyEvenOdd(std::vector<SpectralBin>& bins, float evenOddBalance) {
    if (std::abs(evenOddBalance) < 1e-6f) return;
    // negative -> favor even, positive -> favor odd
    for (size_t i = 1; i < bins.size(); ++i) {
        bool odd = (i & 1) == 1;
        float g = odd ? (1.0f + evenOddBalance) : (1.0f - evenOddBalance);
        bins[i].magnitude *= std::clamp(g, 0.0f, 2.0f);
    }
}

static void applyShelves(std::vector<SpectralBin>& bins, const SpectralShelves& sh, int sampleRate, int fftSize) {
    float binHz = static_cast<float>(sampleRate) / static_cast<float>(fftSize);
    float lowG = dbToLin(sh.lowGainDb);
    float highG = dbToLin(sh.highGainDb);
    for (size_t i = 1; i < bins.size(); ++i) {
        float f = binHz * static_cast<float>(i);
        if (f < sh.lowCutoffHz) bins[i].magnitude *= lowG;
        if (f > sh.highCutoffHz) bins[i].magnitude *= highG;
    }
}

static void applyFormant(std::vector<SpectralBin>& bins, float semis, int sampleRate, int fftSize) {
    if (std::abs(semis) < 1e-6f) return;
    // Simple remap: resample magnitudes on log-frequency axis
    std::vector<float> mags(bins.size());
    for (size_t i = 0; i < bins.size(); ++i) mags[i] = bins[i].magnitude;
    float ratio = std::pow(2.0f, semis / 12.0f);
    float binHz = static_cast<float>(sampleRate) / static_cast<float>(fftSize);
    for (size_t i = 1; i < bins.size(); ++i) {
        float f = binHz * static_cast<float>(i) / ratio;
        float idx = f / binHz;
        size_t i0 = static_cast<size_t>(std::floor(idx));
        size_t i1 = std::min(i0 + 1, bins.size() - 1);
        float t = static_cast<float>(idx - i0);
        float m = (i0 < mags.size()) ? ((1.0f - t) * mags[i0] + t * mags[i1]) : 0.0f;
        bins[i].magnitude = m;
    }
}

SpectralFrame buildSpectralFrame(const SpectralTable& table,
                                 float morph01,
                                 const SpectralOps& ops,
                                 int fftSize,
                                 int sampleRate) {
    SpectralFrame out;
    if (table.frames.empty()) {
        out.fftSize = fftSize; out.sampleRate = sampleRate; out.bins.resize(static_cast<size_t>(fftSize/2 + 1));
        return out;
    }
    // Clamp inputs
    morph01 = std::clamp(morph01, 0.0f, 1.0f);
    size_t last = table.frames.size() - 1;
    float pos = morph01 * static_cast<float>(last);
    size_t i0 = static_cast<size_t>(std::floor(pos));
    size_t i1 = std::min(i0 + 1, last);
    float t = pos - static_cast<float>(i0);

    const SpectralFrame& A = table.frames[i0];
    const SpectralFrame& B = table.frames[i1];
    size_t binsCount = static_cast<size_t>(fftSize/2 + 1);
    out.bins.resize(binsCount);
    out.fftSize = fftSize;
    out.sampleRate = sampleRate;

    auto wrapPI = [](float x){ while (x > 3.14159265f) x -= 6.2831853f; while (x < -3.14159265f) x += 6.2831853f; return x; };

    for (size_t i = 0; i < binsCount; ++i) {
        float magA = (i < A.bins.size()) ? A.bins[i].magnitude : 0.0f;
        float magB = (i < B.bins.size()) ? B.bins[i].magnitude : 0.0f;
        float phA = (i < A.bins.size()) ? A.bins[i].phase : 0.0f;
        float phB = (i < B.bins.size()) ? B.bins[i].phase : 0.0f;
        float mag = (1.0f - t) * magA + t * magB; // linear; could be dB domain
        float dph = wrapPI(phB - phA);
        float ph = wrapPI(phA + t * dph);
        out.bins[i] = {mag, ph};
    }

    // Apply spectral ops
    applyTilt(out.bins, ops.tiltDbPerOct, sampleRate, fftSize);
    applyFormant(out.bins, ops.formantShiftSemitones, sampleRate, fftSize);
    applyEvenOdd(out.bins, ops.evenOddBalance);
    applyShelves(out.bins, ops.shelves, sampleRate, fftSize);
    // harmonicWarp could remap index nonlinearly (todo)

    // Nyquist mask (ensure bins above Nyquist attenuate)
    // For real IFFT, bins up to N/2 included; assume later band-limiting via FFT size + ops is enough.

    return out;
}

static inline void removeDc(std::vector<float>& x) {
    float mean = std::accumulate(x.begin(), x.end(), 0.0f) / std::max<size_t>(1, x.size());
    for (auto& s : x) s -= mean;
}

void removeDcAndNormalize(std::vector<float>& samples, float targetRms) {
    removeDc(samples);
    // Compute RMS
    double acc = 0.0;
    for (float v : samples) acc += static_cast<double>(v) * v;
    double rms = std::sqrt(acc / std::max<size_t>(1, samples.size()));
    if (rms <= 1e-9) return;
    float g = static_cast<float>(targetRms / rms);
    for (auto& v : samples) v = std::clamp(v * g, -1.0f, 1.0f);
}

WavetableBuffer renderTimeDomain(const SpectralFrame& spectral, int sampleRate, bool minPhase) {
    WavetableBuffer out;
    out.fftSize = spectral.fftSize;
    out.samples.resize(static_cast<size_t>(spectral.fftSize));

    // Build complex vector for IFFT from magnitude/phase (Hermitian symmetry implied)
    std::vector<std::complex<float>> bins(static_cast<size_t>(spectral.fftSize));
    size_t half = static_cast<size_t>(spectral.fftSize / 2);
    if (!minPhase) {
        for (size_t i = 0; i <= half; ++i) {
            float mag = (i < spectral.bins.size()) ? spectral.bins[i].magnitude : 0.0f;
            float ph = (i < spectral.bins.size()) ? spectral.bins[i].phase : 0.0f;
            std::complex<float> c = std::polar(mag, ph);
            bins[i] = c;
            if (i != 0 && i != half) bins[spectral.fftSize - i] = std::conj(c); // mirror
        }
    } else {
        // Minimum-phase reconstruction via real-cepstrum method on magnitudes
        // 1) Build log magnitude spectrum (symmetric)
        std::vector<std::complex<float>> logSpec(static_cast<size_t>(spectral.fftSize), {0.0f, 0.0f});
        for (size_t i = 0; i <= half; ++i) {
            float mag = (i < spectral.bins.size()) ? std::max(1e-12f, spectral.bins[i].magnitude) : 1e-12f;
            float ln = std::log(mag);
            logSpec[i] = {ln, 0.0f};
            if (i != 0 && i != half) logSpec[spectral.fftSize - i] = {ln, 0.0f};
        }
        // 2) IFFT to real cepstrum
        FourierTransform ft;
        ft.performIFFT(logSpec);
        // 3) Keep causal half, double it (except DC), zero anti-causal (min-phase cepstrum)
        for (size_t n = 1; n < half; ++n) {
            logSpec[n] = {2.0f * logSpec[n].real(), 0.0f};
            logSpec[spectral.fftSize - n] = {0.0f, 0.0f};
        }
        // 4) FFT back to complex log spectrum
        ft.performFFT(logSpec);
        // 5) Exponentiate to get complex spectrum and enforce Hermitian symmetry
        for (size_t i = 0; i <= half; ++i) {
            float realPart = logSpec[i].real();
            float imagPart = logSpec[i].imag();
            std::complex<float> H = std::exp(std::complex<float>(realPart, imagPart));
            bins[i] = H;
            if (i != 0 && i != half) bins[spectral.fftSize - i] = std::conj(H);
        }
    }

    // IFFT
    FourierTransform ft2;
    ft2.performIFFT(bins);

    // Copy real part to samples
    for (size_t n = 0; n < bins.size(); ++n) out.samples[n] = bins[n].real();

    // DC removal + normalize to target RMS
    removeDcAndNormalize(out.samples, spectral.normalizationRms > 0.0f ? spectral.normalizationRms : 0.2f);

    // Compute and store RMS
    double acc = 0.0; for (float v : out.samples) acc += v*v; out.rms = static_cast<float>(std::sqrt(acc / out.samples.size()));

    return out;
}

std::shared_ptr<SpectralTable> spectralFromWavetable(const Wavetable& wt, int sampleRate) {
    auto out = std::make_shared<SpectralTable>();
    const int frames = wt.getNumFrames();
    const int N = std::max(1024, wt.getFrameSize());
    out->defaultFftSize = N;
    out->frames.reserve(frames);
    for (int f = 0; f < frames; ++f) {
        WaveFrame* wf = const_cast<Wavetable&>(wt).getFrame(f);
        if (!wf) continue;
        // Prepare complex spectrum by forward FFT of time-domain frame
        std::vector<float> td(N, 0.0f);
        const int srcN = wf->getSize();
        const float* src = wf->getData();
        const int copyN = std::min(N, srcN);
        for (int i = 0; i < copyN; ++i) td[i] = src[i];
        // DC removal
        {
            double sum = 0.0; for (int i = 0; i < copyN; ++i) sum += td[i];
            float mean = static_cast<float>(sum / std::max(1, copyN));
            for (int i = 0; i < copyN; ++i) td[i] -= mean;
        }
        // Peak alignment (rotate so max |sample| is at index 0)
        {
            int peakIdx = 0; float peakVal = 0.0f;
            for (int i = 0; i < copyN; ++i) { float a = std::fabs(td[i]); if (a > peakVal) { peakVal = a; peakIdx = i; } }
            if (peakIdx != 0) {
                std::vector<float> rotated(N, 0.0f);
                for (int i = 0; i < N; ++i) rotated[i] = td[(i + peakIdx) % N];
                td.swap(rotated);
            }
        }
        // Pack into complex buffer
        std::vector<std::complex<float>> time(N, {0.0f, 0.0f});
        for (int i = 0; i < N; ++i) time[i] = {td[i], 0.0f};
        FourierTransform ft;
        ft.performFFT(time);
        SpectralFrame sf;
        sf.fftSize = N;
        sf.sampleRate = sampleRate;
        sf.bins.resize(static_cast<size_t>(N/2 + 1));
        // Extract magnitude/phase for positive frequencies
        for (int k = 0; k <= N/2; ++k) {
            const auto& c = time[k];
            sf.bins[k].magnitude = std::abs(c) / static_cast<float>(N); // normalize
            sf.bins[k].phase = std::arg(c);
        }
        sf.normalizationRms = 0.2f;
        out->frames.push_back(std::move(sf));
    }
    return out;
}

} // namespace AIMusicHardware
