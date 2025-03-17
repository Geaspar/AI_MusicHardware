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

# Define basic source files
set(BASIC_AUDIO_SOURCES
    src/audio/AudioEngine.cpp
    src/audio/Synthesizer.cpp
    src/hardware/HardwareInterface.cpp
)

set(SEQUENCER_SOURCES
    src/sequencer/Sequencer.cpp
    src/sequencer/MidiFile.cpp
    src/sequencer/AdaptiveSequencer.cpp
)

set(UI_SOURCES
    src/ui/DisplayManager.cpp
    src/ui/UIContext.cpp
    src/ui/UserInterface.cpp
    src/ui/UIComponents.cpp
    src/ui/Font.cpp
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

# Create necessary directories if they don't exist
foreach(DIR "cmake" "examples" "output")
    if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${DIR}")
        file(MAKE_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/${DIR}")
    endif()
endforeach()

# Create core library
add_library(AIMusicCore STATIC
    ${BASIC_AUDIO_SOURCES}
    ${SEQUENCER_SOURCES}
    ${UI_SOURCES}
)

target_link_libraries(AIMusicCore PRIVATE
    Threads::Threads
    ${AUDIO_LIBS}
)

# RtAudio-based test (requires RtAudio library)
if(RTAUDIO_FOUND)
    add_executable(TestAudio examples/TestAudio.cpp)
    target_link_libraries(TestAudio PRIVATE AIMusicCore)
    message(STATUS "Building TestAudio (real-time audio test)")
    
    add_executable(TestSequencer examples/TestSequencer.cpp)
    target_link_libraries(TestSequencer PRIVATE AIMusicCore)
    message(STATUS "Building TestSequencer (sequencer test)")
else()
    message(STATUS "Skipping real-time audio tests due to missing RtAudio dependency")
endif()

# SimpleTest (generates WAV files, doesn't require RtAudio)
add_executable(SimpleTest examples/SimpleTest.cpp)
target_link_libraries(SimpleTest PRIVATE Threads::Threads)
message(STATUS "Building SimpleTest (WAV file generation test)")

# TestSequencerFile (generates sequencer patterns as WAV files, doesn't require RtAudio)
add_executable(TestSequencerFile examples/TestSequencerFile.cpp ${SEQUENCER_SOURCES})
target_link_libraries(TestSequencerFile PRIVATE Threads::Threads)
message(STATUS "Building TestSequencerFile (sequencer WAV file generation test)")

# TestSequencerAdvanced (advanced sequencer features, doesn't require RtAudio)
add_executable(TestSequencerAdvanced examples/TestSequencerAdvanced.cpp ${SEQUENCER_SOURCES})
target_link_libraries(TestSequencerAdvanced PRIVATE Threads::Threads)
message(STATUS "Building TestSequencerAdvanced (advanced sequencer features test)")

# TestAdaptiveSequencer (adaptive sequencer features, requires RtAudio)
if(RTAUDIO_FOUND)
    add_executable(TestAdaptiveSequencer examples/TestAdaptiveSequencer.cpp)
    target_link_libraries(TestAdaptiveSequencer PRIVATE AIMusicCore)
    message(STATUS "Building TestAdaptiveSequencer (adaptive sequencer test)")
endif()

# Print audio libs for debugging
message(STATUS "Audio libraries: ${AUDIO_LIBS}")

# Update the readme
message(STATUS "Available tests:")
message(STATUS "1. Run ./bin/SimpleTest to generate WAV files in the output directory")
message(STATUS "2. Run ./bin/TestSequencerFile to generate sequencer pattern WAV files")
message(STATUS "3. Run ./bin/TestSequencerAdvanced to explore advanced sequencer features")
if(RTAUDIO_FOUND)
    message(STATUS "4. Run ./bin/TestAudio for real-time audio playback")
    message(STATUS "5. Run ./bin/TestSequencer to test the sequencer component with real-time audio")
    message(STATUS "6. Run ./bin/TestAdaptiveSequencer to explore the adaptive sequencer system")
endif()