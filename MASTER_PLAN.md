# Vallamkali CrewSync — Master Team Plan v1.0

## Goal
Build a reliable wearable Physical AI prototype: IMU -> Band ESP32 -> ESP-NOW -> Central ESP32 -> Raspberry Pi 5 -> stroke/features -> local Gemma decision -> Central ESP32 -> ESP-NOW -> correct band -> LED/haptic feedback. Gemini is a separate longer-term coaching layer.

## Four parallel workstreams
1. Person 1: IMU + MotionPacket generation.
2. Person 2: ESP-NOW sample-packet communication.
3. Person 3: return communication + LED/haptic feedback.
4. Sai: Raspberry Pi + synchronization intelligence + Gemma + final integration.

## Locked rules
- Gemma makes the physical feedback decision.
- Prototype AI loop may take up to ~2 seconds.
- Gemini is outside the immediate actuator loop.
- Crew rhythm is adaptive: if everyone slows together, everyone can remain synchronized.
- Do not send raw high-rate IMU data directly to Gemma. Extract compact features first.
- No OLED in MVP.
- Use fake packets before real hardware integration.
- P0 reliability beats extra features.

## 18-hour execution
[ ] 0-1h: identify hardware, wire first band and central.
[ ] 1-4h: four workstreams run in parallel.
[ ] 4-6h: Band -> Central integration.
[ ] 6-8h: Central -> Pi integration.
[ ] 8-10h: features -> Gemma.
[ ] 10-12h: Gemma -> Central -> Band physical loop.
[ ] 12-14h: clone/test three bands.
[ ] 14-16h: reliability tests.
[ ] 16-18h: freeze, fix, document, rehearse.

## P0
[ ] IMU works
[ ] MotionPacket works
[ ] ESP-NOW uplink works
[ ] Central receives multiple rower IDs
[ ] Pi receives packets
[ ] Stroke timing works
[ ] Crew-relative features work
[ ] Gemma gives validated decision
[ ] Decision returns to correct band
[ ] LED responds to Gemma
[ ] Three-band demo works

## P1
[ ] Vibration patterns
[ ] Better phase detection
[ ] Confidence/rejection
[ ] Packet-loss handling
[ ] Latency measurement

## P2
[ ] Gemini multi-stroke coaching
[ ] Better coaching history

## P3
[ ] CAD polish
[ ] Dashboard/UI polish
