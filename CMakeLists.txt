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

# Find dependencies
find_package(Threads REQUIRED)

# Optional packages
find_package(ALSA)
find_package(JACK)
find_package(PulseAudio)
find_package(RtAudio)
find_package(RtMidi)

# Define source files
set(SOURCES
    src/main.cpp
    src/audio/AudioEngine.cpp
    src/audio/Synthesizer.cpp
    src/effects/EffectProcessor.cpp
    src/sequencer/Sequencer.cpp
    src/midi/MidiInterface.cpp
    src/hardware/HardwareInterface.cpp
    src/ai/LLMInterface.cpp
    src/ui/UserInterface.cpp
)

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Create executable
add_executable(${PROJECT_NAME} ${SOURCES})

# Link libraries
target_link_libraries(${PROJECT_NAME} PRIVATE
    Threads::Threads
)

# Add RtAudio if found
if(RtAudio_FOUND)
    target_link_libraries(${PROJECT_NAME} PRIVATE RtAudio::rtaudio)
    target_compile_definitions(${PROJECT_NAME} PRIVATE HAVE_RTAUDIO)
endif()

# Add RtMidi if found
if(RtMidi_FOUND)
    target_link_libraries(${PROJECT_NAME} PRIVATE RtMidi::rtmidi)
    target_compile_definitions(${PROJECT_NAME} PRIVATE HAVE_RTMIDI)
endif()

# Platform-specific settings
if(UNIX AND NOT APPLE)
    if(ALSA_FOUND)
        target_link_libraries(${PROJECT_NAME} PRIVATE ${ALSA_LIBRARIES})
        target_include_directories(${PROJECT_NAME} PRIVATE ${ALSA_INCLUDE_DIRS})
        target_compile_definitions(${PROJECT_NAME} PRIVATE HAVE_ALSA)
    endif()
    
    if(JACK_FOUND)
        target_link_libraries(${PROJECT_NAME} PRIVATE ${JACK_LIBRARIES})
        target_include_directories(${PROJECT_NAME} PRIVATE ${JACK_INCLUDE_DIRS})
        target_compile_definitions(${PROJECT_NAME} PRIVATE HAVE_JACK)
    endif()
    
    if(PulseAudio_FOUND)
        target_link_libraries(${PROJECT_NAME} PRIVATE ${PULSEAUDIO_LIBRARIES})
        target_include_directories(${PROJECT_NAME} PRIVATE ${PULSEAUDIO_INCLUDE_DIRS})
        target_compile_definitions(${PROJECT_NAME} PRIVATE HAVE_PULSE)
    endif()
    
    target_link_libraries(${PROJECT_NAME} PRIVATE dl)
endif()

if(APPLE)
    target_link_libraries(${PROJECT_NAME} PRIVATE
        "-framework CoreAudio"
        "-framework CoreMIDI"
        "-framework CoreFoundation"
        "-framework Accelerate"
    )
    target_compile_definitions(${PROJECT_NAME} PRIVATE HAVE_COREAUDIO HAVE_COREMIDI)
endif()

if(WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE
        winmm
    )
    target_compile_definitions(${PROJECT_NAME} PRIVATE HAVE_WINMM)
endif()

# Testing
option(BUILD_TESTS "Build test programs" OFF)
if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# Install target
install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
)

# Create directory structure
add_custom_target(create_directories ALL
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_SOURCE_DIR}/src/audio
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_SOURCE_DIR}/src/ai
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_SOURCE_DIR}/src/ui
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_SOURCE_DIR}/src/hardware
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_SOURCE_DIR}/src/sequencer
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_SOURCE_DIR}/src/effects
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_SOURCE_DIR}/src/midi
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_SOURCE_DIR}/tests/audio
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_SOURCE_DIR}/tests/ai
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_SOURCE_DIR}/tests/ui
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_SOURCE_DIR}/tests/hardware
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_SOURCE_DIR}/tests/sequencer
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_SOURCE_DIR}/tests/effects
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_SOURCE_DIR}/tests/midi
)