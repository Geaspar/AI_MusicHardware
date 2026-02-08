# IoTSynth Proof of Concept
**Date:** Mon, Jan 14th, 2026  
**Branch:** juce-migration

## Executive Summary
- JUCE standalone runs on macOS with sensor UI for two sensor streams (Light + Distance). Manual and External paths are separated; env-driven external sources (fake ramps/files) work and headless defaults are in place.
- Elk VST3 build is pending: the Elk toolchain file is not on this machine (but we can also build natively on the Pi).
- Hardware target: Pi 4B + HiFiBerry DAC+ ADC Pro + VL53L0X distance sensor + ADS1115 + LDR divider (light). Plan: wire both on I2C, normalize in code, and map to synth parameters for an audible/visual demo.

## What we did today
- Built and ran `AIMH_JuceStandalone` on macOS (GUI confirmed).
- Added sensor source toggle (Manual vs External) and per-sensor targets (Light->*, Distance->*); manual/external values separated to avoid overrides.
- External helpers (from the environment):
  - `AIMH_LIGHT_FILE=/tmp/light.txt` (0..1) and `AIMH_DISTANCE_FILE=/tmp/distance.txt` (0..1)
  - `AIMH_SENSOR_FILE=/tmp/sensor.txt` (legacy alias for light)
  - `AIMH_FAKE_SENSOR=1` (legacy fake light ramp) and `AIMH_FAKE_DISTANCE=1` (fake distance ramp)
  - `AIMH_SENSOR_MODE=manual|external` (optional override; otherwise external sources default to External mode automatically)
- Added MIDI control handling in the plugin (useful for sensors->MIDI bridges):
  - Pitch wheel -> pitch bend
  - CC74 -> filter cutoff, CC71 -> resonance, CC7 -> master volume
- Searched for Elk toolchain locally; none found.

## Current state
- Targets: `AIMH_JuceStandalone` (tested) and `AIMH_JucePlugin_VST3` (build pending Elk toolchain).
- Two sensor streams (Light + Distance): UI sliders (Manual) or env/poller (External) each routed to Cutoff/Volume/Pitch Bend.
- Build warnings: JUCE `getCurrentPosition` deprecation (benign), RtAudio/RtMidi arm64 vs x86_64 ignored in universal link.

## Plan (detailed steps)
1) Build the plugin for Elk on Raspberry Pi  
   Option A — build natively on the Pi (recommended if you’re already on Elk OS):  
   - Ensure you have build tools: `cmake`, a C++ compiler, and `git`.  
   - Ensure JUCE is installed as a CMake package (so `find_package(JUCE CONFIG)` works).  
   - Configure and build:
     - `cmake -S . -B build -DENABLE_JUCE_TARGETS=ON -DJUCE_DIR=/usr/local/lib/cmake/JUCE`
     - `cmake --build build --target AIMH_JucePlugin_VST3 -j$(nproc)`

   Option B — cross-build on a host machine:
   - Obtain the Elk toolchain (cross-compile toolchain + Elk SDK CMake toolchain file). Typically provided in the Elk Audio OS SDK; installed locally, then referenced via `-DCMAKE_TOOLCHAIN_FILE=<path>/elk_toolchain.cmake`.  
   - Configure: `cmake -S . -B build-elk -DENABLE_JUCE_TARGETS=ON -DCMAKE_TOOLCHAIN_FILE=<path> -DJUCE_DIR=$HOME/juce-install/lib/cmake/JUCE`.  
   - Build: `cmake --build build-elk --target AIMH_JucePlugin_VST3 -j8`.  
   - Output: VST3 to deploy on the Elk host running on the Pi.

2) Wire sensors on Pi (I2C, 3.3V)  
   - I2C: SDA=GPIO2 (pin 3), SCL=GPIO3 (pin 5), 3.3V=pin 1, GND=pin 6.  
   - VL53L0X: VIN→3.3V, GND→GND, SDA/SCL to bus, XSHUT floating or 3.3V (single device).  
   - ADS1115 (0x48): VDD→3.3V, GND→GND, SDA/SCL to bus, ADDR→GND.  
   - LDR divider to ADS1115 A0: 3.3V → LDR → node → 10 kΩ → GND; node → A0. Adjust resistor if range is cramped.  
   - Enable I2C on Pi; verify with `sudo i2cdetect -y 1` (expect 0x29, 0x48).

3) Implement sensor pollers in code (Pi/Elk path)  
   - Phase 1 (already implemented): feed values from files (or fake ramps) using env vars:
     - `AIMH_LIGHT_FILE=/tmp/light.txt`
     - `AIMH_DISTANCE_FILE=/tmp/distance.txt`
   - Phase 2 (hardware): add C++ readers (or a sidecar process) that convert real sensors to normalized 0..1:
     - ADS1115 A0 → normalized 0..1 (invert if needed so “more light → higher value”).
     - VL53L0X distance → clamp (e.g., 50–500 mm) → normalized 0..1.
   - Chosen mappings (per user):  
     - Light (ADS1115 A0) → Filter Cutoff (more light → higher cutoff).  
     - Distance (VL53L0X) → Pitch Bend (simulate Doppler: closer/farther alters pitch bend).

4) Deploy and test on Pi/Elk  
   - Copy VST3 to Elk plugin dir on the Pi.  
   - Run Elk host (Sushi) and load the plugin.  
   - For Phase 1 (file-driven sensors): start Sushi with env vars and update the files while it runs.  
   - Shine torch on LDR → hear cutoff change; move hand over VL53L0X → hear pitch bend change.  
   - Tweak normalization/ranges if response feels too steep/flat.

5) Polish (optional)  
   - Add smoothing/curves (e.g., log cutoff response, dead-zone for distance).  
   - Replace `getCurrentPosition` with `getPosition` to silence JUCE deprecation warning.  
   - Tidy RtAudio/RtMidi universal build config if desired.

## Elk toolchain notes (what it is and when you need it)
- The Elk toolchain is a cross-compilation setup (compiler + CMake toolchain file + Elk SDK) targeting Elk Audio OS on ARM. You need it on macOS if you want to build the VST3 without compiling directly on the Pi.
- If you flash Elk Audio OS to an SD card and build directly on the Pi with Elk SDK packages installed, you can use the on-device toolchain instead of cross-compiling on macOS. You still need Elk’s development packages on the Pi (headers, CMake toolchain file).
- We cannot download it here due to restricted network; please fetch the Elk Audio OS SDK from Elk’s site/docs. Typical deliverables: toolchain tarball/installer plus a `elk_toolchain.cmake` (or similarly named) file.
- Once you provide the path to that toolchain file (on macOS or on the Pi), we can configure the build.

## Notes on running Sushi locally (desktop)
- Sushi can run on macOS/Linux desktop for quick plugin hosting. Download the Sushi binary release for your platform, clear quarantine on macOS (`xattr -rc sushi`), and run with CoreAudio: `./sushi --coreaudio -c config_files/play_brickworks_synth.json` or `./sushi --coreaudio -c config_files/play_vst3.json`.  
- On Linux, use JACK: `./Sushi-x86_64.AppImage -j --connect-ports -c config_files/play_vst3.json`, then connect MIDI via `aconnect`.  
- For Pi/Elk targets, follow the Elk “getting started on Raspberry Pi” guide instead of desktop instructions.
