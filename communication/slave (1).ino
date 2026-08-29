/*
 * slave.ino  -  rower band
 *
 * Detects strokes from the MPU6050, sends them to the Central over ESP-NOW,
 * and shows the Central's decision on the LEDs and vibration motor.
 *
 * SET ROWER_ID TO 1, 2 OR 3 BEFORE FLASHING EACH BAND.
 *
 * FEEDBACK
 *   GREEN         in sync, well done
 *   RED + vibrate timing fault, you are out of rhythm
 *   BLUE          catch angle fault, your reach is off the crew
 *   vibrate only  acceleration fault, your power is off the crew
 *   status LED    blinking = calibrating, steady = ready
 *
 * CHANGES FROM THE FIRST VERSION
 *   50 Hz sampling instead of 20 Hz. Stroke start was carrying up to 50 ms
 *   of error, which is half the sync threshold.
 *   One burst I2C read instead of six transactions per sample.
 *   Compact serial output. The old version printed ~700 bytes every second,
 *   which blocks for ~60 ms and made fast sampling impossible.
 *   Gyro bias measured at boot instead of hardcoded.
 */

#include <Wire.h>
#include <WiFi.h>
#include <esp_now.h>
#include <math.h>

// =====================================================
// SET THIS PER BAND
// =====================================================

#define ROWER_ID 2

// Set to 1 if this band's MPU6050 is dead. It then fakes strokes at a steady
// rhythm so the crew still fills a batch and the demo keeps working.
// Set to 2 to fake strokes that are deliberately LATE, for testing.
#define SIMULATE 0

uint8_t centralMAC[] = {0x94, 0x54, 0xC5, 0x2F, 0x27, 0x28};

// =====================================================
// PINS   change to match your wiring
// =====================================================

#define RED_PIN     3        // D1
#define GREEN_PIN   4        // D2
#define BLUE_PIN    5        // D3

// XIAO ESP32-C3 pin labels in brackets.
// GPIO 2, 8 and 9 are strapping pins on the C3 - do not use them for outputs.
#define VIBE_PIN   10        // D10  through a transistor, never straight to GPIO
#define STATUS_PIN 20        // D7   the separate LED

#define SDA_PIN     6        // D4
#define SCL_PIN     7        // D5

// =====================================================
// MPU6050
// =====================================================

#define MPU_ADDR     0x68
#define ACCEL_XOUT_H 0x3B
#define PWR_MGMT_1   0x6B

const float ACCEL_SCALE = 16384.0;
const float Z_OFFSET    = 3029.0;
const float Z_SCALE     = 16448.0;
const float GYRO_SCALE  = 131.0;
const float GRAVITY     = 9.80665;

// measured at boot, not hardcoded
float gyroBiasX = 0, gyroBiasY = 0, gyroBiasZ = 0;

// =====================================================
// STROKE DETECTION
// =====================================================

const float STROKE_START_ACC  = 1.5;
const float STROKE_START_GYRO = 40.0;
const float STROKE_END_ACC    = 0.8;
const float STROKE_END_GYRO   = 25.0;

const unsigned long MIN_STROKE_DURATION = 300;
const unsigned long MAX_STROKE_DURATION = 2500;
const unsigned long STROKE_QUIET_MS     = 200;
const unsigned long STROKE_COOLDOWN     = 400;

// =====================================================
// TIMING
// =====================================================

const unsigned long SENSOR_INTERVAL  = 20;      // 50 Hz
const unsigned long DISPLAY_INTERVAL = 3000;    // one short line
// Feedback holds until the next decision. Decisions arrive once per batch
// (~3 s), so a shorter hold leaves the band dark half the time and looks
// broken. This is a safety expiry, not the normal clear path.
const unsigned long ALERT_MS         = 5000;

unsigned long lastSensorTime = 0;
unsigned long lastDisplayTime = 0;
unsigned long lastStrokeEndTime = 0;
unsigned long alertUntil = 0;
unsigned long lastBlink = 0;

// =====================================================
// STATE
// =====================================================

bool strokeActive = false;
bool calibrated = false;
bool linked = false;

unsigned long strokeStartTime = 0;
unsigned long lastMotionTime = 0;

float peakAcceleration = 0, peakGyro = 0, peakAngle = 0;
uint32_t strokeID = 0;

float aX = 0, aY = 0, aZ = 0;
float gX = 0, gY = 0, gZ = 0;
float dynAcc = 0, gyroMag = 0, angleNow = 0;

