# Wavetable Synthesis Engine Upgrade Plan: Adopting Vital's Real-time Approach

**Date:** July 22, 2025
**Status:** Complete

## 1. Overview

This document outlines the plan to upgrade the existing wavetable synthesis engine to a real-time, frequency-domain-based approach, inspired by the architecture of the Vital synthesizer. This upgrade will move from a pre-computed, multi-band wavetable system to a more flexible and powerful engine that generates band-limited waveforms in real-time.

The primary goal is to enable advanced, real-time spectral morphing and manipulation, which is not possible with the current "baked-in" wavetable approach. While this will increase CPU usage, the trade-off is a significant leap in sound design capabilities.

## 2. High-Level Plan

1.  **Integrate a Fourier Transform Library:** Introduce a robust FFT library to handle the transformation between the time and frequency domains.
2.  **Redefine the Wavetable Data Structure:** Create a new data structure to store wavetables as a series of harmonics (amplitudes and phases) in the frequency domain.
3.  **Develop a New Real-time Oscillator Engine:** Implement a new oscillator class that generates waveforms dynamically using an Inverse Fast Fourier Transform (IFFT).
4.  **Create a Conversion Path for Existing Wavetables:** Develop a utility to convert our current time-domain wavetables into the new frequency-domain format.
5.  **Incremental Integration and Rigorous Testing:** Integrate the new engine alongside the old one, with comprehensive tests to validate correctness, aliasing, and performance.

## 3. Detailed Implementation Steps

### Step 3.1: Integrate a Fourier Transform (FFT) Library

*   **Library Selection:** We will investigate and integrate a suitable C++ FFT library. A lightweight, header-only library is preferred to minimize build complexity. A potential candidate is `pocketfft`, which is known for its simplicity and performance.
*   **Build System Integration:** The chosen library will be added to the project's `vendor/` directory and integrated into the build system via `CMakeLists.txt`. This will involve updating include paths and linking requirements.
*   **Wrapper Class:** A thin wrapper class, `FourierTransform`, will be created in `include/audio/` to abstract the specific library implementation. This will make it easier to swap out the FFT library in the future if needed. The wrapper will provide two primary methods: `performFFT()` and `performIFFT()`.

### Step 3.2: Redefine Wavetable Data Structure

*   **New Class Definition:** A new class, `FrequencyDomainWavetable`, will be defined in `include/synthesis/`.
*   **Data Members:**
    *   `std::string name`: Name of the wavetable.
    *   `int num_frames`: Number of individual waveframes in the table.
    *   `std::vector<std::vector<std::complex<float>>> harmonic_data`: A nested vector where the outer vector represents the waveframe index and the inner vector stores the complex values (real for amplitude, imaginary for phase) for each harmonic up to the Nyquist frequency of the original recording.
*   **File Format:** We will define a new JSON-based file format for storing these frequency-domain wavetables. This allows for easy editing and inspection.

### Step 3.3: Develop New Real-time Oscillator Engine

*   **New Oscillator Class:** A new class `RealtimeWavetableVoice` will be created in `include/synthesis/` and implemented in `src/synthesis/`. It will inherit from a common `Voice` base class.
*   **Core `process()` Method Logic:** The main audio processing loop will perform these steps for each audio block:
    1.  **Calculate Nyquist:** Determine the Nyquist frequency for the current note's pitch (`current_pitch_frequency * 0.5 * sample_rate`).
    2.  **Build Spectrum:** Create a new `std::vector<std::complex<float>>` representing the frequency spectrum for the output.
    3.  **Copy Harmonics:** Iterate through the `harmonic_data` of the current `FrequencyDomainWavetable` frame. For each harmonic, if its frequency is below the calculated Nyquist limit, copy it to the output spectrum.
    4.  **Apply IFFT:** Use the `FourierTransform::performIFFT()` method on the output spectrum to generate the final, band-limited, time-domain waveform block.
    5.  **Interpolation:** Implement interpolation (linear or cubic) for smooth transitions when morphing between different waveframes in the wavetable.

### Step 3.4: Create a Conversion Utility

*   **Converter Tool:** A command-line tool will be developed in `tools/` named `wavetable_converter`.
*   **Functionality:**
    1.  Load an existing audio file (WAV) or one of our current wavetable files.
    2.  Perform a Forward Fast Fourier Transform (FFT) on the time-domain data to get its harmonic content.
    3.  Populate a `FrequencyDomainWavetable` object with the extracted harmonic data.
    4.  Save the new object to a `.json` file in the new format.
*   **Batch Processing:** The tool will support batch conversion of entire directories of wavetables.

### Step 3.5: Incremental Integration and Testing

*   **Engine Switch:** The main `SynthesisEngine` will be modified to allow switching between the existing `BandLimitedVoiceManager` and a new `RealtimeVoiceManager`. This will be controlled by a setting in the UI to allow for A/B testing.
*   **Unit Tests:**
    *   **Spectrum Analysis Test:** A new test will be written in `tests/` that renders a high-frequency note (e.g., C7) using the new engine and performs an FFT on the output. The test will assert that there are no significant harmonics above the Nyquist frequency, thus verifying the absence of aliasing.
    *   **Reference Waveform Test:** A test will compare the output of the new engine with a known-good, pre-rendered waveform to ensure correctness.
*   **Performance Benchmarks:** A stress test will be created in `examples/` to measure the CPU usage of the new engine with a high number of voices. This will help quantify the performance impact and identify areas for optimization.

## 4. Implementation Summary and Challenges

The implementation of the real-time wavetable synthesis engine is complete. The new `RealtimeWavetableVoiceManager` is integrated into the `Synthesizer` class and can be enabled by calling `setVoiceManagerType(VoiceManagerType::RealTime)`. The process involved several challenges, primarily related to the C++ build system and include paths.

### Key Implementation Steps:

1.  **`RealtimeWavetableVoiceManager` Creation:** A new `RealtimeWavetableVoiceManager` class was created to manage the `RealtimeWavetableVoice` instances.
2.  **`Synthesizer` Integration:** The `Synthesizer` class was modified to support switching between different voice managers (`Standard`, `BandLimited`, and `RealTime`) using the `setVoiceManagerType` method.
3.  **`Voice` Class Refactoring:** The `Voice` class was extracted from `voice_manager.h` and placed into its own dedicated `voice.h` and `voice.cpp` files for better code organization.

### Challenges and Resolutions:

*   **Incorrect Include Paths:** The initial build attempts failed due to incorrect include paths in the new and modified files. This was resolved by carefully correcting the paths to be relative to the `include` directory, which is the standard for the project.
*   **Missing `Voice.h` File:** The build initially failed because of an include for a non-existent `voice.h` file. This was a misunderstanding of the codebase, and the issue was resolved by refactoring the `Voice` class into its own file.
*   **`override` Keyword Errors:** The build failed due to the incorrect use of the `override` keyword on methods that did not override a virtual method in a base class. This was resolved by removing the `override` keyword from the affected methods.
*   **Private Member Access:** The build failed because the `RealtimeWavetableVoiceManager` needed to access a private member of the `VoiceManager` class. This was resolved by changing the access specifier of the member to `protected`.
*   **Typo in `std::vector`:** A typo in the `RealtimeWavetableVoice.cpp` file (`std.vector` instead of `std::vector`) caused a build failure. This was a simple typo that was quickly corrected.

Overall, the implementation was successful, and the new real-time wavetable synthesis engine is now available for use.
