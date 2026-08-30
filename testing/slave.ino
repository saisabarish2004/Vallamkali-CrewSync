/*
 * slave.ino  -  rower band, worn on the wrist
 *
 * A stroke is one rise and fall of acceleration. The band reports the
 * period, the peak acceleration and the peak tilt of the last stroke.
 *
 * WRIST MOUNTING
 *   Every wrist holds the band at a different angle, so the band measures
 *   its own resting tilt at boot and reports the CHANGE from it. It also
 *   re-zeroes slowly whenever the rower stops, so a band that shifts on the
 *   wrist mid-race corrects itself instead of alerting forever.
 *
 * SET ROWER_ID PER BAND: 1 and 2.
 */

#include <Wire.h>
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <math.h>

#define ROWER_ID 1                          // <-- 1 on one band, 2 on the other

uint8_t centralMAC[] = {0x94, 0x54, 0xC5, 0x2F, 0x27, 0x28};

#define ESPNOW_CHANNEL 1

// pins, XIAO ESP32-C3 labels in brackets
#define RED_PIN     3      // D1
#define GREEN_PIN   4      // D2
#define BLUE_PIN    5      // D3
#define STATUS_PIN 20      // D7
#define VIBE_PIN   10      // D10   through a transistor, never straight to GPIO
#define BUTTON_PIN 21      // D6    button to GND, uses the internal pullup
#define SDA_PIN     6      // D4
#define SCL_PIN     7      // D5

#define MPU_ADDR   0x68                     // 0x69 if AD0 is high
const float ACCEL_SCALE = 16384.0;
const float GYRO_SCALE  = 131.0;
const float GRAVITY     = 9.80665;

const unsigned long SEND_INTERVAL = 500;
const unsigned long ALERT_MS      = 4000;

unsigned long lastSend = 0;
unsigned long alertUntil = 0;

// stroke detection: rise past HIGH, fall below LOW
const float STROKE_HIGH = 1.2;
const float STROKE_LOW  = 0.6;
const unsigned long IDLE_MS = 3000;

float smoothAcc = 0;
bool  above = false;
unsigned long strokeStart = 0;
float driveAcc = 0, driveAngle = 0;

uint16_t lastPeriod = 0;
float    lastAcc    = 0;
float    lastAngleP = 0;
unsigned long lastStrokeAt = 0;

// Resting tilt of THIS wrist. Set at boot, then nudged whenever the rower is
// still, so the band re-zeroes itself if it slips or the arm settles into a
// new position. Without this a band that shifts mid-race alerts forever.
float angleBase = 0;
const float RECAL_RATE = 0.02;       // how fast it re-zeroes when at rest

bool imuOk = false;
unsigned long lastGoodRead = 0;

// Button: short press re-zeroes this wrist, long press toggles logging.
bool logging = false;
bool btnDown = false;
unsigned long btnAt = 0;
unsigned long lastLog = 0;

// ---------------------------------------------------------------- packets

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

#define STATUS_CORRECT      1
#define STATUS_OUT_OF_SYNC  3
#define STATUS_NOT_FOLLOWED 4

#define ERROR_NONE         0
#define ERROR_DURATION     1     // timing -> RED
#define ERROR_ACCELERATION 2     // power  -> MAGENTA
#define ERROR_ANGLE        4     // angle  -> BLUE

// ---------------------------------------------------------------- feedback

void rgb(bool r, bool g, bool b) {
  digitalWrite(RED_PIN, r);
  digitalWrite(GREEN_PIN, g);
  digitalWrite(BLUE_PIN, b);
}

// A buzz pattern the rower can feel without looking at the band.
//   timing  two short pulses
//   angle   one long pulse
//   power   three short pulses
// Non blocking: the pattern plays out in the main loop.
int  buzzLeft = 0;                   // pulses still to play
int  buzzOn = 0, buzzOff = 0;        // ms on, ms off
bool buzzing = false;
unsigned long buzzNext = 0;

void startBuzz(int pulses, int onMs, int offMs) {
  buzzLeft = pulses;
  buzzOn = onMs;
  buzzOff = offMs;
  buzzing = false;
  buzzNext = millis();
}

