# Hardware Considerations: LED Matrix + Touch UI

This document outlines a practical, audio–real‑time‑safe way to add a central LED “canvas” with an optional touch layer, using a Raspberry Pi 4 running Elk OS together with a dedicated microcontroller.

## Overview
- Goal: A bright, expressive LED matrix for visual feedback (meters, scopes, sequencer), optionally with multitouch gestures on top.
- Approach: Split responsibilities — Elk/Pi handles synth/UI logic; a co‑processor drives LEDs and scans touch with deterministic timing.
- Why: Keeps LED PWM and touch scanning out of the audio thread to protect real‑time performance.

## System Architecture
- Pi 4 (Elk OS): Runs DSP, UI logic, scene selection. Communicates with the co‑processor via USB.
- Co‑processor (RP2040, Teensy 4.1, ESP32‑S3): Drives LED matrix at high refresh; scans touch; exposes a simple USB interface (CDC/HID/MIDI).
- LED Matrix: HUB75 RGB panels (modular, bright, cost‑effective) or APA102/DotStar SPI matrices (simpler wiring).
- Touch Layer (optional): Clear mutual‑capacitance film (PET/ITO) with an off‑the‑shelf controller; mounted over a diffuser above the LEDs.

## LED Matrix Options
### Option A — HUB75 RGB Panels (Recommended for larger canvases)
- What: 2×2 of 32×32 (P3/P2.5) panels for 64×64, or a single 64×32.
- Drive: Use a HUB75 DMA library (RP2040 pico‑rgb‑matrix/Hub75, Teensy SmartMatrix, ESP32 HUB75 libraries).
- Pros: Very bright, low cost per pixel, mature ecosystem; high refresh with bitplane PWM.
- Cons: Multiplexed driving is non‑trivial; needs proper driver board and a quality 5 V PSU (10–20 A headroom depending on size/brightness).

### Option B — APA102/DotStar Matrix (Simpler wiring, smaller builds)
- What: Premade 16×16/32×32 matrices or custom panels from strips; SPI clocked.
- Pros: Fewer timing headaches than WS2812; good refresh; easy scaling to moderate sizes.
- Cons: Higher cost per pixel; power and EMI still need care; very large matrices become bulky/pricey.

### Option C — Monochrome Matrix + LED Drivers
- What: Custom PCB with dense mono LEDs and drivers (e.g., IS31FL3731/IS31FL3741).
- Pros: Slim assembly, lower power, consistent diffusion; excellent legibility for text/graphics.
- Cons: Custom PCB effort; fewer color options and less “wow” than RGB.

## Touch Layer Options
- Multitouch Film (mutual‑cap): Off‑the‑shelf clear touch foils (7–10″) with FT5x06/Goodix‑class controllers; provides XY multitouch (no per‑finger force).
- Touch + Pressure:
  - Global/Zone force: Corner FSRs or load cells under the panel for overall/zone pressure.
  - Per‑pad FSR grid: Complex and thicker; true poly pressure but higher BOM/assembly.
  - Specialized ICs: Azoteq aXiom or Boréas piezo for richer touch/force; higher cost/complexity.
- MicroFreak‑like feel: Large copper pads sense area/pressure proxy; approximable via coarse capacitive grid + force layer.

## Optics & Mechanics
- Stack: LED panels → spacer → diffuser (opal acrylic ~2–3 mm) → capacitive touch film → top lens (polycarbonate with hard coat).
- Diffusion: Provide enough blur (>5 mm from LED point sources) or use micro‑diffuser films to reduce height.
- Transparency: Touch films dim LEDs slightly; compensate in brightness and diffusion.
- Thermal: Aluminum backplate can act as heatsink; plan airflow if running bright.

## Power & EMI
- Budget: 64×64 RGB worst‑case is large on paper, but multiplexing reduces average current. Still plan a quality 5 V, 10–20 A PSU with headroom.
- Filtering: LC on LED rail, separate return paths, common‑mode chokes on USB if needed.
- Grounding: Star ground with a single tie near the Pi; keep LED currents away from audio grounds; route ribbon cables away from audio/MIDI.

