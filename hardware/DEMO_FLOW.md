# Live Demo Flow

1. Three participants perform synchronized rowing motion.
2. One participant deliberately changes timing.
3. The system collects three strokes from all rowers.
4. Gemma analyzes the combined 9-stroke summary.
5. Gemma identifies the out-of-sync rower.
6. The decision returns through the Central ESP32.
7. Only the incorrect rower's band activates RED LED + VIBRATION.
8. The rower corrects the movement.

## One-line explanation
Sense the crew. Gemma identifies the drift. The band physically alerts the rower.
