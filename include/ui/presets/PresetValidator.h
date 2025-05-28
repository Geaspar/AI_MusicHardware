#pragma once

#include "PresetInfo.h"
#include "PresetErrorHandler.h"
#include <string>
#include <vector>
#include <functional>
#include <map>
#include <set>
#include <memory>
#include <regex>

namespace AIMusicHardware {

/**
 * @brief Validation severity levels
 */
enum class ValidationSeverity {
    Info,       // Non-critical information
    Warning,    // Potential issue but not critical
    Error,      // Serious issue that should be fixed
    Critical    // Issue that prevents proper operation
};

/**
 * @brief Validation result for a single check
 */
struct ValidationResult {
    bool isValid = true;
    ValidationSeverity severity = ValidationSeverity::Info;
    std::string message;
    std::string field;          // Which field failed validation
    std::string suggestion;     // How to fix the issue
    std::vector<std::string> additionalInfo;
    
    ValidationResult() = default;
    
    ValidationResult(bool valid, ValidationSeverity sev, const std::string& msg, 
                    const std::string& fieldName = "", const std::string& sug = "")
        : isValid(valid), severity(sev), message(msg), field(fieldName), suggestion(sug) {}
};

/**
 * @brief Complete validation report for a preset
 */
struct PresetValidationReport {
    bool isValid = true;
    std::vector<ValidationResult> results;
    std::string summary;
    std::chrono::milliseconds validationTime{0};
    
    // Statistics
    int infoCount = 0;
    int warningCount = 0;
    int errorCount = 0;
    int criticalCount = 0;
    
    /**
     * @brief Check if validation passed with no errors or critical issues
     */
    bool hasPassedValidation() const {
        return isValid && criticalCount == 0 && errorCount == 0;
    }
    
    /**
     * @brief Get the highest severity level found
     */
    ValidationSeverity getHighestSeverity() const {
        if (criticalCount > 0) return ValidationSeverity::Critical;
        if (errorCount > 0) return ValidationSeverity::Error;
        if (warningCount > 0) return ValidationSeverity::Warning;
        return ValidationSeverity::Info;
    }
};

/**
 * @brief Validation configuration options
 */
struct ValidationConfig {
    // File validation
    bool validateFileExists = true;
    bool validateFileSize = true;
    size_t maxFileSizeBytes = 50 * 1024 * 1024; // 50MB
    size_t minFileSizeBytes = 100;               // 100 bytes
    
    // JSON validation
    bool validateJsonStructure = true;
    bool validateRequiredFields = true;
    bool validateParameterValues = true;
    bool validateParameterTypes = true;
    
    // Content validation
    bool validatePresetName = true;
    bool validateAuthorName = true;
    bool validateCategory = true;
    bool validateTags = true;
    bool validateDateFields = true;
    
    // Audio characteristics validation
    bool validateAudioCharacteristics = true;
    bool validateParameterRanges = true;
    bool validateModulationValues = true;
    
    // Security validation
    bool validateForMaliciousContent = true;
    bool validateFilePaths = true;
    bool validateExternalReferences = true;
    
    // Performance validation
    bool validatePerformanceImpact = true;
    int maxVoiceCount = 32;
    int maxModulationConnections = 100;
    int maxEffectsChainLength = 10;
    
    // Naming conventions
    std::regex validNamePattern{R"(^[a-zA-Z0-9\s\-_()]+$)"};
    std::regex validAuthorPattern{R"(^[a-zA-Z0-9\s\-_.,]+$)"};
    std::set<std::string> allowedCategories{
        "Bass", "Lead", "Pad", "Keys", "Percussion", "Sequence", 
        "Experimental", "SFX", "Template", "Arp"
    };
    
    size_t maxNameLength = 100;
    size_t maxAuthorLength = 100;
    size_t maxDescriptionLength = 1000;
    size_t maxTagLength = 50;
    int maxTagCount = 20;
};

/**
 * @brief Custom validation rule function type
 */
using ValidationRule = std::function<ValidationResult(const PresetInfo&, const ValidationConfig&)>;

/**
 * @brief Comprehensive preset validation system
 */
class PresetValidator {
public:
    /**
     * @brief Constructor
     * @param config Validation configuration
     */
    explicit PresetValidator(const ValidationConfig& config = ValidationConfig{});
    