## Performance & Bandwidth
- Bandwidth: 64×64 RGB @ 8‑bit per channel ≈ ~12 KB/frame; at 60 FPS ≈ ~720 KB/s — fine over USB Full Speed.
- Refresh/Depth: True 24‑bit on multiplexed HUB75 is expensive; most stacks deliver visually solid 12–16‑bit with gamma correction.
- Latency: With a co‑processor, LED and touch round‑trip of ~5–15 ms is achievable while keeping Elk’s audio thread clean.

## Firmware & USB Protocol
- Endpoints:
  - LEDs (IN to MCU): Bulk stream for frames or draw commands (blits, sprites, palettes), plus a small control channel.
  - Touch (OUT from MCU): HID or simple packets: contact id, x, y, pressure/area.
- Frame Strategies:
  - Full framebuffer at fixed FPS for animation‑heavy scenes.
  - Dirty‑rectangle/sprites to reduce bandwidth and CPU.
  - Optional scene primitives rendered on MCU (meters, scopes, step lights).
- Rendering Core:
  - Double buffering; gamma LUT; dither if needed; configurable brightness cap.

## Software on Elk/Pi
- Client Library (C/C++ or Rust):
  - Non‑RT thread pushes LED updates via lock‑free queue.
  - Touch events feed the UI/event bus or OSC/MIDI for plugins.
  - Optional clock/beat sync; double‑buffered updates to avoid tearing.
- Integration: Keep GPIO/DMA tricks out of Elk; prefer USB to the co‑processor for portability.

## Using the LED Matrix “as a Display” (Difficulty)
- Basic visuals: Easy — patterns/meters/step lights with existing MCU libs (1–2 days).
- UI widgets + text: Moderate — fonts, sprites, gamma, double buffering, USB protocol (1–2 weeks).
- “Display‑like” polish: Hard — stable 60 FPS, good color depth, touch latency <10 ms, uniform brightness, EMI control (3–6+ weeks).
- Readability: 64×64 excels for meters/waveforms/step grids; text must be big (6×8 or 8×8 px fonts). Avoid dense UIs.

## Prototype Plan (Minimal Viable Build)
1. LEDs: 2× 64×32 P3 HUB75 panels (for 64×64) + RP2040 with HUB75 shield.
2. Touch: 7–8″ off‑the‑shelf capacitive touch foil with I2C/USB controller (multitouch XY).
3. MCU Firmware: HUB75 DMA + touch over I2C, expose USB CDC/HID; implement double buffering + gamma.
4. Pi Client: Small library/service to send draw ops and receive touch; integrate with Elk UI/events.
5. Power/EMI: Separate LED 5 V rail with LC filter; tie grounds at one point; route cables carefully.
6. Optics: 2–3 mm opal acrylic diffuser with ~5+ mm air gap from LEDs; clear top lens.

## Indicative BOM (Prototype)
- LED: 2× HUB75 P3 64×32 RGB panels + ribbon/power cables.
- Driver: RP2040 dev board (e.g., Pico or custom) + HUB75 shield/adapter.
- Touch: 7–8″ capacitive touch foil with controller (FT5x06/Goodix‑class) and USB/I2C breakout.
- Power: 5 V 10 A PSU (start conservative); inline fuse; LC filter module; wiring and connectors.
- Mechanics: Aluminum backplate, standoffs, opal acrylic diffuser, clear top lens, mounting hardware.

## Open Questions
- Size/Resolution: Physical size and target resolution (e.g., 64×64 at ~150–200 mm square)?
- Color: RGB required, or is monochrome acceptable?
- Gestures: Arbitrary multitouch XY vs. discrete pads/keys; is per‑finger pressure essential?
- Visuals: Must‑have animations (meters, scopes, sequencer steps, waveforms, text) that influence driver choice?
- I/O Preference: USB‑connected co‑processor okay, or prefer direct Pi GPIO (higher Elk integration complexity)?

## Next Steps
- Confirm size/resolution and touch requirements.
- Choose LED option (HUB75 vs APA102) and MCU (RP2040 vs Teensy/ESP32).
- I can provide: firmware skeleton (RP2040), USB protocol spec, and a minimal Elk/Pi client to render text, meters, and waveforms.

