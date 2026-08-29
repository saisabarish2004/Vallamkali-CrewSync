# Vallamkali AI Sync Coach

## Summary
A Physical AI wearable prototype for Vallamkali synchronization training.

Three rowers wear IMU-based ESP32 bands. Each band detects three strokes and sends compact stroke summaries through ESP-NOW to a Central ESP32. The Central ESP32 forwards combined data to a Raspberry Pi 5 through UART.

A local Gemma model compares all three rowers, identifies an out-of-sync rower, determines whether the error is early or late, and returns a short decision. The command travels back to the target wearable, where the LED and vibration motor activate simultaneously.

## Sense → Think → Act
IMU → Wearable ESP32 → ESP-NOW → Central ESP32 → UART → Raspberry Pi → Gemma
→ UART → Central ESP32 → ESP-NOW → Target Band → LED + Vibration

## Core Decision Window
- 3 rowers
- 3 strokes per rower
- 9 compact stroke summaries per analysis cycle

Gemma is not told which rower is wrong. It compares all three rowers and identifies the outlier.

## Target Latency
After the third stroke:
- Preferred: under 1 second
- Acceptable prototype target: under 2 seconds

## Hardware
- Raspberry Pi 5 × 1
- XIAO ESP32 × 3 preferred for wearable nodes
- ESP32 DevKit V1 × 1 for central hub
- Spare ESP32 × 1
- ISM330 6-DOF IMU × 4 preferred
- Vibration motor × 4
- RGB LED × 3
- Motor drivers/transistors as required
- OLED optional

## Important Rule
Do not send raw high-frequency IMU streams to Gemma.
Send compact stroke summaries. Gemma performs the comparison and decision.
