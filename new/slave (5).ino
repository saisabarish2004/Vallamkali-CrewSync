/*
 * slave.ino  -  rower band, simple version
 *
 * No stroke detection. Just reads the IMU and sends its current motion to
 * the Central twice a second. The Pi compares the two rowers and Gemma
 * decides whether they are moving together.
 *
 * Nothing here can get stuck, because there is no state to get stuck in.
 *
 * SET ROWER_ID PER BAND: 1 and 2.
 */

#include <Wire.h>
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>       // for esp_wifi_set_channel
#include <math.h>

#define ROWER_ID 1                          // <-- 1 on one band, 2 on the other

uint8_t centralMAC[] = {0x94, 0x54, 0xC5, 0x2F, 0x27, 0x28};

#define ESPNOW_CHANNEL 1                    // pinned, not 0, so it cannot drift

// pins, XIAO ESP32-C3 labels in brackets
#define RED_PIN     3      // D1
#define GREEN_PIN   4      // D2
#define BLUE_PIN    5      // D3
#define STATUS_PIN 20      // D7
#define SDA_PIN     6      // D4
#define SCL_PIN     7      // D5

#define MPU_ADDR   0x68                     // 0x69 if AD0 is high
const float ACCEL_SCALE = 16384.0;
const float GYRO_SCALE  = 131.0;
const float GRAVITY     = 9.80665;

const unsigned long SEND_INTERVAL = 500;    // twice a second
const unsigned long ALERT_MS      = 4000;

unsigned long lastSend = 0;
unsigned long alertUntil = 0;

// A stroke is one rise and fall of acceleration.
//   above the line  -> the drive
//   back below it   -> stroke finished, record it
// There is no way to get stuck: if nothing finishes within IDLE_MS the band
// simply reports zeros, and zeros on both bands compare as "in sync".
// Two levels, not one. The stroke starts when acceleration rises past HIGH
// and only ends when it falls below LOW. The gap between them means a small
// wobble mid-stroke cannot split one stroke into two.
// If no strokes are detected, lower both. If it fires constantly, raise both.
const float STROKE_HIGH = 1.2;       // rise past this to start
const float STROKE_LOW  = 0.6;       // fall below this to finish
const unsigned long IDLE_MS = 3000;  // no stroke for this long = at rest

float smoothAcc = 0;                 // smoothed acceleration
bool  above = false;                 // currently in the drive
unsigned long strokeStart = 0;       // when this drive began
float driveAcc = 0, driveAngle = 0;  // peaks during this drive

// the last COMPLETED stroke, sent repeatedly until a new one replaces it
uint16_t lastPeriod = 0;             // ms
float    lastAcc    = 0;             // m/s2
float    lastAngleP = 0;             // degrees from rest
unsigned long lastStrokeAt = 0;

// Resting tilt, measured at boot. We send the CHANGE from this, not the raw
// tilt. Two bands mounted at different angles both read 0 at rest, which is
// what makes them comparable.
float angleBase = 0;

bool imuOk = false;
unsigned long lastGoodRead = 0;

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

#define STATUS_CORRECT     1
#define STATUS_OUT_OF_SYNC  3
#define STATUS_NOT_FOLLOWED 4    // leader only: your crew has drifted

#define ERROR_NONE         0
#define ERROR_DURATION     1     // motion   -> RED
#define ERROR_ACCELERATION 2     // power    -> vibrate, here just blank
#define ERROR_ANGLE        4     // angle    -> BLUE
#define ERROR_FOLLOWER     5     // leader only: your follower drifted -> YELLOW

// ---------------------------------------------------------------- leds

void rgb(bool r, bool g, bool b) {
  digitalWrite(RED_PIN, r);
  digitalWrite(GREEN_PIN, g);
  digitalWrite(BLUE_PIN, b);
}

void showDecision(uint8_t status, uint8_t error) {
  if (status == STATUS_CORRECT) {
    rgb(0, 1, 0);                    // GREEN  in sync
  } else if (status == STATUS_NOT_FOLLOWED) {
    rgb(1, 1, 0);                    // YELLOW leader: crew is not with you
  } else if (error == ERROR_ANGLE) {
    rgb(0, 0, 1);                    // BLUE   follower: your angle is off
  } else {
    rgb(1, 0, 0);                    // RED    follower: you are out of time
  }
  alertUntil = millis() + ALERT_MS;
}

// ---------------------------------------------------------------- imu

