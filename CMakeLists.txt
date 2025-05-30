cmake_minimum_required(VERSION 3.14)
project(AIMusicHardware VERSION 0.1.0)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Default build type if not specified
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
endif()

# Output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Set module path to find our custom FindRtAudio.cmake
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")

# Find dependencies
find_package(Threads REQUIRED)
find_package(RtAudio QUIET)
find_package(RtMidi QUIET)

# Find nlohmann/json
find_package(nlohmann_json QUIET)
if(NOT nlohmann_json_FOUND)
    # If not found, include from vendor
    include_directories(${CMAKE_CURRENT_SOURCE_DIR}/vendor/json/include)
    message(STATUS "Using bundled nlohmann/json")

    # Create directory if it doesn't exist
    if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/vendor/json/include")
        file(MAKE_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/vendor/json/include")
        message(STATUS "Created vendor/json directory")
    endif()
else()
    message(STATUS "Found system nlohmann/json")
endif()

# Find Paho MQTT C++ library
# First check if we can use a simplified approach for testing
set(MQTT_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/vendor)
if(EXISTS "${MQTT_SOURCE_DIR}/paho.mqtt.c" AND EXISTS "${MQTT_SOURCE_DIR}/paho.mqtt.cpp")
    message(STATUS "Found Paho MQTT libraries in vendor directory")

    # Include directories directly from the source
    include_directories(
        ${MQTT_SOURCE_DIR}/paho.mqtt.c/src
        ${MQTT_SOURCE_DIR}/paho.mqtt.cpp/src
    )

    # Define that we have the MQTT libraries
    add_definitions(-DHAVE_PAHO_MQTT)
    set(PAHO_MQTT_CPP_FOUND TRUE)

    # Mock libraries for linking
    set(PAHO_MQTT_CPP_LIBRARY "")
    set(PAHO_MQTT_C_LIBRARY "")
else()
    # Set paths to our local installations
    set(PAHO_MQTT_C_DIR ${CMAKE_CURRENT_SOURCE_DIR}/vendor/paho.mqtt.c/local_install)
    set(PAHO_MQTT_CPP_DIR ${CMAKE_CURRENT_SOURCE_DIR}/vendor/paho.mqtt.cpp/local_install)

    # First try to find the C library
    find_package(eclipse-paho-mqtt-c QUIET
        PATHS
        ${PAHO_MQTT_C_DIR}/lib/cmake/eclipse-paho-mqtt-c
        NO_DEFAULT_PATH
    )

    # Then try to find the C++ library with our local path hints
    find_package(PahoMqttCpp QUIET
        PATHS
        ${PAHO_MQTT_CPP_DIR}/lib/cmake/PahoMqttCpp
        NO_DEFAULT_PATH
    )

    # If not found via package config, try traditional methods
    if(NOT PahoMqttCpp_FOUND)
        # First, find the C library
        find_library(PAHO_MQTT_C_LIBRARY paho-mqtt3a
            PATHS
            ${PAHO_MQTT_C_DIR}/lib
            NO_DEFAULT_PATH
        )

        # Try to find in our local installation first, then fall back to common locations
        find_path(PAHO_MQTT_CPP_INCLUDE_DIR mqtt/async_client.h
            PATHS
            ${PAHO_MQTT_CPP_DIR}/include
            ${CMAKE_CURRENT_SOURCE_DIR}/vendor/paho.mqtt.cpp/include
            /usr/include
            /usr/local/include
            /opt/local/include
            /opt/include
        )

        find_library(PAHO_MQTT_CPP_LIBRARY paho-mqttpp3
            PATHS
            ${PAHO_MQTT_CPP_DIR}/lib
            ${CMAKE_CURRENT_SOURCE_DIR}/vendor/paho.mqtt.cpp/lib
            /usr/lib
            /usr/local/lib
            /opt/local/lib
            /opt/lib
        )

        if(PAHO_MQTT_CPP_INCLUDE_DIR AND PAHO_MQTT_CPP_LIBRARY AND PAHO_MQTT_C_LIBRARY)
            set(PAHO_MQTT_CPP_FOUND TRUE)
            message(STATUS "Found Paho MQTT C++ library: ${PAHO_MQTT_CPP_LIBRARY}")
            message(STATUS "Found Paho MQTT C library: ${PAHO_MQTT_C_LIBRARY}")
            include_directories(${PAHO_MQTT_CPP_INCLUDE_DIR})
            add_definitions(-DHAVE_PAHO_MQTT)
        else()
            message(WARNING "Paho MQTT C++ library not found. IoT functionality will be limited.")
        endif()
    else()
        message(STATUS "Found Paho MQTT C++ package: ${PahoMqttCpp_DIR}")
        # Add both the C and C++ include directories
        include_directories(
            ${PAHO_MQTT_CPP_DIR}/include
            ${PAHO_MQTT_C_DIR}/include
        )
        set(PAHO_MQTT_CPP_LIBRARY ${PahoMqttCpp_LIBRARIES})
        add_definitions(-DHAVE_PAHO_MQTT)
    endif()
endif()

# Define basic source files
set(EFFECTS_SOURCES
    src/effects/EffectProcessor.cpp
    src/effects/BassBoost.cpp
    src/effects/BitCrusher.cpp
    src/effects/Compressor.cpp
    src/effects/Delay.cpp
    src/effects/Distortion.cpp
    src/effects/EQ.cpp
    src/effects/Filter.cpp
    src/effects/Modulation.cpp
    src/effects/Phaser.cpp
    src/effects/Reverb.cpp
    src/effects/Saturation.cpp
    src/effects/ReorderableEffectsChain.cpp
    src/effects/MidiEffectControl.cpp
    # Advanced filter system
    src/effects/AdvancedFilter.cpp
    src/effects/LadderFilter.cpp
    src/effects/CombFilter.cpp
    src/effects/FormantFilter.cpp
)

set(BASIC_AUDIO_SOURCES
    src/audio/AudioEngine.cpp
    src/audio/Synthesizer.cpp
    src/hardware/HardwareInterface.cpp
    src/midi/MidiInterface.cpp
    src/midi/MidiManager.cpp
    src/midi/MultiTimbralMidiRouter.cpp
    src/midi/MpeConfiguration.cpp
    src/midi/MpeChannelAllocator.cpp
    ${EFFECTS_SOURCES}
)

set(SEQUENCER_SOURCES
    src/sequencer/Sequencer.cpp
    src/sequencer/MidiFile.cpp
    src/sequencer/AdaptiveSequencer.cpp
)

set(PRESET_SOURCES
    src/ui/presets/PresetManager.cpp
)

# New modular synthesis framework sources
set(SYNTHESIS_SOURCES
    src/synthesis/framework/processor.cpp
    src/synthesis/wavetable/wavetable.cpp
    src/synthesis/wavetable/oscillator_stack.cpp
    src/synthesis/modulators/envelope.cpp
    src/synthesis/modulators/modulation_matrix.cpp
    src/synthesis/voice/voice_manager.cpp
    src/synthesis/voice/MpeVoice.cpp
    src/synthesis/voice/MpeAwareVoiceManager.cpp
    src/synthesis/voice/stacked_voice.cpp
    src/synthesis/voice/stacked_voice_manager.cpp
)

# Multi-Timbral sources
set(MULTITIMBRAL_SOURCES
    src/synthesis/multitimbral/MultiTimbralEngine.cpp
    src/synthesis/multitimbral/ChannelSynthesizer.cpp
)

# IoT sources
if(PAHO_MQTT_CPP_FOUND)
    set(IOT_SOURCES
        src/iot/MQTTInterface.cpp
        src/iot/IoTParameterConverter.cpp
        src/iot/IoTEventAdapter.cpp
        src/iot/IoTDevice.cpp
        src/iot/IoTConfigManager.cpp
    )
else()
    set(IOT_SOURCES
        src/iot/IoTParameterConverter.cpp
        src/iot/IoTDevice.cpp
    )
    # Disable MQTT for now - we'll use dummy implementations
    add_definitions(-DDISABLE_MQTT)
endif()

# Parameter System sources
set(PARAMETER_SYSTEM_SOURCES
    src/ui/ParameterManager.cpp
    src/ui/parameters/ParameterManager.cpp
)

# Event System sources
set(EVENT_SYSTEM_SOURCES
    src/events/Event.cpp
    src/events/EventBus.cpp
)

# Preset Management System sources
set(PRESET_SYSTEM_SOURCES
    src/ui/presets/PresetInfo.cpp
    src/ui/presets/PresetDatabase.cpp
    src/ui/presets/PresetManager.cpp
    src/ui/presets/PresetBrowserUI.cpp
)

# AI and Smart Features sources
set(AI_SMART_FEATURES_SOURCES
    src/ai/PresetMLAnalyzer.cpp
    src/ai/PresetRecommendationEngine.cpp
    src/ai/SmartCollectionManager.cpp
)

# UI Core sources
set(UI_CORE_SOURCES
    src/ui/UserInterface.cpp
    src/ui/UIComponents.cpp
    src/ui/UIContext.cpp
    src/ui/DisplayManager.cpp
    src/ui/Font.cpp
    src/ui/ParameterBridge.cpp
    src/ui/ParameterUpdateQueue.cpp
    src/ui/SynthKnob.cpp
    src/ui/PresetBrowserUIComponent.cpp
    src/ui/VisualizationComponents.cpp
)

# Set up RtAudio if found
if(RTAUDIO_FOUND)
    message(STATUS "Found RtAudio: ${RTAUDIO_INCLUDE_DIRS}")
    include_directories(${RTAUDIO_INCLUDE_DIRS})
    set(AUDIO_LIBS ${RTAUDIO_LIBRARIES})
    add_definitions(-DHAVE_RTAUDIO)
else()
    message(WARNING "RtAudio not found. Using dummy implementation.")
    set(AUDIO_LIBS "")
endif()

# Set up RtMidi if found
if(RTMIDI_FOUND)
    message(STATUS "Found RtMidi: ${RTMIDI_INCLUDE_DIRS}")
    include_directories(${RTMIDI_INCLUDE_DIRS})
    list(APPEND AUDIO_LIBS ${RTMIDI_LIBRARIES})
    add_definitions(-DHAVE_RTMIDI)
else()
    message(WARNING "RtMidi not found. Using dummy implementation.")
endif()

# Platform-specific settings
if(APPLE)
    # Core Audio frameworks
    list(APPEND AUDIO_LIBS
        "-framework CoreAudio"
        "-framework CoreFoundation"
    )
endif()

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Create core library
add_library(AIMusicCore STATIC
    ${BASIC_AUDIO_SOURCES}
    ${SEQUENCER_SOURCES}
    ${SYNTHESIS_SOURCES}
    ${MULTITIMBRAL_SOURCES}
    ${IOT_SOURCES}
    ${PARAMETER_SYSTEM_SOURCES}
    ${EVENT_SYSTEM_SOURCES}
    ${PRESET_SYSTEM_SOURCES}
    ${AI_SMART_FEATURES_SOURCES}
    ${UI_CORE_SOURCES}
)

# Add IoT library link if found
if(PAHO_MQTT_CPP_FOUND)
    target_link_libraries(AIMusicCore PRIVATE
        Threads::Threads
        ${AUDIO_LIBS}
        ${PAHO_MQTT_CPP_LIBRARY}
        ${PAHO_MQTT_C_LIBRARY} # Use our found C library
    )
else()
    target_link_libraries(AIMusicCore PRIVATE
        Threads::Threads
        ${AUDIO_LIBS}
    )
endif()

# Create minimal preset library (without UI dependencies)
add_library(PresetLib STATIC
    ${PRESET_SOURCES}
)

target_link_libraries(PresetLib PRIVATE
    AIMusicCore
)

# Simple Preset Manager Demo (no UI or audio requirements)
add_executable(SimplePresetManagerDemo examples/SimplePresetManagerDemo.cpp)
target_link_libraries(SimplePresetManagerDemo PRIVATE
    PresetLib
    # AIMusicCore is already included via PresetLib
)
message(STATUS "Building SimplePresetManagerDemo")

# Print audio libs for debugging
message(STATUS "Audio libraries: ${AUDIO_LIBS}")

# Create Multi-Timbral Demo executable
add_executable(MultiTimbralDemo examples/MultiTimbralDemo.cpp)
target_link_libraries(MultiTimbralDemo PRIVATE
    PresetLib
    # AIMusicCore is already included via PresetLib
)
message(STATUS "Building MultiTimbralDemo")

# Create MPE Voice Manager Test executable
add_executable(TestMpeVoiceManager examples/TestMpeVoiceManager.cpp)
target_link_libraries(TestMpeVoiceManager PRIVATE
    AIMusicCore
)
message(STATUS "Building TestMpeVoiceManager")

# Create TestAudio executable for real-time audio testing
add_executable(TestAudio examples/TestAudio.cpp)
target_link_libraries(TestAudio PRIVATE
    AIMusicCore
)
message(STATUS "Building TestAudio")

# List files we're not building for now due to issues
set_target_properties(MultiTimbralDemo PROPERTIES EXCLUDE_FROM_ALL TRUE)

# Create Oscillator Stack Demo executable
add_executable(OscillatorStackDemo examples/OscillatorStackDemo.cpp)
target_link_libraries(OscillatorStackDemo PRIVATE
    AIMusicCore
)
message(STATUS "Building OscillatorStackDemo")

# Create Advanced Filter Demo executable
add_executable(AdvancedFilterDemo examples/AdvancedFilterDemo.cpp)
target_link_libraries(AdvancedFilterDemo PRIVATE
    AIMusicCore
)
message(STATUS "Building AdvancedFilterDemo")

message(STATUS "Available tests:")
message(STATUS "- Run ./bin/SimplePresetManagerDemo to try the preset management system")
message(STATUS "- Run ./bin/MultiTimbralDemo to try the multi-timbral capabilities")
message(STATUS "- Run ./bin/TestMpeVoiceManager to try the MPE-aware voice manager")
message(STATUS "- Run ./bin/TestAudio to test real-time audio generation")
message(STATUS "- Run ./bin/OscillatorStackDemo to try the oscillator stacking capabilities")
message(STATUS "- Run ./bin/AdvancedFilterDemo to try the enhanced filter system with multiple filter types")

# Create Synchronized Sequencer Demo executable
add_executable(SynchronizedSequencerDemo examples/SynchronizedSequencerDemo.cpp)
target_link_libraries(SynchronizedSequencerDemo PRIVATE
    AIMusicCore
)
message(STATUS "Building SynchronizedSequencerDemo")
message(STATUS "- Run ./bin/SynchronizedSequencerDemo to test the improved sequencer-audio engine synchronization")

# Create IoT Parameter Demo executable
if(PAHO_MQTT_CPP_FOUND)
    add_executable(IoTParameterDemo examples/IoTParameterDemo.cpp)
    target_link_libraries(IoTParameterDemo PRIVATE
        AIMusicCore
    )
    message(STATUS "Building IoTParameterDemo")
    message(STATUS "- Run ./bin/IoTParameterDemo to test the IoT message handling")

    # Create IoT Parameter Integration Test
    add_executable(IoTParameterIntegrationTest examples/IoTParameterIntegrationTest.cpp)
    target_link_libraries(IoTParameterIntegrationTest PRIVATE
        AIMusicCore
    )
    message(STATUS "Building IoTParameterIntegrationTest")
    message(STATUS "- Run ./bin/IoTParameterIntegrationTest to test the full IoT-Parameter System integration")

    # Create IoT Event Integration Test
    add_executable(IoTEventIntegrationTest examples/IoTEventIntegrationTest.cpp)
    target_link_libraries(IoTEventIntegrationTest PRIVATE
        AIMusicCore
    )
    message(STATUS "Building IoTEventIntegrationTest")
    message(STATUS "- Run ./bin/IoTEventIntegrationTest to test the IoT-Event System integration")

    # Create IoT Configuration Manager Demo
    add_executable(IoTConfigManagerDemo examples/IoTConfigManagerDemo.cpp)
    target_link_libraries(IoTConfigManagerDemo PRIVATE
        AIMusicCore
    )
    message(STATUS "Building IoTConfigManagerDemo")
    message(STATUS "- Run ./bin/IoTConfigManagerDemo to test the IoT Configuration Manager with device discovery and configuration")

    # Create MQTT Integration Test
    add_executable(TestMQTTIntegration examples/TestMQTTIntegration.cpp)
    target_link_libraries(TestMQTTIntegration PRIVATE
        AIMusicCore
    )
    message(STATUS "Building TestMQTTIntegration")
    message(STATUS "- Run ./bin/TestMQTTIntegration to test basic MQTT connectivity and message handling")
else()
    # Create installation guide executables for IoT tools
    add_executable(IoTConfigManagerDemo examples/VerySimpleMQTTTest.cpp)
    add_executable(MQTTGuide examples/VerySimpleMQTTTest.cpp)
    add_executable(TestMQTTIntegration examples/VerySimpleMQTTTest.cpp)

    message(STATUS "Building installation guides for MQTT tools (Paho MQTT C++ library not found)")
    message(STATUS "- Run bin/IoTConfigManagerDemo, bin/MQTTGuide, or bin/TestMQTTIntegration")
    message(STATUS "  to see MQTT installation instructions")
endif()

# Create Simple MQTT Test executable
add_executable(SimpleMQTTTest examples/SimpleMQTTTest.cpp)
target_link_libraries(SimpleMQTTTest PRIVATE
    # No need to link to AIMusicCore, since it only tests MQTT
)
message(STATUS "Building SimpleMQTTTest")
message(STATUS "- Run ./bin/SimpleMQTTTest to test the basic MQTT mock implementation and messaging")

# Create Simple MQTT Interface Test executable
add_executable(SimpleMQTTInterfaceTest examples/SimpleMQTTInterfaceTest.cpp)
target_link_libraries(SimpleMQTTInterfaceTest PRIVATE
    AIMusicCore
)
message(STATUS "Building SimpleMQTTInterfaceTest")
message(STATUS "- Run ./bin/SimpleMQTTInterfaceTest to test the MQTTInterface class with mock implementation")

# Create Real MQTT Test executable
add_executable(RealMQTTTest examples/RealMQTTTest.cpp)
target_link_libraries(RealMQTTTest PRIVATE
    AIMusicCore
)
message(STATUS "Building RealMQTTTest")
message(STATUS "- Run ./bin/RealMQTTTest to test the MQTTInterface class with real MQTT implementation")

# Create Comprehensive MQTT Test executable
add_executable(ComprehensiveMQTTTest examples/ComprehensiveMQTTTest.cpp)
target_link_libraries(ComprehensiveMQTTTest PRIVATE
    AIMusicCore
)
message(STATUS "Building ComprehensiveMQTTTest")
message(STATUS "- Run ./bin/ComprehensiveMQTTTest to thoroughly test all MQTT mock implementation functionality")

# Create Quick MQTT Test executable
add_executable(QuickMQTTTest examples/QuickMQTTTest.cpp)
target_link_libraries(QuickMQTTTest PRIVATE
    # No need to link to AIMusicCore, since it only tests basic MQTT mock
)
message(STATUS "Building QuickMQTTTest")
message(STATUS "- Run ./bin/QuickMQTTTest for fast validation of MQTT mock implementation")

# Create Paho MQTT C Test executable (for testing the C implementation)
if(FALSE) # Disable for now until we have the proper Paho MQTT C library
    add_executable(PahoMQTTCTest examples/PahoMQTTCTest.cpp)
    # Link directly with the C library for this test
    target_link_libraries(PahoMQTTCTest PRIVATE
        ${PAHO_MQTT_C_LIBRARY}
    )
    message(STATUS "Building PahoMQTTCTest")
    message(STATUS "- Run ./bin/PahoMQTTCTest to test the Paho MQTT C implementation directly")
endif()

# Create Simple Audio Test executable
add_executable(SimpleAudioTest examples/SimpleAudioTest.cpp)
target_link_libraries(SimpleAudioTest PRIVATE
    AIMusicCore
)
message(STATUS "Building SimpleAudioTest")
message(STATUS "- Run ./bin/SimpleAudioTest to play a C major scale and test basic audio functionality")

# Create Enhanced Preset Database Test executable
add_executable(EnhancedPresetDatabaseTest examples/EnhancedPresetDatabaseTest.cpp)
target_link_libraries(EnhancedPresetDatabaseTest PRIVATE
    AIMusicCore
)
message(STATUS "Building EnhancedPresetDatabaseTest")
message(STATUS "- Run ./bin/EnhancedPresetDatabaseTest to test the enhanced preset database system with performance benchmarking")

# Create Preset Browser UI Demo executable
add_executable(PresetBrowserUIDemo examples/PresetBrowserUIDemo.cpp)
target_link_libraries(PresetBrowserUIDemo PRIVATE
    AIMusicCore
)
message(STATUS "Building PresetBrowserUIDemo")
message(STATUS "- Run ./bin/PresetBrowserUIDemo to test the Phase 2 preset browser UI with multi-panel interface")

# Create Phase 3 Smart Features Demo executable
add_executable(Phase3SmartFeaturesDemo examples/Phase3SmartFeaturesDemo.cpp)
target_link_libraries(Phase3SmartFeaturesDemo PRIVATE
    AIMusicCore
)
message(STATUS "Building Phase3SmartFeaturesDemo")
message(STATUS "- Run ./bin/Phase3SmartFeaturesDemo to test Phase 3 smart features with ML analysis and intelligent recommendations")

# Find SDL2 for UI tests
find_package(SDL2 QUIET)
if(SDL2_FOUND)
    message(STATUS "Found SDL2: ${SDL2_INCLUDE_DIRS}")
    
    # Create TestUI executable
    add_executable(TestUI examples/TestUI.cpp)
    target_include_directories(TestUI PRIVATE ${SDL2_INCLUDE_DIRS})
    target_link_libraries(TestUI PRIVATE
        AIMusicCore
        ${SDL2_LIBRARIES}
    )
    message(STATUS "Building TestUI")
    message(STATUS "- Run ./bin/TestUI to test the basic UI system with interactive knobs and buttons")
    
    # Create Enhanced UI Integration Test executable
    add_executable(EnhancedUIIntegrationTest examples/EnhancedUIIntegrationTest.cpp)
    target_include_directories(EnhancedUIIntegrationTest PRIVATE ${SDL2_INCLUDE_DIRS})
    target_link_libraries(EnhancedUIIntegrationTest PRIVATE
        AIMusicCore
        ${SDL2_LIBRARIES}
    )
    message(STATUS "Building EnhancedUIIntegrationTest")
    message(STATUS "- Run ./bin/EnhancedUIIntegrationTest to test the enhanced UI with parameter binding and thread-safe updates")
    
    # Create Comprehensive UI Test executable
    add_executable(ComprehensiveUITest examples/ComprehensiveUITest.cpp)
    target_include_directories(ComprehensiveUITest PRIVATE ${SDL2_INCLUDE_DIRS})
    target_link_libraries(ComprehensiveUITest PRIVATE
        AIMusicCore
        ${SDL2_LIBRARIES}
    )
    message(STATUS "Building ComprehensiveUITest")
    message(STATUS "- Run ./bin/ComprehensiveUITest to test all UI components including preset browser and visualizations")
else()
    message(WARNING "SDL2 not found. UI test applications will not be built.")
    message(WARNING "Install SDL2 to enable UI testing: brew install sdl2 (macOS) or sudo apt-get install libsdl2-dev (Linux)")
endif()