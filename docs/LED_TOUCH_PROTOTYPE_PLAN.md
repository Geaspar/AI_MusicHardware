# LED Matrix + Touch Prototype Plan

## Top Picks

- LED matrix: HUB75 64×64 RGB (2× 64×32 P3 panels)
  - Why: Bright, low cost per pixel, mature libraries, modular sizing, looks great under diffusion.
- Co-processor: RP2040 (Raspberry Pi Pico or equivalent)
  - Why: Proven HUB75 DMA support, deterministic timing, cheap, excellent community libs.
- Touch layer: 7–8″ mutual-cap touch foil (FT5x06/Goodix-class controller)
  - Why: Off‑the‑shelf, true multitouch XY, thin and transparent, straightforward I2C/USB integration.
- Power: Dedicated 5 V, 10–15 A LED supply + filtered 5 V for touch/MCU
  - Why: Keeps LED current noise off audio; headroom for brightness; predictable behavior.
- Host: Raspberry Pi 4 with Elk OS (USB link to RP2040)
  - Why: Keeps real-time audio safe; USB protocol is portable and simple.

## Why These Choices

- Real‑time safety: RP2040 drives LED PWM and scans touch deterministically; Elk OS audio thread never touches tight GPIO/DMA.
- Scalability: HUB75 lets you start at 64×32 and scale to 64×64 or 96×64 without redesigning everything.
- Availability: RP2040 boards, HUB75 panels, and touch foils are easy to source; ecosystem and libraries are proven.
- Legibility: 64×64 with proper diffusion is ideal for meters, steps, waveforms, symbols, and large text.
- Development velocity: Existing HUB75/USB/touch libs reduce bring‑up time.

## Prototype BOM (Indicative)

- LED panels: 2× HUB75 P3 64×32 RGB panels with ribbon + power jumpers.
- HUB75 adapter: RP2040 HUB75 shield/adapter (or simple level‑shifted breakout).
- MCU: 1× RP2040 dev board (Raspberry Pi Pico/Pico W or compatible).
- Touch foil: 7–8″ clear mutual‑cap foil + controller (FT5x46/Goodix; USB or I2C).
- PSU (LEDs): 5 V, 10–15 A regulated supply, inline fuse, wiring, barrel or terminal connector.
- Filters: LC filter module for LED rail; ferrites/common-mode choke for USB if needed.
- Mechanics: Aluminum backplate, standoffs, 2–3 mm opal acrylic diffuser, clear top lens (polycarbonate), mounting hardware.
- Cables: USB‑C/Micro‑USB for MCU, short HUB75 ribbon, 5 V power wiring, I2C/USB for touch.
- Tools: Soldering iron, multimeter, basic hand tools, double‑sided tape/foam for stack.

## Electrical & Power

- Rails: Separate 5 V LED rail; MCU/touch can share Pi 5 V via USB or isolated DC/DC.
- Grounding: Star ground at a single tie near the Pi; keep LED returns off audio ground plane.
- Filtering: LC on LED rail; place filter close to panel input; add ferrites to LED power leads if needed.

## Mechanical Stack

- Order: LED panels → 5–7 mm air gap/spacer → 2–3 mm opal acrylic diffuser → touch foil → clear top lens.
- Notes: Air gap improves blending; use micro‑diffuser film if height must be reduced. Aluminum backplate acts as heatsink and stiffener.

## Firmware Plan (RP2040)

- Drivers: Integrate HUB75 DMA library; configure for 64×64, 12–16‑bit effective depth, high refresh.
- Buffers: Double buffer + gamma LUT; optional ordered dither to smooth gradients.
- Touch: I2C driver for FT5x06/Goodix; normalize coordinates; track contacts with ids.
- USB endpoints:
  - LEDs IN: Bulk endpoint for draw ops (blits/sprites/text) and optional raw frames.
  - Control IN: Small config channel (brightness, gamma, mode).
  - Touch OUT: HID‑like packets {ts, id, x, y, area/pressure}.
- Renderer: Minimal scene primitives on MCU (bars, steps, waveforms); dirty‑rect compositor.
- Diagnostics: FPS counter, max frame time, dropped rects, touch latency stamps.

## Host Software Plan (Elk/Pi)

- Daemon/service: Non‑RT process handling USB I/O; lock‑free queues between UI and USB threads.
- API: Small C++ interface to draw primitives, text, and handle touch callbacks.
- Sync: Optional tempo/beat phase to align visuals to audio buffers; double‑buffer handoff.
- Integration: Map touch gestures to existing event bus and parameter controls; simple JSON layout config.