int i2cErrors = 0;

// =====================================================
// PACKETS   unchanged, Central must match
// =====================================================

typedef struct {
  uint8_t  type;
  uint8_t  rower;
  uint32_t stroke_id;
  uint32_t duration_ms;
  float    peak_acc;
  float    gyro_peak;
  float    angle;
} StrokePacket;

typedef struct {
  uint8_t target;
  uint8_t status;
  uint8_t error;
  uint8_t led;
} DecisionPacket;

#define PACKET_TYPE_STROKE 1

#define STATUS_CORRECT     1
#define STATUS_WRONG       2
#define STATUS_OUT_OF_SYNC 3

#define ERROR_NONE         0
#define ERROR_DURATION     1     // timing  -> RED + vibrate
#define ERROR_ACCELERATION 2     // power   -> vibrate only
#define ERROR_GYRO         3
#define ERROR_ANGLE        4     // angle   -> BLUE

#define LED_COMMAND_OFF    0
#define LED_COMMAND_GREEN  1
#define LED_COMMAND_RED    2

// =====================================================
// OUTPUT
// =====================================================

void rgb(bool r, bool g, bool b) {
  digitalWrite(RED_PIN, r);
  digitalWrite(GREEN_PIN, g);
  digitalWrite(BLUE_PIN, b);
}

void vibrate(bool on) { digitalWrite(VIBE_PIN, on); }

void clearFeedback() {
  rgb(0, 0, 0);
  vibrate(false);
  alertUntil = 0;
}

void showInSync() {
  rgb(0, 1, 0);
  vibrate(false);
  alertUntil = millis() + ALERT_MS;
}

void showFault(uint8_t error) {
  switch (error) {
    case ERROR_DURATION:                     // timing
      rgb(1, 0, 0); vibrate(true);  break;
    case ERROR_ANGLE:                        // catch angle
    case ERROR_GYRO:
      rgb(0, 0, 1); vibrate(false); break;
    case ERROR_ACCELERATION:                 // power
      rgb(0, 0, 0); vibrate(true);  break;
    default:
      rgb(1, 0, 0); vibrate(true);  break;
  }
  alertUntil = millis() + ALERT_MS;
}

const char *errorName(uint8_t e) {
  if (e == ERROR_DURATION)     return "TIMING";
  if (e == ERROR_ANGLE)        return "CATCH ANGLE";
  if (e == ERROR_GYRO)         return "CATCH ANGLE";
  if (e == ERROR_ACCELERATION) return "POWER";
  return "NONE";
}

// status LED: blinking while calibrating, steady once linked, off if no link
void updateStatusLed() {
  unsigned long now = millis();
  if (!calibrated) {
    if (now - lastBlink >= 150) {
      lastBlink = now;
      digitalWrite(STATUS_PIN, !digitalRead(STATUS_PIN));
    }
  } else {
    digitalWrite(STATUS_PIN, linked ? HIGH : LOW);
  }
}

// =====================================================
// MPU
// =====================================================

void writeRegister(uint8_t reg, uint8_t value) {
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(reg);
  Wire.write(value);
  Wire.endTransmission();
}

// One burst read of 14 bytes instead of six separate transactions.
bool readIMU(int16_t *ax, int16_t *ay, int16_t *az,
             int16_t *gx, int16_t *gy, int16_t *gz) {
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(ACCEL_XOUT_H);
  if (Wire.endTransmission(false) != 0) return false;

  if (Wire.requestFrom(MPU_ADDR, (uint8_t)14) != 14) return false;

  uint8_t b[14];
  for (int i = 0; i < 14; i++) b[i] = Wire.read();

  *ax = (int16_t)((b[0]  << 8) | b[1]);
  *ay = (int16_t)((b[2]  << 8) | b[3]);
  *az = (int16_t)((b[4]  << 8) | b[5]);
  // b[6],b[7] are temperature
  *gx = (int16_t)((b[8]  << 8) | b[9]);
  *gy = (int16_t)((b[10] << 8) | b[11]);
  *gz = (int16_t)((b[12] << 8) | b[13]);
  return true;
}

