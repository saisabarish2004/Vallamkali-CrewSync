/*
 * central_pi.ino  -  Central ESP32, Gemma in the loop
 *
 * Works with your EXISTING slave sketch. No changes needed on the bands.
 *
 * WHAT CHANGED FROM YOUR CENTRAL
 *   Your version decided everything here, with fixed thresholds, one stroke
 *   at a time. That is not synchronisation: it checks each rower against
 *   absolute ranges, so a rower 400 ms behind the crew still passes.
 *
 *   This version does no judging. It collects 3 strokes from each rower,
 *   sends them to the Pi as one compact line, and relays back whatever
 *   Gemma decides.
 *
 * WIRING TO THE PI
 *   ESP32 TX2 (GPIO17) -> Pi pin 10   (GPIO15 / RXD)
 *   ESP32 RX2 (GPIO16) <- Pi pin  8   (GPIO14 / TXD)
 *   ESP32 GND          -- Pi pin  6   (GND)    <-- required
 *
 * ON THE PI
 *   python coach.py --port /dev/ttyAMA0 --model gemma3:1b --fast
 *
 * SENT TO THE PI, one line per batch:
 *   D|t1,t2,t3,ang,pow|t1,t2,t3,ang,pow|t1,t2,t3,ang,pow
 *     \___rower 1____/ \___rower 2____/ \___rower 3____/
 *   t   stroke start, ms from batch start
 *   ang catch angle, degrees
 *   pow peak acceleration x10
 *
 * RECEIVED FROM THE PI, two digits: rower then fault
 *   "00"  crew in sync          -> all bands GREEN
 *   "31"  rower 3, timing       -> ERROR_DURATION      band RED + vibrate
 *   "32"  rower 3, catch angle  -> ERROR_ANGLE         band BLUE
 *   "33"  rower 3, power        -> ERROR_ACCELERATION  band vibrates
 */

#include <WiFi.h>
#include <esp_now.h>
#include <string.h>

// ---------------------------------------------------------------- pins

#define PI_RX_PIN 16
#define PI_TX_PIN 17
#define PI_BAUD   115200

// ---------------------------------------------------------------- rovers

uint8_t ROVER1_MAC[] = {0xE8, 0xF6, 0x0A, 0x14, 0x5F, 0x90};
uint8_t ROVER2_MAC[] = {0xE8, 0xF6, 0x0A, 0x14, 0x32, 0x6C};
uint8_t ROVER3_MAC[] = {0xE8, 0xF6, 0x0A, 0x15, 0x07, 0x7C};

// ---------------------------------------------------------------- packets
// Unchanged from your sketches. The bands need no edits.

#define PACKET_TYPE_STROKE 1

#define STATUS_CORRECT     1
#define STATUS_WRONG       2
#define STATUS_OUT_OF_SYNC 3

#define ERROR_NONE         0
#define ERROR_DURATION     1
#define ERROR_ACCELERATION 2
#define ERROR_GYRO         3
#define ERROR_ANGLE        4

#define LED_COMMAND_OFF    0
#define LED_COMMAND_GREEN  1
#define LED_COMMAND_RED    2

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

// ---------------------------------------------------------------- batch

#define STROKES 3
// 3 strokes x slowest stroke period x 2. Raise it if you see "batch timeout"
// in the monitor while people are still rowing.
#define BATCH_TIMEOUT_MS 6000

struct Slot {
  uint32_t t0;                    // stroke START, ms, stamped here
  float    angle;
  float    acc;
};

Slot  slot[3][STROKES];           // [rower 0..2][stroke 0..2]
int   count[3] = {0, 0, 0};
uint32_t batchStart = 0;
bool  batchOpen = false;

// ESP-NOW callbacks must stay short. Strokes land in this queue and the
// main loop does the printing and the UART work.
StrokePacket  pendingPkt[8];
volatile int  qHead = 0, qTail = 0;

char piLine[16];
size_t piLen = 0;

// ---------------------------------------------------------------- helpers

uint8_t *getRoverMAC(uint8_t r) {
  if (r == 1) return ROVER1_MAC;
  if (r == 2) return ROVER2_MAC;
  if (r == 3) return ROVER3_MAC;
  return NULL;
}

