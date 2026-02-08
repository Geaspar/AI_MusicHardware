# Device Deployment Plan (Linux)

This document outlines the plan to deploy AIMusicHardware on a Linux‑based device where MQTT is integral to operation.

## Build Profiles
- macOS/dev: MQTT optional (kept flexible for local development).
- Linux/device: MQTT required. CMake should fail if Paho C/C++ is missing.

### CMake Preset (to add later)
- Name: `device-linux`
- Behavior:
  - `find_package` Paho C and C++ REQUIRED.
  - Define `HAVE_PAHO_MQTT` and disallow mock paths.
  - Emit a clear error if libraries are missing.

## MQTT Requirements
- Libraries: Eclipse Paho MQTT C (libpaho-mqtt3a) and C++ (libpaho-mqttpp3).
- Broker: Local (e.g., Mosquitto) or reachable on LAN.
- Credentials: Username/password or certificates as required by deployment.

## Startup Health Check
- On startup, verify connectivity to the configured broker before enabling UI/processing:
  - Attempt connect with timeout and backoff.
  - Subscribe to a small test topic and publish a probe message.
  - If unavailable: present a clear UI notice and keep retrying in background.

## Packaging on Device
- Provide install steps or a one‑shot script to install Paho and dependencies.
- Example (Ubuntu/Debian):
  - `sudo apt install build-essential cmake git libasound2-dev libsdl2-dev libsdl2-ttf-dev librtaudio-dev librtmidi-dev`
  - Build Paho from source if distro packages are insufficient (documented flags).
- Broker setup (Mosquitto):
  - `sudo apt install mosquitto mosquitto-clients`
  - Configure auth, persistence, and service enablement.

## Service Management (Optional)
- Provide a systemd unit to auto‑start the app at boot, restart on failure:
  - `Restart=on-failure`, `RestartSec=2s`
  - Optionally run after `network-online.target` and `mosquitto.service`.
  - Consider setcap or RT priority limits for low‑latency audio.

## Diagnostics & Tests
- Add a small integration test binary for device:
  - Publishes and subscribes round‑trip to assert broker functionality.
  - Exits non‑zero if checks fail; useful for CI and field diagnostics.
- Runtime telemetry: periodic MQTT health status topic (latency, reconnect count).

## Security & Hardening
- Support username+password and TLS certs.
- Avoid blocking operations on the audio thread; MQTT runs off the audio callback.
- Validate and sanitize inbound topics/JSON payloads.

## Next Steps
- Add `device-linux` CMake preset requiring Paho.
- Implement startup health check and user feedback.
- Provide systemd service template and broker quickstart doc.
- Gate release builds on MQTT integration tests.

