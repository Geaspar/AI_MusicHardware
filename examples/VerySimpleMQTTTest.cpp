#include <iostream>
#include <string>

// Simple program to test MQTT installation guidance
int main() {
    std::cout << "IoT Configuration Manager - Installation Guide" << std::endl;
    std::cout << "==============================================" << std::endl;
    std::cout << "To use the IoT Configuration Manager, you need to install the following:" << std::endl;
    std::cout << std::endl;
    
    std::cout << "1. Paho MQTT C Library:" << std::endl;
    std::cout << "   git clone https://github.com/eclipse/paho.mqtt.c.git" << std::endl;
    std::cout << "   cd paho.mqtt.c" << std::endl;
    std::cout << "   cmake -DPAHO_WITH_SSL=TRUE -DPAHO_BUILD_SHARED=TRUE ." << std::endl;
    std::cout << "   make" << std::endl;
    std::cout << "   sudo make install" << std::endl;
    std::cout << std::endl;
    
    std::cout << "2. Paho MQTT C++ Library:" << std::endl;
    std::cout << "   git clone https://github.com/eclipse/paho.mqtt.cpp.git" << std::endl;
    std::cout << "   cd paho.mqtt.cpp" << std::endl;
    std::cout << "   cmake -DPAHO_BUILD_SHARED=TRUE ." << std::endl;
    std::cout << "   make" << std::endl;
    std::cout << "   sudo make install" << std::endl;
    std::cout << std::endl;
    
    std::cout << "3. Or use our automated script (recommended):" << std::endl;
    std::cout << "   ./tools/install_mqtt_libs.sh" << std::endl;
    std::cout << std::endl;
    
    std::cout << "After installing the libraries, rebuild the project with:" << std::endl;
    std::cout << "   ./build.sh" << std::endl;
    
    return 0;
}