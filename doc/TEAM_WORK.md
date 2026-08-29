# Team Work Structure

## Member 1 - IMU and Stroke Detection
- IMU communication
- sensor reading
- stroke detection
- compact stroke summary
- sensor testing

## Member 2 - ESP-NOW and CAD
- ESP-NOW testing
- wearable communication
- sample packet transmission
- CAD/enclosure work

## Member 3 - Return Communication and Feedback
- command handling
- ESP-NOW return path
- LED control
- vibration control
- simultaneous feedback testing

## Team Lead - Raspberry Pi + Gemma
- UART with Central ESP32
- collect 3 strokes from each rower
- organize 9 summaries
- compact Gemma input
- local Gemma execution
- output parsing
- decision → UART response
- final integration

## Integration Contract
Agree before development:
1. packet names
2. field names
3. rower IDs
4. UART baud rate
5. ESP-NOW format
6. command format