## USB Protocol Outline

- Header (draw): `uint8 type, uint8 flags, uint16 len, uint16 seq`
- Ops: `SET_MODE, CLEAR, BLIT_RGBA8888(x,y,w,h,pixels), FILL_RECT, DRAW_TEXT(font_id,x,y,color,str), SET_BRIGHTNESS, COMMIT`
- Touch packet: `uint32 ts_us, uint8 n, [id,x(16),y(16),area(8),state] × n`
- Modes: `FRAME_STREAM` (video‑ish), `DRAW_OPS` (efficient UI), `SCENE` (MCU‑rendered primitives)

## Build Steps

- Panel bring‑up:
  - Assemble HUB75 + RP2040 + PSU; flash demo to verify refresh and brightness.
  - Add diffuser and measure flicker visually and with smartphone camera.
- Touch bring‑up:
  - Connect foil; verify multitouch coordinates; apply basic smoothing and edge calibration.
- USB link:
  - Implement CDC/Bulk + HID endpoints; stream test patterns; echo touch events on host.
- Renderer:
  - Add dirty‑rect blits; font rendering (6×8, 8×8); gamma LUT; brightness control.
- Host integration:
  - Create service; expose simple API; render meters, steps, and text from synth state.
- Optics pass:
  - Tune air gap/diffuser; test brightness uniformity; add per‑panel correction if needed.
- Noise pass:
  - Add LC filter; route grounds; test audio path for EMI; add ferrites if necessary.

## Validation & Test Plan

- Refresh/flicker: Stable >200 Hz row refresh; camera test for rolling‑band artifacts.
- Latency: Touch‑to‑visual and host‑to‑visual round‑trip (<15 ms target).
- Bandwidth: Stress‑test dirty‑rects at 60 FPS; verify no USB drops.
- Thermals: 30‑minute full‑white at capped brightness; backplate temp check.
- Audio integrity: Noise floor with LEDs idle vs active; ensure no whine/hash.

## Risks & Mitigations

- EMI into audio: LC filtering, star ground, cable routing; reduce PWM brightness if needed.
- Panel variance: Per‑panel brightness correction; gamma tuning.
- Touch dimming: Compensate with brightness; pick high‑transmittance foil; reassess diffusion stack.
- Complexity creep: Start with draw‑ops; defer full framebuffer until needed.

## Timeline & Milestones

- Week 0–1 (Procure & Prep): Order parts; bench power/bring‑up of one panel and RP2040 demo.
- Week 2 (Firmware Core): HUB75 DMA + double buffer + gamma; USB endpoints; touch readout.
- Week 3 (Host Service): USB comms; draw‑ops API; minimal text/sprites; render demo scenes.
- Week 4 (Integration): Hook to synth state; sequencer steps, meters, waveforms; layout config.
- Week 5 (Optics/EMI): Diffuser tuning, brightness caps; LC filters; latency and audio noise validation.
- Week 6 (Polish): Calibration tools, per‑panel correction, UX tweaks; enclosure draft.

## Responsibilities & Support

- I’ll deliver: RP2040 firmware skeleton, USB protocol, host C++ service/API, repo integration, calibration tools, docs.
- You/Support: Mechanical CAD/enclosure, sourcing panels/touch foils, final EMI validation in your studio.

## Getting Started

- Confirm target size (64×64) and touch foil size (7–8″).
- Order the BOM (I can provide vendor links/part numbers on request).
- Allocate bench space, 5 V PSU, and basic tools.
- Create branches for `firmware-rp2040/` and `host-led-service/` in the repo.

## Sourcing Links (64×64 matrix + 7–8″ touch)

- HUB75 64×32 P3 RGB panels (need 2 for 64×64):
  - ElectroDragon: https://www.electrodragon.com/product/rgb-led-matrix-panel-64x32-3mm-pitch/
  - AliExpress search (many vendors): https://www.aliexpress.com/wholesale?SearchText=HUB75+P3+64x32+RGB
  - Amazon search: https://www.amazon.com/s?k=HUB75+P3+64x32+RGB
- RP2040 HUB75 driver (recommended):
  - Pimoroni Interstate 75 (RP2040 + HUB75 connector): https://shop.pimoroni.com/products/interstate-75
