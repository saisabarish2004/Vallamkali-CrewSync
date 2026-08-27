# Packet Protocol

## 1. Purpose

This document defines the communication structure between:

- Wearable ESP32 bands
- Receiver ESP32
- Raspberry Pi
- Gemma decision system

The protocol supports two main directions:

```text
Sensor Data:
Band → Receiver → Raspberry Pi → Gemma

Feedback Command:
Gemma → Raspberry Pi → Receiver → Band
```

## 2. Device Identification

Each wearable band has a unique rower ID.

| Rower ID | Device |
|---|---|
| 1 | Band 1 |
| 2 | Band 2 |
| 3 | Band 3 |

## 3. Sensor Packet

The wearable sends IMU information to the receiver.

Conceptual packet:

```text
{
  rower_id,
  timestamp,
  ax,
  ay,
  az,
  gx,
  gy,
  gz
}
```

Example JSON representation:

```json
{
  "type": "SENSOR_DATA",
  "rower_id": 2,
  "timestamp": 1720000000,
  "accelerometer": {
    "x": 0.12,
    "y": 0.85,
    "z": 0.42
  },
  "gyroscope": {
    "x": 15.2,
    "y": 120.6,
    "z": 8.4
  }
}
```

For real ESP-NOW implementation, the final packet can use a compact binary or C struct representation instead of JSON.

## 4. Receiver to Raspberry Pi

The receiver forwards sensor packets to the Raspberry Pi.

The Raspberry Pi collects data from all three rowers into short analysis windows.

```text
Band 1 Data ─┐
Band 2 Data ─┼──► Raspberry Pi Analysis Window
Band 3 Data ─┘
```

## 5. Gemma Analysis Request

Python formats the collected information for Gemma.

The request should ask Gemma to:

- Compare the three rowers
- Determine synchronization
- Identify the mismatched rower
- Identify the issue
- Determine severity
- Select the feedback action

## 6. Gemma Decision Packet

Gemma should return structured output.

```json
{
  "sync_score": 72,
  "rower_id": 2,
  "status": "MISMATCH",
  "issue": "LATE_STROKE",
  "severity": "HIGH",
  "action": "RED_VIBRATE"
}
```

## 7. Feedback Command

The Raspberry Pi sends the AI decision to the receiver.

The receiver then sends the command to the selected wearable.

Conceptual command:

```text
{
  target_rower,
  action
}
```

Example:

```json
{
  "type": "FEEDBACK_COMMAND",
  "target_rower": 2,
  "action": "RED_VIBRATE"
}
```

## 8. Action Codes

| Action | Meaning |
|---|---|
| GREEN | Green LED |
| YELLOW | Yellow LED |
| RED | Red LED |
| RED_VIBRATE | Red LED and vibration |

## 9. Communication Flow

```text
SENSOR FLOW
===========

Band 1 ─┐
Band 2 ─┼── ESP-NOW ──► Receiver ESP32
Band 3 ─┘
                            │
                            ▼
                      Raspberry Pi
                            │
                            ▼
                          Gemma


DECISION FLOW
=============

Gemma
  │
  ▼
Raspberry Pi
  │
  ▼
Receiver ESP32
  │
  ▼
ESP-NOW
  │
  ▼
Target Band
  │
  ▼
LED / Vibration
```

## 10. Reliability

For the prototype:

- Include rower ID in every packet
- Include timestamps in sensor data
- Validate packet type before processing
- Ignore malformed commands
- Use explicit action names or action codes
- Log Gemma decisions during testing

The protocol can later be extended with:

- Sequence numbers
- Acknowledgements
- Packet loss detection
- Battery status
- Device health information
