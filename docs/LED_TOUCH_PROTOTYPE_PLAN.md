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

