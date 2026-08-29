# AI Algorithm - Raspberry Pi + Gemma

## Input Window
3 strokes × 3 rowers = 9 stroke summaries.

## Flow
UART DATA
↓
Validate packets
↓
Group by rower
↓
Wait for 3 strokes per rower
↓
Create compact Gemma input
↓
GEMMA
↓
Parse short decision
↓
Send command through UART

## Possible Features Per Stroke
- stroke duration
- timing marker
- peak acceleration
- peak gyro value
- motion angle

## Gemma Task
Compare the recent three strokes of all three rowers.
Do not assume any rower is incorrect.
Identify whether one rower is consistently different from the other two.
Return a compact structured decision.

## Example Output
{
  "r": 1,
  "s": "OUT_OF_SYNC",
  "e": "LATE",
  "led": "RED",
  "vib": "ON"
}

## Optimization
1. Keep Gemma running.
2. Use a suitable small quantized model.
3. Keep prompts short.
4. Send summaries, not raw sensor streams.
5. Request short output.

## Latency
Preferred after third stroke: under 1 second.
Acceptable prototype target: under 2 seconds.