void updateBuzz() {
  if (buzzLeft <= 0) {
    digitalWrite(VIBE_PIN, LOW);
    return;
  }
  unsigned long now = millis();
  if ((int32_t)(now - buzzNext) < 0) return;

  if (!buzzing) {
    digitalWrite(VIBE_PIN, HIGH);
    buzzing = true;
    buzzNext = now + buzzOn;
  } else {
    digitalWrite(VIBE_PIN, LOW);
    buzzing = false;
    buzzLeft--;
    buzzNext = now + buzzOff;
  }
}

void showDecision(uint8_t status, uint8_t error) {
  if (status == STATUS_CORRECT) {
    rgb(0, 1, 0);                       // GREEN   in sync
    buzzLeft = 0;                       // no buzz when everything is right
    digitalWrite(VIBE_PIN, LOW);
  } else if (status == STATUS_NOT_FOLLOWED) {
    rgb(1, 1, 0);                       // YELLOW  leader: crew adrift
    startBuzz(1, 120, 120);             // one light tap
  } else if (error == ERROR_ANGLE) {
    rgb(0, 0, 1);                       // BLUE    your angle is off
    startBuzz(1, 450, 150);             // one long buzz
  } else if (error == ERROR_ACCELERATION) {
    rgb(1, 0, 1);                       // MAGENTA your power is off
    startBuzz(3, 120, 120);             // three short
  } else {
    rgb(1, 0, 0);                       // RED     your timing is off
    startBuzz(2, 150, 150);             // two short
  }
  alertUntil = millis() + ALERT_MS;
}

bool readIMUraw(float *acc, float *gyro, float *angle);   // defined below

// ---------------------------------------------------------------- calibration

// Measures what "at rest" means for THIS wrist. Everything the band reports
// afterwards is the change from this, so two people wearing the band at
// completely different angles both read zero when still.
void calibrate() {
  Serial.println("hold your arm still...");
  rgb(0, 0, 1);                          // blue while measuring

  float sum = 0; int n = 0;
  for (int i = 0; i < 50; i++) {
    float a, g, ang;
    if (readIMUraw(&a, &g, &ang)) { sum += ang; n++; }
    delay(20);
  }

  if (n > 10) {
    imuOk = true;
    lastGoodRead = millis();
    angleBase = sum / n;
    smoothAcc = 0;
    above = false;
    lastPeriod = 0; lastAcc = 0; lastAngleP = 0;
    Serial.printf("calibrated: resting tilt %.0f degrees\n", angleBase);
    rgb(0, 1, 0);                        // green flash = done
    digitalWrite(VIBE_PIN, HIGH); delay(120); digitalWrite(VIBE_PIN, LOW);
    delay(300);
  } else {
    Serial.println("CALIBRATION FAILED - check wiring, or try address 0x69");
    rgb(1, 0, 1);
    delay(600);
  }
  rgb(0, 0, 0);
}

// ---------------------------------------------------------------- button

void checkButton() {
  bool pressed = (digitalRead(BUTTON_PIN) == LOW);
  unsigned long now = millis();

  if (pressed && !btnDown) {
    btnDown = true;
    btnAt = now;
  } else if (!pressed && btnDown) {
    btnDown = false;
    unsigned long held = now - btnAt;

    if (held >= 1500) {                  // long press: logging on or off
      logging = !logging;
      Serial.println(logging
        ? "\nLOGGING ON  ms,acc,smooth,gyro,angle,tilt,driving"
        : "\nLOGGING OFF");
      for (int i = 0; i < 2; i++) {      // two blinks to confirm
        rgb(0,1,1); delay(120); rgb(0,0,0); delay(120);
      }
    } else if (held >= 50) {             // short press: re-zero this wrist
      Serial.println("\nre-calibrating");
      calibrate();
    }
  }
}

// ---------------------------------------------------------------- imu

