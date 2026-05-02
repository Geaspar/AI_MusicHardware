# Weekend Pi Light POC

Last updated: 2026-05-02

This is the shortest path to a filmable hardware proof-of-concept:
- Raspberry Pi
- local audio output
- one physical light sensor
- one synth parameter change you can hear immediately

The ESP32 / MQTT direction is intentionally out of scope for this weekend.

If you need the full breadboard-level setup steps, use:
- `docs/PI_LIGHT_SENSOR_HARDWARE_SETUP.md`

## Goal

Get a real light change to audibly move a synth parameter using the existing JUCE host path.

Recommended first mapping:
- Light -> Filter Cutoff

Why:
- easiest to explain visually
- easiest to tune musically
- lower risk than distance on a first pass

## Hardware

- Raspberry Pi 4 or 5
- audio output that already works on the Pi
- ADS1115 breakout
- LDR
- 10k resistor
- breadboard + jumper wires

## Wiring

ADS1115 to Pi I2C:
- Pi pin 1 -> ADS1115 VDD
- Pi pin 6 -> ADS1115 GND
- Pi pin 3 -> ADS1115 SDA
- Pi pin 5 -> ADS1115 SCL
- ADS1115 ADDR -> GND for address `0x48`

LDR divider to ADS1115 A0:
- 3.3V -> LDR -> divider node
- divider node -> ADS1115 A0
- divider node -> 10k resistor -> GND

## Pi Setup

Enable I2C, then install the minimal packages:

```bash
sudo raspi-config
sudo apt update
sudo apt install -y i2c-tools python3-smbus
```

Verify the ADS1115 is visible:

```bash
sudo i2cdetect -y 1
```

You should see `48`.

## JUCE Host Setup

Build the JUCE standalone on the Pi:

```bash
cmake -S . -B build -DENABLE_JUCE_TARGETS=ON
cmake --build build --target AIMH_JuceStandalone -j"$(nproc)"
```

On Linux, JUCE typically puts the executable here:

```bash
build/juce/AIMH_JuceStandalone_artefacts/Debug/AIMH\ JUCE\ Standalone
```

If you are not sure, locate it explicitly:

```bash
find build -path '*AIMH_JuceStandalone_artefacts/Debug/*' -type f | grep 'AIMH JUCE Standalone'
```

Run the standalone in external sensor mode, pointing it at the light file:

```bash
AIMH_SENSOR_MODE=external \
AIMH_LIGHT_FILE=/tmp/light.txt \
"build/juce/AIMH_JuceStandalone_artefacts/Debug/AIMH JUCE Standalone"
```

Inside the app:
- set `Sensor Src` to `External`
- set `Light->` to `Cutoff`
- start with a simple sustained note or repeated sequence

## External-Path Smoke Test

Before wiring the real sensor, prove that the JUCE app reacts to file input:

```bash
printf "0.10\n" > /tmp/light.txt
printf "0.90\n" > /tmp/light.txt
```

For a repeating test sweep:

```bash
while true; do
  printf "0.10\n" > /tmp/light.txt
  sleep 1
  printf "0.90\n" > /tmp/light.txt
  sleep 1
done
```

If that does not audibly move the filter, fix the app-side mapping before debugging I2C.

## Sensor Bridge

Use the new bridge script:

```bash
python3 tools/pi_ads1115_light_bridge.py \
  --bus 1 \
  --address 0x48 \
  --channel 0 \
  --output /tmp/light.txt \
  --min-raw 2000 \
  --max-raw 22000
```

If the musical response goes backwards, add `--invert`.

If the motion is too jumpy, lower the smoothing response:

```bash
python3 tools/pi_ads1115_light_bridge.py \
  --output /tmp/light.txt \
  --smoothing 0.1
```

If the response is too small or too compressed:
- look at the printed `raw=` values
- adjust `--min-raw` and `--max-raw`

Example:

```bash
python3 tools/pi_ads1115_light_bridge.py \
  --output /tmp/light.txt \
  --min-raw 6000 \
  --max-raw 18000
```

## Fast Tuning Procedure

1. Run the bridge and observe the printed raw values in normal room light.
2. Shine a phone torch on the LDR and note the new raw range.
3. Set `--min-raw` near the darkest useful value.
4. Set `--max-raw` near the brightest useful value.
5. Adjust `--invert` if bright light closes the filter instead of opening it.
6. Adjust app cutoff and resonance so the motion is obvious on camera.

## What to Film

- Start with the synth playing a held note or simple loop
- Show the sensor and your hand / torch in frame
- Show the audible filter movement from dark to bright
- Capture one clean take before trying to improve architecture

## If You Hit Trouble

If `i2cdetect` does not show `48`:
- re-check SDA/SCL
- check power and ground
- verify I2C is enabled

If the app does not react:
- confirm `/tmp/light.txt` is changing
- confirm `Sensor Src` is `External`
- confirm `Light->` is `Cutoff`
- verify the standalone was launched with `AIMH_SENSOR_MODE=external`
- run the smoke test above before reconnecting the ADS1115

If the response is unstable:
- increase `--smoothing`
- narrow the `--min-raw` / `--max-raw` range
- reduce ambient light variation

## After the Video

Once the local Pi light demo works reliably:
- add distance as a second sensor
- replace the standalone-first path with the Elk plugin deployment path
- revisit ESP32 / MQTT only after the local proof is already solid
