#pragma once

#include <string>
#include <vector>
#include <memory>
#include <cstdint>
#include <unordered_map>

namespace AIMusicHardware {

// Forward declaration
struct Color;

// Character glyph data
struct Glyph {
    uint32_t codepoint;    // Unicode codepoint
    int width;             // Width in pixels
    int height;            // Height in pixels
    int xAdvance;          // Horizontal advance (next character position)
    int xOffset;           // X offset from cursor position
    int yOffset;           // Y offset from cursor position
    std::vector<uint8_t> bitmap; // Bitmap data (one byte per pixel, 0-255 alpha)
};

// Font class for text rendering
class Font {
public:
    Font();
    ~Font();
    
    // Load a bitmap font from a file
    bool loadFromFile(const std::string& filename);
    
    // Create a bitmap font from memory
    bool loadFromMemory(const uint8_t* data, size_t size);
    
    // Create an empty font
    bool create(int fontSize);
    
    // Add a character glyph to the font
    void addGlyph(const Glyph& glyph);
    
    // Get a glyph from a character
    const Glyph* getGlyph(uint32_t codepoint) const;
    
    // Calculate text dimensions
    void getTextDimensions(const std::string& text, int& width, int& height) const;
    
    // Get font metrics
    int getLineHeight() const;
    int getBaseline() const;
    int getAscent() const;
    int getDescent() const;
    int getKerning(uint32_t first, uint32_t second) const;
    int getFontSize() const;
    
private:
    int fontSize_;
    int lineHeight_;
    int baseline_;
    int ascent_;
    int descent_;
    std::unordered_map<uint32_t, Glyph> glyphs_;
    std::unordered_map<uint64_t, int> kerningPairs_;
    
    // Utility function to create a 64-bit key from two 32-bit codepoints
    static uint64_t makeKerningKey(uint32_t first, uint32_t second) {
        return (static_cast<uint64_t>(first) << 32) | static_cast<uint64_t>(second);
    }
};

// Factory for built-in fonts
class FontFactory {
public:
    static std::unique_ptr<Font> createDefaultFont();
    static std::unique_ptr<Font> createMonospaceFont();
    static std::unique_ptr<Font> createTitleFont();
    static std::unique_ptr<Font> createIconFont();
    
private:
    // Embedding default font data as byte arrays
    static const uint8_t defaultFontData_[];
    static const uint8_t monospaceFontData_[];
    static const uint8_t titleFontData_[];
    static const uint8_t iconFontData_[];
};

} // namespace AIMusicHardware