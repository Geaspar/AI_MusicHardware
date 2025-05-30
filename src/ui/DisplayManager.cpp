#include "../../include/ui/DisplayManager.h"
#include "../../include/ui/Font.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <stdexcept>

namespace AIMusicHardware {

// Color helper implementation
Color Color::fromHSV(float h, float s, float v) {
    float c = v * s;
    float x = c * (1 - std::abs(std::fmod(h / 60.0f, 2) - 1));
    float m = v - c;
    
    float r, g, b;
    
    if (h < 60) {
        r = c; g = x; b = 0;
    } else if (h < 120) {
        r = x; g = c; b = 0;
    } else if (h < 180) {
        r = 0; g = c; b = x;
    } else if (h < 240) {
        r = 0; g = x; b = c;
    } else if (h < 300) {
        r = x; g = 0; b = c;
    } else {
        r = c; g = 0; b = x;
    }
    
    return Color(
        static_cast<uint8_t>((r + m) * 255),
        static_cast<uint8_t>((g + m) * 255),
        static_cast<uint8_t>((b + m) * 255)
    );
}

DisplayManager::DisplayManager()
    : width_(0), height_(0), bytesPerPixel_(4), pitch_(0),
      blendMode_(BlendMode::Alpha), hasClipRect_(false) {
}

DisplayManager::~DisplayManager() {
    shutdown();
}

bool DisplayManager::initialize(int width, int height) {
    if (width <= 0 || height <= 0) {
        return false;
    }
    
    width_ = width;
    height_ = height;
    pitch_ = width_ * bytesPerPixel_;
    
    // Allocate front and back buffers
    size_t bufferSize = static_cast<size_t>(pitch_ * height_);
    frontBuffer_.resize(bufferSize);
    backBuffer_.resize(bufferSize);
    
    // Clear buffers
    std::fill(frontBuffer_.begin(), frontBuffer_.end(), 0);
    std::fill(backBuffer_.begin(), backBuffer_.end(), 0);
    
    // Set default clip rectangle to entire screen
    clipRect_ = Rect(0, 0, width_, height_);
    hasClipRect_ = false;
    
    return true;
}

void DisplayManager::shutdown() {
    // Free buffer memory
    frontBuffer_.clear();
    backBuffer_.clear();
    width_ = 0;
    height_ = 0;
}

void DisplayManager::clear(const Color& color) {
    // Fill the back buffer with the specified color
    for (int y = 0; y < height_; y++) {
        for (int x = 0; x < width_; x++) {
            setPixel(x, y, color);
        }
    }
}

void DisplayManager::swapBuffers() {
    // Swap front and back buffers
    frontBuffer_.swap(backBuffer_);
}

void DisplayManager::setPixel(int x, int y, const Color& color) {
    // Check bounds
    if (x < 0 || x >= width_ || y < 0 || y >= height_) {
        return;
    }
    
    // Apply clipping
    if (hasClipRect_ && !isInClipRect(x, y)) {
        return;
    }
    
    // Blend based on current blend mode
    blendPixel(x, y, color);
}

Color DisplayManager::getPixel(int x, int y) const {
    // Check bounds
    if (x < 0 || x >= width_ || y < 0 || y >= height_) {
        return Color(0, 0, 0, 0);
    }
    
    // Calculate pixel offset
    size_t offset = static_cast<size_t>(y * pitch_ + x * bytesPerPixel_);
    
    // Extract color components from the back buffer
    Color result;
    result.r = backBuffer_[offset];
    result.g = backBuffer_[offset + 1];
    result.b = backBuffer_[offset + 2];
    result.a = backBuffer_[offset + 3];
    
    return result;
}

void DisplayManager::drawLine(int x1, int y1, int x2, int y2, const Color& color) {
    // Bresenham's line algorithm
    int dx = std::abs(x2 - x1);
    int dy = std::abs(y2 - y1);
    int sx = (x1 < x2) ? 1 : -1;
    int sy = (y1 < y2) ? 1 : -1;
    int err = dx - dy;
    
    while (true) {
        setPixel(x1, y1, color);
        
        if (x1 == x2 && y1 == y2) {
            break;
        }
        
        int e2 = 2 * err;
        if (e2 > -dy) {
            err -= dy;
            x1 += sx;
        }
        if (e2 < dx) {
            err += dx;
            y1 += sy;
        }
    }
}

void DisplayManager::drawRect(int x, int y, int width, int height, const Color& color) {
    // Draw horizontal lines
    drawLine(x, y, x + width - 1, y, color);
    drawLine(x, y + height - 1, x + width - 1, y + height - 1, color);
    
    // Draw vertical lines
    drawLine(x, y, x, y + height - 1, color);
    drawLine(x + width - 1, y, x + width - 1, y + height - 1, color);
}

void DisplayManager::fillRect(int x, int y, int width, int height, const Color& color) {
    // Apply clipping to rectangle
    if (hasClipRect_) {
        Rect rect(x, y, width, height);
        if (!rect.intersects(clipRect_)) {
            return;
        }
        
        // Clip the rectangle to the clip rect
        int x1 = std::max(x, clipRect_.x);
        int y1 = std::max(y, clipRect_.y);
        int x2 = std::min(x + width, clipRect_.x + clipRect_.width);
        int y2 = std::min(y + height, clipRect_.y + clipRect_.height);
        
        // Update rectangle dimensions
        x = x1;
        y = y1;
        width = x2 - x1;
        height = y2 - y1;
    }
    
    // Fill the rectangle
    for (int j = 0; j < height; j++) {
        for (int i = 0; i < width; i++) {
            setPixel(x + i, y + j, color);
        }
    }
}

void DisplayManager::drawCircle(int x, int y, int radius, const Color& color) {
    // Midpoint circle algorithm
    int f = 1 - radius;
    int ddF_x = 0;
    int ddF_y = -2 * radius;
    int px = 0;
    int py = radius;
    
    // Draw four initial points
    setPixel(x, y + radius, color);
    setPixel(x, y - radius, color);
    setPixel(x + radius, y, color);
    setPixel(x - radius, y, color);
    
    while (px < py) {
        if (f >= 0) {
            py--;
            ddF_y += 2;
            f += ddF_y;
        }
        px++;
        ddF_x += 2;
        f += ddF_x + 1;
        
        setPixel(x + px, y + py, color);
        setPixel(x - px, y + py, color);
        setPixel(x + px, y - py, color);
        setPixel(x - px, y - py, color);
        setPixel(x + py, y + px, color);
        setPixel(x - py, y + px, color);
        setPixel(x + py, y - px, color);
        setPixel(x - py, y - px, color);
    }
}

void DisplayManager::fillCircle(int x, int y, int radius, const Color& color) {
    // Filled circle algorithm based on midpoint
    int f = 1 - radius;
    int ddF_x = 0;
    int ddF_y = -2 * radius;
    int px = 0;
    int py = radius;
    
    // Draw horizontal line through the center
    for (int i = -radius; i <= radius; i++) {
        setPixel(x + i, y, color);
    }
    
    while (px < py) {
        if (f >= 0) {
            py--;
            ddF_y += 2;
            f += ddF_y;
        }
        px++;
        ddF_x += 2;
        f += ddF_x + 1;
        
        // Draw horizontal lines between points
        for (int i = -px; i <= px; i++) {
            setPixel(x + i, y + py, color);
            setPixel(x + i, y - py, color);
        }
        
        for (int i = -py; i <= py; i++) {
            setPixel(x + i, y + px, color);
            setPixel(x + i, y - px, color);
        }
    }
}

void DisplayManager::drawTriangle(int x1, int y1, int x2, int y2, int x3, int y3, const Color& color) {
    // Draw three lines to form a triangle
    drawLine(x1, y1, x2, y2, color);
    drawLine(x2, y2, x3, y3, color);
    drawLine(x3, y3, x1, y1, color);
}

void DisplayManager::fillTriangle(int x1, int y1, int x2, int y2, int x3, int y3, const Color& color) {
    // Simple implementation using scanline algorithm
    
    // Sort vertices by y-coordinate (y1 <= y2 <= y3)
    if (y1 > y2) {
        std::swap(x1, x2);
        std::swap(y1, y2);
    }
    if (y2 > y3) {
        std::swap(x2, x3);
        std::swap(y2, y3);
    }
    if (y1 > y2) {
        std::swap(x1, x2);
        std::swap(y1, y2);
    }
    
    // Flat top or flat bottom triangle
    if (y2 == y3) {
        // Flat bottom triangle
        fillFlatBottomTriangle(x1, y1, x2, y2, x3, y3, color);
    } else if (y1 == y2) {
        // Flat top triangle
        fillFlatTopTriangle(x1, y1, x2, y2, x3, y3, color);
    } else {
        // General case - split the triangle
        int x4 = x1 + ((y2 - y1) * (x3 - x1)) / (y3 - y1);
        int y4 = y2;
        
        fillFlatBottomTriangle(x1, y1, x2, y2, x4, y4, color);
        fillFlatTopTriangle(x2, y2, x4, y4, x3, y3, color);
    }
}

// Helper method for fillTriangle
void DisplayManager::fillFlatBottomTriangle(int x1, int y1, int x2, int y2, int x3, int y3, const Color& color) {
    float slope1 = (float)(x2 - x1) / (y2 - y1);
    float slope2 = (float)(x3 - x1) / (y3 - y1);
    
    float x_start = x1;
    float x_end = x1;
    
    for (int y = y1; y <= y2; y++) {
        drawLine(std::round(x_start), y, std::round(x_end), y, color);
        x_start += slope1;
        x_end += slope2;
    }
}

// Helper method for fillTriangle
void DisplayManager::fillFlatTopTriangle(int x1, int y1, int x2, int y2, int x3, int y3, const Color& color) {
    float slope1 = (float)(x3 - x1) / (y3 - y1);
    float slope2 = (float)(x3 - x2) / (y3 - y2);
    
    float x_start = x3;
    float x_end = x3;
    
    for (int y = y3; y >= y1; y--) {
        drawLine(std::round(x_start), y, std::round(x_end), y, color);
        x_start -= slope1;
        x_end -= slope2;
    }
}

void DisplayManager::drawText(int x, int y, const std::string& text, Font* font, const Color& color) {
    if (!font) {
        return;
    }
    
    int cursorX = x;
    int cursorY = y;
    uint32_t prevChar = 0;
    
    // Process each character
    for (char c : text) {
        uint32_t codepoint = static_cast<uint32_t>(c);
        
        // Apply kerning
        cursorX += font->getKerning(prevChar, codepoint);
        
        // Get the glyph for this character
        const Glyph* glyph = font->getGlyph(codepoint);
        if (!glyph) {
            // Skip characters without glyphs
            continue;
        }
        
        // Calculate position for the glyph
        int glyphX = cursorX + glyph->xOffset;
        int glyphY = cursorY + glyph->yOffset;
        
        // Draw the glyph
        if (!glyph->bitmap.empty()) {
            for (int j = 0; j < glyph->height; j++) {
                for (int i = 0; i < glyph->width; i++) {
                    int index = j * glyph->width + i;
                    if (index < glyph->bitmap.size()) {
                        uint8_t alpha = glyph->bitmap[index];
                        if (alpha > 0) {
                            Color pixelColor = color;
                            pixelColor.a = (pixelColor.a * alpha) / 255;
                            setPixel(glyphX + i, glyphY + j, pixelColor);
                        }
                    }
                }
            }
        }
        
        // Advance cursor position
        cursorX += glyph->xAdvance;
        prevChar = codepoint;
    }
}

void DisplayManager::setBlendMode(BlendMode mode) {
    blendMode_ = mode;
}

DisplayManager::BlendMode DisplayManager::getBlendMode() const {
    return blendMode_;
}

void DisplayManager::drawImage(int x, int y, const uint8_t* imageData, int imgWidth, int imgHeight, 
                             int srcX, int srcY, int srcWidth, int srcHeight) {
    if (!imageData || imgWidth <= 0 || imgHeight <= 0) {
        return;
    }
    
    // Handle default source rectangle
    if (srcWidth < 0) srcWidth = imgWidth - srcX;
    if (srcHeight < 0) srcHeight = imgHeight - srcY;
    
    // Make sure source rect is valid
    if (srcX < 0 || srcY < 0 || srcX + srcWidth > imgWidth || srcY + srcHeight > imgHeight) {
        return;
    }
    
    // Draw the image
    for (int j = 0; j < srcHeight; j++) {
        for (int i = 0; i < srcWidth; i++) {
            // Calculate source pixel position
            int srcPixelX = srcX + i;
            int srcPixelY = srcY + j;
            int srcIndex = (srcPixelY * imgWidth + srcPixelX) * 4; // Assuming 4 bytes per pixel
            
            // Extract color
            Color pixelColor(
                imageData[srcIndex],
                imageData[srcIndex + 1],
                imageData[srcIndex + 2],
                imageData[srcIndex + 3]
            );
            
            // Draw pixel
            setPixel(x + i, y + j, pixelColor);
        }
    }
}

void DisplayManager::setClipRect(const Rect& rect) {
    clipRect_ = rect;
    
    // Clamp to screen bounds
    clipRect_.x = std::max(0, clipRect_.x);
    clipRect_.y = std::max(0, clipRect_.y);
    clipRect_.width = std::min(width_ - clipRect_.x, clipRect_.width);
    clipRect_.height = std::min(height_ - clipRect_.y, clipRect_.height);
    
    hasClipRect_ = true;
}

void DisplayManager::clearClipRect() {
    hasClipRect_ = false;
}

Rect DisplayManager::getClipRect() const {
    return clipRect_;
}

uint8_t* DisplayManager::getFramebuffer() {
    return backBuffer_.data();
}

void DisplayManager::blendPixel(int x, int y, const Color& color) {
    // Calculate pixel offset
    size_t offset = static_cast<size_t>(y * pitch_ + x * bytesPerPixel_);
    
    // Get destination color
    uint8_t destR = backBuffer_[offset];
    uint8_t destG = backBuffer_[offset + 1];
    uint8_t destB = backBuffer_[offset + 2];
    uint8_t destA = backBuffer_[offset + 3];
    
    // Apply blending based on mode
    uint8_t resultR, resultG, resultB, resultA;
    
    if (blendMode_ == BlendMode::None) {
        // Direct overwrite
        resultR = color.r;
        resultG = color.g;
        resultB = color.b;
        resultA = color.a;
    }
    else if (blendMode_ == BlendMode::Alpha) {
        // Alpha blending
        float srcAlpha = color.a / 255.0f;
        float destAlpha = destA / 255.0f;
        float outAlpha = srcAlpha + destAlpha * (1.0f - srcAlpha);
        
        if (outAlpha > 0.0f) {
            resultR = static_cast<uint8_t>((color.r * srcAlpha + destR * destAlpha * (1.0f - srcAlpha)) / outAlpha);
            resultG = static_cast<uint8_t>((color.g * srcAlpha + destG * destAlpha * (1.0f - srcAlpha)) / outAlpha);
            resultB = static_cast<uint8_t>((color.b * srcAlpha + destB * destAlpha * (1.0f - srcAlpha)) / outAlpha);
        } else {
            resultR = 0;
            resultG = 0;
            resultB = 0;
        }
        
        resultA = static_cast<uint8_t>(outAlpha * 255.0f);
    }
    else if (blendMode_ == BlendMode::Add) {
        // Additive blending
        resultR = std::min(255, destR + color.r);
        resultG = std::min(255, destG + color.g);
        resultB = std::min(255, destB + color.b);
        resultA = std::max(destA, color.a);
    }
    else if (blendMode_ == BlendMode::Multiply) {
        // Multiplicative blending
        resultR = (destR * color.r) / 255;
        resultG = (destG * color.g) / 255;
        resultB = (destB * color.b) / 255;
        resultA = (destA * color.a) / 255;
    }
    else {
        // Default to direct copy if mode not recognized
        resultR = color.r;
        resultG = color.g;
        resultB = color.b;
        resultA = color.a;
    }
    
    // Write the result back to the buffer
    backBuffer_[offset] = resultR;
    backBuffer_[offset + 1] = resultG;
    backBuffer_[offset + 2] = resultB;
    backBuffer_[offset + 3] = resultA;
}

bool DisplayManager::isInClipRect(int x, int y) const {
    if (!hasClipRect_) {
        return true;
    }
    
    return x >= clipRect_.x && x < clipRect_.x + clipRect_.width &&
           y >= clipRect_.y && y < clipRect_.y + clipRect_.height;
}

} // namespace AIMusicHardware