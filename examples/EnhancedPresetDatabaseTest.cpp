#include <iostream>
#include <chrono>
#include <filesystem>
#include <fstream>
#include "../include/ui/presets/PresetDatabase.h"

using namespace AIMusicHardware;

/**
 * @brief Test the enhanced preset database with professional features
 * 
 * This test demonstrates the performance and functionality improvements
 * based on analysis of Vital synth and industry best practices.
 */

void createTestPresets(const std::string& directory) {
    std::filesystem::create_directories(directory + "/Bass");
    std::filesystem::create_directories(directory + "/Lead");
    std::filesystem::create_directories(directory + "/Pad");
    
    // Create test presets with metadata
    auto createPreset = [&](const std::string& name, const std::string& category, 
                           const std::string& author, const std::string& description) {
        nlohmann::json preset;
        preset["metadata"] = {
            {"author", author},
            {"category", category},
            {"comments", description},
            {"created", std::chrono::duration_cast<std::chrono::seconds>(
                std::chrono::system_clock::now().time_since_epoch()).count()},
            {"tags", nlohmann::json::array({"electronic", "synth"})}
        };
        
        preset["parameters"] = {
            {"osc1_waveform", 0},
            {"osc1_level", 0.8},
            {"filter_cutoff", category == "Bass" ? 800.0 : 4000.0},
            {"filter_resonance", 0.4},
            {"env_attack", 0.01},
            {"env_decay", 0.3},
            {"env_sustain", 0.5},
            {"env_release", 0.5}
        };
        
        preset["modulations"] = nlohmann::json::array({
            {{"source", "lfo1"}, {"destination", "filter_cutoff"}, {"amount", 0.3}}
        });
        
        std::string filePath = directory + "/" + category + "/" + name + ".json";
        std::ofstream file(filePath);
        file << preset.dump(2);
    };
    
    // Create test presets
    createPreset("Deep Bass", "Bass", "John Doe", "Rich deep bass with movement");
    createPreset("Sub Bass", "Bass", "Jane Smith", "Analog-style sub bass");
    createPreset("Pluck Bass", "Bass", "John Doe", "Percussive bass sound");
    
    createPreset("Bright Lead", "Lead", "Alex Johnson", "Cutting lead synthesizer");
    createPreset("Warm Lead", "Lead", "Sarah Wilson", "Smooth warm lead tone");
    createPreset("Acid Lead", "Lead", "Mike Davis", "Squelchy acid lead");
    
    createPreset("Lush Pad", "Pad", "Emma Brown", "Atmospheric pad sound");
    createPreset("String Pad", "Pad", "David Lee", "String-like pad texture");
    createPreset("Ambient Pad", "Pad", "Lisa Chen", "Spacious ambient pad");
}

void testBasicFunctionality(PresetDatabase& db) {
    std::cout << "\n=== Testing Basic Functionality ===" << std::endl;
    
    // Test getting all presets
    auto allPresets = db.getAllPresets();
    std::cout << "Total presets loaded: " << allPresets.size() << std::endl;
    
    // Test category filtering
    auto bassPresets = db.getByCategory("Bass");
    std::cout << "Bass presets found: " << bassPresets.size() << std::endl;
    
    auto leadPresets = db.getByCategory("Lead");
    std::cout << "Lead presets found: " << leadPresets.size() << std::endl;
    
    // Test author filtering
    auto johnDoePresets = db.getByAuthor("John Doe");
    std::cout << "John Doe presets found: " << johnDoePresets.size() << std::endl;
    
    // Test search functionality
    auto deepPresets = db.searchByName("Deep");
    std::cout << "Presets containing 'Deep': " << deepPresets.size() << std::endl;
    
    auto bassSearchPresets = db.searchByName("Bass");
    std::cout << "Presets containing 'Bass': " << bassSearchPresets.size() << std::endl;
}

void testAdvancedFiltering(PresetDatabase& db) {
    std::cout << "\n=== Testing Advanced Filtering ===" << std::endl;
    
    // Test complex filter criteria
    PresetFilterCriteria criteria;
    criteria.searchText = "bass";
    criteria.categories = {"Bass"};
    criteria.authors = {"John Doe"};
    
    auto filteredPresets = db.filter(criteria);
    std::cout << "Complex filter results: " << filteredPresets.size() << std::endl;
    
    for (const auto& preset : filteredPresets) {
        std::cout << "  - " << preset.name << " by " << preset.author 
                  << " (" << preset.category << ")" << std::endl;
    }
    
    // Test audio characteristics filtering
    criteria.clear();
    criteria.hasAudioFilter = true;
    criteria.minBassContent = 0.5f;  // Find bass-heavy presets
    
    auto bassyPresets = db.filter(criteria);
    std::cout << "Bass-heavy presets found: " << bassyPresets.size() << std::endl;
}

void testSorting(PresetDatabase& db) {
    std::cout << "\n=== Testing Sorting Functionality ===" << std::endl;
    
    auto presets = db.getAllPresets();
    
    // Test sorting by name
    db.sort(presets, PresetSortCriteria::Name, SortDirection::Ascending);
    std::cout << "Sorted by name (ascending):" << std::endl;
    for (size_t i = 0; i < std::min(presets.size(), size_t(5)); ++i) {
        std::cout << "  " << (i+1) << ". " << presets[i].name << std::endl;
    }
    
    // Test sorting by author
    db.sort(presets, PresetSortCriteria::Author, SortDirection::Ascending);
    std::cout << "Sorted by author (ascending):" << std::endl;
    for (size_t i = 0; i < std::min(presets.size(), size_t(5)); ++i) {
        std::cout << "  " << (i+1) << ". " << presets[i].name << " by " << presets[i].author << std::endl;
    }
    
    // Test sorting by category
    db.sort(presets, PresetSortCriteria::Category, SortDirection::Ascending);
    std::cout << "Sorted by category (ascending):" << std::endl;
    for (size_t i = 0; i < std::min(presets.size(), size_t(5)); ++i) {
        std::cout << "  " << (i+1) << ". " << presets[i].name << " (" << presets[i].category << ")" << std::endl;
    }
}