void addRover(uint8_t r, uint8_t *mac) {
  if (esp_now_is_peer_exist(mac)) return;
  esp_now_peer_info_t p = {};
  memcpy(p.peer_addr, mac, 6);
  p.channel = 0;
  p.encrypt = false;
  Serial.printf("ROVER %u PEER : %s\n", r,
                esp_now_add_peer(&p) == ESP_OK ? "ADDED" : "FAILED");
}

void sendDecision(uint8_t rower, uint8_t status, uint8_t error, uint8_t led) {
  uint8_t *mac = getRoverMAC(rower);
  if (!mac) return;

  DecisionPacket d;
  d.target = rower;
  d.status = status;
  d.error  = error;
  d.led    = led;

  esp_now_send(mac, (uint8_t *)&d, sizeof(d));
}

// ---------------------------------------------------------------- espnow rx

void onDataReceive(const esp_now_recv_info_t *info,
                   const uint8_t *data, int len) {
  if (len != sizeof(StrokePacket)) return;

  StrokePacket p;
  memcpy(&p, data, sizeof(p));
  if (p.type != PACKET_TYPE_STROKE) return;
  if (p.rower < 1 || p.rower > 3) return;

  int next = (qHead + 1) % 8;
  if (next == qTail) return;            // queue full, drop
  pendingPkt[qHead] = p;
  qHead = next;
}

void onDataSent(const wifi_tx_info_t *info, esp_now_send_status_t status) { }

// ---------------------------------------------------------------- batching

void resetBatch() {
  count[0] = count[1] = count[2] = 0;
  batchOpen = false;
}

void sendBatchToPi() {
  char msg[160];
  int n = snprintf(msg, sizeof(msg), "D");

  // earliest stroke start across all nine becomes time zero
  uint32_t origin = slot[0][0].t0;
  for (int r = 0; r < 3; r++)
    for (int i = 0; i < STROKES; i++)
      if ((int32_t)(slot[r][i].t0 - origin) < 0) origin = slot[r][i].t0;

  for (int r = 0; r < 3; r++) {
    // angle and power averaged: they describe technique and barely change
    // stroke to stroke, so three values each would be wasted bytes
    float aSum = 0, pSum = 0;
    for (int i = 0; i < STROKES; i++) {
      aSum += slot[r][i].angle;
      pSum += slot[r][i].acc;
    }
    n += snprintf(msg + n, sizeof(msg) - n, "|%lu,%lu,%lu,%d,%d",
                  (unsigned long)(slot[r][0].t0 - origin),
                  (unsigned long)(slot[r][1].t0 - origin),
                  (unsigned long)(slot[r][2].t0 - origin),
                  (int)(aSum / STROKES + 0.5),
                  (int)(pSum / STROKES * 10 + 0.5));
  }

  Serial1.println(msg);
  Serial.print("[to Pi] ");
  Serial.println(msg);

  resetBatch();
}

void storeStroke(StrokePacket &p) {
  uint32_t now = millis();

  if (!batchOpen) {
    batchStart = now;
    batchOpen = true;
  }

  int r = p.rower - 1;
  if (count[r] >= STROKES) return;      // this rower is ahead, ignore extras

  // The band sends AFTER the stroke finishes, so the stroke began
  // duration_ms ago. Store it absolute; offsets are worked out at send time
  // against the earliest of all nine. Do NOT clamp to batchStart here -
  // batchStart is an arrival time, and every stroke starts before it.
  uint32_t t0 = now - p.duration_ms;

  slot[r][count[r]].t0    = t0;
  slot[r][count[r]].angle = p.angle;
  slot[r][count[r]].acc   = p.peak_acc;
  count[r]++;

  Serial.printf("R%u stroke %lu  dur=%lu acc=%.2f ang=%.1f   [%d %d %d]\n",
                p.rower, (unsigned long)p.stroke_id,
                (unsigned long)p.duration_ms, p.peak_acc, p.angle,
                count[0], count[1], count[2]);

  if (count[0] >= STROKES && count[1] >= STROKES && count[2] >= STROKES)
    sendBatchToPi();
}

// ---------------------------------------------------------------- pi rx

