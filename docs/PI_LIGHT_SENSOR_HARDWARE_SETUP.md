# Raspberry Pi Light Sensor Hardware Setup

Last updated: 2026-05-02

This guide is for the weekend proof-of-concept:
- Raspberry Pi 4 or 5
- ADS1115 breakout
- LDR
- 10k resistor
- breadboard
- jumper wires

Target result:
- the Pi sees the ADS1115 on I2C
- the LDR changes the raw ADC reading
- the JUCE host reads `/tmp/light.txt`
- light audibly moves the synth filter cutoff

## 1. Parts Checklist

You need:
- Raspberry Pi 4 or 5 with power supply and microSD
- monitor, keyboard, and network access or SSH
- audio output that already works on the Pi
- 1 x ADS1115 breakout board
- 1 x LDR / photoresistor
- 1 x 10k ohm resistor
- 1 x solderless breadboard
- male-to-male jumper wires

Nice to have:
- multimeter
- phone torch for testing
- labels or masking tape for wires

## 2. Safety and Setup Rules

Before wiring:
- fully power the Pi off
- unplug USB power from the Pi
- do not wire anything while the Pi is powered

Electrical rules for this build:
- use the Pi `3.3V` pin, not `5V`
- use a Pi `GND` pin for all grounds
- do not connect the LDR divider to `5V`
- do not connect any ADS1115 analog input above its supply voltage

For this guide, everything runs at `3.3V`.

## 3. Raspberry Pi Pin Reference

Use these physical header pins on the Raspberry Pi:

- Pin `1` = `3.3V`
- Pin `3` = `SDA1` / GPIO2
- Pin `5` = `SCL1` / GPIO3
- Pin `6` = `GND`

That is enough for the first demo.

## 4. Understand the Circuit First

You are building two small things:

1. The I2C connection from the Pi to the ADS1115
- Pi `3.3V` powers the ADS1115
- Pi `SDA` and `SCL` talk to the ADS1115

2. A light-dependent voltage divider into ADS1115 channel `A0`
- one side of the divider goes to `3.3V`
- the other side goes to `GND`
- the middle node changes voltage depending on light
- the ADS1115 reads that middle node on `A0`

The LDR is not polarized, so it can be inserted either way around.
The 10k resistor is also not polarized.

## 4a. Simple Wiring Diagram

Use this as the quick visual reference:

```text
Raspberry Pi                     ADS1115
------------                    -------
Pin 1  (3.3V)  ----------------> VDD
Pin 6  (GND)   ----------------> GND
Pin 3  (SDA1)  ----------------> SDA
Pin 5  (SCL1)  ----------------> SCL
GND            ----------------> ADDR

Light sensor divider
--------------------
3.3V ---- LDR ----+----> ADS1115 A0
                  |
                10k
                  |
                 GND
```

The `+` node in the divider is the only analog point you feed into `A0`.

## 5. Identify the ADS1115 Pins

Most ADS1115 breakouts expose pins similar to:

- `VDD` or `VIN`
- `GND`
- `SCL`
- `SDA`
- `ADDR`
- `ALERT` or `ALRT`
- `A0`
- `A1`
- `A2`
- `A3`

For this proof-of-concept:
- use `VDD` or `VIN` with Pi `3.3V`
- use `GND`
- use `SCL`
- use `SDA`
- tie `ADDR` to `GND` so the address becomes `0x48`
- leave `ALERT` or `ALRT` unconnected
- use analog input `A0`

If your breakout labels `VDD` and `VIN` separately, use the pin the board documentation says is the logic supply. If unsure and the board is a simple hobby breakout, `VDD` is usually the safer choice.

## 6. Place the ADS1115 on the Breadboard

1. Put the breadboard on the table with the long power rails at the top and bottom.
2. Insert the ADS1115 breakout so each pin goes into its own row.
3. If the breakout has two rows of pins, place it so the pins do not short together across one breadboard row.
4. Leave enough empty rows next to it for the LDR and resistor wiring.

Do a visual check:
- every ADS1115 pin should be in a different breadboard row
- no two adjacent pins should accidentally share the same row unless you intended that

## 7. Wire the Pi to the ADS1115

With the Pi still powered off, make these four core connections:

1. Pi pin `1` -> ADS1115 `VDD` or `VIN`
2. Pi pin `6` -> ADS1115 `GND`
3. Pi pin `3` -> ADS1115 `SDA`
4. Pi pin `5` -> ADS1115 `SCL`

Then set the I2C address:

5. ADS1115 `ADDR` -> ADS1115 `GND`

Do not connect:
- `ALERT` / `ALRT`
- `A1`
- `A2`
- `A3`

At this point, the ADS1115 power and bus wiring should be complete.

## 8. Build the Light Sensor Voltage Divider

You now need one middle node that goes to ADS1115 `A0`.

Wire it like this:

1. Put one leg of the LDR into an empty row.
2. Put the other leg of the LDR into a different empty row.
3. Connect one LDR leg to Pi `3.3V`.
4. Choose the other LDR leg as the divider node.
5. Run a jumper from that divider node to ADS1115 `A0`.
6. Insert one leg of the 10k resistor into the same divider-node row.
7. Insert the other leg of the 10k resistor into a new row.
8. Connect that second resistor leg to `GND`.

That creates:

`3.3V -> LDR -> divider node -> 10k resistor -> GND`

And:

`divider node -> ADS1115 A0`

The divider node is the only point that should touch `A0`.

## 9. Visual Inspection Before Powering On

Check all of these slowly:

