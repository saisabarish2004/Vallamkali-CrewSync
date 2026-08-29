# Packet Protocol

## Wearable → Central ESP32
Conceptual stroke packet:

{
  "type": "STROKE",
  "rower": 1,
  "stroke_id": 3,
  "duration_ms": 1020,
  "peak_acc": 2.8,
  "gyro_peak": 135,
  "angle": 42
}

## Central ESP32 → Raspberry Pi
UART development format: newline-delimited JSON.

## Raspberry Pi → Central ESP32
Example decision:

{"type":"DECISION","target":1,"status":"OUT_OF_SYNC","error":"LATE","led":"RED","vibration":"ON"}

## Central ESP32 → Wearable
Conceptual command:

{"type":"ALERT","target":1,"led":"RED","vibration":"ON"}

## Packet Movement
Band → STROKE → Central ESP32 → UART → Raspberry Pi
Raspberry Pi → DECISION → Central ESP32 → ESP-NOW → Target Band
