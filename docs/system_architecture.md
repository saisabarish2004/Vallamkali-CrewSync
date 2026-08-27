# System Architecture

## 1. Overview

Vallamkali-CrewSync is a distributed Physical AI system consisting of three wearable sensing bands, a central communication receiver, and a Raspberry Pi running Gemma.

The system follows a continuous:

```text
SENSE → THINK → ACT
```

loop.

## 2. Complete Architecture

```text
                         SENSE

        ┌──────────────┬──────────────┬──────────────┐
        │              │              │
        ▼              ▼              ▼

      BAND 1         BAND 2         BAND 3
    ESP32 + IMU    ESP32 + IMU    ESP32 + IMU
        │              │              │
        └──────────────┼──────────────┘
                       │
                    ESP-NOW
                       │
                       ▼

                 RECEIVER ESP32
                       │
                       │ USB / Serial
                       ▼

                  RASPBERRY PI
                       │
                       ▼

               PYTHON INTERFACE
          Receive + Organize + Format
                       │
                       ▼

                    GEMMA AI
              MAIN DECISION ENGINE
                       │
             ┌─────────┼─────────┐
             ▼         ▼         ▼

         Analyze     Identify   Decide
         Sync        Rower      Action
                       │
                       ▼

              STRUCTURED OUTPUT
                       │
                       ▼

               PYTHON INTERFACE
                       │
                       ▼

                 RECEIVER ESP32
                       │
                    ESP-NOW
                       │
        ┌──────────────┼──────────────┐
        ▼              ▼              ▼

      BAND 1         BAND 2         BAND 3
        │              │              │
        ▼              ▼              ▼

       LED          LED/Vibration   LED
```

## 3. Sensing Layer

Each rower wears a band containing:

- ESP32
- IMU sensor
- RGB LED
- Optional vibration motor
- Battery and power circuitry

The IMU provides motion information such as:

- Accelerometer readings: Ax, Ay, Az
- Gyroscope readings: Gx, Gy, Gz
- Timestamp

The system observes relative movement between rowers rather than relying only on one rower's movement.

## 4. Communication Layer

The wearable bands communicate with the central receiver using ESP-NOW.

```text
Band 1 ─┐
Band 2 ─┼──── ESP-NOW ────► Receiver ESP32
Band 3 ─┘
```

The receiver provides a bridge between the ESP32 network and the Raspberry Pi.

## 5. Data Interface Layer

Python runs on the Raspberry Pi as the interface between hardware and Gemma.

Python performs:

- Data reception
- Data buffering
- Time-window organization
- Formatting sensor information
- Sending the analysis request to Gemma
- Parsing Gemma's structured response
- Forwarding the selected action to the receiver ESP32

Python is not intended to independently decide which rower is wrong or what feedback action should be used.

## 6. Gemma AI Decision Layer

Gemma is the central intelligence of the system.

Gemma receives organized movement information from all three rowers and determines:

1. Overall synchronization status
2. Which rower differs from the group
3. Type of mismatch
4. Severity of the mismatch
5. Required physical feedback

Example output:

```json
{
  "sync_score": 72,
  "rowers": [
    {
      "id": 1,
      "status": "GOOD",
      "issue": "NONE",
      "severity": "LOW",
      "action": "GREEN"
    },
    {
      "id": 2,
      "status": "MISMATCH",
      "issue": "LATE_STROKE",
      "severity": "HIGH",
      "action": "RED_VIBRATE"
    },
    {
      "id": 3,
      "status": "GOOD",
      "issue": "NONE",
      "severity": "LOW",
      "action": "GREEN"
    }
  ]
}
```

## 7. Action Layer

Gemma's decision is converted into a hardware command.

```text
Gemma Decision
      ↓
Python Interface
      ↓
Receiver ESP32
      ↓
ESP-NOW
      ↓
Target Wearable
      ↓
RGB LED / Vibration
```

## 8. Feedback Logic

| Gemma Action | Physical Response |
|---|---|
| GREEN | Green LED |
| YELLOW | Yellow LED |
| RED | Red LED |
| RED_VIBRATE | Red LED + vibration |

## 9. Physical AI Demonstration

The live demonstration should visibly prove the complete loop:

```text
One rower changes movement
          ↓
IMU detects the physical change
          ↓
Data reaches Raspberry Pi
          ↓
Gemma analyzes the group
          ↓
Gemma selects the mismatched rower
          ↓
Feedback command is generated
          ↓
That rower's band changes state
```

This demonstrates a genuine:

```text
Sense → Think → Act → Sense
```

Physical AI feedback loop.
