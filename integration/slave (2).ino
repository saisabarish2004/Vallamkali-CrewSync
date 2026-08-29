/*
 * slave.ino  -  rower band
 *
 * ONE PACKET PER COMPLETE STROKE.
 *
 * The old version defined a stroke as "motion until it goes quiet". During
 * the recovery phase the band goes quiet mid-stroke, so finishStroke() fired
 * early and the second half of the same stroke started a new one - two
 * packets for one stroke.
 *
 * This version measures CATCH TO CATCH: a stroke is the time from one drive
 * onset to the next. One packet per stroke, and duration_ms is the true
 * stroke period, which is what the Pi needs for cadence.
 *
 * SET ROWER_ID PER BAND. With two rowers use 1 and 2, not 2 and 3.
 * Rower 1 is the stroke seat and sets the rhythm.
 */

#include <Wire.h>
#include <WiFi.h>
#include <esp_now.h>
#include <math.h>

// =====================================================
// SET THIS PER BAND
// =====================================================

#define ROWER_ID 2

uint8_t centralMAC[] = {0x94, 0x54, 0xC5, 0x2F, 0x27, 0x28};

// Set to 1 if this band's MPU6050 is dead. It fakes a steady rhythm so the
// crew still fills a batch. Set to 2 to fake strokes that are late.
#define SIMULATE 0

// =====================================================
// PINS   XIAO ESP32-C3 labels in brackets
// =====================================================

#define RED_PIN     3        // D1
#define GREEN_PIN   4        // D2
#define BLUE_PIN    5        // D3

#define VIBE_PIN   10        // D10  via a transistor, never straight to GPIO
#define STATUS_PIN 20        // D7   the separate LED

#define SDA_PIN     6        // D4
#define SCL_PIN     7        // D5

// =====================================================
// MPU6050
// =====================================================

#define MPU_ADDR     0x68    // 0x69 if AD0 is high
#define I2C_CLOCK    100000  // 100 kHz. try 400000 only with short wires
#define ACCEL_XOUT_H 0x3B
#define PWR_MGMT_1   0x6B

const float ACCEL_SCALE = 16384.0;
const float Z_OFFSET    = 3029.0;
const float Z_SCALE     = 16448.0;
const float GYRO_SCALE  = 131.0;
const float GRAVITY     = 9.80665;

float gyroBiasX = 0, gyroBiasY = 0, gyroBiasZ = 0;   // measured at boot

// =====================================================
// STROKE DETECTION
// =====================================================

// A drive has begun when motion rises above these.
const float CATCH_ACC  = 1.5;
const float CATCH_GYRO = 40.0;

// Motion has stopped when it falls below these.
const float QUIET_ACC  = 0.8;
const float QUIET_GYRO = 25.0;

// How long the band must stay quiet before the next burst counts as a NEW
// catch. Must be longer than the quiet dip inside a normal recovery, or one
// stroke gets split into two. This is the key number for the double packet
// problem - raise it if you still see two packets per stroke.
const unsigned long QUIET_MS = 400;

// A stroke period outside this range is not a real stroke.
const unsigned long MIN_STROKE_PERIOD = 600;
const unsigned long MAX_STROKE_PERIOD = 2500;

// =====================================================
// TIMING
// =====================================================

const unsigned long SENSOR_INTERVAL  = 20;      // 50 Hz
const unsigned long DISPLAY_INTERVAL = 3000;
const unsigned long ALERT_MS         = 5000;
const unsigned long HEARTBEAT_MS     = 3000;    // tell Central we are alive

unsigned long lastSensorTime = 0;
unsigned long lastDisplayTime = 0;
unsigned long alertUntil = 0;
unsigned long lastBlink = 0;
unsigned long lastHeartbeat = 0;

// =====================================================
// STATE
// =====================================================

bool driving = false;              // inside a drive right now
bool calibrated = false;
bool linked = false;

unsigned long prevCatchTime = 0;   // when the LAST catch happened
unsigned long lastMotionTime = 0;

float peakAcceleration = 0, peakGyro = 0, peakAngle = 0;
uint32_t strokeID = 0;

float aX = 0, aY = 0, aZ = 0;
float gX = 0, gY = 0, gZ = 0;
float dynAcc = 0, gyroMag = 0, angleNow = 0;

int i2cErrors = 0;
int samples = 0;

// =====================================================
// PACKETS   Central must match
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
#define PACKET_TYPE_READY  2      // sent after calibration, then as heartbeat

