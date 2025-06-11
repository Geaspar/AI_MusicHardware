#pragma once

#include "UIComponents.h"
#include "../synthesis/modulators/envelope.h"
#include <vector>
#include <deque>
#include <cmath>
#include <mutex>
#include <atomic>

namespace AIMusicHardware {

/**
 * @brief Real-time waveform visualization component
 * 
 * Displays audio waveforms with various visualization modes
 */
class WaveformVisualizer : public UIComponent {
public:
    enum class DisplayMode {
        Waveform,      // Traditional oscilloscope view
        Spectrum,      // Frequency spectrum (FFT)
        Waterfall,     // Scrolling spectrogram
        Lissajous      // X-Y phase display
    };
    
    WaveformVisualizer(const std::string& id, int bufferSize = 1024);
    ~WaveformVisualizer();
    
    /**
     * @brief Push audio samples for visualization
     * Thread-safe - can be called from audio thread
     */
    void pushSamples(const float* samples, int numSamples, int channels = 1);
    
    /**
     * @brief Set display mode
     */
    void setDisplayMode(DisplayMode mode) { displayMode_ = mode; }
    DisplayMode getDisplayMode() const { return displayMode_; }
    
    /**
     * @brief Visual settings
     */
    void setWaveformColor(const Color& color) { waveformColor_ = color; }
    void setBackgroundColor(const Color& color) { backgroundColor_ = color; }
    void setGridColor(const Color& color) { gridColor_ = color; }
    void showGrid(bool show) { showGrid_ = show; }
    void setLineThickness(int thickness) { lineThickness_ = thickness; }
    
    /**
     * @brief Zoom and scale controls
     */
    void setZoomLevel(float zoom) { zoomLevel_ = std::max(0.1f, std::min(10.0f, zoom)); }
    void setYScale(float scale) { yScale_ = std::max(0.1f, std::min(2.0f, scale)); }
    
    void update(float deltaTime) override;
    void render(DisplayManager* display) override;
    bool handleInput(const InputEvent& event) override;
    
private:
    // Audio buffer (ring buffer for thread safety)
    std::vector<float> audioBuffer_;
    int bufferSize_;
    std::atomic<int> writePos_{0};
    std::atomic<int> readPos_{0};
    mutable std::mutex bufferMutex_;
    
    // Display settings
    DisplayMode displayMode_ = DisplayMode::Waveform;
    Color waveformColor_{0, 255, 128};
    Color backgroundColor_{20, 20, 20};
    Color gridColor_{40, 40, 40};
    bool showGrid_ = true;
    int lineThickness_ = 2;
    float zoomLevel_ = 1.0f;
    float yScale_ = 1.0f;
    
    // FFT data for spectrum mode
    std::vector<float> fftMagnitudes_;
    std::vector<float> fftPhases_;
    
    // Waterfall data
    std::deque<std::vector<float>> waterfallHistory_;
    static constexpr int WATERFALL_HISTORY_SIZE = 100;
    
    // Helper methods
    void drawWaveform(DisplayManager* display);
    void drawSpectrum(DisplayManager* display);
    void drawWaterfall(DisplayManager* display);
    void drawLissajous(DisplayManager* display);
    void drawGrid(DisplayManager* display);
    void performFFT();
};

/**
 * @brief ADSR envelope visualizer
 * 
 * Displays envelope shape and current playback position
 */
class EnvelopeVisualizer : public UIComponent {
public:
    EnvelopeVisualizer(const std::string& id);
    
    /**
     * @brief Bind to an envelope
     */
    // void bindToEnvelope(vital::Envelope* envelope) { envelope_ = envelope; }
    
    /**
     * @brief Set envelope parameters for visualization
     */
    void setADSR(float attack, float decay, float sustain, float release);
    
    /**
     * @brief Update current envelope phase (0=off, 1=attack, 2=decay, 3=sustain, 4=release)
     */
    void setCurrentPhase(int phase) { currentPhase_ = phase; }
    
    /**
     * @brief Set phase progress (0-1)
     */
    void setPhaseProgress(float progress) { phaseProgress_ = progress; }
    
    /**
     * @brief Visual settings
     */
    void setEnvelopeColor(const Color& color) { envelopeColor_ = color; }
    void setActiveColor(const Color& color) { activeColor_ = color; }
    void setBackgroundColor(const Color& color) { backgroundColor_ = color; }
    void setGridColor(const Color& color) { gridColor_ = color; }
    void showGrid(bool show) { showGrid_ = show; }
    
    /**
     * @brief Enable interactive editing
     */
    void setEditable(bool editable) { isEditable_ = editable; }
    
    void update(float deltaTime) override;
    void render(DisplayManager* display) override;
    bool handleInput(const InputEvent& event) override;
    