bool readIMUraw(float *acc, float *gyro, float *angle) {
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x3B);
  if (Wire.endTransmission(false) != 0) return false;
  if (Wire.requestFrom(MPU_ADDR, (uint8_t)14) != 14) return false;

  uint8_t b[14];
  for (int i = 0; i < 14; i++) b[i] = Wire.read();

  // An MPU answering with all zeros is not measuring. Without this the maths
  // turns those zeros into a constant 9.8 and it alerts forever.
  bool allZero = true;
  for (int i = 0; i < 14; i++) if (b[i] != 0) { allZero = false; break; }
  if (allZero) return false;

  float ax = (int16_t)((b[0]  << 8) | b[1])  / ACCEL_SCALE * GRAVITY;
  float ay = (int16_t)((b[2]  << 8) | b[3])  / ACCEL_SCALE * GRAVITY;
  float az = (int16_t)((b[4]  << 8) | b[5])  / ACCEL_SCALE * GRAVITY;
  float gx = (int16_t)((b[8]  << 8) | b[9])  / GYRO_SCALE;
  float gy = (int16_t)((b[10] << 8) | b[11]) / GYRO_SCALE;
  float gz = (int16_t)((b[12] << 8) | b[13]) / GYRO_SCALE;

  *acc   = fabs(sqrt(ax*ax + ay*ay + az*az) - GRAVITY);
  *gyro  = sqrt(gx*gx + gy*gy + gz*gz);
  *angle = atan2(sqrt(ax*ax + ay*ay), fabs(az)) * 180.0 / PI;
  return true;
}

// ---------------------------------------------------------------- espnow

void onRecv(const esp_now_recv_info_t *info, const uint8_t *data, int len) {
  if (len != sizeof(DecisionPacket)) return;
  DecisionPacket d;
  memcpy(&d, data, sizeof(d));
  if (d.target != ROWER_ID) return;

  showDecision(d.status, d.error);

  const char *what = d.status == STATUS_CORRECT      ? "IN SYNC (green)"
                   : d.status == STATUS_NOT_FOLLOWED ? "CREW NOT FOLLOWING YOU (yellow)"
                   : d.error  == ERROR_ANGLE         ? "YOUR ANGLE IS OFF (blue)"
                   : d.error  == ERROR_ACCELERATION  ? "YOUR POWER IS OFF (magenta)"
                                                     : "YOU ARE OUT OF TIME (red)";
  Serial.printf("<- %s\n", what);
}

void onSent(const wifi_tx_info_t *info, esp_now_send_status_t status) {
  digitalWrite(STATUS_PIN, status == ESP_NOW_SEND_SUCCESS);
}

void sendReading() {
  if (!imuOk) {
    Serial.printf("R%d  IMU NOT READING - check wiring, or try 0x69\n", ROWER_ID);
    rgb(1, 0, 1);
    return;
  }

  StrokePacket p;
  p.type        = 1;
  p.rower       = ROWER_ID;
  p.stroke_id   = 0;
  p.duration_ms = lastPeriod;
  p.peak_acc    = lastAcc;
  p.gyro_peak   = 0;
  p.angle       = lastAngleP;

  esp_now_send(centralMAC, (uint8_t *)&p, sizeof(p));

  Serial.printf("R%d  period %ums  acc %.1f  angle %.0f  (rest %.0f)\n",
                ROWER_ID, lastPeriod, lastAcc, lastAngleP, angleBase);
}

// ---------------------------------------------------------------- setup

