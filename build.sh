#!/bin/bash
set -e  # Exit on error

# Print message with color
print_message() {
    local color=$1
    local message=$2
    
    case $color in
        red)    echo -e "\033[0;31m$message\033[0m" ;;
        green)  echo -e "\033[0;32m$message\033[0m" ;;
        yellow) echo -e "\033[0;33m$message\033[0m" ;;
        blue)   echo -e "\033[0;34m$message\033[0m" ;;
        cyan)   echo -e "\033[0;36m$message\033[0m" ;;
        *)      echo "$message" ;;
    esac
}

print_message blue "=== AIMusicHardware Build System ==="
print_message blue "=================================="

# Function to check if RtAudio is installed at common paths
check_rtaudio_installed() {
    # Check common include paths
    local include_paths=(
        "/usr/include/RtAudio.h"
        "/usr/local/include/RtAudio.h"
        "/opt/homebrew/include/RtAudio.h"
        "/usr/include/rtaudio/RtAudio.h"
        "/usr/local/include/rtaudio/RtAudio.h"
        "/opt/homebrew/include/rtaudio/RtAudio.h"
    )
    
    # Check if any of these files exist
    for path in "${include_paths[@]}"; do
        if [ -f "$path" ]; then
            return 0  # Found RtAudio
        fi
    done
    
    return 1  # RtAudio not found
}

# Try to install RtAudio if on macOS, but don't fail if it doesn't work
rtaudio_available=false
if [[ "$(uname)" == "Darwin" ]]; then
    if check_rtaudio_installed; then
        print_message green "RtAudio is already installed."
        rtaudio_available=true
    else
        # Check if brew is installed
        if command -v brew &> /dev/null; then
            print_message yellow "RtAudio not found. Attempting to install with Homebrew..."
            if brew install rtaudio 2>/dev/null; then
                print_message green "RtAudio installed successfully!"
                rtaudio_available=true
            else
                print_message yellow "Could not install RtAudio. Will use file-based audio test only."
            fi
        else
            print_message yellow "Homebrew not found. Will use file-based audio test only."
        fi
    fi
fi

# Check for MQTT libraries
mqtt_available=false
if [ -d "vendor/paho.mqtt.c/local_install" ] && [ -d "vendor/paho.mqtt.cpp/local_install" ]; then
    print_message green "Found MQTT libraries in vendor directory."
    mqtt_available=true
else
    print_message yellow "MQTT libraries not found in vendor directory."
    print_message yellow "IoT functionality will be limited."
    print_message yellow "To enable full IoT functionality, run: ./tools/install_mqtt_libs.sh"
fi

# Clean build directory first to avoid any cached issues
print_message blue "Cleaning previous build..."
rm -rf build
mkdir -p build
cd build

# Configure the project
print_message blue "Configuring project with CMake..."
cmake .. 

if [ $? -ne 0 ]; then
    print_message red "CMake configuration failed!"
    exit 1
fi

# Build
print_message blue "Building project..."
cmake --build .

if [ $? -ne 0 ]; then
    print_message red "Build failed!"
    exit 1
fi

print_message green "Build completed successfully!"

# Check which test programs were built
if [ -f "./bin/SimplePresetManagerDemo" ]; then
    has_preset_demo=true
else
    has_preset_demo=false
fi

if [ -f "./bin/TestMpeVoiceManager" ]; then
    has_mpe_test=true
else
    has_mpe_test=false
fi

if [ -f "./bin/TestAudio" ]; then
    has_audio_test=true
else
    has_audio_test=false
fi

if [ -f "./bin/TestMQTTIntegration" ]; then
    has_mqtt_test=true
else
    has_mqtt_test=false
fi

if [ -f "./bin/IoTConfigManagerDemo" ]; then
    has_iot_demo=true
else
    has_iot_demo=false
fi

# Display instructions based on available programs
print_message blue "==========================================="
print_message blue "Available Test Applications:"
print_message blue "==========================================="

if [ "$has_preset_demo" = true ]; then
    print_message cyan "Preset Management Demo:"
    print_message yellow "          ./bin/SimplePresetManagerDemo"
    print_message cyan "          Test the preset management system"
    echo ""
fi

if [ "$has_mpe_test" = true ]; then
    print_message cyan "MPE Voice Manager Demo:"
    print_message yellow "          ./bin/TestMpeVoiceManager"
    print_message cyan "          Test the MPE-aware voice manager with expression control"
    echo ""
fi

if [ "$has_audio_test" = true ]; then
    print_message cyan "Real-Time Audio Test:"
    print_message yellow "          ./bin/TestAudio"
    print_message cyan "          Interactive demo of sound generation capabilities"
    echo ""

    print_message blue "Tip: If you're not hearing sound, check that:"
    print_message blue "  1. Your system volume is turned up"
    print_message blue "  2. The correct audio output device is selected"
    print_message blue "  3. No other application is using the audio device"
else
    print_message yellow "Real-time audio test (TestAudio) was not built due to missing RtAudio."
fi

if [ "$has_mqtt_test" = true ]; then
    if [ "$mqtt_available" = true ]; then
        print_message cyan "MQTT Integration Test:"
        print_message yellow "          ./bin/TestMQTTIntegration"
        print_message cyan "          Test basic MQTT connectivity and message handling"
        echo ""
    else
        print_message yellow "MQTT Integration Test (TestMQTTIntegration) built with limited functionality."
        print_message yellow "For full functionality, install MQTT libraries with ./tools/install_mqtt_libs.sh"
        echo ""
    fi
fi

if [ "$has_iot_demo" = true ]; then
    if [ "$mqtt_available" = true ]; then
        print_message cyan "IoT Configuration Manager Demo:"
        print_message yellow "          ./bin/IoTConfigManagerDemo"
        print_message cyan "          Interactive demo for IoT device discovery and configuration"
        echo ""
    else
        print_message yellow "IoT Configuration Manager Demo (IoTConfigManagerDemo) built with limited functionality."
        print_message yellow "For full functionality, install MQTT libraries with ./tools/install_mqtt_libs.sh"
        echo ""
    fi
fi

print_message blue "==========================================="

# Show IoT-specific instructions if MQTT is not available but IoT demos are
if [ "$mqtt_available" = false ] && ([ "$has_mqtt_test" = true ] || [ "$has_iot_demo" = true ]); then
    print_message blue "==========================================="
    print_message blue "To enable full IoT functionality:"
    print_message yellow "1. Install MQTT libraries:     ./tools/install_mqtt_libs.sh"
    print_message yellow "2. Rebuild the project:        ./build.sh"
    print_message blue "==========================================="
fi