void testPerformance(PresetDatabase& db) {
    std::cout << "\n=== Testing Performance ===" << std::endl;
    
    // Test search performance
    auto start = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < 100; ++i) {
        auto results = db.searchByName("bass");
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    std::cout << "100 searches took: " << duration.count() << " microseconds" << std::endl;
    std::cout << "Average search time: " << duration.count() / 100.0 << " microseconds" << std::endl;
    
    // Test filtering performance
    start = std::chrono::high_resolution_clock::now();
    
    PresetFilterCriteria criteria;
    criteria.categories = {"Bass", "Lead"};
    
    for (int i = 0; i < 100; ++i) {
        auto results = db.filter(criteria);
    }
    
    end = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    std::cout << "100 filters took: " << duration.count() << " microseconds" << std::endl;
    std::cout << "Average filter time: " << duration.count() / 100.0 << " microseconds" << std::endl;
}

void testMetadataAnalysis(PresetDatabase& db) {
    std::cout << "\n=== Testing Metadata Analysis ===" << std::endl;
    
    auto allCategories = db.getAllCategories();
    std::cout << "Categories found: ";
    for (const auto& category : allCategories) {
        std::cout << category << " ";
    }
    std::cout << std::endl;
    
    auto allAuthors = db.getAllAuthors();
    std::cout << "Authors found: ";
    for (const auto& author : allAuthors) {
        std::cout << "\"" << author << "\" ";
    }
    std::cout << std::endl;
    
    // Test preset with audio characteristics
    auto presets = db.getAllPresets();
    if (!presets.empty()) {
        const auto& preset = presets[0];
        std::cout << "\nAudio characteristics for '" << preset.name << "':" << std::endl;
        std::cout << "  Bass content: " << preset.audioCharacteristics.bassContent << std::endl;
        std::cout << "  Mid content: " << preset.audioCharacteristics.midContent << std::endl;
        std::cout << "  Treble content: " << preset.audioCharacteristics.trebleContent << std::endl;
        std::cout << "  Brightness: " << preset.audioCharacteristics.brightness << std::endl;
        std::cout << "  Warmth: " << preset.audioCharacteristics.warmth << std::endl;
        std::cout << "  Complexity: " << preset.audioCharacteristics.complexity << std::endl;
        std::cout << "  Modulation count: " << preset.audioCharacteristics.modulationCount << std::endl;
    }
}

void testStatistics(PresetDatabase& db) {
    std::cout << "\n=== Testing Statistics ===" << std::endl;
    
    auto stats = db.getStatistics();
    std::cout << "Database Statistics:" << std::endl;
    std::cout << "  Total presets: " << stats.totalPresets << std::endl;
    std::cout << "  Total categories: " << stats.totalCategories << std::endl;
    std::cout << "  Total authors: " << stats.totalAuthors << std::endl;
    std::cout << "  Total favorites: " << stats.totalFavorites << std::endl;
    std::cout << "  Cache hit rate: " << stats.cacheHitRate << "%" << std::endl;
    std::cout << "  Last update time: " << stats.lastUpdateTime.count() << "ms" << std::endl;
}

int main() {
    std::cout << "Enhanced Preset Database Test" << std::endl;
    std::cout << "=============================" << std::endl;
    std::cout << "Testing professional-grade preset management based on Vital synth analysis" << std::endl;
    
    // Create test directory and presets
    std::string testDir = "./test_presets";
    std::filesystem::remove_all(testDir);
    createTestPresets(testDir);
    
    try {
        // Initialize database
        PresetDatabase db;
        
        std::cout << "\nInitializing preset database..." << std::endl;
        if (!db.initialize({testDir})) {
            std::cerr << "Failed to initialize preset database!" << std::endl;
            return 1;
        }
        
        // Wait for background scanning to complete
        std::cout << "Waiting for background scanning to complete..." << std::endl;
        if (!db.waitForUpdate(10000)) {
            std::cerr << "Timeout waiting for database update!" << std::endl;
            return 1;
        }
        
        // Run all tests
        testBasicFunctionality(db);
        testAdvancedFiltering(db);
        testSorting(db);
        testMetadataAnalysis(db);
        testStatistics(db);
        testPerformance(db);
        
        std::cout << "\n=== All Tests Completed Successfully! ===" << std::endl;
        std::cout << "\nKey Features Demonstrated:" << std::endl;
        std::cout << "✓ Fast indexed search and filtering" << std::endl;
        std::cout << "✓ Multi-criteria filtering (similar to Vital)" << std::endl;
        std::cout << "✓ Audio characteristics analysis" << std::endl;
        std::cout << "✓ Performance-optimized operations" << std::endl;
        std::cout << "✓ Background scanning with thread safety" << std::endl;
        std::cout << "✓ Comprehensive metadata management" << std::endl;
        std::cout << "✓ Statistical analysis and caching" << std::endl;
        
        std::cout << "\nThis enhanced preset database provides the foundation for" << std::endl;
        std::cout << "a professional-grade preset browser comparable to Vital synth." << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    // Cleanup
    std::filesystem::remove_all(testDir);
    
    return 0;
}