# System Architecture

## Overview

Vallamkali Sync Coach is a Physical AI system consisting of three wearable sensing nodes, a central receiver, and a Raspberry Pi running Gemma.

## Layer 1: Sensing Layer

Each wearable band contains an ESP32 and IMU sensor.

```text
Rower Movement
      ↓
IMU
      ↓
Acceleration + Gyroscope Data
      ↓
ESP32
```

## Layer 2: Communication Layer

The wearable ESP32 nodes transmit sensor data using ESP-NOW.

```text
Band 1 ─┐
Band 2 ─┼── ESP-NOW ──► Receiver ESP32
Band 3 ─┘
```

## Layer 3: Raspberry Pi Interface

The receiver transfers data to the Raspberry Pi.

Python is used as an interface layer to:

- Receive sensor data
- Organize data
- Create time windows
- Prepare information for Gemma
- Parse Gemma's response
- Forward commands to the hardware

## Layer 4: Gemma AI Decision Engine

Gemma is the central AI intelligence layer.

Gemma analyzes the relationships between the rowers and determines:

- Synchronization condition
- Timing differences
- Movement mismatch
- Rower requiring correction
- Severity
- Required action

## Layer 5: Physical Action

```text
Gemma
   ↓
Structured AI Decision
   ↓
Python Interface
   ↓
Receiver ESP32
   ↓
ESP-NOW
   ↓
Target Wearable
   ↓
LED / Vibration
```

## Complete Flow

```text
SENSE
─────

3 Rowers
   ↓
IMU Sensors
   ↓
ESP32 Wearables


COMMUNICATE
───────────

ESP-NOW
   ↓
Receiver ESP32
   ↓
Raspberry Pi


THINK
─────

Gemma AI

Analyzes:
- Synchronization
- Timing
- Movement patterns
- Error severity
- Required feedback


ACT
───

Gemma Decision
   ↓
ESP32 Command
   ↓
Wearable Feedback
   ↓
LED / Vibration
```

## Core Design Principle

The ESP32 devices sense and act.

Python connects the software and hardware layers.

Gemma performs the main AI interpretation and decision-making.
