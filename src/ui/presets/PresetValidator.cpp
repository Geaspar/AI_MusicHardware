#include "ui/presets/PresetValidator.h"
#include <filesystem>
#include <fstream>
#include <algorithm>
#include <sstream>
#include <chrono>
#include <cctype>

namespace AIMusicHardware {

PresetValidator::PresetValidator(const ValidationConfig& config) 
    : config_(config) {
    initializeDefaultRules();
}

PresetValidator::~PresetValidator() = default;

PresetValidationReport PresetValidator::validatePreset(const PresetInfo& preset) {
    auto startTime = std::chrono::high_resolution_clock::now();
    
    PresetValidationReport report;
    report.isValid = true;
    
    // Run all validation categories
    if (config_.validateFileExists || config_.validateFileSize) {
        auto fileResults = validateFile(preset);
        report.results.insert(report.results.end(), fileResults.begin(), fileResults.end());
    }
    
    if (config_.validateJsonStructure || config_.validateRequiredFields || 
        config_.validateParameterValues || config_.validateParameterTypes) {
        auto jsonResults = validateJson(preset);
        report.results.insert(report.results.end(), jsonResults.begin(), jsonResults.end());
    }
    
    if (config_.validatePresetName || config_.validateAuthorName || 
        config_.validateCategory || config_.validateTags || config_.validateDateFields) {
        auto metadataResults = validateMetadata(preset);
        report.results.insert(report.results.end(), metadataResults.begin(), metadataResults.end());
    }
    
    if (config_.validateAudioCharacteristics) {
        auto audioResults = validateAudioCharacteristics(preset);
        report.results.insert(report.results.end(), audioResults.begin(), audioResults.end());
    }
    
    if (config_.validateParameterRanges || config_.validateModulationValues) {
        auto paramResults = validateParameters(preset);
        report.results.insert(report.results.end(), paramResults.begin(), paramResults.end());
    }
    
    if (config_.validateForMaliciousContent || config_.validateFilePaths || 
        config_.validateExternalReferences) {
        auto securityResults = validateSecurity(preset);
        report.results.insert(report.results.end(), securityResults.begin(), securityResults.end());
    }
    
    if (config_.validatePerformanceImpact) {
        auto perfResults = validatePerformance(preset);
        report.results.insert(report.results.end(), perfResults.begin(), perfResults.end());
    }
    
    // Run custom rules
    for (const auto& [name, rulePair] : customRules_) {
        try {
            auto result = rulePair.first(preset, config_);
            if (!result.isValid && result.severity == ValidationSeverity::Info) {
                result.severity = rulePair.second; // Use rule's default severity
            }
            report.results.push_back(result);
        } catch (const std::exception& e) {
            if (errorHandler_) {
                errorHandler_->reportError(PresetErrorCode::ValidationFailed, 
                                         PresetErrorSeverity::Warning,
                                         "Custom validation rule failed: " + std::string(e.what()),
                                         "Custom rule: " + name);
            }
        }
    }
    
    // Count results by severity
    for (const auto& result : report.results) {
        switch (result.severity) {
            case ValidationSeverity::Info:
                report.infoCount++;
                break;
            case ValidationSeverity::Warning:
                report.warningCount++;
                break;
            case ValidationSeverity::Error:
                report.errorCount++;
                if (result.isValid == false) report.isValid = false;
                break;
            case ValidationSeverity::Critical:
                report.criticalCount++;
                report.isValid = false;
                break;
        }
    }
    
    // Generate summary
    report.summary = createValidationSummary(report.results);
    
    // Calculate validation time
    auto endTime = std::chrono::high_resolution_clock::now();
    report.validationTime = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
    
    // Update statistics
    updateStatistics(report);
    
    return report;
}

std::map<std::string, PresetValidationReport> PresetValidator::validatePresets(
    const std::vector<PresetInfo>& presets,
    std::function<void(int, int)> progressCallback) {
    
    std::map<std::string, PresetValidationReport> reports;
    
    for (size_t i = 0; i < presets.size(); ++i) {
        reports[presets[i].filePath] = validatePreset(presets[i]);
        
        if (progressCallback) {
            progressCallback(static_cast<int>(i + 1), static_cast<int>(presets.size()));
        }
    }
    
    return reports;
}

bool PresetValidator::quickValidate(const PresetInfo& preset) {
    // Quick validation - only essential checks
    ValidationConfig quickConfig = config_;
    quickConfig.validateAudioCharacteristics = false;
    quickConfig.validatePerformanceImpact = false;
    quickConfig.validateForMaliciousContent = false;
    
    auto originalConfig = config_;
    config_ = quickConfig;
    
    auto report = validatePreset(preset);
    
    config_ = originalConfig;
    
    return report.hasPassedValidation();
}

std::vector<ValidationResult> PresetValidator::validateFile(const PresetInfo& preset) {
    std::vector<ValidationResult> results;
    
    if (config_.validateFileExists) {
        results.push_back(validateFileExists(preset.filePath));
    }
    
    if (config_.validateFileSize) {
        results.push_back(validateFileSize(preset.filePath));
    }
    
    return results;
}

std::vector<ValidationResult> PresetValidator::validateJson(const PresetInfo& preset) {
    std::vector<ValidationResult> results;
    
    if (config_.validateJsonStructure) {
        results.push_back(validateJsonSyntax(preset.filePath));
    }
    
    // Only validate JSON content if file exists and is valid JSON
    if (std::filesystem::exists(preset.filePath)) {
        try {
            std::ifstream file(preset.filePath);
            nlohmann::json json;
            file >> json;
            
            if (config_.validateRequiredFields) {
                results.push_back(validateRequiredJsonFields(json));
            }
            
            if (config_.validateParameterTypes && json.contains("parameters")) {
                results.push_back(validateParameterTypes(json["parameters"]));
            }
            
            if (config_.validateParameterValues && json.contains("parameters")) {
                results.push_back(validateParameterRanges(json["parameters"]));
            }
            
        } catch (const std::exception& e) {
            results.push_back(createResult(false, ValidationSeverity::Error,
                "Failed to parse JSON: " + std::string(e.what()), "JSON", 
                "Check JSON syntax and structure"));
        }
    }
    
    return results;
}

std::vector<ValidationResult> PresetValidator::validateMetadata(const PresetInfo& preset) {
    std::vector<ValidationResult> results;
    
    if (config_.validatePresetName) {
        results.push_back(validatePresetName(preset.name));
    }
    
    if (config_.validateAuthorName) {
        results.push_back(validateAuthorName(preset.author));
    }
    
    if (config_.validateCategory) {
        results.push_back(validateCategory(preset.category));
    }
    
    if (config_.validateTags) {
        auto tagResults = validateTags(preset.tags);
        results.insert(results.end(), tagResults.begin(), tagResults.end());
    }
    
    if (config_.validateDateFields) {
        results.push_back(validateDateField(preset.created, "created"));
        results.push_back(validateDateField(preset.modified, "modified"));
    }
    
    return results;
}

std::vector<ValidationResult> PresetValidator::validateAudioCharacteristics(const PresetInfo& preset) {
    std::vector<ValidationResult> results;
    
    const auto& audio = preset.audioCharacteristics;
    
    results.push_back(validateFloatRange(audio.bassContent, "bassContent", 0.0f, 1.0f));
    results.push_back(validateFloatRange(audio.midContent, "midContent", 0.0f, 1.0f));
    results.push_back(validateFloatRange(audio.trebleContent, "trebleContent", 0.0f, 1.0f));
    results.push_back(validateFloatRange(audio.brightness, "brightness", 0.0f, 1.0f));
    results.push_back(validateFloatRange(audio.warmth, "warmth", 0.0f, 1.0f));
    results.push_back(validateFloatRange(audio.complexity, "complexity", 0.0f, 1.0f));
    
    if (audio.modulationCount < 0) {
        results.push_back(createResult(false, ValidationSeverity::Error,
            "Modulation count cannot be negative", "modulationCount",
            "Set modulation count to 0 or positive value"));
    }
    
    // Check for reasonable audio characteristic values
    float totalFreqContent = audio.bassContent + audio.midContent + audio.trebleContent;
    if (totalFreqContent > 3.0f) {
        results.push_back(createResult(false, ValidationSeverity::Warning,
            "Total frequency content seems unusually high", "audioCharacteristics",
            "Check bass, mid, and treble content values"));
    }
    
    return results;
}

std::vector<ValidationResult> PresetValidator::validateParameters(const PresetInfo& preset) {
    std::vector<ValidationResult> results;
    
    if (!preset.parameterData.empty()) {
        try {
            if (config_.validateParameterRanges) {
                results.push_back(validateParameterRanges(preset.parameterData));
            }
            
            if (config_.validateModulationValues) {
                results.push_back(validateModulationComplexity(preset.parameterData));
            }
        } catch (const std::exception& e) {
            results.push_back(createResult(false, ValidationSeverity::Error,
                "Parameter validation failed: " + std::string(e.what()), "parameters",
                "Check parameter data structure"));
        }
    }
    
    return results;
}

std::vector<ValidationResult> PresetValidator::validateSecurity(const PresetInfo& preset) {
    std::vector<ValidationResult> results;
    
    if (config_.validateForMaliciousContent) {
        results.push_back(validateForScriptInjection(preset.name));
        results.push_back(validateForScriptInjection(preset.author));
        results.push_back(validateForScriptInjection(preset.description));
    }
    
    if (config_.validateFilePaths) {
        results.push_back(validateFilePath(preset.filePath));
    }
    
    if (config_.validateExternalReferences && !preset.parameterData.empty()) {
        results.push_back(validateExternalReferences(preset.parameterData));
    }
    
    return results;
}

std::vector<ValidationResult> PresetValidator::validatePerformance(const PresetInfo& preset) {
    std::vector<ValidationResult> results;
    
    if (!preset.parameterData.empty()) {
        results.push_back(validateVoiceCount(preset.parameterData));
        results.push_back(validateModulationComplexity(preset.parameterData));
        results.push_back(validateEffectsChain(preset.parameterData));
    }
    
    return results;
}

ValidationResult PresetValidator::validatePresetName(const std::string& name) {
    return validateStringField(name, "name", config_.validNamePattern, config_.maxNameLength);
}

ValidationResult PresetValidator::validateAuthorName(const std::string& author) {
    return validateStringField(author, "author", config_.validAuthorPattern, config_.maxAuthorLength);
}

ValidationResult PresetValidator::validateCategory(const std::string& category) {
    if (category.empty()) {
        return createResult(false, ValidationSeverity::Warning,
            "Category is empty", "category", "Set a valid category");
    }
    
    if (config_.allowedCategories.find(category) == config_.allowedCategories.end()) {
        return createResult(false, ValidationSeverity::Warning,
            "Category '" + category + "' is not in allowed list", "category",
            "Use one of the standard categories");
    }
    
    return createResult(true, ValidationSeverity::Info, "Category is valid", "category");
}

std::vector<ValidationResult> PresetValidator::validateTags(const std::vector<std::string>& tags) {
    std::vector<ValidationResult> results;
    
    if (tags.size() > static_cast<size_t>(config_.maxTagCount)) {
        results.push_back(createResult(false, ValidationSeverity::Warning,
            "Too many tags (" + std::to_string(tags.size()) + ")", "tags",
            "Reduce to " + std::to_string(config_.maxTagCount) + " or fewer tags"));
    }
    
    for (const auto& tag : tags) {
        if (tag.empty()) {
            results.push_back(createResult(false, ValidationSeverity::Error,
                "Empty tag found", "tags", "Remove empty tags"));
            continue;
        }
        
        if (tag.length() > config_.maxTagLength) {
            results.push_back(createResult(false, ValidationSeverity::Warning,
                "Tag '" + tag + "' is too long", "tags",
                "Shorten to " + std::to_string(config_.maxTagLength) + " characters or less"));
        }
        
        // Check for invalid characters
        if (std::any_of(tag.begin(), tag.end(), [](char c) { 
            return !std::isalnum(c) && c != '-' && c != '_' && c != ' '; 
        })) {
            results.push_back(createResult(false, ValidationSeverity::Warning,
                "Tag '" + tag + "' contains invalid characters", "tags",
                "Use only alphanumeric characters, spaces, hyphens, and underscores"));
        }
    }
    
    return results;
}

// Implementation of private helper methods...

ValidationResult PresetValidator::validateFileExists(const std::string& filePath) {
    if (!std::filesystem::exists(filePath)) {
        return createResult(false, ValidationSeverity::Critical,
            "Preset file does not exist", "filePath", "Check file path and ensure file exists");
    }
    return createResult(true, ValidationSeverity::Info, "File exists", "filePath");
}

ValidationResult PresetValidator::validateFileSize(const std::string& filePath) {
    try {
        auto fileSize = std::filesystem::file_size(filePath);
        
        if (fileSize < config_.minFileSizeBytes) {
            return createResult(false, ValidationSeverity::Error,
                "File is too small (" + std::to_string(fileSize) + " bytes)", "fileSize",
                "File may be corrupted or incomplete");
        }
        
        if (fileSize > config_.maxFileSizeBytes) {
            return createResult(false, ValidationSeverity::Warning,
                "File is very large (" + std::to_string(fileSize) + " bytes)", "fileSize",
                "Consider optimizing preset data");
        }
        
        return createResult(true, ValidationSeverity::Info, "File size is reasonable", "fileSize");
    } catch (const std::exception& e) {
        return createResult(false, ValidationSeverity::Error,
            "Cannot determine file size: " + std::string(e.what()), "fileSize",
            "Check file permissions and integrity");
    }
}

ValidationResult PresetValidator::validateJsonSyntax(const std::string& filePath) {
    try {
        std::ifstream file(filePath);
        if (!file.is_open()) {
            return createResult(false, ValidationSeverity::Error,
                "Cannot open file for JSON validation", "JSON", "Check file permissions");
        }
        
        nlohmann::json json;
        file >> json;
        
        return createResult(true, ValidationSeverity::Info, "JSON syntax is valid", "JSON");
    } catch (const std::exception& e) {
        return createResult(false, ValidationSeverity::Error,
            "Invalid JSON syntax: " + std::string(e.what()), "JSON",
            "Fix JSON syntax errors");
    }
}

ValidationResult PresetValidator::validateStringField(const std::string& value, const std::string& fieldName,
                                                     const std::regex& pattern, size_t maxLength) {
    if (value.empty()) {
        return createResult(false, ValidationSeverity::Warning,
            fieldName + " is empty", fieldName, "Provide a " + fieldName);
    }
    
    if (value.length() > maxLength) {
        return createResult(false, ValidationSeverity::Warning,
            fieldName + " is too long (" + std::to_string(value.length()) + " characters)",
            fieldName, "Shorten to " + std::to_string(maxLength) + " characters or less");
    }
    
    if (!std::regex_match(value, pattern)) {
        return createResult(false, ValidationSeverity::Warning,
            fieldName + " contains invalid characters", fieldName,
            "Use only allowed characters");
    }
    
    return createResult(true, ValidationSeverity::Info, fieldName + " is valid", fieldName);
}

// Additional implementation methods would continue here...
// Due to length constraints, showing key methods

void PresetValidator::updateStatistics(const PresetValidationReport& report) {
    stats_.totalValidated++;
    if (report.isValid) {
        stats_.validPresets++;
    } else {
        stats_.invalidPresets++;
    }
    
    stats_.totalIssues += static_cast<int>(report.results.size());
    stats_.criticalIssues += report.criticalCount;
    stats_.errorIssues += report.errorCount;
    stats_.warningIssues += report.warningCount;
    
    stats_.totalValidationTime += report.validationTime;
    stats_.averageValidationTime = static_cast<float>(stats_.totalValidationTime.count()) / stats_.totalValidated;
}

ValidationResult PresetValidator::createResult(bool valid, ValidationSeverity severity, 
                                             const std::string& message, const std::string& field,
                                             const std::string& suggestion) {
    ValidationResult result;
    result.isValid = valid;
    result.severity = severity;
    result.message = message;
    result.field = field;
    result.suggestion = suggestion;
    return result;
}

std::string PresetValidator::createValidationSummary(const std::vector<ValidationResult>& results) {
    int errors = 0, warnings = 0, infos = 0, criticals = 0;
    
    for (const auto& result : results) {
        switch (result.severity) {
            case ValidationSeverity::Critical: criticals++; break;
            case ValidationSeverity::Error: errors++; break;
            case ValidationSeverity::Warning: warnings++; break;
            case ValidationSeverity::Info: infos++; break;
        }
    }
    
    std::ostringstream summary;
    if (criticals > 0 || errors > 0) {
        summary << "Validation failed: ";
    } else {
        summary << "Validation passed: ";
    }
    
    summary << criticals << " critical, " << errors << " errors, " 
            << warnings << " warnings, " << infos << " info";
    
    return summary.str();
}

void PresetValidator::initializeDefaultRules() {
    // Default rules are already implemented in the main validation methods
    // Custom rules would be added here if needed
}

PresetValidator::ValidationStatistics PresetValidator::getStatistics() const {
    return stats_;
}

void PresetValidator::resetStatistics() {
    stats_ = ValidationStatistics{};
}

// ValidationUtils namespace implementation
namespace ValidationUtils {

std::string formatResult(const ValidationResult& result) {
    std::ostringstream ss;
    ss << "[" << (result.isValid ? "PASS" : "FAIL") << "] ";
    ss << result.message;
    if (!result.field.empty()) {
        ss << " (Field: " << result.field << ")";
    }
    if (!result.suggestion.empty()) {
        ss << " - Suggestion: " << result.suggestion;
    }
    return ss.str();
}

std::string severityToColorCode(ValidationSeverity severity) {
    switch (severity) {
        case ValidationSeverity::Info: return "#2196F3";      // Blue
        case ValidationSeverity::Warning: return "#FF9800";   // Orange
        case ValidationSeverity::Error: return "#F44336";     // Red
        case ValidationSeverity::Critical: return "#9C27B0";  // Purple
        default: return "#000000";
    }
}

std::string severityToIcon(ValidationSeverity severity) {
    switch (severity) {
        case ValidationSeverity::Info: return "ℹ";
        case ValidationSeverity::Warning: return "⚠";
        case ValidationSeverity::Error: return "❌";
        case ValidationSeverity::Critical: return "🔥";
        default: return "?";
    }
}

} // namespace ValidationUtils

} // namespace AIMusicHardware