- Alternative driver (if switching MCU):
  - Pixelmatix SmartLED Shield for Teensy 3.x/4.x: https://www.pixelmatix.com/smartled-shield/
- LED power supply (5 V):
  - Mean Well LRS-100-5 (5 V, 18 A): https://www.mouser.com/ProductDetail/Mean-Well/LRS-100-5
  - Mean Well LRS-150-5 (5 V, 30 A) for larger headroom: https://www.mouser.com/ProductDetail/Mean-Well/LRS-150-5
- LC/EMI filtering and ferrites:
  - Murata BNX series DC line filters: https://www.murata.com/en-us/products/emicon-filtr/emi/rf/bnx
  - Ferrite beads/chokes (selector): https://www.digikey.com/en/products/filter/ferrite-beads-and-chips
- 7–8″ capacitive touch foil + controller (USB/I2C):
  - 7″ USB capacitive touch overlay (search): https://www.amazon.com/s?k=7+inch+capacitive+touch+usb+overlay
  - 8″ USB capacitive touch overlay (search): https://www.amazon.com/s?k=8+inch+capacitive+touch+usb+overlay
  - AliExpress search (GT911/FT5x06 controllers): https://www.aliexpress.com/wholesale?SearchText=7+inch+capacitive+touch+USB+panel+GT911
- Diffuser/top materials:
  - Opal acrylic 2–3 mm (supplier category): https://www.theplasticpeople.co.uk/category/opal-acrylic-sheets
  - Polycarbonate clear sheet (top lens): https://www.theplasticpeople.co.uk/category/polycarbonate-sheets

Notes
- Panel pinout and HUB75 scan type can vary; prefer panels known to work with SmartMatrix/Interstate 75. If in doubt, order two from the same batch/vendor.
- Touch overlays often list controller ICs (e.g., GT911, FT5436). Prefer ones that expose USB HID or I2C with a small breakout.
- If you want EU/UK‑specific distributors for everything, I can localize the BOM further.


## Sourcing Links (64×64 matrix + 7–8″ touch)

- HUB75 64×32 P3 RGB panels (need 2 for 64×64):
  - ElectroDragon: https://www.electrodragon.com/product/rgb-led-matrix-panel-64x32-3mm-pitch/
  - AliExpress search (many vendors): https://www.aliexpress.com/wholesale?SearchText=HUB75+P3+64x32+RGB
  - Amazon search: https://www.amazon.com/s?k=HUB75+P3+64x32+RGB
- RP2040 HUB75 driver (recommended):
  - Pimoroni Interstate 75 (RP2040 + HUB75 connector): https://shop.pimoroni.com/products/interstate-75
- Alternative driver (if switching MCU):
  - Pixelmatix SmartLED Shield for Teensy 3.x/4.x: https://www.pixelmatix.com/smartled-shield/
- LED power supply (5 V):
  - Mean Well LRS-100-5 (5 V, 18 A): https://www.mouser.com/ProductDetail/Mean-Well/LRS-100-5
  - Mean Well LRS-150-5 (5 V, 30 A) for larger headroom: https://www.mouser.com/ProductDetail/Mean-Well/LRS-150-5
- LC/EMI filtering and ferrites:
  - Murata BNX series DC line filters: https://www.murata.com/en-us/products/emicon-filtr/emi/rf/bnx
  - Ferrite beads/chokes (selector): https://www.digikey.com/en/products/filter/ferrite-beads-and-chips
- 7–8″ capacitive touch foil + controller (USB/I2C):
  - 7″ USB capacitive touch overlay (search): https://www.amazon.com/s?k=7+inch+capacitive+touch+usb+overlay
  - 8″ USB capacitive touch overlay (search): https://www.amazon.com/s?k=8+inch+capacitive+touch+usb+overlay
  - AliExpress search (GT911/FT5x06 controllers): https://www.aliexpress.com/wholesale?SearchText=7+inch+capacitive+touch+USB+panel+GT911
- Diffuser/top materials:
  - Opal acrylic 2–3 mm (supplier category): https://www.theplasticpeople.co.uk/category/opal-acrylic-sheets
  - Polycarbonate clear sheet (top lens): https://www.theplasticpeople.co.uk/category/polycarbonate-sheets

Notes
- Panel pinout and HUB75 scan type can vary; prefer panels known to work with SmartMatrix/Interstate 75. If in doubt, order two from the same batch/vendor.
- Touch overlays often list controller ICs (e.g., GT911, FT5436). Prefer ones that expose USB HID or I2C with a small breakout.
- If you want EU/UK‑specific distributors for everything, I can localize the BOM further.



