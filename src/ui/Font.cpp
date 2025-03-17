#include "../../include/ui/Font.h"
#include <algorithm>

namespace AIMusicHardware {

Font::Font()
    : fontSize_(12), lineHeight_(14), baseline_(12), ascent_(10), descent_(2) {
}

Font::~Font() {
}

bool Font::loadFromFile(const std::string& filename) {
    // This would typically load a bitmap font from a file
    // Placeholder implementation
    return false;
}

bool Font::loadFromMemory(const uint8_t* data, size_t size) {
    // This would typically load a bitmap font from memory
    // Placeholder implementation
    return false;
}

bool Font::create(int fontSize) {
    fontSize_ = fontSize;
    lineHeight_ = static_cast<int>(fontSize * 1.2f);
    baseline_ = fontSize;
    ascent_ = static_cast<int>(fontSize * 0.8f);
    descent_ = fontSize - ascent_;
    
    // Clear existing glyphs
    glyphs_.clear();
    kerningPairs_.clear();
    
    return true;
}

void Font::addGlyph(const Glyph& glyph) {
    glyphs_[glyph.codepoint] = glyph;
}

const Glyph* Font::getGlyph(uint32_t codepoint) const {
    auto it = glyphs_.find(codepoint);
    if (it != glyphs_.end()) {
        return &it->second;
    }
    return nullptr;
}

void Font::getTextDimensions(const std::string& text, int& width, int& height) const {
    width = 0;
    height = lineHeight_;
    
    int maxWidth = 0;
    int lineCount = 1;
    int currentWidth = 0;
    uint32_t prevChar = 0;
    
    for (char c : text) {
        uint32_t codepoint = static_cast<uint32_t>(c);
        
        if (c == '\n') {
            // Handle newline
            lineCount++;
            maxWidth = std::max(maxWidth, currentWidth);
            currentWidth = 0;
            prevChar = 0;
            continue;
        }
        
        // Get kerning
        int kerning = getKerning(prevChar, codepoint);
        
        // Get glyph
        const Glyph* glyph = getGlyph(codepoint);
        if (glyph) {
            currentWidth += glyph->xAdvance + kerning;
        } else {
            // Use default width for missing glyphs
            currentWidth += fontSize_ / 2;
        }
        
        prevChar = codepoint;
    }
    
    maxWidth = std::max(maxWidth, currentWidth);
    width = maxWidth;
    height = lineHeight_ * lineCount;
}

int Font::getLineHeight() const {
    return lineHeight_;
}

int Font::getBaseline() const {
    return baseline_;
}

int Font::getAscent() const {
    return ascent_;
}

int Font::getDescent() const {
    return descent_;
}

int Font::getKerning(uint32_t first, uint32_t second) const {
    if (first == 0 || second == 0) {
        return 0;
    }
    
    uint64_t key = makeKerningKey(first, second);
    auto it = kerningPairs_.find(key);
    if (it != kerningPairs_.end()) {
        return it->second;
    }
    
    return 0;
}

int Font::getFontSize() const {
    return fontSize_;
}

// Define the static data arrays with minimal placeholder data
const uint8_t FontFactory::defaultFontData_[] = { 0 };
const uint8_t FontFactory::monospaceFontData_[] = { 0 };
const uint8_t FontFactory::titleFontData_[] = { 0 };
const uint8_t FontFactory::iconFontData_[] = { 0 };

std::unique_ptr<Font> FontFactory::createDefaultFont() {
    auto font = std::make_unique<Font>();
    font->create(12);
    
    // Create some basic glyphs for testing
    // This would be replaced with proper font data in a real implementation
    
    return font;
}

std::unique_ptr<Font> FontFactory::createMonospaceFont() {
    auto font = std::make_unique<Font>();
    font->create(12);
    
    // Create monospace glyphs
    
    return font;
}

std::unique_ptr<Font> FontFactory::createTitleFont() {
    auto font = std::make_unique<Font>();
    font->create(18);
    
    // Create title font glyphs
    
    return font;
}

std::unique_ptr<Font> FontFactory::createIconFont() {
    auto font = std::make_unique<Font>();
    font->create(16);
    
    // Create icon font glyphs
    
    return font;
}

} // namespace AIMusicHardware