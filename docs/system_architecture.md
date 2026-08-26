# Vallamkali CrewSync - System Architecture

## Project Purpose

Vallamkali CrewSync is a wearable Physical AI system designed to monitor and improve synchronization between Vallamkali rowers.

The system senses rower movement using IMU sensors, communicates the motion data wirelessly, analyzes synchronization using a Raspberry Pi and local Gemma model, and provides physical feedback through LEDs and vibration motors.

---

# System Components

## 1. Wearable Rower Nodes

Each selected rower wears a motion-sensing band.

### Main Components

- XIAO ESP32 or ESP32
- IMU sensor
- LED
- Vibration motor
- Battery

### Responsibilities

The wearable node:

1. Reads movement data from the IMU.
2. Creates a MotionPacket.
3. Sends the MotionPacket to the Central ESP32 using ESP-NOW.
4. Receives feedback commands.
5. Activates LED and vibration feedback.

---

# 2. Central ESP32

The Central ESP32 acts as the wireless communication gateway.

## Responsibilities

### Incoming

- Receives MotionPackets from wearable nodes using ESP-NOW.
- Identifies the rower using `rower_id`.
- Validates received packets.

### Outgoing

- Sends received motion data to the Raspberry Pi.
- Receives feedback decisions from the Raspberry Pi.
- Sends feedback commands to the correct wearable using ESP-NOW.

## Important

The Central ESP32 does not make the final AI decision.

Its primary responsibility is communication between:

```text
Wearables
↓
Raspberry Pi
3. Raspberry Pi

The Raspberry Pi acts as the central processing system.

Responsibilities
Receive motion data from the Central ESP32.
Store recent movement data.
Compare movement patterns.
Detect synchronization differences.
Prepare meaningful information for Gemma.
Run the local Gemma model.
Receive or interpret the AI decision.
Generate feedback commands.
Send the feedback command to the Central ESP32.
4. Local Gemma Model

Gemma is the local AI reasoning component.

Input

Gemma receives processed synchronization information.

Example:

Crew synchronization status:

Rower 1: Normal
Rower 2: Late movement
Rower 3: Normal

Detected timing difference: 350 ms
Output

Gemma produces a structured decision.

Example:

target_rower: 2
status: OUT_OF_SYNC
error_type: LATE_STROKE
intensity: MEDIUM
duration: 500
Important

Gemma should receive processed and summarized information rather than raw continuous IMU data.

Complete Architecture
                 ROWERS

       Rower 1    Rower 2    Rower 3
          │          │          │
          ▼          ▼          ▼

       IMU + ESP32 Wearable Bands

          │          │          │
          └──────────┼──────────┘
                     │
                  ESP-NOW
                     │
                     ▼

               CENTRAL ESP32

                     │
             Serial / Wi-Fi
                     │
                     ▼

                RASPBERRY PI

                     │
                     ▼

             SYNC DETECTION

                     │
                     ▼

               LOCAL GEMMA

                     │
               AI DECISION

                     │
                     ▼

                RASPBERRY PI

                     │
                     ▼

               CENTRAL ESP32

                     │
                  ESP-NOW
                     │

          ┌──────────┼──────────┐
          ▼          ▼          ▼

      Band 1      Band 2      Band 3

          │          │          │
          ▼          ▼          ▼

      LED + Vibration Feedback
Data Flow
Motion Data
IMU
↓
Wearable ESP32
↓
MotionPacket
↓
ESP-NOW
↓
Central ESP32
↓
Raspberry Pi
AI Decision Flow
Motion Data
↓
Synchronization Analysis
↓
Processed Movement Information
↓
Gemma
↓
AI Decision
Feedback Flow
Gemma Decision
↓
Raspberry Pi
↓
FeedbackPacket
↓
Central ESP32
↓
ESP-NOW
↓
Target Wearable
↓
LED + Vibration
Responsibility Matrix
Component	Primary Responsibility
IMU	Sense movement
Wearable ESP32	Read sensors and send/receive data
ESP-NOW	Wireless communication
Central ESP32	Communication gateway
Raspberry Pi	Processing and system control
Sync Detection	Detect synchronization errors
Local Gemma	AI reasoning and decision
LED	Visual feedback
Vibration Motor	Physical feedback
Important System Rules
Sensor data must be collected before AI analysis.
Communication must be tested independently before full integration.
Synchronization detection should work before Gemma is added.
Gemma should receive processed movement information.
Feedback must always target the correct rower.
Individual modules should not depend on unfinished modules during early development.
Each module must provide test results before integration.
Development Priority

Phase 1:

IMU
↓
MotionPacket

Phase 2:

ESP-NOW Communication

Phase 3:

Central ESP32
↓
Raspberry Pi Communication

Phase 4:

Synchronization Detection

Phase 5:

Local Gemma Integration

Phase 6:

Feedback Communication

Phase 7:

Full System Integration
Final Physical AI Loop
SENSE
  ↓
COMMUNICATE
  ↓
PROCESS
  ↓
AI REASON
  ↓
DECIDE
  ↓
PHYSICAL FEEDBACK

The system is complete only when this entire loop works end-to-end.


### Commit message

```text
Add system architecture documentation

Once this is committed, we have the two most important foundation documents:

docs/
├── packet_protocol.md
└── system_architecture.md

Next we should create hardware_connections.md, but we will make it carefully with a primary hardware list + alternatives, since hackathon hardware availability is unpredictable and apparently 20 teams may all want the same sensor at the exact same moment.