    /**
     * @brief Destructor
     */
    ~PresetValidator();
    
    // Core validation methods
    
    /**
     * @brief Validate a single preset completely
     * @param preset Preset to validate
     * @return Complete validation report
     */
    PresetValidationReport validatePreset(const PresetInfo& preset);
    
    /**
     * @brief Validate multiple presets efficiently
     * @param presets Vector of presets to validate
     * @param progressCallback Optional progress callback
     * @return Map of preset paths to validation reports
     */
    std::map<std::string, PresetValidationReport> validatePresets(
        const std::vector<PresetInfo>& presets,
        std::function<void(int, int)> progressCallback = nullptr
    );
    
    /**
     * @brief Quick validation check (essential validations only)
     * @param preset Preset to validate
     * @return True if preset passes basic validation
     */
    bool quickValidate(const PresetInfo& preset);
    
    // Individual validation categories
    
    /**
     * @brief Validate file system aspects
     */
    std::vector<ValidationResult> validateFile(const PresetInfo& preset);
    
    /**
     * @brief Validate JSON structure and content
     */
    std::vector<ValidationResult> validateJson(const PresetInfo& preset);
    
    /**
     * @brief Validate metadata fields
     */
    std::vector<ValidationResult> validateMetadata(const PresetInfo& preset);
    
    /**
     * @brief Validate audio characteristics
     */
    std::vector<ValidationResult> validateAudioCharacteristics(const PresetInfo& preset);
    
    /**
     * @brief Validate parameter values and ranges
     */
    std::vector<ValidationResult> validateParameters(const PresetInfo& preset);
    
    /**
     * @brief Validate for security issues
     */
    std::vector<ValidationResult> validateSecurity(const PresetInfo& preset);
    
    /**
     * @brief Validate performance impact
     */
    std::vector<ValidationResult> validatePerformance(const PresetInfo& preset);
    
    // Custom validation rules
    
    /**
     * @brief Add custom validation rule
     * @param name Rule name for identification
     * @param rule Validation function
     * @param severity Default severity for this rule
     */
    void addCustomRule(const std::string& name, ValidationRule rule, 
                      ValidationSeverity severity = ValidationSeverity::Error);
    
    /**
     * @brief Remove custom validation rule
     * @param name Rule name to remove
     */
    void removeCustomRule(const std::string& name);
    
    /**
     * @brief Clear all custom rules
     */
    void clearCustomRules();
    
    // Configuration management
    
    /**
     * @brief Update validation configuration
     * @param config New configuration
     */
    void setConfig(const ValidationConfig& config);
    
    /**
     * @brief Get current configuration
     * @return Current validation config
     */
    const ValidationConfig& getConfig() const { return config_; }
    
    /**
     * @brief Set error handler for validation issues
     * @param errorHandler Error handler instance
     */
    void setErrorHandler(std::shared_ptr<PresetErrorHandler> errorHandler);
    
    // Utility methods
    
    /**
     * @brief Auto-fix common validation issues
     * @param preset Preset to fix (modified in place)
     * @return Vector of fixes applied
     */
    std::vector<std::string> autoFix(PresetInfo& preset);
    
    /**
     * @brief Suggest improvements for preset
     * @param preset Preset to analyze
     * @return Vector of improvement suggestions
     */
    std::vector<std::string> suggestImprovements(const PresetInfo& preset);
    
    /**
     * @brief Validate preset name according to conventions
     * @param name Preset name to validate
     * @return Validation result
     */
    ValidationResult validatePresetName(const std::string& name);
    
    /**
     * @brief Validate author name
     * @param author Author name to validate
     * @return Validation result
     */
    ValidationResult validateAuthorName(const std::string& author);
    
