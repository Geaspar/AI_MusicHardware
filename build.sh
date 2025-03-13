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

print_message blue "=== AIMusicHardware Audio Test Build ==="
print_message blue "======================================"

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
if [ -f "./bin/SimpleTest" ]; then
    has_simple_test=true
else
    has_simple_test=false
fi

if [ -f "./bin/TestAudio" ]; then
    has_audio_test=true
else
    has_audio_test=false
fi

# Display instructions based on available programs
print_message blue "==========================================="
print_message blue "Audio Test Options:"
print_message blue "==========================================="

if [ "$has_simple_test" = true ]; then
    print_message cyan "Option 1: Generate WAV files to test audio synthesis"
    print_message yellow "          ./bin/SimpleTest"
    print_message cyan "          This will create WAV files in the 'output' directory"
    print_message cyan "          You can play these files with any audio player"
    echo ""
fi

if [ "$has_audio_test" = true ]; then
    print_message cyan "Option 2: Test real-time audio playback"
    print_message yellow "          ./bin/TestAudio"
    print_message cyan "          This will play audio through your speakers"
    echo ""
    
    print_message blue "Tip: If you're not hearing sound, check that:"
    print_message blue "  1. Your system volume is turned up"
    print_message blue "  2. The correct audio output device is selected"
    print_message blue "  3. No other application is using the audio device"
else
    print_message yellow "Real-time audio test (TestAudio) was not built due to missing RtAudio."
    print_message yellow "You can only use the WAV file generation test (SimpleTest)."
fi

print_message blue "==========================================="