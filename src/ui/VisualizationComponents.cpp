#include "../../include/ui/VisualizationComponents.h"
#include <algorithm>
#include <cstring>

namespace AIMusicHardware {

// WaveformVisualizer implementation
WaveformVisualizer::WaveformVisualizer(const std::string& id, int bufferSize)
    : UIComponent(id)
    , bufferSize_(bufferSize) {
    
    audioBuffer_.resize(bufferSize * 2); // Stereo buffer
    fftMagnitudes_.resize(bufferSize / 2);
    fftPhases_.resize(bufferSize / 2);
}

WaveformVisualizer::~WaveformVisualizer() = default;

void WaveformVisualizer::pushSamples(const float* samples, int numSamples, int channels) {
    if (!samples || numSamples <= 0) return;
    
    std::lock_guard<std::mutex> lock(bufferMutex_);
    
    int writeIdx = writePos_.load(std::memory_order_relaxed);
    
    for (int i = 0; i < numSamples; ++i) {
        if (channels == 2) {
            // Stereo: interleave samples
            audioBuffer_[writeIdx * 2] = samples[i * 2];
            audioBuffer_[writeIdx * 2 + 1] = samples[i * 2 + 1];
        } else {
            // Mono: duplicate to both channels
            audioBuffer_[writeIdx * 2] = samples[i];
            audioBuffer_[writeIdx * 2 + 1] = samples[i];
        }
        
        writeIdx = (writeIdx + 1) % bufferSize_;
    }
    
    writePos_.store(writeIdx, std::memory_order_release);
}

void WaveformVisualizer::update(float deltaTime) {
    // Update waterfall history if in waterfall mode
    if (displayMode_ == DisplayMode::Waterfall) {
        performFFT();
        
        // Add current spectrum to history
        waterfallHistory_.push_back(fftMagnitudes_);
        if (waterfallHistory_.size() > WATERFALL_HISTORY_SIZE) {
            waterfallHistory_.pop_front();
        }
    }
}

void WaveformVisualizer::render(DisplayManager* display) {
    if (!display) return;
    
    // Draw background
    display->fillRect(x_, y_, width_, height_, backgroundColor_);
    
    // Draw grid if enabled
    if (showGrid_) {
        drawGrid(display);
    }
    
    // Draw based on display mode
    switch (displayMode_) {
        case DisplayMode::Waveform:
            drawWaveform(display);
            break;
        case DisplayMode::Spectrum:
            performFFT();
            drawSpectrum(display);
            break;
        case DisplayMode::Waterfall:
            drawWaterfall(display);
            break;
        case DisplayMode::Lissajous:
            drawLissajous(display);
            break;
    }
    
    // Draw border
    display->drawRect(x_, y_, width_, height_, Color(60, 60, 60));
}

bool WaveformVisualizer::handleInput(const InputEvent& event) {
    if (!visible_ || !enabled_) return false;
    
    bool inBounds = (event.value >= x_ && event.value < x_ + width_ &&
                    event.value2 >= y_ && event.value2 < y_ + height_);
    
    if (!inBounds) return false;
    
    // Handle zoom with mouse wheel or pinch
    if (event.type == InputEventType::EncoderRotate) {
        float zoomDelta = event.value * 0.1f;
        setZoomLevel(zoomLevel_ + zoomDelta);
        return true;
    }
    
    return false;
}

void WaveformVisualizer::drawWaveform(DisplayManager* display) {
    std::lock_guard<std::mutex> lock(bufferMutex_);
    
    int numPoints = width_ / std::max(1, static_cast<int>(zoomLevel_));
    float xStep = static_cast<float>(width_) / numPoints;
    
    int readIdx = readPos_.load(std::memory_order_acquire);
    int centerY = y_ + height_ / 2;
    
    // Draw left channel (or mono)
    int prevX = x_;
    int prevY = centerY;
    
    for (int i = 0; i < numPoints; ++i) {
        int bufferIdx = (readIdx + i) % bufferSize_;
        float sample = audioBuffer_[bufferIdx * 2]; // Left channel
        
        int x = x_ + static_cast<int>(i * xStep);
        int y = centerY - static_cast<int>(sample * height_ * 0.4f * yScale_);
        
        if (i > 0) {
            // Draw line from previous point
            for (int t = 0; t < lineThickness_; ++t) {
                display->drawLine(prevX, prevY + t, x, y + t, waveformColor_);
            }
        }
        
        prevX = x;
        prevY = y;
    }
}

void WaveformVisualizer::drawSpectrum(DisplayManager* display) {
    int numBins = fftMagnitudes_.size();
    if (numBins == 0) return;
    
    float binWidth = static_cast<float>(width_) / numBins;
    
    for (int i = 0; i < numBins; ++i) {
        float magnitude = fftMagnitudes_[i];
        float dB = 20.0f * std::log10(magnitude + 0.00001f);
        float normalized = (dB + 60.0f) / 60.0f; // Normalize -60dB to 0dB
        normalized = std::clamp(normalized, 0.0f, 1.0f);
        
        int barHeight = static_cast<int>(normalized * height_ * yScale_);
        int barX = x_ + static_cast<int>(i * binWidth);
        int barY = y_ + height_ - barHeight;
        
        // Color based on frequency (low=red, mid=yellow, high=green)
        float hue = static_cast<float>(i) / numBins;
        Color barColor = Color::fromHSV(hue * 120.0f, 0.8f, 0.9f);
        
        display->fillRect(barX, barY, static_cast<int>(binWidth - 1), barHeight, barColor);
    }
}

void WaveformVisualizer::drawWaterfall(DisplayManager* display) {
    if (waterfallHistory_.empty()) return;
    
    int rowHeight = std::max(1, height_ / static_cast<int>(waterfallHistory_.size()));
    
    for (size_t row = 0; row < waterfallHistory_.size(); ++row) {
        const auto& spectrum = waterfallHistory_[row];
        int y = y_ + row * rowHeight;
        
        for (size_t bin = 0; bin < spectrum.size(); ++bin) {
            float magnitude = spectrum[bin];
            float intensity = std::clamp(magnitude * 10.0f, 0.0f, 1.0f);
            
            int x = x_ + (bin * width_) / spectrum.size();
            int w = std::max(1, width_ / static_cast<int>(spectrum.size()));
            
            uint8_t brightness = static_cast<uint8_t>(intensity * 255);
            Color pixelColor(brightness, brightness * 0.5f, brightness * 0.2f);
            
            display->fillRect(x, y, w, rowHeight, pixelColor);
        }
    }
}

void WaveformVisualizer::drawLissajous(DisplayManager* display) {
    std::lock_guard<std::mutex> lock(bufferMutex_);
    
    int numPoints = std::min(512, bufferSize_);
    int readIdx = readPos_.load(std::memory_order_acquire);
    int centerX = x_ + width_ / 2;
    int centerY = y_ + height_ / 2;
    
    for (int i = 1; i < numPoints; ++i) {
        int bufferIdx = (readIdx + i) % bufferSize_;
        int prevIdx = (readIdx + i - 1) % bufferSize_;
        
        float x1 = audioBuffer_[prevIdx * 2];     // Left
        float y1 = audioBuffer_[prevIdx * 2 + 1]; // Right
        float x2 = audioBuffer_[bufferIdx * 2];
        float y2 = audioBuffer_[bufferIdx * 2 + 1];
        
        int px1 = centerX + static_cast<int>(x1 * width_ * 0.4f);
        int py1 = centerY - static_cast<int>(y1 * height_ * 0.4f);
        int px2 = centerX + static_cast<int>(x2 * width_ * 0.4f);
        int py2 = centerY - static_cast<int>(y2 * height_ * 0.4f);
        
        // Fade based on age
        float fade = 1.0f - (static_cast<float>(i) / numPoints);
        Color lineColor = waveformColor_;
        lineColor.a = static_cast<uint8_t>(255 * fade);
        
        display->drawLine(px1, py1, px2, py2, lineColor);
    }
    
    // Draw center crosshair
    display->drawLine(centerX - 10, centerY, centerX + 10, centerY, gridColor_);
    display->drawLine(centerX, centerY - 10, centerX, centerY + 10, gridColor_);
}

void WaveformVisualizer::drawGrid(DisplayManager* display) {
    // Vertical lines
    for (int i = 1; i < 8; ++i) {
        int x = x_ + (width_ * i) / 8;
        display->drawLine(x, y_, x, y_ + height_, gridColor_);
    }
    
    // Horizontal lines
    for (int i = 1; i < 4; ++i) {
        int y = y_ + (height_ * i) / 4;
        display->drawLine(x_, y, x_ + width_, y, gridColor_);
    }
    
    // Center line
    int centerY = y_ + height_ / 2;
    Color centerColor = gridColor_;
    centerColor.r += 20;
    centerColor.g += 20;
    centerColor.b += 20;
    display->drawLine(x_, centerY, x_ + width_, centerY, centerColor);
}

void WaveformVisualizer::performFFT() {
    // Simplified FFT placeholder
    // In a real implementation, use a proper FFT library like FFTW or KissFFT
    
    std::lock_guard<std::mutex> lock(bufferMutex_);
    int readIdx = readPos_.load(std::memory_order_acquire);
    
    // Simple spectrum estimation (not a real FFT)
    for (size_t i = 0; i < fftMagnitudes_.size(); ++i) {
        float sum = 0.0f;
        int samplesPerBin = bufferSize_ / fftMagnitudes_.size();
        
        for (int j = 0; j < samplesPerBin; ++j) {
            int idx = (readIdx + i * samplesPerBin + j) % bufferSize_;
            sum += std::abs(audioBuffer_[idx * 2]); // Left channel
        }
        
        fftMagnitudes_[i] = sum / samplesPerBin;
    }
}

// EnvelopeVisualizer implementation
EnvelopeVisualizer::EnvelopeVisualizer(const std::string& id)
    : UIComponent(id) {
    height_ = 150; // Default height
}

void EnvelopeVisualizer::setADSR(float attack, float decay, float sustain, float release) {
    attack_ = attack;
    decay_ = decay;
    sustain_ = sustain;
    release_ = release;
}

void EnvelopeVisualizer::update(float deltaTime) {
    // Update from bound envelope if available
    // if (envelope_) {
    //     // TODO: Get ADSR values from envelope
    // }
}

void EnvelopeVisualizer::render(DisplayManager* display) {
    if (!display) return;
    
    // Draw background
    display->fillRect(x_, y_, width_, height_, backgroundColor_);
    
    // Draw grid
    if (showGrid_) {
        drawGrid(display);
    }
    
    // Calculate envelope points
    std::vector<Point> points;
    calculateEnvelopePoints(points);
    
    // Draw envelope
    drawEnvelope(display, points);
    
    // Draw phase indicator
    if (currentPhase_ > 0) {
        drawPhaseIndicator(display, points);
    }
    
    // Draw handles if editable
    if (isEditable_) {
        drawHandles(display, points);
    }
    
    // Draw border
    display->drawRect(x_, y_, width_, height_, Color(60, 60, 60));
}

bool EnvelopeVisualizer::handleInput(const InputEvent& event) {
    if (!visible_ || !enabled_ || !isEditable_) return false;
    
    bool inBounds = (event.value >= x_ && event.value < x_ + width_ &&
                    event.value2 >= y_ && event.value2 < y_ + height_);
    
    if (!inBounds && dragHandle_ == -1) return false;
    
    std::vector<Point> points;
    calculateEnvelopePoints(points);
    
    if (event.type == InputEventType::TouchPress) {
        dragHandle_ = getHandleAtPosition(event.value, event.value2, points);
        return dragHandle_ >= 0;
    } else if (event.type == InputEventType::TouchRelease) {
        dragHandle_ = -1;
        return true;
    } else if (event.type == InputEventType::TouchMove && dragHandle_ >= 0) {
        updateParameterFromHandle(dragHandle_, event.value, event.value2);
        return true;
    }
    
    return false;
}

void EnvelopeVisualizer::calculateEnvelopePoints(std::vector<Point>& points) {
    points.clear();
    
    int margin = 20;
    int usableWidth = width_ - 2 * margin;
    int usableHeight = height_ - 2 * margin;
    
    // Use fixed time scale: map 0-4 seconds to full width
    const float maxDisplayTime = 4.0f; // 4 seconds max display
    const float pixelsPerSecond = usableWidth / maxDisplayTime;
    
    // Fixed sustain display time
    const float sustainDisplayTime = 0.3f; // Display sustain as 0.3 seconds
    
    // Calculate X positions based on fixed time scale
    int startX = x_ + margin;
    int attackX = startX + static_cast<int>(attack_ * pixelsPerSecond);
    int decayX = attackX + static_cast<int>(decay_ * pixelsPerSecond);
    int sustainEndX = decayX + static_cast<int>(sustainDisplayTime * pixelsPerSecond);
    int releaseX = sustainEndX + static_cast<int>(release_ * pixelsPerSecond);
    
    // Clamp positions to stay within bounds
    attackX = std::min(attackX, x_ + width_ - margin);
    decayX = std::min(decayX, x_ + width_ - margin);
    sustainEndX = std::min(sustainEndX, x_ + width_ - margin);
    releaseX = std::min(releaseX, x_ + width_ - margin);
    
    // Calculate Y positions
    int bottomY = y_ + height_ - margin;
    int topY = y_ + margin;
    int sustainY = topY + static_cast<int>((1.0f - sustain_) * usableHeight);
    
    // Build envelope points
    // Point 0: Start (bottom left)
    points.push_back(Point(startX, bottomY));
    
    // Point 1: Attack peak (top)
    points.push_back(Point(attackX, topY));
    
    // Point 2: Decay end / Sustain start
    points.push_back(Point(decayX, sustainY));
    
    // Point 3: Sustain end (where release begins)
    points.push_back(Point(sustainEndX, sustainY));
    
    // Point 4: Release end (at bottom after release time)
    points.push_back(Point(releaseX, bottomY));
}

void EnvelopeVisualizer::drawEnvelope(DisplayManager* display, const std::vector<Point>& points) {
    if (points.size() < 2) return;
    
    // Draw envelope lines
    for (size_t i = 1; i < points.size(); ++i) {
        Color color = (currentPhase_ > 0 && i <= currentPhase_) ? activeColor_ : envelopeColor_;
        display->drawLine(points[i-1].x, points[i-1].y, points[i].x, points[i].y, color);
    }
    
    // Fill under curve (semi-transparent)
    // Simplified: just draw vertical lines
    for (size_t i = 1; i < points.size(); ++i) {
        int x1 = points[i-1].x;
        int x2 = points[i].x;
        
        for (int x = x1; x < x2; ++x) {
            float t = static_cast<float>(x - x1) / (x2 - x1);
            int y = points[i-1].y + static_cast<int>(t * (points[i].y - points[i-1].y));
            
            Color fillColor = envelopeColor_;
            fillColor.a = 50;
            display->drawLine(x, y, x, y_ + height_ - 20, fillColor);
        }
    }
}

void EnvelopeVisualizer::drawHandles(DisplayManager* display, const std::vector<Point>& points) {
    const int handleSize = 8;
    
    // Draw only the 3 key draggable handles
    // Skip point 0 (start) and point 3 (sustain end - not independently adjustable)
    std::vector<int> handleIndices = {1, 2, 4}; // Attack peak, Decay/Sustain, Release end
    int handleNum = 0;
    
    for (int i : handleIndices) {
        if (i < points.size()) {
            Color handleColor = (dragHandle_ == handleNum) ? 
                               Color(255, 255, 0) : Color(200, 200, 200);
            
            // Draw handle with filled square and border
            display->fillRect(points[i].x - handleSize/2, points[i].y - handleSize/2,
                             handleSize, handleSize, handleColor);
            display->drawRect(points[i].x - handleSize/2, points[i].y - handleSize/2,
                             handleSize, handleSize, Color(100, 100, 100));
        }
        handleNum++;
    }
}

void EnvelopeVisualizer::drawGrid(DisplayManager* display) {
    // Vertical lines for time divisions
    for (int i = 1; i < 4; ++i) {
        int x = x_ + (width_ * i) / 4;
        display->drawLine(x, y_, x, y_ + height_, gridColor_);
    }
    
    // Horizontal lines for level divisions
    for (int i = 1; i < 4; ++i) {
        int y = y_ + (height_ * i) / 4;
        display->drawLine(x_, y, x_ + width_, y, gridColor_);
    }
}

void EnvelopeVisualizer::drawPhaseIndicator(DisplayManager* display, const std::vector<Point>& points) {
    if (currentPhase_ <= 0 || currentPhase_ > 4) return;
    
    // Calculate position based on phase and progress
    int startIdx = currentPhase_ - 1;
    int endIdx = std::min(currentPhase_, static_cast<int>(points.size() - 1));
    
    if (startIdx >= 0 && endIdx < static_cast<int>(points.size())) {
        int x = points[startIdx].x + static_cast<int>(
            phaseProgress_ * (points[endIdx].x - points[startIdx].x));
        
        // Draw vertical line at current position
        display->drawLine(x, y_ + 10, x, y_ + height_ - 10, activeColor_);
        
        // Draw small circle at envelope height
        float t = phaseProgress_;
        int y = points[startIdx].y + static_cast<int>(
            t * (points[endIdx].y - points[startIdx].y));
        
        display->fillRect(x - 3, y - 3, 6, 6, activeColor_);
    }
}

int EnvelopeVisualizer::getHandleAtPosition(int x, int y, const std::vector<Point>& points) {
    const int handleSize = 12; // Slightly larger hit area
    
    // Check the 3 handles at indices 1, 2, and 4
    std::vector<int> handleIndices = {1, 2, 4}; // Attack peak, Decay/Sustain, Release end
    
    for (int i = 0; i < handleIndices.size(); ++i) {
        int pointIndex = handleIndices[i];
        if (pointIndex < points.size() &&
            std::abs(x - points[pointIndex].x) < handleSize/2 &&
            std::abs(y - points[pointIndex].y) < handleSize/2) {
            return i; // Return handle index (0, 1, or 2)
        }
    }
    
    return -1;
}

void EnvelopeVisualizer::updateParameterFromHandle(int handle, int x, int y) {
    int margin = 20;
    int usableWidth = width_ - 2 * margin;
    int usableHeight = height_ - 2 * margin;
    
    // Fixed time scale parameters
    const float maxDisplayTime = 4.0f;
    const float pixelsPerSecond = usableWidth / maxDisplayTime;
    
    // Get current envelope points for reference
    std::vector<Point> points;
    calculateEnvelopePoints(points);
    
    switch (handle) {
        case 0: { // Attack - horizontal movement only
            // Calculate new attack time based on X position
            float pixels = static_cast<float>(x - (x_ + margin));
            pixels = std::clamp(pixels, 0.0f, static_cast<float>(usableWidth));
            
            // Convert pixel position to time using fixed scale
            attack_ = pixels / pixelsPerSecond;
            attack_ = std::clamp(attack_, 0.001f, 2.0f);
            break;
        }
        
        case 1: { // Decay/Sustain - handle both time and level
            // For sustain level - vertical movement
            float normY = 1.0f - static_cast<float>(y - y_ - margin) / usableHeight;
            sustain_ = std::clamp(normY, 0.0f, 1.0f);
            
            // For decay time - horizontal movement from attack point
            if (points.size() > 2) {
                float attackEndX = static_cast<float>(points[1].x);
                float pixels = static_cast<float>(x) - attackEndX;
                pixels = std::clamp(pixels, 0.0f, static_cast<float>(usableWidth));
                
                // Convert pixel distance to time using fixed scale
                decay_ = pixels / pixelsPerSecond;
                decay_ = std::clamp(decay_, 0.001f, 2.0f);
            }
            break;
        }
        
        case 2: { // Release - horizontal movement only
            // Calculate release time based on pixel distance from sustain end
            if (points.size() > 3) {
                float sustainEndX = static_cast<float>(points[3].x);
                float pixels = static_cast<float>(x) - sustainEndX;
                pixels = std::clamp(pixels, 0.0f, static_cast<float>(usableWidth));
                
                // Convert pixel distance to time using fixed scale
                release_ = pixels / pixelsPerSecond;
                release_ = std::clamp(release_, 0.001f, 4.0f);
            }
            break;
        }
    }
    
    if (parameterChangeCallback_) {
        parameterChangeCallback_(attack_, decay_, sustain_, release_);
    }
}

// LevelMeter implementation
LevelMeter::LevelMeter(const std::string& id, Orientation orientation)
    : UIComponent(id)
    , orientation_(orientation) {
    
    if (orientation == Orientation::Vertical) {
        width_ = 20;
        height_ = 200;
    } else {
        width_ = 200;
        height_ = 20;
    }
}

void LevelMeter::setLevel(float level) {
    currentLevel_ = std::clamp(level, 0.0f, 1.0f);
    
    // Update peak
    if (currentLevel_ > peakLevel_) {
        peakLevel_ = currentLevel_;
        peakHoldTimer_ = 0.0f;
    }
}

void LevelMeter::setMeterColors(const Color& low, const Color& mid, const Color& high) {
    lowColor_ = low;
    midColor_ = mid;
    highColor_ = high;
}

void LevelMeter::update(float deltaTime) {
    // Smooth level changes
    float diff = currentLevel_ - displayLevel_;
    if (diff > 0) {
        // Attack: fast
        displayLevel_ += diff * 0.5f;
    } else {
        // Release: slower
        displayLevel_ += diff * 0.1f;
    }
    
    // Update peak hold timer
    if (peakHoldTimer_ < peakHoldTime_) {
        peakHoldTimer_ += deltaTime;
    } else if (peakLevel_ > 0.0f) {
        // Slowly drop peak
        peakLevel_ *= 0.95f;
        if (peakLevel_ < 0.01f) {
            peakLevel_ = 0.0f;
        }
    }
}

void LevelMeter::render(DisplayManager* display) {
    if (!display) return;
    
    // Draw background
    display->fillRect(x_, y_, width_, height_, Color(20, 20, 20));
    
    if (orientation_ == Orientation::Vertical) {
        drawVerticalMeter(display);
    } else {
        drawHorizontalMeter(display);
    }
    
    // Draw border
    display->drawRect(x_, y_, width_, height_, Color(60, 60, 60));
}

void LevelMeter::drawVerticalMeter(DisplayManager* display) {
    int meterHeight = static_cast<int>(displayLevel_ * height_);
    int meterY = y_ + height_ - meterHeight;
    
    // Draw gradient meter
    for (int y = meterY; y < y_ + height_; ++y) {
        float position = static_cast<float>(y_ + height_ - y) / height_;
        
        Color color;
        if (position < 0.6f) {
            color = lowColor_;
        } else if (position < 0.85f) {
            color = midColor_;
        } else {
            color = highColor_;
        }
        
        display->drawLine(x_ + 2, y, x_ + width_ - 2, y, color);
    }
    
    // Draw peak line
    if (peakLevel_ > 0.0f) {
        int peakY = y_ + height_ - static_cast<int>(peakLevel_ * height_);
        Color peakColor = (peakLevel_ > 0.85f) ? highColor_ : Color(255, 255, 255);
        display->drawLine(x_ + 2, peakY, x_ + width_ - 2, peakY, peakColor);
    }
    
    // Draw scale
    if (showdBScale_) {
        drawdBScale(display);
    }
}

void LevelMeter::drawHorizontalMeter(DisplayManager* display) {
    int meterWidth = static_cast<int>(displayLevel_ * width_);
    
    // Draw gradient meter
    for (int x = x_; x < x_ + meterWidth; ++x) {
        float position = static_cast<float>(x - x_) / width_;
        
        Color color;
        if (position < 0.6f) {
            color = lowColor_;
        } else if (position < 0.85f) {
            color = midColor_;
        } else {
            color = highColor_;
        }
        
        display->drawLine(x, y_ + 2, x, y_ + height_ - 2, color);
    }
    
    // Draw peak line
    if (peakLevel_ > 0.0f) {
        int peakX = x_ + static_cast<int>(peakLevel_ * width_);
        Color peakColor = (peakLevel_ > 0.85f) ? highColor_ : Color(255, 255, 255);
        display->drawLine(peakX, y_ + 2, peakX, y_ + height_ - 2, peakColor);
    }
}

void LevelMeter::drawdBScale(DisplayManager* display) {
    // Draw dB markings
    std::vector<std::pair<float, std::string>> marks = {
        {1.0f, "0"},
        {0.891f, "-3"},
        {0.708f, "-6"},
        {0.501f, "-12"},
        {0.316f, "-20"},
        {0.1f, "-40"}
    };
    
    for (const auto& mark : marks) {
        if (orientation_ == Orientation::Vertical) {
            int y = y_ + height_ - static_cast<int>(mark.first * height_);
            display->drawLine(x_ - 3, y, x_, y, Color(100, 100, 100));
            // Text would be drawn here with proper font support
        } else {
            int x = x_ + static_cast<int>(mark.first * width_);
            display->drawLine(x, y_ + height_, x, y_ + height_ + 3, Color(100, 100, 100));
        }
    }
}

float LevelMeter::todB(float linear) {
    if (linear <= 0.0f) return -96.0f;
    return 20.0f * std::log10(linear);
}

// SpectrumAnalyzer implementation
void SpectrumAnalyzer::render(DisplayManager* display) {
    // Call parent render which handles spectrum drawing
    WaveformVisualizer::render(display);
}

// PhaseMeter implementation
PhaseMeter::PhaseMeter(const std::string& id)
    : UIComponent(id) {
    width_ = 150;
    height_ = 150;
    
    tracePoints_.resize(TRACE_POINTS);
    traceIntensity_.resize(TRACE_POINTS, 0.0f);
}

void PhaseMeter::pushSamples(const float* left, const float* right, int numSamples) {
    if (!left || !right || numSamples <= 0) return;
    
    int writeIdx = writeIndex_.load(std::memory_order_relaxed);
    
    for (int i = 0; i < numSamples; ++i) {
        tracePoints_[writeIdx] = std::make_pair(left[i], right[i]);
        traceIntensity_[writeIdx] = 1.0f;
        writeIdx = (writeIdx + 1) % TRACE_POINTS;
    }
    
    writeIndex_.store(writeIdx, std::memory_order_release);
}

void PhaseMeter::update(float deltaTime) {
    // Decay trace intensity
    for (auto& intensity : traceIntensity_) {
        intensity *= decayRate_;
    }
}

void PhaseMeter::render(DisplayManager* display) {
    if (!display) return;
    
    // Draw background
    display->fillRect(x_, y_, width_, height_, Color(10, 10, 10));
    
    // Draw grid
    int centerX = x_ + width_ / 2;
    int centerY = y_ + height_ / 2;
    
    // Crosshair
    display->drawLine(centerX, y_, centerX, y_ + height_, gridColor_);
    display->drawLine(x_, centerY, x_ + width_, centerY, gridColor_);
    
    // Diagonal lines (L+R and L-R)
    display->drawLine(x_, y_, x_ + width_, y_ + height_, gridColor_);
    display->drawLine(x_ + width_, y_, x_, y_ + height_, gridColor_);
    
    // Draw trace
    for (int i = 0; i < TRACE_POINTS; ++i) {
        if (traceIntensity_[i] > 0.01f) {
            float l = tracePoints_[i].first;
            float r = tracePoints_[i].second;
            
            // Convert to X-Y coordinates
            float x = (l - r) * 0.707f; // L-R (width)
            float y = (l + r) * 0.707f; // L+R (mono)
            
            int px = centerX + static_cast<int>(x * width_ * 0.4f);
            int py = centerY - static_cast<int>(y * height_ * 0.4f);
            
            // Ensure within bounds
            px = std::clamp(px, x_, x_ + width_ - 1);
            py = std::clamp(py, y_, y_ + height_ - 1);
            
            Color pointColor = traceColor_;
            pointColor.a = static_cast<uint8_t>(255 * traceIntensity_[i]);
            
            display->fillRect(px - 1, py - 1, 2, 2, pointColor);
        }
    }
    
    // Draw border
    display->drawRect(x_, y_, width_, height_, Color(60, 60, 60));
}

} // namespace AIMusicHardware