# System Architecture

## Flow
Rower Bands → ESP-NOW → Central ESP32 → UART → Raspberry Pi 5 → Gemma
Gemma Decision → UART → Central ESP32 → ESP-NOW → Target Band → LED + Vibration

## Sense
Each wearable:
1. Reads the ISM330 IMU.
2. Detects strokes.
3. Creates a compact stroke summary.
4. Sends it using ESP-NOW.

## Think
The Pi collects 3 strokes from each rower.

Gemma:
- compares all rowers,
- finds the outlier,
- identifies early or late behavior,
- decides whether feedback is required.

## Act
The target wearable activates LED and vibration simultaneously.

## Intelligence Division
Preprocessing:
- collect
- detect stroke
- summarize
- organize

Gemma:
- analyze
- compare
- identify
- decide

ESP32:
- execute the physical command