- Pi pin `1` goes only to ADS1115 power and the LDR side that should be at `3.3V`
- Pi pin `6` goes to ADS1115 `GND` and the resistor side that should be at `GND`
- Pi pin `3` goes to ADS1115 `SDA`
- Pi pin `5` goes to ADS1115 `SCL`
- ADS1115 `ADDR` is tied to `GND`
- ADS1115 `A0` goes to the divider node
- the divider node is where the LDR and resistor meet
- the divider node is not directly shorted to `3.3V`
- the divider node is not directly shorted to `GND`
- nothing is connected to Pi `5V`

If you have a multimeter:
- with power still off, confirm there is no hard short between `3.3V` and `GND`

## 10. Power On and Verify I2C

Now power the Pi on.

Enable I2C if you have not already:

```bash
sudo raspi-config
```

In `Interface Options`, enable `I2C`, then reboot if prompted.

Install the minimum tools:

```bash
sudo apt update
sudo apt install -y i2c-tools python3-smbus
```

Probe the I2C bus:

```bash
sudo i2cdetect -y 1
```

Expected result:
- you should see device address `48`

If you do not see `48`:
- power off and re-check `SDA`, `SCL`, `VDD`, `GND`, and `ADDR`
- make sure `ADDR` really goes to `GND`
- confirm you used Pi pins `3` and `5`, not GPIO numbers by mistake

## 11. Confirm the JUCE App Responds Before Testing the Sensor

Before testing the actual light sensor, make sure the synth side is already working.

Build the standalone:

```bash
cmake -S . -B build -DENABLE_JUCE_TARGETS=ON
cmake --build build --target AIMH_JuceStandalone -j"$(nproc)"
```

Run it:

```bash
AIMH_SENSOR_MODE=external \
AIMH_LIGHT_FILE=/tmp/light.txt \
"build/juce/AIMH_JuceStandalone_artefacts/Debug/AIMH JUCE Standalone"
```

Inside the app:
- set `Sensor Src` to `External`
- set `Light->` to `Cutoff`
- play a held note or start a simple sequence

Now prove file input works:

```bash
printf "0.10\n" > /tmp/light.txt
printf "0.90\n" > /tmp/light.txt
```

If you do not hear a clear cutoff change here, stop and fix the app setup first. Do not debug hardware yet.

## 12. Run the Light Bridge

When the app-side smoke test works, run the bridge:

```bash
python3 tools/pi_ads1115_light_bridge.py \
  --bus 1 \
  --address 0x48 \
  --channel 0 \
  --output /tmp/light.txt \
  --min-raw 2000 \
  --max-raw 22000
```

You should see status lines like:
- `raw=...`
- `norm=...`
- `filtered=...`

Now test the sensor physically:

1. Look at the reported `raw` value in room light.
2. Cover the LDR with your hand and note the new value.
3. Shine a phone torch on the LDR and note the new value.
4. Listen for the filter changing while the synth is playing.

## 13. Tune the Response

If the filter moves in the wrong direction:

```bash
python3 tools/pi_ads1115_light_bridge.py \
  --output /tmp/light.txt \
  --invert
```

If the motion is too jumpy:

```bash
python3 tools/pi_ads1115_light_bridge.py \
  --output /tmp/light.txt \
  --smoothing 0.1
```

If the useful range is too narrow:
- note the darkest and brightest `raw` values you actually see
- set `--min-raw` near the dark end
- set `--max-raw` near the bright end

Example:

```bash
python3 tools/pi_ads1115_light_bridge.py \
  --output /tmp/light.txt \
  --min-raw 6000 \
  --max-raw 18000
```

## 14. Fast Troubleshooting by Symptom

Symptom: `i2cdetect` shows nothing at `48`
- ADS1115 power is wrong
- `SDA` and `SCL` are swapped
- `ADDR` is not tied to `GND`
- I2C is not enabled on the Pi

Symptom: bridge runs but `raw` barely changes
- the LDR and resistor are not actually forming a divider
- `A0` is not connected to the divider node
- ambient light range is too small
- the resistor value is poor for that particular LDR

Symptom: bridge runs but `raw` is stuck near zero or full scale
- divider node is effectively shorted to `GND` or `3.3V`
- `A0` is wired to the wrong breadboard row
- the LDR or resistor lead is one row off

Symptom: app does not react but bridge values change
- `AIMH_SENSOR_MODE=external` was not used
- `/tmp/light.txt` is not the file the app is reading
- `Light->` is not set to `Cutoff`
- the synth is not actually producing a sustained sound to hear the change

Symptom: response is noisy
- increase smoothing
- keep your hand from casting inconsistent shadows
- stabilize the breadboard and wires
- reduce strong ambient flicker sources

## 15. Recommended Filming Setup

For a clean video:

1. Put the Pi and breadboard on a stable surface.
2. Frame the shot so the LDR is visible.
3. Keep the JUCE app or speaker audible in the recording.
4. Start a sustained note or looping pattern.
5. Move from dark to bright in a slow, obvious way.
6. Do one clean take before changing wiring or tuning again.

## 16. If You Want the Short Version

If you already know breadboards and just want the minimal wiring:

- Pi pin `1` -> ADS1115 `VDD`
- Pi pin `6` -> ADS1115 `GND`
- Pi pin `3` -> ADS1115 `SDA`
- Pi pin `5` -> ADS1115 `SCL`
- ADS1115 `ADDR` -> `GND`
- `3.3V -> LDR -> divider node -> 10k resistor -> GND`
- divider node -> ADS1115 `A0`

Then:

- run `sudo i2cdetect -y 1`
- expect `48`
- run the JUCE standalone in external mode
- run `tools/pi_ads1115_light_bridge.py`