bool readIMU(float *acc, float *gyro, float *angle) {
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x3B);
  if (Wire.endTransmission(false) != 0) return false;
  if (Wire.requestFrom(MPU_ADDR, (uint8_t)14) != 14) return false;

  uint8_t b[14];
  for (int i = 0; i < 14; i++) b[i] = Wire.read();

  // An MPU that answers with all zeros is not actually measuring. Without
  // this check the maths turns those zeros into a constant 9.8 acceleration,
  // which looks like real data and alerts forever.
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

  const char *what = d.status == STATUS_CORRECT     ? "IN SYNC (green)"
                   : d.status == STATUS_NOT_FOLLOWED ? "CREW NOT FOLLOWING YOU (yellow)"
                   : d.error  == ERROR_ANGLE         ? "YOUR ANGLE IS OFF (blue)"
                                                     : "YOU ARE OUT OF TIME (red)";
  Serial.printf("<- %s\n", what);
}

void onSent(const wifi_tx_info_t *info, esp_now_send_status_t status) {
  digitalWrite(STATUS_PIN, status == ESP_NOW_SEND_SUCCESS);
}

void sendReading() {
  if (!imuOk) {
    Serial.printf("R%d  IMU NOT READING - not sending. check wiring, "
                  "or try address 0x69\n", ROWER_ID);
    rgb(1, 0, 1);                   // purple = this band's sensor is dead
    return;
  }

  StrokePacket p;
  p.type        = 1;
  p.rower       = ROWER_ID;
  p.stroke_id   = 0;
  p.duration_ms = lastPeriod;      // stroke period, ms
  p.peak_acc    = lastAcc;         // peak acceleration of that stroke
  p.gyro_peak   = 0;
  p.angle       = lastAngleP;      // peak tilt of that stroke, from rest

  esp_now_send(centralMAC, (uint8_t *)&p, sizeof(p));

  Serial.printf("R%d  period %ums  acc %.1f  angle %.0f\n",
                ROWER_ID, lastPeriod, lastAcc, lastAngleP);
}

// ---------------------------------------------------------------- setup

void setup() {
  Serial.begin(115200);
  delay(600);

  pinMode(RED_PIN, OUTPUT);
  pinMode(GREEN_PIN, OUTPUT);
  pinMode(BLUE_PIN, OUTPUT);
  pinMode(STATUS_PIN, OUTPUT);
  rgb(0, 0, 0);

  Wire.begin(SDA_PIN, SCL_PIN);
  Wire.setClock(100000);

  Wire.beginTransmission(MPU_ADDR);         // wake the MPU
  Wire.write(0x6B);
  Wire.write(0x00);
  Wire.endTransmission();
  delay(200);

  Serial.printf("\n=== ROWER %d ===\n", ROWER_ID);

  // baseline: average resting tilt over one second. Keep the band still.
  Serial.println("hold the band still...");
  {
    float sum = 0; int n = 0;
    for (int i = 0; i < 50; i++) {
      float a, g, ang;
      if (readIMU(&a, &g, &ang)) { sum += ang; n++; }
      delay(20);
    }
    if (n > 10) {
      imuOk = true;
      lastGoodRead = millis();
      angleBase = sum / n;
      Serial.printf("resting tilt %.0f degrees, will report change from this\n",
                    angleBase);
    } else {
      Serial.println("IMU NOT READING - check wiring, or try address 0x69");
    }
  }

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

  rgb(1,0,0); delay(300);
  rgb(0,1,0); delay(300);
  rgb(0,0,1); delay(300);
  rgb(0,0,0);

  Serial.printf("ROWER %d READY\n\n", ROWER_ID);
}

// ---------------------------------------------------------------- loop

void loop() {
  unsigned long now = millis();

  if (alertUntil && (int32_t)(now - alertUntil) >= 0) {
    rgb(0, 0, 0);
    alertUntil = 0;
  }

  float acc, gyro, angle;
  if (readIMU(&acc, &gyro, &angle)) {
    imuOk = true;
    lastGoodRead = now;

    // Light smoothing. One noisy sample can no longer start or end a stroke.
    smoothAcc = smoothAcc * 0.7 + acc * 0.3;
    acc = smoothAcc;

    float tilt = fabs(angle - angleBase);

    if (!above) {
      if (acc > STROKE_HIGH) {             // drive has started
        above = true;
        strokeStart = now;
        driveAcc = acc;
        driveAngle = tilt;
      }
    } else {
      if (acc > driveAcc)   driveAcc = acc;
      if (tilt > driveAngle) driveAngle = tilt;

      if (acc < STROKE_LOW) {              // came back down, stroke complete
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
        above = false;                     // safety, never stay in a drive
      }
    }

    // no stroke for a while: report zeros so a resting band matches another
    if (now - lastStrokeAt > IDLE_MS) {
      lastPeriod = 0; lastAcc = 0; lastAngleP = 0;
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