    /**
     * @brief Callback when envelope parameters change
     */
    using ParameterChangeCallback = std::function<void(float attack, float decay, float sustain, float release)>;
    void setParameterChangeCallback(ParameterChangeCallback callback) { parameterChangeCallback_ = callback; }
    
private:
    // vital::Envelope* envelope_ = nullptr;
    
    // ADSR parameters
    float attack_ = 0.01f;
    float decay_ = 0.1f;
    float sustain_ = 0.7f;
    float release_ = 0.5f;
    
    // Visualization state
    int currentPhase_ = 0;
    float phaseProgress_ = 0.0f;
    
    // Visual settings
    Color envelopeColor_{100, 150, 255};
    Color activeColor_{150, 200, 255};
    Color backgroundColor_{20, 20, 20};
    Color gridColor_{40, 40, 40};
    bool showGrid_ = true;
    
    // Interaction
    bool isEditable_ = false;
    int dragHandle_ = -1; // -1=none, 0=attack, 1=decay, 2=sustain, 3=release
    
    ParameterChangeCallback parameterChangeCallback_;
    
    // Helper methods
    void calculateEnvelopePoints(std::vector<Point>& points);
    void drawEnvelope(DisplayManager* display, const std::vector<Point>& points);
    void drawHandles(DisplayManager* display, const std::vector<Point>& points);
    void drawGrid(DisplayManager* display);
    void drawPhaseIndicator(DisplayManager* display, const std::vector<Point>& points);
    int getHandleAtPosition(int x, int y, const std::vector<Point>& points);
    void updateParameterFromHandle(int handle, int x, int y);
    
    using Point = ::AIMusicHardware::Point;
};

/**
 * @brief Simple oscilloscope for monitoring audio
 */
class Oscilloscope : public WaveformVisualizer {
public:
    Oscilloscope(const std::string& id, int bufferSize = 512)
        : WaveformVisualizer(id, bufferSize) {
        setDisplayMode(DisplayMode::Waveform);
        setLineThickness(2);
    }
    
    /**
     * @brief Set trigger level for stable display
     */
    void setTriggerLevel(float level) { triggerLevel_ = level; }
    
    /**
     * @brief Enable auto-triggering
     */
    void setAutoTrigger(bool enable) { autoTrigger_ = enable; }
    
private:
    float triggerLevel_ = 0.0f;
    bool autoTrigger_ = true;
};

/**
 * @brief Spectrum analyzer with customizable bands
 */
class SpectrumAnalyzer : public WaveformVisualizer {
public:
    SpectrumAnalyzer(const std::string& id, int numBands = 32)
        : WaveformVisualizer(id, 1024), numBands_(numBands) {
        setDisplayMode(DisplayMode::Spectrum);
    }
    
    /**
     * @brief Set number of frequency bands
     */
    void setNumBands(int bands) { numBands_ = std::max(8, std::min(128, bands)); }
    
    /**
     * @brief Set bar style (true=bars, false=line)
     */
    void setBarStyle(bool bars) { useBarStyle_ = bars; }
    
    /**
     * @brief Set peak hold time in seconds
     */
    void setPeakHoldTime(float seconds) { peakHoldTime_ = seconds; }
    
    void render(DisplayManager* display) override;
    
private:
    int numBands_;
    bool useBarStyle_ = true;
    float peakHoldTime_ = 2.0f;
    
    std::vector<float> bandPeaks_;
    std::vector<float> peakTimers_;
};

/**
 * @brief Phase correlation meter (goniometer)
 */
class PhaseMeter : public UIComponent {
public:
    PhaseMeter(const std::string& id);
    
    /**
     * @brief Push stereo samples
     */
    void pushSamples(const float* left, const float* right, int numSamples);
    
    /**
     * @brief Visual settings
     */
    void setTraceColor(const Color& color) { traceColor_ = color; }
    void setGridColor(const Color& color) { gridColor_ = color; }
    void setDecayRate(float rate) { decayRate_ = rate; }
    
    void update(float deltaTime) override;
    void render(DisplayManager* display) override;
    bool handleInput(const InputEvent& event) override { return false; }
    
private:
    static constexpr int TRACE_POINTS = 256;
    std::vector<std::pair<float, float>> tracePoints_;
    std::atomic<int> writeIndex_{0};
    
    Color traceColor_{0, 255, 128};
    Color gridColor_{40, 40, 40};
    float decayRate_ = 0.95f;
    
    std::vector<float> traceIntensity_;
};

/**
 * @brief Filter frequency response visualizer (Vital-style)
 * 
 * Displays the frequency response curve of a filter with
 * interactive cutoff and resonance control
 */
class FilterVisualizer : public UIComponent {
public:
    enum class FilterType {
        LowPass,
        HighPass,
        BandPass,
        Notch
    };
    
