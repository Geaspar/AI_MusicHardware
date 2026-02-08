cmake_minimum_required(VERSION 3.14)

# JUCE-based targets for AIMusicHardware.
# This directory is optional and only builds when JUCE is available and
# ENABLE_JUCE_TARGETS is set in the top-level CMake configuration.

if(NOT TARGET AIMusicCore)
    message(STATUS "[JUCE] AIMusicCore not built yet; skipping JUCE targets.")
    return()
endif()

find_package(JUCE CONFIG QUIET)

if(NOT JUCE_FOUND)
    message(STATUS "[JUCE] JUCE library not found; JUCE targets will not be built.")
    return()
endif()

message(STATUS "[JUCE] JUCE found; enabling JUCE-based Elk targets.")

# Plugin target (for Elk host and desktop plugin hosts)
juce_add_plugin(AIMH_JucePlugin
    PRODUCT_NAME "AIMH Elk Synth"
    COMPANY_NAME "AIMusicHardware"
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    PLUGIN_MANUFACTURER_CODE AIMH
    PLUGIN_CODE AMHS
    FORMATS VST3
)

target_sources(AIMH_JucePlugin PRIVATE
    ElkSynthPluginProcessor.cpp
    ElkSynthPluginProcessor.h
    ElkSynthPluginEditor.cpp
    ElkSynthPluginEditor.h
)

target_link_libraries(AIMH_JucePlugin PRIVATE
    AIMusicCore
    juce::juce_audio_utils
    juce::juce_audio_processors
    juce::juce_audio_devices
    juce::juce_core
)

# Avoid VST2/VST3 automation ID conflicts; we are only interested in VST3.
target_compile_definitions(AIMH_JucePlugin PRIVATE
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_IGNORE_VST3_MISMATCHED_PARAMETER_ID_WARNING=1
)

if(APPLE)
    # Build the JUCE plugin as a universal binary so it can be loaded
    # by both native Apple Silicon and Intel/Rosetta hosts.
    set_target_properties(AIMH_JucePlugin PROPERTIES
        OSX_ARCHITECTURES "x86_64;arm64"
    )
    if(TARGET AIMH_JucePlugin_VST3)
        set_target_properties(AIMH_JucePlugin_VST3 PROPERTIES
            OSX_ARCHITECTURES "x86_64;arm64"
        )
    endif()
endif()

# Minimal standalone app for development bring-up.
juce_add_gui_app(AIMH_JuceStandalone
    PRODUCT_NAME "AIMH JUCE Standalone"
    COMPANY_NAME "AIMusicHardware"
)

target_sources(AIMH_JuceStandalone PRIVATE
    ElkSynthStandaloneApp.cpp
    ElkSynthPluginProcessor.cpp
    ElkSynthPluginProcessor.h
    ElkSynthPluginEditor.cpp
    ElkSynthPluginEditor.h
)

target_link_libraries(AIMH_JuceStandalone PRIVATE
    AIMusicCore
    juce::juce_gui_extra
    juce::juce_audio_utils
    juce::juce_audio_devices
    juce::juce_audio_processors
    juce::juce_core
)

if(TARGET AIMH_JucePlugin_VST3)
    target_compile_definitions(AIMH_JucePlugin_VST3 PRIVATE
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_IGNORE_VST3_MISMATCHED_PARAMETER_ID_WARNING=1
    )
endif()
