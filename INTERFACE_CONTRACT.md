# Interface Contract v1.0

## MotionPacket
```c
struct MotionPacket {
    uint8_t  rower_id;
    uint32_t sequence;
    uint32_t timestamp;
    float ax, ay, az;
    float gx, gy, gz;
};
```

## FeaturePacket
Minimum fields:
- rower_id
- crew_period_ms
- rower_period_ms
- catch_offset_ms
- drive_ratio
- angle_deviation
- recent_sync_rate

## VerdictPacket
```c
struct VerdictPacket {
    uint8_t rower_id;
    uint8_t status;
    uint8_t error_type;
    uint8_t feedback;
    uint8_t confidence;
};
```

Status:
0 SYNC
1 EARLY
2 LATE
3 ANGLE_ERROR
4 WEAK_DRIVE
5 UNKNOWN

Feedback:
0 OFF
1 GREEN
2 RED_SHORT
3 RED_LONG
4 RED_DOUBLE
5 RED_TRIPLE

## Rule
Do not change these interfaces without a shared decision and a version update.