#define STATUS_CORRECT     1
#define STATUS_WRONG       2
#define STATUS_OUT_OF_SYNC 3

#define ERROR_NONE         0
#define ERROR_DURATION     1     // timing -> RED + vibrate
#define ERROR_ACCELERATION 2     // power  -> vibrate only
#define ERROR_GYRO         3
#define ERROR_ANGLE        4     // angle  -> BLUE

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
    case ERROR_DURATION:                      // timing
      rgb(1, 0, 0); vibrate(true);  break;
    case ERROR_ANGLE:                         // catch angle
    case ERROR_GYRO:
      rgb(0, 0, 1); vibrate(false); break;
    case ERROR_ACCELERATION:                  // power
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
bool readIMUOnce(int16_t *ax, int16_t *ay, int16_t *az,
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
  *gx = (int16_t)((b[8]  << 8) | b[9]);
  *gy = (int16_t)((b[10] << 8) | b[11]);
  *gz = (int16_t)((b[12] << 8) | b[13]);
  return true;
}

// One retry. A single dropped read is common on a breadboard and should not
// cost a whole sample.
bool readIMU(int16_t *ax, int16_t *ay, int16_t *az,
             int16_t *gx, int16_t *gy, int16_t *gz) {
  if (readIMUOnce(ax, ay, az, gx, gy, gz)) return true;
  delayMicroseconds(200);
  return readIMUOnce(ax, ay, az, gx, gy, gz);
}

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
    Serial.printf("calibrated  bias %.2f %.2f %.2f  (%d/%d)\n",
                  gyroBiasX, gyroBiasY, gyroBiasZ, good, N);
  } else {
    Serial.println("CALIBRATION FAILED - check I2C wiring, or AD0 (try 0x69)");
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
    Serial.println("DECISION: in sync -> GREEN");
  } else {
    showFault(d.error);
    Serial.printf("DECISION: OUT OF SYNC  %s\n", errorName(d.error));
  }
}

void onDataSent(const wifi_tx_info_t *info, esp_now_send_status_t status) {
  linked = (status == ESP_NOW_SEND_SUCCESS);
}

// Tells the Central this band is powered, calibrated and in range.
// Same struct, different type, so nothing else has to change.
void sendReady() {
  StrokePacket p;
  p.type        = PACKET_TYPE_READY;
  p.rower       = ROWER_ID;
  p.stroke_id   = 0;
  p.duration_ms = calibrated ? 1 : 0;    // 1 = calibrated ok
  p.peak_acc    = gyroBiasX;             // send the bias so it is visible
  p.gyro_peak   = gyroBiasY;
  p.angle       = gyroBiasZ;

  esp_now_send(centralMAC, (uint8_t *)&p, sizeof(p));
  lastHeartbeat = millis();
}

void sendStrokePacket(unsigned long period) {
  StrokePacket p;
  p.type        = PACKET_TYPE_STROKE;
  p.rower       = ROWER_ID;
  p.stroke_id   = strokeID++;
  p.duration_ms = period;
  p.peak_acc    = peakAcceleration;
  p.gyro_peak   = peakGyro;
  p.angle       = peakAngle;

  esp_err_t r = esp_now_send(centralMAC, (uint8_t *)&p, sizeof(p));

  Serial.printf("STROKE %lu  period=%lums acc=%.2f gyro=%.0f ang=%.1f  %s\n",
                (unsigned long)p.stroke_id, (unsigned long)period,
                p.peak_acc, p.gyro_peak, p.angle,
                r == ESP_OK ? "sent" : "SEND FAILED");
}

// =====================================================
// STROKE   catch to catch
// =====================================================

// A new drive has begun. The stroke that just ENDED is the one we report.
void onCatch() {
  unsigned long now = millis();

  if (prevCatchTime != 0) {
    unsigned long period = now - prevCatchTime;

    // Exactly one packet per completed stroke.
    if (period >= MIN_STROKE_PERIOD && period <= MAX_STROKE_PERIOD) {
      sendStrokePacket(period);
    } else {
      Serial.printf("ignored: period %lums out of range\n",
                    (unsigned long)period);
    }
  } else {
    Serial.println("first catch, waiting for the next one to measure a stroke");
  }

  prevCatchTime = now;

  // start collecting peaks for the stroke that begins now
  peakAcceleration = dynAcc;
  peakGyro         = gyroMag;
  peakAngle        = angleNow;

  driving = true;
  lastMotionTime = now;
}

