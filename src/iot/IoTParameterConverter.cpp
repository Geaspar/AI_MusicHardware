#include "../../include/iot/IoTParameterTypes.h"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <regex>
#include <sstream>

namespace AIMusicHardware {

std::function<float(const std::string&)> IoTParameterConverter::getConverter(
    SensorType type, float minValue, float maxValue, bool normalized) {
    
    std::function<float(const std::string&)> baseConverter;
    
    // Select base converter based on sensor type
    switch(type) {
        case SensorType::TEMPERATURE:
            baseConverter = convertTemperature;
            break;
        case SensorType::HUMIDITY:
            baseConverter = convertHumidity;
            break;
        case SensorType::PRESSURE:
            baseConverter = convertPressure;
            break;
        case SensorType::LIGHT:
            baseConverter = convertLight;
            break;
        case SensorType::DISTANCE:
            baseConverter = convertDistance;
            break;
        case SensorType::ACCELERATION:
            baseConverter = convertAcceleration;
            break;
        case SensorType::GYROSCOPE:
            baseConverter = convertGyroscope;
            break;
        case SensorType::MOTION:
            baseConverter = convertMotion;
            break;
        case SensorType::SOUND:
            baseConverter = convertSound;
            break;
        case SensorType::AIR_QUALITY:
            baseConverter = convertAirQuality;
            break;
        case SensorType::GPS:
            baseConverter = convertGps;
            break;
        case SensorType::BUTTON:
            baseConverter = convertButton;
            break;
        case SensorType::ANALOG:
            baseConverter = convertAnalog;
            break;
        case SensorType::DIGITAL:
            baseConverter = convertDigital;
            break;
        case SensorType::JSON:
            // JSON requires specific key, so we'll use a default handler
            baseConverter = [](const std::string& payload) {
                try {
                    // Simple numeric extraction from JSON
                    std::regex valueRegex("\"value\"\\s*:\\s*([0-9.]+)");
                    std::smatch match;
                    if (std::regex_search(payload, match, valueRegex) && match.size() > 1) {
                        return std::stof(match[1].str());
                    }
                } catch (...) {}
                
                return 0.0f;
            };
            break;
        case SensorType::CUSTOM:
            // For custom type, use a simple float parser as a default
            baseConverter = [](const std::string& payload) {
                try {
                    return std::stof(payload);
                } catch (...) {
                    return 0.0f;
                }
            };
            break;
    }
    
    // If normalization is requested, wrap the converter
    if (normalized) {
        return [baseConverter, minValue, maxValue](const std::string& payload) {
            float value = baseConverter(payload);
            return normalizeValue(value, minValue, maxValue);
        };
    }
    
    // Otherwise return the base converter
    return baseConverter;
}

std::function<float(const std::string&)> IoTParameterConverter::createCustomConverter(
    std::function<float(const std::string&)> converter,
    float minValue, float maxValue, bool normalized) {
    
    if (normalized) {
        return [converter, minValue, maxValue](const std::string& payload) {
            float value = converter(payload);
            return normalizeValue(value, minValue, maxValue);
        };
    }
    
    return converter;
}

std::string IoTParameterConverter::parseJsonValue(const std::string& payload, const std::string& key) {
    // Simple regex-based JSON parser for specific key
    std::string pattern = "\"" + key + "\"\\s*:\\s*([^,}\\]]+)";
    std::regex keyRegex(pattern);
    std::smatch match;
    
    if (std::regex_search(payload, match, keyRegex) && match.size() > 1) {
        std::string value = match[1].str();
        
        // Clean up the value
        value = std::regex_replace(value, std::regex("^\\s+|\\s+$"), ""); // trim
        value = std::regex_replace(value, std::regex("^\""), ""); // remove leading quote
        value = std::regex_replace(value, std::regex("\"$"), ""); // remove trailing quote
        
        return value;
    }
    
    return "";
}

std::string IoTParameterConverter::parseCsvValue(const std::string& payload, int index) {
    std::stringstream ss(payload);
    std::string item;
    int currentIndex = 0;
    
    while (std::getline(ss, item, ',')) {
        if (currentIndex == index) {
            // Found the requested index
            return item;
        }
        currentIndex++;
    }
    
    return ""; // Index not found
}

float IoTParameterConverter::normalizeValue(float value, float minValue, float maxValue) {
    // Prevent division by zero
    if (std::abs(maxValue - minValue) < 0.0001f) {
        return 0.0f;
    }
    
    // Normalize to 0.0-1.0 range
    float normalized = (value - minValue) / (maxValue - minValue);
    
    // Clamp to valid range
    return std::clamp(normalized, 0.0f, 1.0f);
}

// Converter implementations
float IoTParameterConverter::convertTemperature(const std::string& payload) {
    try {
        // Extract numeric value
        std::regex tempRegex("([+-]?[0-9.]+)");
        std::smatch match;
        if (std::regex_search(payload, match, tempRegex)) {
            return std::stof(match[1].str());
        }
    } catch (...) {}
    
    return 20.0f; // Default room temperature
}

float IoTParameterConverter::convertHumidity(const std::string& payload) {
    try {
        // Extract numeric value
        std::regex humidityRegex("([0-9.]+)");
        std::smatch match;
        if (std::regex_search(payload, match, humidityRegex)) {
            return std::stof(match[1].str());
        }
    } catch (...) {}
    
    return 50.0f; // Default 50% humidity
}

float IoTParameterConverter::convertPressure(const std::string& payload) {
    try {
        // Extract numeric value
        std::regex pressureRegex("([0-9.]+)");
        std::smatch match;
        if (std::regex_search(payload, match, pressureRegex)) {
            return std::stof(match[1].str());
        }
    } catch (...) {}
    
    return 1013.25f; // Default sea level pressure
}

float IoTParameterConverter::convertLight(const std::string& payload) {
    try {
        // Extract numeric value
        std::regex lightRegex("([0-9.]+)");
        std::smatch match;
        if (std::regex_search(payload, match, lightRegex)) {
            return std::stof(match[1].str());
        }
    } catch (...) {}
    
    return 500.0f; // Default moderate indoor light
}

float IoTParameterConverter::convertDistance(const std::string& payload) {
    try {
        // Extract numeric value
        std::regex distanceRegex("([0-9.]+)");
        std::smatch match;
        if (std::regex_search(payload, match, distanceRegex)) {
            return std::stof(match[1].str());
        }
    } catch (...) {}
    
    return 100.0f; // Default 100cm
}

float IoTParameterConverter::convertAcceleration(const std::string& payload) {
    try {
        // Check for JSON format with x,y,z components
        if (payload.find('{') != std::string::npos) {
            float x = std::stof(parseJsonValue(payload, "x"));
            float y = std::stof(parseJsonValue(payload, "y"));
            float z = std::stof(parseJsonValue(payload, "z"));
            
            // Return magnitude of acceleration vector
            return std::sqrt(x*x + y*y + z*z);
        }
        
        // Check for CSV format x,y,z
        if (payload.find(',') != std::string::npos) {
            float x = std::stof(parseCsvValue(payload, 0));
            float y = std::stof(parseCsvValue(payload, 1));
            float z = std::stof(parseCsvValue(payload, 2));
            
            // Return magnitude of acceleration vector
            return std::sqrt(x*x + y*y + z*z);
        }
        
        // Simple numeric value
        return std::stof(payload);
    } catch (...) {}
    
    return 0.0f; // Default no acceleration
}

float IoTParameterConverter::convertGyroscope(const std::string& payload) {
    try {
        // Check for JSON format with x,y,z components
        if (payload.find('{') != std::string::npos) {
            float x = std::stof(parseJsonValue(payload, "x"));
            float y = std::stof(parseJsonValue(payload, "y"));
            float z = std::stof(parseJsonValue(payload, "z"));
            
            // Return magnitude of rotation vector
            return std::sqrt(x*x + y*y + z*z);
        }
        
        // Check for CSV format x,y,z
        if (payload.find(',') != std::string::npos) {
            float x = std::stof(parseCsvValue(payload, 0));
            float y = std::stof(parseCsvValue(payload, 1));
            float z = std::stof(parseCsvValue(payload, 2));
            
            // Return magnitude of rotation vector
            return std::sqrt(x*x + y*y + z*z);
        }
        
        // Simple numeric value
        return std::stof(payload);
    } catch (...) {}
    
    return 0.0f; // Default no rotation
}

float IoTParameterConverter::convertMotion(const std::string& payload) {
    // Convert various motion detection formats to binary (0/1)
    if (payload == "1" || payload == "true" || payload == "True" || 
        payload == "detected" || payload == "motion" || payload == "on") {
        return 1.0f;
    }
    
    return 0.0f; // Default no motion
}

float IoTParameterConverter::convertSound(const std::string& payload) {
    try {
        // Extract numeric value
        std::regex soundRegex("([0-9.]+)");
        std::smatch match;
        if (std::regex_search(payload, match, soundRegex)) {
            return std::stof(match[1].str());
        }
    } catch (...) {}
    
    return 0.0f; // Default silence
}

float IoTParameterConverter::convertAirQuality(const std::string& payload) {
    try {
        // Extract numeric value
        std::regex aqiRegex("([0-9.]+)");
        std::smatch match;
        if (std::regex_search(payload, match, aqiRegex)) {
            return std::stof(match[1].str());
        }
    } catch (...) {}
    
    return 50.0f; // Default moderate air quality
}

float IoTParameterConverter::convertGps(const std::string& payload) {
    try {
        // Extract latitude and longitude
        if (payload.find(',') != std::string::npos) {
            // Format: "latitude,longitude"
            float latitude = std::stof(parseCsvValue(payload, 0));
            float longitude = std::stof(parseCsvValue(payload, 1));
            
            // Return average (not very useful but simple)
            return (latitude + longitude) / 2.0f;
        }
        else if (payload.find('{') != std::string::npos) {
            // JSON format
            float latitude = std::stof(parseJsonValue(payload, "lat"));
            float longitude = std::stof(parseJsonValue(payload, "lon"));
            
            // Return average (not very useful but simple)
            return (latitude + longitude) / 2.0f;
        }
    } catch (...) {}
    
    return 0.0f; // Default origin
}

float IoTParameterConverter::convertButton(const std::string& payload) {
    // Convert various button state formats to binary (0/1)
    if (payload == "1" || payload == "true" || payload == "True" || 
        payload == "pressed" || payload == "on" || payload == "down") {
        return 1.0f;
    }
    
    return 0.0f; // Default not pressed
}

float IoTParameterConverter::convertAnalog(const std::string& payload) {
    try {
        // Simple numeric value
        return std::stof(payload);
    } catch (...) {}
    
    return 0.0f; // Default zero
}

float IoTParameterConverter::convertDigital(const std::string& payload) {
    // Convert various digital state formats to binary (0/1)
    if (payload == "1" || payload == "true" || payload == "True" || 
        payload == "high" || payload == "on") {
        return 1.0f;
    }
    
    return 0.0f; // Default low
}

// IoTParameterMappings implementation
std::function<float(float)> IoTParameterMappings::createMapping(
    MappingMode mode, float threshold, float exponent) {
    
    switch(mode) {
        case MappingMode::LINEAR:
            return [](float value) {
                return value;
            };
            
        case MappingMode::EXPONENTIAL:
            return [exponent](float value) {
                return std::pow(value, exponent);
            };
            
        case MappingMode::LOGARITHMIC:
            return [](float value) {
                // Avoid log(0)
                if (value < 0.001f) return 0.0f;
                
                // Log mapping scaled to 0-1 range (approximately)
                // ln(0.001) ≈ -6.9, ln(1) = 0
                float result = (std::log(value) + 6.9f) / 6.9f;
                return std::clamp(result, 0.0f, 1.0f);
            };
            
        case MappingMode::THRESHOLD:
            return [threshold](float value) {
                return (value >= threshold) ? 1.0f : 0.0f;
            };
            
        case MappingMode::INVERSE:
            return [](float value) {
                return 1.0f - value;
            };
            
        case MappingMode::TOGGLE:
            return [](float value) {
                // Any non-zero value becomes 1.0
                return (value > 0.0f) ? 1.0f : 0.0f;
            };
            
        default:
            return [](float value) { return value; };
    }
}

std::function<float(const std::string&)> IoTParameterMappings::chainConversions(
    std::function<float(const std::string&)> converter,
    std::function<float(float)> mapping) {
    
    return [converter, mapping](const std::string& payload) {
        float value = converter(payload);
        return mapping(value);
    };
}

} // namespace AIMusicHardware