// Second digit from the Pi -> your existing ERROR_ codes.
//   1 timing  -> ERROR_DURATION
//   2 angle   -> ERROR_ANGLE
//   3 power   -> ERROR_ACCELERATION
uint8_t faultToError(int fault) {
  if (fault == 1) return ERROR_DURATION;
  if (fault == 2) return ERROR_ANGLE;
  if (fault == 3) return ERROR_ACCELERATION;
  return ERROR_NONE;
}

const char *faultName(int fault) {
  if (fault == 1) return "TIMING       -> band RED + vibrate";
  if (fault == 2) return "CATCH ANGLE  -> band BLUE";
  if (fault == 3) return "POWER        -> band vibrate";
  return "NONE";
}

void applyDecision(int rower, int fault) {
  if (fault == 0 || rower == 0) {
    Serial.println(">>> PI: 00   crew in sync, all bands GREEN");
    for (uint8_t r = 1; r <= 3; r++)
      sendDecision(r, STATUS_CORRECT, ERROR_NONE, LED_COMMAND_GREEN);
    return;
  }

  Serial.printf(">>> PI: %d%d   ROWER %d  %s\n",
                rower, fault, rower, faultName(fault));

  for (uint8_t r = 1; r <= 3; r++) {
    if (r == rower)
      sendDecision(r, STATUS_OUT_OF_SYNC, faultToError(fault),
                   LED_COMMAND_RED);
    else
      sendDecision(r, STATUS_CORRECT, ERROR_NONE, LED_COMMAND_GREEN);
  }
}

void pumpPi() {
  while (Serial1.available()) {
    char c = Serial1.read();
    if (c == '\n') {
      piLine[piLen] = '\0';
      if (piLen >= 2) {
        int rower = piLine[0] - '0';
        int fault = piLine[1] - '0';
        if (rower >= 0 && rower <= 3 && fault >= 0 && fault <= 3)
          applyDecision(rower, fault);
      }
      piLen = 0;
    } else if (c != '\r') {
      if (piLen < sizeof(piLine) - 1) piLine[piLen++] = c;
      else piLen = 0;
    }
  }
}

// ---------------------------------------------------------------- setup

void setup() {
  Serial.begin(115200);
  Serial1.begin(PI_BAUD, SERIAL_8N1, PI_RX_PIN, PI_TX_PIN);
  delay(800);

  WiFi.mode(WIFI_STA);

  Serial.println();
  Serial.println("========================================");
  Serial.println("       CENTRAL  -  Gemma in the loop");
  Serial.println("========================================");
  Serial.print("Central MAC : ");
  Serial.println(WiFi.macAddress());
  Serial.printf("UART to Pi  : RX=GPIO%d TX=GPIO%d @ %d\n",
                PI_RX_PIN, PI_TX_PIN, PI_BAUD);

  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW INIT FAILED");
    while (true) delay(1000);
  }
  Serial.println("ESP-NOW INIT OK");

  esp_now_register_recv_cb(onDataReceive);
  esp_now_register_send_cb(onDataSent);

  addRover(1, ROVER1_MAC);
  addRover(2, ROVER2_MAC);
  addRover(3, ROVER3_MAC);

  Serial.println();
  Serial.println("Collecting 3 strokes per rower, then asking the Pi.");
  Serial.println("CENTRAL READY");
}

void loop() {
  // drain the stroke queue outside the ESP-NOW callback
  while (qTail != qHead) {
    StrokePacket p = pendingPkt[qTail];
    qTail = (qTail + 1) % 8;
    storeStroke(p);
  }

  pumpPi();

  // One rower stopped mid batch. Do not wait forever, and do not leave a
  // band stuck on RED with no decision coming to clear it.
  if (batchOpen && millis() - batchStart > BATCH_TIMEOUT_MS) {
    Serial.printf("batch timeout [%d %d %d]", count[0], count[1], count[2]);
    for (int r = 0; r < 3; r++)
      if (count[r] < STROKES) Serial.printf("  R%d is short", r + 1);
    Serial.println();

    for (uint8_t r = 1; r <= 3; r++)          // clear every band
      sendDecision(r, STATUS_CORRECT, ERROR_NONE, LED_COMMAND_GREEN);

    resetBatch();
  }
}