void setup() {
  Serial.begin(115200);
  delay(600);

  pinMode(RED_PIN, OUTPUT);
  pinMode(GREEN_PIN, OUTPUT);
  pinMode(BLUE_PIN, OUTPUT);
  pinMode(STATUS_PIN, OUTPUT);
  pinMode(VIBE_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  rgb(0, 0, 0);
  digitalWrite(VIBE_PIN, LOW);

  Wire.begin(SDA_PIN, SCL_PIN);
  Wire.setClock(100000);

  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x6B);
  Wire.write(0x00);
  Wire.endTransmission();
  delay(200);

  Serial.printf("\n=== ROWER %d ===\n", ROWER_ID);

  // Wear the band FIRST, then power it on. Press the button any time to
  // re-zero, for example after adjusting the strap.
  calibrate();

  WiFi.mode(WIFI_STA);
  Serial.print("MAC : ");
  Serial.println(WiFi.macAddress());

  esp_wifi_set_channel(ESPNOW_CHANNEL, WIFI_SECOND_CHAN_NONE);

  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW INIT FAILED");
    while (true) delay(1000);
  }
  esp_now_register_recv_cb(onRecv);
  esp_now_register_send_cb(onSent);

  esp_now_peer_info_t peer = {};
  memcpy(peer.peer_addr, centralMAC, 6);
  peer.channel = ESPNOW_CHANNEL;
  peer.encrypt = false;
  Serial.printf("central peer : %s\n",
                esp_now_add_peer(&peer) == ESP_OK ? "added" : "FAILED");

  // colour and buzz test, so you know every output is wired
  Serial.println("test: green yellow red blue magenta, then a buzz");
  rgb(0,1,0); delay(300);
  rgb(1,1,0); delay(300);
  rgb(1,0,0); delay(300);
  rgb(0,0,1); delay(300);
  rgb(1,0,1); delay(300);
  rgb(0,0,0);
  digitalWrite(VIBE_PIN, HIGH); delay(200); digitalWrite(VIBE_PIN, LOW);

  Serial.printf("ROWER %d READY\n", ROWER_ID);
  Serial.println("button: short press = re-calibrate, "
                 "hold 1.5s = logging on/off\n");
}

// ---------------------------------------------------------------- loop

void loop() {
  unsigned long now = millis();

  updateBuzz();
  checkButton();

  // Logging mode: raw numbers at 20 Hz, so you can row for a minute and see
  // what the real motion looks like before choosing thresholds.
  if (logging && now - lastLog >= 50) {
    lastLog = now;
    float a, g, ang;
    if (readIMUraw(&a, &g, &ang))
      Serial.printf("%lu,%.2f,%.2f,%.0f,%.1f,%.1f,%d\n",
                    now, a, smoothAcc, g, ang, fabs(ang - angleBase), above);
    rgb(0, 1, 1);                        // cyan while logging
    return;                              // no strokes, no sending
  }

  if (alertUntil && (int32_t)(now - alertUntil) >= 0) {
    rgb(0, 0, 0);
    alertUntil = 0;
  }

  float acc, gyro, angle;
  if (readIMUraw(&acc, &gyro, &angle)) {
    imuOk = true;
    lastGoodRead = now;

    smoothAcc = smoothAcc * 0.7 + acc * 0.3;
    acc = smoothAcc;

    float tilt = fabs(angle - angleBase);

    if (!above) {
      if (acc > STROKE_HIGH) {
        above = true;
        strokeStart = now;
        driveAcc = acc;
        driveAngle = tilt;
      }
    } else {
      if (acc > driveAcc)    driveAcc = acc;
      if (tilt > driveAngle) driveAngle = tilt;

      if (acc < STROKE_LOW) {
        above = false;
        unsigned long period = now - strokeStart;
        if (period >= 150 && period <= 3000) {
          lastPeriod   = period;
          lastAcc      = driveAcc;
          lastAngleP   = driveAngle;
          lastStrokeAt = now;
          Serial.printf("stroke: %lums  acc %.1f  angle %.0f\n",
                        period, driveAcc, driveAngle);
        }
      } else if (now - strokeStart > 3000) {
        above = false;
      }
    }

    if (now - lastStrokeAt > IDLE_MS) {
      lastPeriod = 0; lastAcc = 0; lastAngleP = 0;

      // At rest, drift the resting tilt toward wherever the arm is now.
      // A band that slips on the wrist re-zeroes itself in a few seconds
      // instead of reporting a permanent angle error.
      if (acc < STROKE_LOW)
        angleBase = angleBase * (1.0 - RECAL_RATE) + angle * RECAL_RATE;
    }
  } else if (now - lastGoodRead > 2000) {
    imuOk = false;
  }

  if (now - lastSend >= SEND_INTERVAL) {
    lastSend = now;
    sendReading();
  }

  delay(20);
}
