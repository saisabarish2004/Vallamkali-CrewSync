# Gemma AI Workflow

## Purpose

Gemma is the main AI decision engine of Vallamkali Sync Coach.

It receives organized movement information from multiple rowers and determines the synchronization status and required physical feedback.

## Input Pipeline

```text
IMU
 ↓
ESP32
 ↓
ESP-NOW
 ↓
Receiver ESP32
 ↓
Raspberry Pi
 ↓
Python Interface
 ↓
Gemma
```

## Data Preparation

Python prepares sensor information for the AI.

The information may include:

- Accelerometer readings
- Gyroscope readings
- Timestamps
- Multiple rower data streams

## Gemma Prompt

```text
You are the AI decision engine of a Vallamkali synchronization training system.

Analyze the movement information from three rowers.

Determine:

1. Overall synchronization status
2. Which rower is most mismatched
3. Type of mismatch
4. Severity
5. Required physical feedback

Return only valid JSON.
```

## Expected Output

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

## Decision to Action

```text
Gemma Output
      ↓
Python Interface
      ↓
Receiver ESP32
      ↓
ESP-NOW
      ↓
Target Wristband
      ↓
Physical Feedback
```

## Physical Feedback States

| AI Action | Wearable Response |
|---|---|
| GREEN | Green LED |
| YELLOW | Yellow LED |
| RED | Red LED |
| RED_VIBRATE | Red LED + Vibration |

## Physical AI Loop

```text
Sense
 ↓
IMU detects physical movement
 ↓
Think
 ↓
Gemma analyzes the movement relationship
 ↓
Decide
 ↓
Gemma generates a structured command
 ↓
Act
 ↓
Wearable provides physical feedback
 ↓
Human adjusts movement
 ↓
Sense again
```
