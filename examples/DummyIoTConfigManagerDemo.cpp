#include <iostream>
#include <string>

int main() {
    std::cout << "IoT Configuration Manager Demo - DUMMY MODE" << std::endl;
    std::cout << "============================================" << std::endl;
    std::cout << "Sorry, this demo requires the Paho MQTT library which was not found." << std::endl;
    std::cout << "Please install the Paho MQTT library and rebuild to use this demo." << std::endl;
    std::cout << std::endl;
    std::cout << "Installation instructions:" << std::endl;
    std::cout << "  1. Install the Paho MQTT C library:" << std::endl;
    std::cout << "     git clone https://github.com/eclipse/paho.mqtt.c.git" << std::endl;
    std::cout << "     cd paho.mqtt.c" << std::endl;
    std::cout << "     cmake -DPAHO_WITH_SSL=TRUE -DPAHO_BUILD_SHARED=TRUE ." << std::endl;
    std::cout << "     make" << std::endl;
    std::cout << "     sudo make install" << std::endl;
    std::cout << std::endl;
    std::cout << "  2. Install the Paho MQTT C++ library:" << std::endl;
    std::cout << "     git clone https://github.com/eclipse/paho.mqtt.cpp.git" << std::endl;
    std::cout << "     cd paho.mqtt.cpp" << std::endl;
    std::cout << "     cmake -DPAHO_BUILD_SHARED=TRUE ." << std::endl;
    std::cout << "     make" << std::endl;
    std::cout << "     sudo make install" << std::endl;
    std::cout << std::endl;
    std::cout << "  3. Rebuild this project." << std::endl;
    std::cout << std::endl;
    
    std::cout << "Press Enter to exit...";
    std::string dummy;
    std::getline(std::cin, dummy);
    
    return 0;
}