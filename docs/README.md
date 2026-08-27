# Vallamkali Sync Coach

An AI-powered wearable synchronization coaching system inspired by Kerala's traditional Vallamkali boat racing.

## Problem

In Vallamkali, synchronization between rowers is critical. During training, identifying which rower is consistently early, late, out of rhythm, or showing a mismatched movement pattern can be difficult.

## Solution

Vallamkali Sync Coach uses wearable IMU-based sensor bands to capture the movements of multiple rowers.

The sensor data is transmitted to a Raspberry Pi, where Gemma acts as the AI decision engine.

Gemma analyzes the relationship between the rowers' movements and determines:

- Synchronization status
- Timing mismatch
- Movement pattern mismatch
- Which rower requires correction
- Severity of the error
- Required physical feedback

The decision is sent back to the corresponding wearable band, which provides LED and optional vibration feedback.

## Sense → Think → Act

### SENSE

Each rower wears:

- ESP32
- IMU sensor
- RGB LED
- Optional vibration motor
- Battery

The IMU captures acceleration, angular velocity, motion patterns, and timing information.

### THINK

Gemma running on the Raspberry Pi is the main AI decision engine.

Python acts as the interface layer to:

- Receive sensor data
- Organize data into time windows
- Format information for Gemma
- Send Gemma's decisions to the hardware

### ACT

Gemma generates a structured decision such as:

```json
{
  "rower": 2,
  "status": "OUT_OF_SYNC",
  "issue": "LATE_STROKE",
  "severity": "HIGH",
  "action": "RED_VIBRATE"
}
```

The decision is transmitted to the target wearable.

- Green LED: Good synchronization
- Yellow LED: Minor mismatch
- Red LED: Major mismatch
- Vibration: Immediate correction required

## System Architecture

```text
3 IMU Wristbands
        ↓
      ESP-NOW
        ↓
  Receiver ESP32
        ↓
   Raspberry Pi
        ↓
Python Interface Layer
(Collect + Format Data)
        ↓
      GEMMA AI
(Analysis + Decision)
        ↓
 Structured AI Output
        ↓
Python Communication
        ↓
  Receiver ESP32
        ↓
      ESP-NOW
        ↓
LED / Vibration Feedback
```

## Gemma as the AI Decision Engine

Gemma determines:

- Overall synchronization status
- Relative timing mismatch
- Movement pattern mismatch
- Which rower needs correction
- Error severity
- Required physical feedback

## Hardware

### Wearable Bands

Each band contains:

- ESP32
- 6-DOF IMU
- RGB LED
- Optional vibration motor
- Battery

### Central Processing

- Raspberry Pi 4
- Gemma
- Python communication interface

### Communication

- ESP-NOW between wearable ESP32 devices
- Receiver ESP32 connected to Raspberry Pi

## Physical AI Loop

```text
PHYSICAL MOVEMENT
       ↓
IMU SENSING
       ↓
ESP32
       ↓
RASPBERRY PI
       ↓
GEMMA AI
       ↓
DECISION
       ↓
ESP-NOW
       ↓
WEARABLE FEEDBACK
       ↓
ROWER CORRECTS MOVEMENT
```

## Project Goal

The system acts as an AI-powered training assistant that provides individual, data-driven feedback to help rowers improve synchronization.

## Cultural Relevance

Vallamkali Sync Coach combines:

- Kerala's Vallamkali tradition
- Wearable embedded systems
- Real-time motion sensing
- Edge AI with Gemma
- Physical feedback
