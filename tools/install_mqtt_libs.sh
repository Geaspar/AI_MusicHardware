#!/bin/bash
# Script to install Paho MQTT C and C++ libraries in the vendor directory

# Exit on error
set -e

# Get the root directory of the project
PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
VENDOR_DIR="$PROJECT_ROOT/vendor"

# Create vendor directory if it doesn't exist
mkdir -p "$VENDOR_DIR"

# Colors for output
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
RED='\033[0;31m'
NC='\033[0m' # No Color

echo -e "${GREEN}Installing Paho MQTT libraries to $VENDOR_DIR${NC}"

# Function to check if git is available
check_git() {
    if ! command -v git &> /dev/null; then
        echo -e "${RED}ERROR: Git is not installed. Please install Git and try again.${NC}"
        exit 1
    fi
}

# Function to check if CMake is available
check_cmake() {
    if ! command -v cmake &> /dev/null; then
        echo -e "${RED}ERROR: CMake is not installed. Please install CMake and try again.${NC}"
        exit 1
    fi
}

# Install Paho MQTT C library
install_paho_mqtt_c() {
    echo -e "${YELLOW}Installing Paho MQTT C library...${NC}"
    
    # Clone the repository if it doesn't exist
    if [ ! -d "$VENDOR_DIR/paho.mqtt.c" ]; then
        cd "$VENDOR_DIR"
        git clone https://github.com/eclipse/paho.mqtt.c.git
    fi
    
    # Build and install
    cd "$VENDOR_DIR/paho.mqtt.c"
    
    # Create a local install directory
    mkdir -p local_install
    
    # Configure, build and install
    # Create build directory for out-of-source build
    rm -rf build
    mkdir -p build
    cd build

    cmake -DPAHO_WITH_SSL=OFF \
          -DPAHO_BUILD_SHARED=TRUE \
          -DPAHO_BUILD_STATIC=TRUE \
          -DCMAKE_POSITION_INDEPENDENT_CODE=ON \
          -DCMAKE_INSTALL_PREFIX="$VENDOR_DIR/paho.mqtt.c/local_install" \
          ..

    cmake --build .
    cmake --install .
    
    echo -e "${GREEN}Paho MQTT C library installed successfully!${NC}"
}

# Install Paho MQTT C++ library
install_paho_mqtt_cpp() {
    echo -e "${YELLOW}Installing Paho MQTT C++ library...${NC}"

    # Clone the repository if it doesn't exist
    if [ ! -d "$VENDOR_DIR/paho.mqtt.cpp" ]; then
        cd "$VENDOR_DIR"
        git clone https://github.com/eclipse/paho.mqtt.cpp.git
    fi

    # Build and install
    cd "$VENDOR_DIR/paho.mqtt.cpp"

    # Create build directory for out-of-source build
    rm -rf build
    mkdir -p build
    cd build

    # Create a local install directory
    mkdir -p "$VENDOR_DIR/paho.mqtt.cpp/local_install"

    # Configure with path to Paho MQTT C library
    cmake -DPAHO_BUILD_SHARED=TRUE \
          -DPAHO_BUILD_STATIC=TRUE \
          -DCMAKE_POSITION_INDEPENDENT_CODE=ON \
          -Declipse-paho-mqtt-c_DIR="$VENDOR_DIR/paho.mqtt.c/local_install/lib/cmake/eclipse-paho-mqtt-c" \
          -DCMAKE_PREFIX_PATH="$VENDOR_DIR/paho.mqtt.c/local_install" \
          -DCMAKE_INSTALL_PREFIX="$VENDOR_DIR/paho.mqtt.cpp/local_install" \
          ..

    cmake --build .
    cmake --install .

    echo -e "${GREEN}Paho MQTT C++ library installed successfully!${NC}"
}

# Main execution
check_git
check_cmake

install_paho_mqtt_c
install_paho_mqtt_cpp

echo -e "${GREEN}Installation complete!${NC}"
echo -e "${YELLOW}Now you can build the project with:${NC}"
echo -e "  cd \"$PROJECT_ROOT\" && mkdir -p build && cd build && cmake .. && make"