    /**
     * @brief Validate category
     * @param category Category to validate
     * @return Validation result
     */
    ValidationResult validateCategory(const std::string& category);
    
    /**
     * @brief Validate tags
     * @param tags Vector of tags to validate
     * @return Vector of validation results (one per tag)
     */
    std::vector<ValidationResult> validateTags(const std::vector<std::string>& tags);
    
    // Statistics and reporting
    
    /**
     * @brief Get validation statistics
     */
    struct ValidationStatistics {
        int totalValidated = 0;
        int validPresets = 0;
        int invalidPresets = 0;
        int totalIssues = 0;
        int criticalIssues = 0;
        int errorIssues = 0;
        int warningIssues = 0;
        std::map<std::string, int> commonIssues;
        std::chrono::milliseconds totalValidationTime{0};
        float averageValidationTime = 0.0f;
    };
    ValidationStatistics getStatistics() const;
    
    /**
     * @brief Reset statistics
     */
    void resetStatistics();
    
    /**
     * @brief Generate validation report summary
     * @param reports Vector of validation reports
     * @return Human-readable summary
     */
    static std::string generateSummaryReport(const std::vector<PresetValidationReport>& reports);

private:
    ValidationConfig config_;
    std::shared_ptr<PresetErrorHandler> errorHandler_;
    std::map<std::string, std::pair<ValidationRule, ValidationSeverity>> customRules_;
    mutable ValidationStatistics stats_;
    
    // Internal validation helpers
    ValidationResult validateFileExists(const std::string& filePath);
    ValidationResult validateFileSize(const std::string& filePath);
    ValidationResult validateJsonSyntax(const std::string& filePath);
    ValidationResult validateRequiredJsonFields(const nlohmann::json& json);
    ValidationResult validateParameterTypes(const nlohmann::json& parameters);
    ValidationResult validateParameterRanges(const nlohmann::json& parameters);
    
    // Content validation helpers
    ValidationResult validateStringField(const std::string& value, const std::string& fieldName,
                                       const std::regex& pattern, size_t maxLength);
    ValidationResult validateDateField(const std::chrono::system_clock::time_point& timePoint,
                                     const std::string& fieldName);
    ValidationResult validateFloatRange(float value, const std::string& fieldName,
                                      float minVal, float maxVal);
    
    // Security validation helpers
    ValidationResult validateForScriptInjection(const std::string& content);
    ValidationResult validateFilePath(const std::string& path);
    ValidationResult validateExternalReferences(const nlohmann::json& json);
    
    // Performance validation helpers
    ValidationResult validateVoiceCount(const nlohmann::json& parameters);
    ValidationResult validateModulationComplexity(const nlohmann::json& parameters);
    ValidationResult validateEffectsChain(const nlohmann::json& parameters);
    
    // Utility methods
    void updateStatistics(const PresetValidationReport& report);
    std::string createValidationSummary(const std::vector<ValidationResult>& results);
    ValidationResult createResult(bool valid, ValidationSeverity severity, 
                                const std::string& message, const std::string& field = "",
                                const std::string& suggestion = "");
    
    // Default validation rules
    void initializeDefaultRules();
    
    // Auto-fix implementations
    std::string fixPresetName(std::string& name);
    std::string fixAuthorName(std::string& author);
    std::string fixCategory(std::string& category);
    std::vector<std::string> fixTags(std::vector<std::string>& tags);
    std::string fixDescription(std::string& description);
};

/**
 * @brief Validation result formatting utilities
 */
namespace ValidationUtils {
    /**
     * @brief Format validation result as string
     */
    std::string formatResult(const ValidationResult& result);
    
    /**
     * @brief Format validation report as string
     */
    std::string formatReport(const PresetValidationReport& report);
    
    /**
     * @brief Convert severity to color code for UI display
     */
    std::string severityToColorCode(ValidationSeverity severity);
    
    /**
     * @brief Convert severity to icon for UI display
     */
    std::string severityToIcon(ValidationSeverity severity);
}

} // namespace AIMusicHardware