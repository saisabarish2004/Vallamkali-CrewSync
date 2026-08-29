# Test Checklist

## Member 1
- [ ] IMU detected
- [ ] accelerometer valid
- [ ] gyroscope valid
- [ ] stroke detection works
- [ ] stroke summary generated
- [ ] rower ID included

## Member 2
- [ ] ESP-NOW initialized
- [ ] sample packet sent
- [ ] sample packet received
- [ ] repeated transmission tested

## Member 3
- [ ] command received
- [ ] LED green works
- [ ] LED red works
- [ ] vibration works
- [ ] LED and vibration simultaneous
- [ ] correct target responds

## Team Lead
- [ ] UART receives packets
- [ ] packets grouped by rower
- [ ] 3 strokes per rower collected
- [ ] Gemma input created
- [ ] Gemma running locally
- [ ] output parsed
- [ ] decision sent
- [ ] latency measured

## Final
- [ ] all synchronized case
- [ ] R1 deliberately late
- [ ] R2 deliberately late
- [ ] R3 deliberately late
- [ ] correct rower alerted
- [ ] full Sense → Think → Act loop works