    FilterVisualizer(const std::string& id);
    
    /**
     * @brief Set filter parameters
     */
    void setFilterType(FilterType type) { filterType_ = type; }
    void setCutoffFrequency(float freq) { cutoffFreq_ = freq; }
    void setResonance(float res) { resonance_ = res; }
    void setSampleRate(float rate) { sampleRate_ = rate; }
    
    /**
     * @brief Visual settings
     */
    void setCurveColor(const Color& color) { curveColor_ = color; }
    void setFillColor(const Color& color) { fillColor_ = color; }
    void setGridColor(const Color& color) { gridColor_ = color; }
    void setBackgroundColor(const Color& color) { backgroundColor_ = color; }
    void showGrid(bool show) { showGrid_ = show; }
    void showFill(bool show) { showFill_ = show; }
    void setLineThickness(int thickness) { lineThickness_ = thickness; }
    
    /**
     * @brief Enable interactive editing
     */
    void setEditable(bool editable) { isEditable_ = editable; }
    
    void update(float deltaTime) override;
    void render(DisplayManager* display) override;
    bool handleInput(const InputEvent& event) override;
    
    /**
     * @brief Callback when filter parameters change
     */
    using ParameterChangeCallback = std::function<void(float cutoff, float resonance)>;
    void setParameterChangeCallback(ParameterChangeCallback callback) { parameterChangeCallback_ = callback; }
    
private:
    // Filter parameters
    FilterType filterType_ = FilterType::LowPass;
    float cutoffFreq_ = 1000.0f;
    float resonance_ = 0.7f;
    float sampleRate_ = 44100.0f;
    
    // Visual settings
    Color curveColor_{100, 200, 255};
    Color fillColor_{100, 200, 255, 50};
    Color gridColor_{40, 40, 40};
    Color backgroundColor_{20, 20, 20};
    bool showGrid_ = true;
    bool showFill_ = true;
    int lineThickness_ = 2;
    
    // Interaction
    bool isEditable_ = false;
    bool isDragging_ = false;
    Point dragStart_;
    float dragStartCutoff_;
    float dragStartResonance_;
    
    ParameterChangeCallback parameterChangeCallback_;
    
    // Frequency response calculation
    static constexpr int NUM_POINTS = 128;
    std::vector<float> frequencyResponse_;
    
    // Helper methods
    void calculateFrequencyResponse();
    float calculateMagnitudeResponse(float frequency);
    void drawGrid(DisplayManager* display);
    void drawFrequencyResponse(DisplayManager* display);
    void drawCutoffMarker(DisplayManager* display);
    float frequencyToX(float freq) const;
    float xToFrequency(int x) const;
    float magnitudeToY(float mag) const;
    float yToMagnitude(int y) const;
};

/**
 * @brief Level meter with peak hold
 */
class LevelMeter : public UIComponent {
public:
    enum class Orientation {
        Vertical,
        Horizontal
    };
    
    LevelMeter(const std::string& id, Orientation orientation = Orientation::Vertical);
    
    /**
     * @brief Set current level (0-1, where 1 = 0dB)
     */
    void setLevel(float level);
    
    /**
     * @brief Set peak level
     */
    void setPeak(float peak) { peakLevel_ = peak; }
    
    /**
     * @brief Reset peak hold
     */
    void resetPeak() { peakLevel_ = 0.0f; peakHoldTimer_ = 0.0f; }
    
    /**
     * @brief Visual settings
     */
    void setMeterColors(const Color& low, const Color& mid, const Color& high);
    void setPeakHoldTime(float seconds) { peakHoldTime_ = seconds; }
    void showdBScale(bool show) { showdBScale_ = show; }
    
    void update(float deltaTime) override;
    void render(DisplayManager* display) override;
    bool handleInput(const InputEvent& event) override { return false; }
    
private:
    Orientation orientation_;
    float currentLevel_ = 0.0f;
    float displayLevel_ = 0.0f; // Smoothed display value
    float peakLevel_ = 0.0f;
    float peakHoldTimer_ = 0.0f;
    float peakHoldTime_ = 2.0f;
    
    Color lowColor_{0, 200, 0};     // Green
    Color midColor_{200, 200, 0};   // Yellow
    Color highColor_{200, 0, 0};    // Red
    
    bool showdBScale_ = true;
    
    float smoothingFactor_ = 0.1f;
    
    void drawVerticalMeter(DisplayManager* display);
    void drawHorizontalMeter(DisplayManager* display);
    void drawdBScale(DisplayManager* display);
    float todB(float linear);
};

} // namespace AIMusicHardware