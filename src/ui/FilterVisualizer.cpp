#include "../../include/ui/VisualizationComponents.h"
#include <cmath>
#include <algorithm>

namespace AIMusicHardware {

FilterVisualizer::FilterVisualizer(const std::string& id)
    : UIComponent(id) {
    frequencyResponse_.resize(NUM_POINTS);
    calculateFrequencyResponse();
}

void FilterVisualizer::update(float deltaTime) {
    // Recalculate frequency response if parameters changed
    static float lastCutoff = 0.0f;
    static float lastResonance = 0.0f;
    static FilterType lastType = FilterType::LowPass;
    
    if (cutoffFreq_ != lastCutoff || resonance_ != lastResonance || filterType_ != lastType) {
        calculateFrequencyResponse();
        lastCutoff = cutoffFreq_;
        lastResonance = resonance_;
        lastType = filterType_;
    }
}

void FilterVisualizer::render(DisplayManager* display) {
    if (!display || !visible_) return;
    
    // Draw background
    display->fillRect(x_, y_, width_, height_, backgroundColor_);
    
    // Draw grid
    if (showGrid_) {
        drawGrid(display);
    }
    
    // Draw frequency response
    drawFrequencyResponse(display);
    
    // Draw cutoff marker
    if (isEditable_) {
        drawCutoffMarker(display);
    }
    
    // Draw border
    display->drawRect(x_, y_, width_, height_, Color(60, 60, 60));
}

bool FilterVisualizer::handleInput(const InputEvent& event) {
    if (!visible_ || !isEditable_) return false;
    
    switch (event.type) {
        case InputEventType::TouchPress:
            if (event.value >= x_ && event.value < x_ + width_ &&
                event.value2 >= y_ && event.value2 < y_ + height_) {
                isDragging_ = true;
                dragStart_ = Point(event.value, event.value2);
                dragStartCutoff_ = cutoffFreq_;
                dragStartResonance_ = resonance_;
                return true;
            }
            break;
            
        case InputEventType::TouchMove:
            if (isDragging_) {
                // Only process if we started the drag (don't steal events from other components)
                // This prevents the filter visualizer from processing mouse movements
                // when dragging other UI elements like sliders
                
                // Horizontal movement controls cutoff
                float deltaX = event.value - dragStart_.x;
                float freqRange = std::log10(20000.0f / 20.0f);
                float normalizedDelta = deltaX / width_;
                float newCutoff = dragStartCutoff_ * std::pow(10.0f, normalizedDelta * freqRange);
                cutoffFreq_ = std::max(20.0f, std::min(20000.0f, newCutoff));
                
                // Vertical movement controls resonance
                float deltaY = dragStart_.y - event.value2; // Inverted Y
                float resonanceDelta = deltaY / height_ * 20.0f; // Max 20 resonance
                resonance_ = std::max(0.7f, std::min(20.0f, dragStartResonance_ + resonanceDelta));
                
                // Notify callback
                if (parameterChangeCallback_) {
                    parameterChangeCallback_(cutoffFreq_, resonance_);
                }
                
                return true;
            }
            break;
            
        case InputEventType::TouchRelease:
            if (isDragging_) {
                isDragging_ = false;
                return true;
            }
            break;
            
        default:
            break;
    }
    
    return false;
}

void FilterVisualizer::calculateFrequencyResponse() {
    // Calculate frequency points on logarithmic scale
    float minFreq = 20.0f;
    float maxFreq = 20000.0f;
    float logMin = std::log10(minFreq);
    float logMax = std::log10(maxFreq);
    
    for (int i = 0; i < NUM_POINTS; ++i) {
        float t = static_cast<float>(i) / (NUM_POINTS - 1);
        float logFreq = logMin + t * (logMax - logMin);
        float freq = std::pow(10.0f, logFreq);
        
        frequencyResponse_[i] = calculateMagnitudeResponse(freq);
    }
}

float FilterVisualizer::calculateMagnitudeResponse(float frequency) {
    // Simplified filter response calculation
    // For accurate response, we'd need the actual filter coefficients
    
    float w = 2.0f * M_PI * frequency / sampleRate_;
    float wc = 2.0f * M_PI * cutoffFreq_ / sampleRate_;
    
    float magnitude = 1.0f;
    
    switch (filterType_) {
        case FilterType::LowPass: {
            // 2nd order Butterworth approximation
            float normalized = frequency / cutoffFreq_;
            float denominator = std::sqrt(1.0f + std::pow(normalized, 4.0f));
            magnitude = 1.0f / denominator;
            
            // Add resonance peak
            if (std::abs(frequency - cutoffFreq_) < cutoffFreq_ * 0.3f) {
                float distance = std::abs(frequency - cutoffFreq_) / (cutoffFreq_ * 0.3f);
                float peakCurve = std::exp(-distance * distance * 4.0f); // Gaussian-like peak
                
                // Calculate resonance boost (Q factor effect)
                // resonance_ ranges from 0.7 to 20, we want boost from 1x to 10x
                float boost = 1.0f + (resonance_ - 0.7f) * 0.5f;
                magnitude *= (1.0f + (boost - 1.0f) * peakCurve);
            }
            break;
        }
            
        case FilterType::HighPass: {
            // 2nd order Butterworth approximation
            float normalized = cutoffFreq_ / frequency;
            float denominator = std::sqrt(1.0f + std::pow(normalized, 4.0f));
            magnitude = 1.0f / denominator;
            
            // Add resonance peak
            if (std::abs(frequency - cutoffFreq_) < cutoffFreq_ * 0.3f) {
                float distance = std::abs(frequency - cutoffFreq_) / (cutoffFreq_ * 0.3f);
                float peakCurve = std::exp(-distance * distance * 4.0f); // Gaussian-like peak
                
                // Calculate resonance boost (Q factor effect)
                float boost = 1.0f + (resonance_ - 0.7f) * 0.5f;
                magnitude *= (1.0f + (boost - 1.0f) * peakCurve);
            }
            break;
        }
            
        case FilterType::BandPass: {
            // Simple bandpass approximation
            float bw = cutoffFreq_ / (resonance_ * 0.5f + 0.5f);
            float lower = cutoffFreq_ - bw / 2.0f;
            float upper = cutoffFreq_ + bw / 2.0f;
            
            if (frequency >= lower && frequency <= upper) {
                float t = (frequency - lower) / bw;
                magnitude = std::sin(t * M_PI);
                magnitude *= resonance_ / 0.7f; // Scale by resonance
            } else {
                magnitude = 0.1f; // Small but non-zero for visibility
            }
            break;
        }
            
        case FilterType::Notch: {
            // Notch filter (inverse of bandpass)
            float bw = cutoffFreq_ / (resonance_ * 0.5f + 0.5f);
            float lower = cutoffFreq_ - bw / 2.0f;
            float upper = cutoffFreq_ + bw / 2.0f;
            
            if (frequency >= lower && frequency <= upper) {
                float t = (frequency - lower) / bw;
                magnitude = 1.0f - std::sin(t * M_PI) * 0.9f;
            } else {
                magnitude = 1.0f;
            }
            break;
        }
    }
    
    return magnitude;
}

void FilterVisualizer::drawGrid(DisplayManager* display) {
    // Draw frequency grid lines (logarithmic)
    const float frequencies[] = {20, 50, 100, 200, 500, 1000, 2000, 5000, 10000, 20000};
    const char* labels[] = {"20", "50", "100", "200", "500", "1k", "2k", "5k", "10k", "20k"};
    
    for (int i = 0; i < 10; ++i) {
        int x = x_ + frequencyToX(frequencies[i]);
        
        // Vertical grid line
        display->drawLine(x, y_, x, y_ + height_, gridColor_);
        
        // Frequency label
        if (i % 2 == 0 || i == 5) { // Show fewer labels to avoid clutter
            display->drawText(x - 10, y_ + height_ + 15, labels[i], nullptr, Color(100, 100, 100));
        }
    }
    
    // Draw magnitude grid lines (dB scale)
    const float dBLevels[] = {24, 12, 0, -12, -24, -48};
    const float magnitudes[] = {15.85f, 3.98f, 1.0f, 0.25f, 0.063f, 0.004f};
    
    for (int i = 0; i < 6; ++i) {
        int y = y_ + magnitudeToY(magnitudes[i]);
        
        // Horizontal grid line
        display->drawLine(x_, y, x_ + width_, y, gridColor_);
        
        // dB label (only show key levels to avoid clutter)
        if (i == 2 || i == 3 || i == 5) { // 0dB, -12dB, -48dB
            char label[10];
            snprintf(label, sizeof(label), "%+d dB", static_cast<int>(dBLevels[i]));
            display->drawText(x_ - 40, y - 5, label, nullptr, Color(100, 100, 100));
        }
    }
}

void FilterVisualizer::drawFrequencyResponse(DisplayManager* display) {
    if (frequencyResponse_.empty()) return;
    
    // Create points for the curve
    std::vector<Point> curvePoints;
    curvePoints.reserve(NUM_POINTS);
    
    float minFreq = 20.0f;
    float maxFreq = 20000.0f;
    float logMin = std::log10(minFreq);
    float logMax = std::log10(maxFreq);
    
    for (int i = 0; i < NUM_POINTS; ++i) {
        float t = static_cast<float>(i) / (NUM_POINTS - 1);
        float logFreq = logMin + t * (logMax - logMin);
        float freq = std::pow(10.0f, logFreq);
        
        int x = x_ + frequencyToX(freq);
        int y = y_ + magnitudeToY(frequencyResponse_[i]);
        
        curvePoints.push_back(Point(x, y));
    }
    
    // Draw filled area under curve if enabled
    if (showFill_ && curvePoints.size() > 1) {
        for (size_t i = 0; i < curvePoints.size() - 1; ++i) {
            // Draw vertical lines to create fill effect
            int x1 = curvePoints[i].x;
            int y1 = curvePoints[i].y;
            int x2 = curvePoints[i + 1].x;
            int y2 = curvePoints[i + 1].y;
            
            // Fill with gradient effect (darker at bottom)
            for (int x = x1; x <= x2; ++x) {
                float t = (x2 > x1) ? static_cast<float>(x - x1) / (x2 - x1) : 0.0f;
                int y = y1 + static_cast<int>(t * (y2 - y1));
                
                for (int fillY = y; fillY < y_ + height_; ++fillY) {
                    float alpha = 1.0f - static_cast<float>(fillY - y) / (y_ + height_ - y);
                    alpha *= 0.3f; // Overall transparency
                    
                    Color fillColor(
                        fillColor_.r * alpha,
                        fillColor_.g * alpha,
                        fillColor_.b * alpha,
                        fillColor_.a
                    );
                    display->drawLine(x, fillY, x, fillY, fillColor);
                }
            }
        }
    }
    
    // Draw the curve
    for (size_t i = 0; i < curvePoints.size() - 1; ++i) {
        for (int t = 0; t < lineThickness_; ++t) {
            display->drawLine(
                curvePoints[i].x, curvePoints[i].y + t,
                curvePoints[i + 1].x, curvePoints[i + 1].y + t,
                curveColor_
            );
        }
    }
}

void FilterVisualizer::drawCutoffMarker(DisplayManager* display) {
    // Draw vertical line at cutoff frequency
    int cutoffX = x_ + frequencyToX(cutoffFreq_);
    
    // Draw marker line
    Color markerColor(255, 255, 255, 100);
    display->drawLine(cutoffX, y_, cutoffX, y_ + height_, markerColor);
    
    // Draw handle circle at cutoff/resonance position
    float cutoffMagnitude = calculateMagnitudeResponse(cutoffFreq_);
    int handleY = y_ + magnitudeToY(cutoffMagnitude);
    
    // Outer circle
    for (int dy = -8; dy <= 8; ++dy) {
        for (int dx = -8; dx <= 8; ++dx) {
            if (dx * dx + dy * dy <= 64) { // radius = 8
                display->drawLine(
                    cutoffX + dx, handleY + dy,
                    cutoffX + dx, handleY + dy,
                    Color(255, 255, 255, 50)
                );
            }
        }
    }
    
    // Inner circle
    for (int dy = -4; dy <= 4; ++dy) {
        for (int dx = -4; dx <= 4; ++dx) {
            if (dx * dx + dy * dy <= 16) { // radius = 4
                display->drawLine(
                    cutoffX + dx, handleY + dy,
                    cutoffX + dx, handleY + dy,
                    curveColor_
                );
            }
        }
    }
    
    // Show frequency and resonance values when dragging
    if (isDragging_) {
        char freqText[32];
        if (cutoffFreq_ >= 1000.0f) {
            snprintf(freqText, sizeof(freqText), "%.1f kHz", cutoffFreq_ / 1000.0f);
        } else {
            snprintf(freqText, sizeof(freqText), "%.0f Hz", cutoffFreq_);
        }
        
        char resText[32];
        snprintf(resText, sizeof(resText), "Q: %.1f", resonance_);
        
        display->drawText(cutoffX + 10, handleY - 20, freqText, nullptr, Color(255, 255, 255));
        display->drawText(cutoffX + 10, handleY - 5, resText, nullptr, Color(255, 255, 255));
    }
}

float FilterVisualizer::frequencyToX(float freq) const {
    // Logarithmic scale
    float minFreq = 20.0f;
    float maxFreq = 20000.0f;
    float logMin = std::log10(minFreq);
    float logMax = std::log10(maxFreq);
    float logFreq = std::log10(freq);
    
    float normalized = (logFreq - logMin) / (logMax - logMin);
    return normalized * width_;
}

float FilterVisualizer::xToFrequency(int x) const {
    float normalized = static_cast<float>(x - x_) / width_;
    normalized = std::max(0.0f, std::min(1.0f, normalized));
    
    float minFreq = 20.0f;
    float maxFreq = 20000.0f;
    float logMin = std::log10(minFreq);
    float logMax = std::log10(maxFreq);
    
    float logFreq = logMin + normalized * (logMax - logMin);
    return std::pow(10.0f, logFreq);
}

float FilterVisualizer::magnitudeToY(float mag) const {
    // Convert magnitude to Y coordinate (inverted, 0 at top)
    // Use logarithmic scale for better visualization
    float dB = 20.0f * std::log10(std::max(0.001f, mag));
    
    // Map -48dB to +24dB range to use full height
    // This allows resonance peaks up to +24dB
    float normalized = (dB + 48.0f) / 72.0f; // Map -48dB to +24dB -> 0 to 1
    normalized = std::max(0.0f, std::min(1.0f, normalized));
    
    // Use only bottom 2/3 of height for 0dB baseline, leaving top 1/3 for resonance peaks
    return height_ - (normalized * height_);
}

float FilterVisualizer::yToMagnitude(int y) const {
    float normalized = static_cast<float>(y_ + height_ - y) / height_;
    normalized = std::max(0.0f, std::min(1.0f, normalized));
    
    // Convert from 0-1 range back to -48dB to +24dB
    float dB = normalized * 72.0f - 48.0f;
    return std::pow(10.0f, dB / 20.0f);
}

} // namespace AIMusicHardware