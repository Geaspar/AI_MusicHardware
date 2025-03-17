#pragma once

#include <vector>
#include <memory>
#include <cstdint>
#include <string>

namespace AIMusicHardware {

// Color structure
struct Color {
    uint8_t r, g, b, a;
    
    Color() : r(0), g(0), b(0), a(255) {}
    Color(uint8_t _r, uint8_t _g, uint8_t _b, uint8_t _a = 255) 
        : r(_r), g(_g), b(_b), a(_a) {}
    
    static Color Black() { return Color(0, 0, 0); }
    static Color White() { return Color(255, 255, 255); }
    static Color Red() { return Color(255, 0, 0); }
    static Color Green() { return Color(0, 255, 0); }
    static Color Blue() { return Color(0, 0, 255); }
    static Color Yellow() { return Color(255, 255, 0); }
    static Color Cyan() { return Color(0, 255, 255); }
    static Color Magenta() { return Color(255, 0, 255); }
    static Color Orange() { return Color(255, 165, 0); }
};

// Point structure
struct Point {
    int x, y;
    
    Point() : x(0), y(0) {}
    Point(int _x, int _y) : x(_x), y(_y) {}
};

// Rectangle structure
struct Rect {
    int x, y, width, height;
    
    Rect() : x(0), y(0), width(0), height(0) {}
    Rect(int _x, int _y, int _w, int _h) 
        : x(_x), y(_y), width(_w), height(_h) {}
        
    bool contains(const Point& p) const {
        return p.x >= x && p.x < x + width && 
               p.y >= y && p.y < y + height;
    }
    
    bool intersects(const Rect& other) const {
        return !(x + width <= other.x || other.x + other.width <= x ||
                 y + height <= other.y || other.y + other.height <= y);
    }
};

// Forward declaration
class Font;

// DisplayManager class - abstraction for framebuffer access
class DisplayManager {
public:
    DisplayManager();
    ~DisplayManager();
    
    // Initialization and shutdown
    bool initialize(int width, int height);
    void shutdown();
    
    // Basic framebuffer operations
    void clear(const Color& color = Color::Black());
    void swapBuffers();
    void setPixel(int x, int y, const Color& color);
    Color getPixel(int x, int y) const;
    
    // Drawing primitives
    void drawLine(int x1, int y1, int x2, int y2, const Color& color);
    void drawRect(int x, int y, int width, int height, const Color& color);
    void fillRect(int x, int y, int width, int height, const Color& color);
    void drawCircle(int x, int y, int radius, const Color& color);
    void fillCircle(int x, int y, int radius, const Color& color);
    void drawTriangle(int x1, int y1, int x2, int y2, int x3, int y3, const Color& color);
    void fillTriangle(int x1, int y1, int x2, int y2, int x3, int y3, const Color& color);
    
    // Text rendering (will work with Font class)
    void drawText(int x, int y, const std::string& text, Font* font, const Color& color);
    
    // Blending modes
    enum class BlendMode {
        None,       // Overwrite destination
        Alpha,      // Alpha blending
        Add,        // Additive blending
        Multiply    // Multiplicative blending
    };
    
    void setBlendMode(BlendMode mode);
    BlendMode getBlendMode() const;
    
    // Image drawing
    void drawImage(int x, int y, const uint8_t* imageData, int imgWidth, int imgHeight, 
                  int srcX = 0, int srcY = 0, int srcWidth = -1, int srcHeight = -1);
    
    // Clipping
    void setClipRect(const Rect& rect);
    void clearClipRect();
    Rect getClipRect() const;
    
    // Utility
    int getWidth() const { return width_; }
    int getHeight() const { return height_; }
    
    // Direct access to framebuffer (use with care)
    uint8_t* getFramebuffer();
    
private:
    int width_;
    int height_;
    int bytesPerPixel_;
    int pitch_;
    BlendMode blendMode_;
    Rect clipRect_;
    bool hasClipRect_;
    
    std::vector<uint8_t> frontBuffer_;
    std::vector<uint8_t> backBuffer_;
    
    // Utility functions
    void blendPixel(int x, int y, const Color& color);
    bool isInClipRect(int x, int y) const;
    void fillFlatBottomTriangle(int x1, int y1, int x2, int y2, int x3, int y3, const Color& color);
    void fillFlatTopTriangle(int x1, int y1, int x2, int y2, int x3, int y3, const Color& color);
};

} // namespace AIMusicHardware