# Vallamkali-CrewSync Documentation

## Vallamkali Sync Coach

Vallamkali-CrewSync is a Physical AI wearable system designed to help a team of Vallamkali rowers improve synchronization.

Three rowers wear motion-sensing bands. Each band captures movement data using an IMU and sends it to a central system. Gemma running on a Raspberry Pi analyzes the relationship between the rowers' movements and decides the required feedback.

## Core Physical AI Loop

```text
SENSE → THINK → ACT
```

```text
Physical Movement
        ↓
IMU Sensors
        ↓
ESP32 Wearable Bands
        ↓
Communication
        ↓
Raspberry Pi
        ↓
Gemma AI Decision Engine
        ↓
Feedback Command
        ↓
LED / Vibration
        ↓
Rower Adjusts Movement
```

## System Roles

### ESP32 Wearable Bands

The wearable nodes are responsible for:

- Reading IMU data
- Sending sensor data
- Receiving feedback commands
- Controlling the RGB LED
- Controlling the vibration motor

### Raspberry Pi

The Raspberry Pi hosts the AI system and communication interface.

Python is used to:

- Receive incoming sensor data
- Organize data into analysis windows
- Format data for Gemma
- Send Gemma output back to the receiver

### Gemma

Gemma is the main AI decision engine.

Gemma determines:

- Overall synchronization condition
- Which rower is mismatched
- Relative timing mismatch
- Movement pattern mismatch
- Severity of the issue
- Required feedback action

## Documentation

| File | Description |
|---|---|
| [system_architecture.md](system_architecture.md) | Complete system architecture and Sense → Think → Act flow |
| [packet_protocol.md](packet_protocol.md) | Sensor and feedback communication packet structure |

## Feedback States

| Status | LED | Vibration |
|---|---|---|
| GOOD | Green | No |
| MINOR_MISMATCH | Yellow | No |
| MAJOR_MISMATCH | Red | Optional |
| IMMEDIATE_CORRECTION | Red | Yes |

## Design Principle

```text
ESP32 = Sense + Act
Python = Interface + Communication
Gemma = AI Analysis + Decision
```