## JUCE Adaptation Plan for Elk OS

Goal
- Wrap the existing synth into a JUCE AudioProcessor that builds as a desktop Standalone for iteration and as a plugin/binary for Elk’s host, reusing your DSP/engine intact.

Why JUCE (for Elk)
- Portable audio/MIDI I/O with minimal platform quirks.
- Same codebase for Standalone (fast dev loop) and plugin (Elk deployment).
- Clean parameter/state model via AudioProcessorValueTreeState (APVTS).
- CMake-based workflow aligns with Elk guidance; fewer integration surprises.

Target Shape
- Audio: `AIHardwareAudioProcessor` calls your engine in `prepareToPlay/processBlock/releaseResources` with strict RT-safety.
- Parameters: APVTS exposes canonical parameter IDs; atomics/lock-free mirroring into the engine.
- State: Preset save/restore via `getStateInformation`/`setStateInformation`.
- MIDI/MPE: JUCE MPE mapped to your voice manager.
- Hardware I/O (LED/touch): Non-RT thread or service (USB/OSC) — never in the audio thread.

Repo Changes
- New `clients/juce/` (or `juce/`) with:
  - `CMakeLists.txt` using `juce_add_plugin(...)` (Standalone + plugin).
  - `AIHardwareAudioProcessor.{h,cpp}` thin wrapper around your engine.
  - `ParameterLayout.{h,cpp}` APVTS parameter defs + engine mapping.
  - Optional `HardwareBridge.{h,cpp}` for LED/touch service.
  - Optional minimal `AudioProcessorEditor` for desktop meters/debug.

Real-Time Safety
- No allocations/logging in `processBlock`.
- Use atomics or lock-free queues for parameter updates.
- Pre-allocate temp buffers; linear MIDI parsing.

Build Strategy
- Desktop: Standalone app on macOS/Linux for fast iteration.
- Elk: Cross-compile per Elk’s toolchain (VST3/LV2/hosted AudioProcessor per Elk’s preference).

Plan & Timeline (estimate)
- Week 0 (Decisions/Scaffold): Confirm Elk plugin target; add JUCE via FetchContent/submodule; scaffold target.
- Week 1 (Audio Core): Wire `processBlock` to your engine; verify audio/MIDI; basic tests.
- Week 2 (Params/State): APVTS mapping for priority params; smoothing; preset save/restore.
- Week 3 (MPE/Multi-timbral): Enable MPE; verify per-note controls; channel/part mapping.
- Week 4 (Hardware Bridge, optional): Non-RT LED/touch link via USB/OSC; map gestures to parameters/events.
- Week 5 (Packaging): Cross-compile for Elk; runtime test; latency/CPU/XRUN checks.
- Week 6 (Polish): Complete parameter coverage, preset browser glue, docs.

Effort Estimate
- 4–6 weeks total elapsed for a solid, shippable first pass.
- ~8–12 engineer-days for core wrapper + params + state (Weeks 0–2).
- ~3–5 days for MPE/multi-timbral (Week 3) depending on coverage.
- ~3–5 days for Elk cross-compile, testing, and polishing (Weeks 5–6).
- Hardware bridge (optional): ~3–5 days baseline, independent of Elk.

Risks & Mitigations
- RT stalls from parameter churn: use atomics, precomputed curves; push only final values into RT path.
- Elk toolchain quirks: keep CMake simple, pin JUCE version known good on ARM/Linux; provide reproducible presets.
- GUI on Elk: keep headless/minimal; rely on hardware/OSC for control.

Does this change the Elk OS choice?
- No — if we adopt JUCE, Elk remains a strong option: real-time kernel, predictable scheduling, and a clean deployment story.
- Consider alternatives only if you need heavy GPU UI or bespoke kernel/driver features; otherwise Elk reduces ops/integration overheads.

Responsibilities
- I will implement: JUCE scaffold, AudioProcessor wrapper, APVTS mapping, MIDI/MPE, state/presets, desktop debug UI (minimal), Elk build instructions.
- You/Support: Confirm Elk plugin target, provide cross-compile environment access; mechanical/CAD remains out-of-scope.

Immediate Next Steps
- Confirm Elk’s preferred plugin format (VST3 vs LV2 vs hosted AudioProcessor).
- I can scaffold `clients/juce/` and wire `processBlock` to your engine next.