// Measure gyro bias at boot. The band must be still.
void calibrateGyro() {
  Serial.println("calibrating, keep the band still...");

  const int N = 200;
  long sx = 0, sy = 0, sz = 0;
  int good = 0;

  for (int i = 0; i < N; i++) {
    int16_t ax, ay, az, gx, gy, gz;
    if (readIMU(&ax, &ay, &az, &gx, &gy, &gz)) {
      sx += gx; sy += gy; sz += gz;
      good++;
    }
    updateStatusLed();
    delay(10);
  }

  if (good > N / 2) {
    gyroBiasX = (sx / (float)good) / GYRO_SCALE;
    gyroBiasY = (sy / (float)good) / GYRO_SCALE;
    gyroBiasZ = (sz / (float)good) / GYRO_SCALE;
    calibrated = true;
    Serial.printf("calibrated  bias %.2f %.2f %.2f  (%d/%d samples)\n",
                  gyroBiasX, gyroBiasY, gyroBiasZ, good, N);
  } else {
    Serial.println("CALIBRATION FAILED - check I2C wiring");
  }
}

// =====================================================
// ESP-NOW
// =====================================================

void onDataReceive(const esp_now_recv_info_t *info,
                   const uint8_t *data, int len) {
  if (len != sizeof(DecisionPacket)) return;

  DecisionPacket d;
  memcpy(&d, data, sizeof(d));
  if (d.target != ROWER_ID) return;

  if (d.status == STATUS_CORRECT) {
    showInSync();
    Serial.println("DECISION: in sync  -> GREEN");
  } else {
    showFault(d.error);
    Serial.printf("DECISION: OUT OF SYNC  %s  -> %s\n",
                  errorName(d.error),
                  d.error == ERROR_ANGLE || d.error == ERROR_GYRO ? "BLUE" :
                  d.error == ERROR_ACCELERATION ? "VIBRATE" : "RED + VIBRATE");
  }
}

void onDataSent(const wifi_tx_info_t *info, esp_now_send_status_t status) {
  linked = (status == ESP_NOW_SEND_SUCCESS);
}

void sendStrokePacket(unsigned long duration) {
  StrokePacket p;
  p.type        = PACKET_TYPE_STROKE;
  p.rower       = ROWER_ID;
  p.stroke_id   = strokeID++;
  p.duration_ms = duration;
  p.peak_acc    = peakAcceleration;
  p.gyro_peak   = peakGyro;
  p.angle       = peakAngle;

  esp_err_t r = esp_now_send(centralMAC, (uint8_t *)&p, sizeof(p));

  Serial.printf("stroke %lu  dur=%lums acc=%.2f gyro=%.0f ang=%.1f  %s\n",
                (unsigned long)p.stroke_id, (unsigned long)duration,
                p.peak_acc, p.gyro_peak, p.angle,
                r == ESP_OK ? "sent" : "SEND FAILED");
}

// =====================================================
// STROKE
// =====================================================

void finishStroke();

void startStroke() {
  strokeActive = true;
  strokeStartTime = millis();
  lastMotionTime = strokeStartTime;
  peakAcceleration = dynAcc;
  peakGyro = gyroMag;
  peakAngle = angleNow;
}

void updateStroke() {
  if (dynAcc   > peakAcceleration) peakAcceleration = dynAcc;
  if (gyroMag  > peakGyro)         peakGyro = gyroMag;
  if (angleNow > peakAngle)        peakAngle = angleNow;

  if (dynAcc > STROKE_END_ACC || gyroMag > STROKE_END_GYRO)
    lastMotionTime = millis();

  if (millis() - lastMotionTime > STROKE_QUIET_MS) { finishStroke(); return; }
  if (millis() - strokeStartTime > MAX_STROKE_DURATION) finishStroke();
}

void finishStroke() {
  if (!strokeActive) return;
  strokeActive = false;

  unsigned long duration = millis() - strokeStartTime;

  // The Central reconstructs stroke start as (arrival - duration_ms), so
  // this must be sent immediately, with no delay in between.
  if (duration >= MIN_STROKE_DURATION) sendStrokePacket(duration);

  lastStrokeEndTime = millis();
}

// =====================================================
// SIMULATED STROKES   only when SIMULATE != 0
// =====================================================

#if SIMULATE
unsigned long nextFake = 0;

void pollFakeStrokes() {
  unsigned long now = millis();
  if ((int32_t)(now - nextFake) < 0) return;

  // SIMULATE 2 makes this band deliberately late, so you can prove the
  // coach catches it without needing a working IMU.
  unsigned long dur = 1000 + random(-30, 31);
  peakAcceleration = 2.80 + random(-10, 11) / 100.0;
  peakGyro         = 135.0 + random(-5, 6);
  peakAngle        = 42.0 + random(-10, 11) / 10.0;

  strokeStartTime = now - dur;
  sendStrokePacket(dur);

  nextFake = now + 1000 + (SIMULATE == 2 ? 400 : 0);
}
#endif