void updateDrive() {
  if (dynAcc   > peakAcceleration) peakAcceleration = dynAcc;
  if (gyroMag  > peakGyro)         peakGyro = gyroMag;
  if (angleNow > peakAngle)        peakAngle = angleNow;

  if (dynAcc > QUIET_ACC || gyroMag > QUIET_GYRO)
    lastMotionTime = millis();

  // Quiet long enough that the next burst is a genuine new catch, not the
  // recovery phase of the stroke we are already inside.
  if (millis() - lastMotionTime > QUIET_MS)
    driving = false;
}

// =====================================================
// SIMULATED STROKES   only when SIMULATE != 0
// =====================================================

#if SIMULATE
unsigned long nextFake = 0;

void pollFakeStrokes() {
  unsigned long now = millis();
  if ((int32_t)(now - nextFake) < 0) return;

  peakAcceleration = 2.80 + random(-10, 11) / 100.0;
  peakGyro         = 135.0 + random(-5, 6);
  peakAngle        = 42.0 + random(-10, 11) / 10.0;

  sendStrokePacket(1000 + random(-30, 31));

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
  // 100 kHz. 400 kHz needs short wires and good pullups; on breadboard or
  // long jumper leads it produces read failures. Raise it only if you see
  // no [i2c errors] for a while.
  Wire.setClock(I2C_CLOCK);

  writeRegister(PWR_MGMT_1, 0x00);
  delay(300);

  Serial.printf("\n=== ROWER %d ===\n", ROWER_ID);

#if SIMULATE
  calibrated = true;
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

  // output self test, so you know every LED and the motor are wired right
  Serial.println("output test: red, green, blue, buzz");
  rgb(1,0,0); delay(400);
  rgb(0,1,0); delay(400);
  rgb(0,0,1); delay(400);
  rgb(0,0,0);
  vibrate(true); delay(200); vibrate(false);

  // tell the Central we are calibrated and in range
  sendReady();

  // three green blinks = calibrated and announced
  for (int i = 0; i < 3; i++) {
    rgb(0,1,0); delay(120);
    rgb(0,0,0); delay(120);
  }
  clearFeedback();

  if (!calibrated)
    Serial.println("WARNING: not calibrated, strokes will be unreliable");

  Serial.printf("ROWER %d READY\n\n", ROWER_ID);
}

// =====================================================
// LOOP
// =====================================================

void loop() {
  unsigned long now = millis();

  updateStatusLed();

  if (alertUntil && (int32_t)(now - alertUntil) >= 0) clearFeedback();

  // heartbeat, so the Central can show which bands are alive
  if (now - lastHeartbeat >= HEARTBEAT_MS) sendReady();

#if SIMULATE
  pollFakeStrokes();
  return;
#endif

  if (now - lastSensorTime < SENSOR_INTERVAL) return;
  lastSensorTime = now;

  int16_t rax, ray, raz, rgx, rgy, rgz;
  samples++;
  if (!readIMU(&rax, &ray, &raz, &rgx, &rgy, &rgz)) {
    i2cErrors++;
    return;                              // never feed garbage into detection
  }

  aX = (rax / ACCEL_SCALE) * GRAVITY;
  aY = (ray / ACCEL_SCALE) * GRAVITY;
  aZ = ((raz - Z_OFFSET) / Z_SCALE) * GRAVITY;

  gX = (rgx / GYRO_SCALE) - gyroBiasX;
  gY = (rgy / GYRO_SCALE) - gyroBiasY;
  gZ = (rgz / GYRO_SCALE) - gyroBiasZ;

  float mag = sqrt(aX * aX + aY * aY + aZ * aZ);
  dynAcc   = fabs(mag - GRAVITY);
  gyroMag  = sqrt(gX * gX + gY * gY + gZ * gZ);
  angleNow = atan2(sqrt(aX * aX + aY * aY), fabs(aZ)) * 180.0 / PI;

  if (driving) {
    updateDrive();
  } else {
    if (dynAcc > CATCH_ACC || gyroMag > CATCH_GYRO) onCatch();
  }

  if (now - lastDisplayTime >= DISPLAY_INTERVAL) {
    lastDisplayTime = now;
    Serial.printf("R%d acc=%.1f gyro=%.0f ang=%.0f %s",
                  ROWER_ID, dynAcc, gyroMag, angleNow,
                  driving ? "DRIVE" : "idle");
    if (i2cErrors) {
      Serial.printf("  [i2c fails %d of %d = %d%%]",
                    i2cErrors, samples, (100 * i2cErrors) / max(samples, 1));
      i2cErrors = 0;
      samples = 0;
    }
    Serial.println();
  }
}
