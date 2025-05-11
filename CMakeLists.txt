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
)

target_link_libraries(AIMusicCore PRIVATE
    Threads::Threads
    ${AUDIO_LIBS}
)

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
    AIMusicCore
    PresetLib
)
message(STATUS "Building SimplePresetManagerDemo")

# Print audio libs for debugging
message(STATUS "Audio libraries: ${AUDIO_LIBS}")

# Create Multi-Timbral Demo executable
add_executable(MultiTimbralDemo examples/MultiTimbralDemo.cpp)
target_link_libraries(MultiTimbralDemo PRIVATE
    AIMusicCore
    PresetLib
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