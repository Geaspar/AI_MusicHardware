#pragma once

#include <string>
#include <functional>
#include <stdexcept>
#include <sstream>
#include <vector>
#include <memory>
#include <any>
#include <cmath>

namespace AIMusicHardware {

/**
 * @brief Utility class for IoT parameter type conversion
 * 
 * This class provides utilities for converting between IoT message payloads
 * and parameter values of different types.
 */
class IoTParameterConverter {
public:
    /**
     * @brief Sensor value types
     */
    enum class SensorType {
        TEMPERATURE,    // Temperature in Celsius
        HUMIDITY,       // Humidity percentage (0-100)
        PRESSURE,       // Pressure in hPa
        LIGHT,          // Light level in lux
        DISTANCE,       // Distance in cm
        ACCELERATION,   // Acceleration in m/s²
        GYROSCOPE,      // Gyroscope in degrees/second
        MOTION,         // Motion detection (binary)
        SOUND,          // Sound level in dB
        AIR_QUALITY,    // Air quality index
        GPS,            // GPS coordinates
        BUTTON,         // Button press/release
        ANALOG,         // Generic analog reading
        DIGITAL,        // Generic digital reading
        JSON,           // JSON object
        CUSTOM          // Custom format
    };
    
    /**
     * @brief Get a converter function for a sensor type
     * 
     * Returns a function that converts a string payload to a float value
     * appropriate for the given sensor type.
     * 
     * @param type The sensor type
     * @param minValue Minimum expected sensor value
     * @param maxValue Maximum expected sensor value
     * @param normalized Whether to normalize the output to 0.0-1.0 range
     * @return Function that converts string payload to float
     */
    static std::function<float(const std::string&)> getConverter(
        SensorType type, 
        float minValue = 0.0f, 
        float maxValue = 1.0f,
        bool normalized = true
    );
    
    /**
     * @brief Create a custom converter
     * 
     * @param converter Function that converts string to float
     * @param minValue Minimum expected value
     * @param maxValue Maximum expected value
     * @param normalized Whether to normalize the output to 0.0-1.0 range
     * @return Function that handles conversion and normalization
     */
    static std::function<float(const std::string&)> createCustomConverter(
        std::function<float(const std::string&)> converter,
        float minValue = 0.0f,
        float maxValue = 1.0f,
        bool normalized = true
    );
    
    /**
     * @brief Parse JSON payload
     * 
     * Extracts a specific value from a JSON payload.
     * 
     * @param payload The JSON string payload
     * @param key The key to extract
     * @return std::string The extracted value as a string
     */
    static std::string parseJsonValue(const std::string& payload, const std::string& key);
    
    /**
     * @brief Parse CSV payload
     * 
     * Extracts a specific value from a CSV payload.
     * 
     * @param payload The CSV string payload
     * @param index The index to extract (0-based)
     * @return std::string The extracted value as a string
     */
    static std::string parseCsvValue(const std::string& payload, int index);
    
    /**
     * @brief Normalize a value to 0.0-1.0 range
     * 
     * @param value The value to normalize
     * @param minValue Minimum expected value
     * @param maxValue Maximum expected value
     * @return float The normalized value
     */
    static float normalizeValue(float value, float minValue, float maxValue);
    
private:
    // Converter implementations
    static float convertTemperature(const std::string& payload);
    static float convertHumidity(const std::string& payload);
    static float convertPressure(const std::string& payload);
    static float convertLight(const std::string& payload);
    static float convertDistance(const std::string& payload);
    static float convertAcceleration(const std::string& payload);
    static float convertGyroscope(const std::string& payload);
    static float convertMotion(const std::string& payload);
    static float convertSound(const std::string& payload);
    static float convertAirQuality(const std::string& payload);
    static float convertGps(const std::string& payload);
    static float convertButton(const std::string& payload);
    static float convertAnalog(const std::string& payload);
    static float convertDigital(const std::string& payload);
};

/**
 * @brief Utility class for IoT parameter mappings
 * 
 * This class provides common parameter mappings for different sensor types.
 */
class IoTParameterMappings {
public:
    /**
     * @brief Mapping mode for parameters
     */
    enum class MappingMode {
        LINEAR,           // Direct linear mapping
        EXPONENTIAL,      // Exponential response
        LOGARITHMIC,      // Logarithmic response
        THRESHOLD,        // Binary threshold (on/off)
        INVERSE,          // Inverted response
        TOGGLE            // Toggle on message
    };
    
    /**
     * @brief Create a parameter value mapper
     * 
     * @param mode The mapping mode
     * @param threshold Threshold value for threshold mode
     * @param exponent Exponent for exponential mode
     * @return Function that maps sensor values to parameter values
     */
    static std::function<float(float)> createMapping(
        MappingMode mode,
        float threshold = 0.5f,
        float exponent = 2.0f
    );
    
    /**
     * @brief Chain multiple conversions
     * 
     * Creates a converter that first parses the payload into a float,
     * then applies a mapping function to the result.
     * 
     * @param converter Function that converts string payload to float
     * @param mapping Function that maps the float value
     * @return Combined conversion function
     */
    static std::function<float(const std::string&)> chainConversions(
        std::function<float(const std::string&)> converter,
        std::function<float(float)> mapping
    );
};

} // namespace AIMusicHardware