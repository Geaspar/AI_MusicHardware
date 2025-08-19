## Sensor Matrix — IoT Sensor → Modulation Hub (Aug 2025)

A robust sensor aggregation and mapping system that turns IoT inputs (accelerometer, gyro, light, proximity, temperature, heart rate, ambient mic, etc.) into normalized, musical modulation sources. Each sensor passes through conditioning (deadband, smoothing, hysteresis), safety (range/clamp), and mapping (curve, scale, offset) into lanes addressable by the modulation matrix or directly into effect parameters.

### Goals
- Plug‑and‑play expressive control from sensors without jitter or drift
- Per‑sensor conditioning for stable, low‑latency response
- Flexible mapping (curve/scale/offset/invert) and routing to any parameter
- RT‑safe, minimal CPU; easy to extend new sensors

---

## Supported Sensors (examples)
- Accelerometer (x/y/z), Gyro (pitch/yaw/roll)
- Light (lux), Proximity (distance), Temperature/Humidity
- Heart Rate (BPM, HRV), Ambient Microphone (level/crest)
- GPS (speed/heading/altitude)

---

## Per‑Sensor Conditioning
- Deadband: ignore micro jitter within small range
- Smoothing: one‑pole or median filter; adjustable time constant
- Hysteresis: stable toggling for thresholded mappings
- Range Clamp: min/max bounds; automatic re‑centering (for drift)
- Normalization: map raw → [0..1] or [−1..+1]

---

## Mapping & Lanes
- Lanes: up to 8 sensor lanes, each with:
  - Source (sensor channel), Mode (absolute, delta, bipolar)
  - Curve (lin/exp/log/power), Scale (gain), Offset, Invert
  - Slew (ms) and Hold (ms)
  - Routing: Mod Matrix publish (source name) and/or direct target override
- Combiners: sum/mix lanes; gate by conditions (e.g., light<thresh)

---

## Targets & Integration
- Publish lanes as modulation sources: `Sensor1..Sensor8` (and named aliases)
- Direct map helpers for common FX:
  - MotionFormant: accel → formant center/width; gyro → mode morph
  - ProxiSpace Imager: proximity → width & bass mono crossover
  - EnvSense Tilt: light/temp/mic → tone tilt, damping, ducking
  - BioRhythm: heart rate → trem/delay rate; HRV → depth/swing
  - GeoVerb: GPS → pre‑delay/size/width/damping

---

## Parameters (per lane)
- Enable (bool), Source (enum), Mode (enum), Curve (enum)
- Scale (−2 .. +2), Offset (−1 .. +1), Invert (bool)
- Slew (ms): 0 .. 500; Hold (ms): 0 .. 1000
- Publish Name (string) and Destinations (list of param IDs + ranges)

Global
- Sample Rate (for smoothing), Update Rate (Hz), Safety clamps

---

## Architecture
- Sensor Ingest: pull from hardware or API at its native rate; queue to RT thread via lock‑free ring
- RT Conditioning: per‑lane filters applied at audio block rate
- Publisher: write normalized values to modulation matrix atomics and emit to direct targets (smoothed)
- Safety: bounds check; timeouts if sensors stall; fallback to neutral values

---

## Implementation Plan

Phase 0 — Scaffolding (0.5 day)
- Add `include/iot/SensorMatrix.h`, `src/iot/SensorMatrix.cpp`; plumbing to hardware interface; config UI section

Phase 1 — Conditioning & Mapping (1–1.5 days)
- Implement per‑lane filters (deadband/smoothing/hysteresis), curve/scale/offset/invert; slew/hold

Phase 2 — Integration (0.5–1 day)
- Publish to modulation matrix; direct map helpers; preset save/load; example presets

Phase 3 — Extensions (ongoing)
- Additional sensors; auto‑calibration tools; visualization (scope)

---

## Pseudocode
```cpp
struct Lane { Source src; Mode mode; Curve curve; float scale, offset; bool invert; float slewMs, holdMs; float value; };

float condition(float raw, Lane& L) {
  float x = applyDeadband(raw);
  x = smoothOnePole(x);
  x = mapMode(x, L.mode);
  x = applyCurve(x, L.curve);
  x = L.invert ? (1.0f - x) : x;
  x = L.scale * x + L.offset;
  return slewLimit(x, L.slewMs);
}

void processBlock() {
  for (auto& L : lanes) {
    float raw = readSensor(L.src);
    float v = condition(raw, L);
    publish(L.publishName, v);
    for (auto& d : L.destinations) setParameterSmoothed(d.id, lerp(d.min, d.max, v));
  }
}
```

---

## References
- IoT sensor conditioning (deadband/slew/hysteresis)
- Modulation architectures (Vital, modular synths)