// =====================================================
// SETUP
// =====================================================

void setup() {
  Serial.begin(115200);
  delay(600);

  pinMode(RED_PIN, OUTPUT);
  pinMode(GREEN_PIN, OUTPUT);
  pinMode(BLUE_PIN, OUTPUT);
  pinMode(VIBE_PIN, OUTPUT);
  pinMode(STATUS_PIN, OUTPUT);
  clearFeedback();
  digitalWrite(STATUS_PIN, LOW);

  Wire.begin(SDA_PIN, SCL_PIN);
  Wire.setClock(400000);            // 400 kHz, needed for 50 Hz sampling

  writeRegister(PWR_MGMT_1, 0x00);
  delay(300);

  Serial.printf("\n=== ROWER %d ===\n", ROWER_ID);

#if SIMULATE
  calibrated = true;                 // no IMU needed
  Serial.printf("SIMULATE %d - faking strokes, IMU not used\n", SIMULATE);
#else
  calibrateGyro();
#endif

  WiFi.mode(WIFI_STA);
  Serial.print("MAC : ");
  Serial.println(WiFi.macAddress());

  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW INIT FAILED");
    while (true) { updateStatusLed(); delay(10); }
  }

  esp_now_register_recv_cb(onDataReceive);
  esp_now_register_send_cb(onDataSent);

  esp_now_peer_info_t peer = {};
  memcpy(peer.peer_addr, centralMAC, 6);
  peer.channel = 0;
  peer.encrypt = false;
  Serial.printf("central peer : %s\n",
                esp_now_add_peer(&peer) == ESP_OK ? "added" : "FAILED");

  // quick colour test so you know the wiring is right
  rgb(1,0,0); delay(200);
  rgb(0,1,0); delay(200);
  rgb(0,0,1); delay(200);
  vibrate(true); delay(150); vibrate(false);
  clearFeedback();

  Serial.printf("ROWER %d READY\n\n", ROWER_ID);
}

// =====================================================
// LOOP
// =====================================================

void loop() {
  unsigned long now = millis();

  updateStatusLed();

  if (alertUntil && (int32_t)(now - alertUntil) >= 0) clearFeedback();

#if SIMULATE
  pollFakeStrokes();
  return;
#endif

  if (now - lastSensorTime < SENSOR_INTERVAL) return;
  lastSensorTime = now;

  int16_t rax, ray, raz, rgx, rgy, rgz;
  if (!readIMU(&rax, &ray, &raz, &rgx, &rgy, &rgz)) {
    i2cErrors++;
    return;                          // do not feed garbage into detection
  }

  aX = (rax / ACCEL_SCALE) * GRAVITY;
  aY = (ray / ACCEL_SCALE) * GRAVITY;
  aZ = ((raz - Z_OFFSET) / Z_SCALE) * GRAVITY;

  gX = (rgx / GYRO_SCALE) - gyroBiasX;
  gY = (rgy / GYRO_SCALE) - gyroBiasY;
  gZ = (rgz / GYRO_SCALE) - gyroBiasZ;

  float mag = sqrt(aX * aX + aY * aY + aZ * aZ);
  dynAcc  = fabs(mag - GRAVITY);
  gyroMag = sqrt(gX * gX + gY * gY + gZ * gZ);
  angleNow = atan2(sqrt(aX * aX + aY * aY), fabs(aZ)) * 180.0 / PI;

  if (!strokeActive) {
    if (now - lastStrokeEndTime >= STROKE_COOLDOWN &&
        (dynAcc > STROKE_START_ACC || gyroMag > STROKE_START_GYRO))
      startStroke();
  } else {
    updateStroke();
  }

  // one short line, not 700 bytes
  if (now - lastDisplayTime >= DISPLAY_INTERVAL) {
    lastDisplayTime = now;
    Serial.printf("R%d acc=%.1f gyro=%.0f ang=%.0f %s%s\n",
                  ROWER_ID, dynAcc, gyroMag, angleNow,
                  strokeActive ? "STROKE" : "idle",
                  i2cErrors ? "  [i2c errors]" : "");
  }
}
