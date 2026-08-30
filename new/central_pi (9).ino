/*
 * central_pi.ino  -  Central ESP32, simple version
 *
 * Keeps the latest reading from each band and sends both to the Pi twice a
 * second. No buffering, no batching, no waiting. If a band has sent anything
 * recently, its numbers go out.
 *
 * WIRING TO THE PI
 *   ESP32 GPIO17 (TX) -> Pi pin 10   (GPIO15 / RXD)
 *   ESP32 GPIO16 (RX) <- Pi pin  8   (GPIO14 / TXD)
 *   ESP32 GND         -- Pi pin  6   (GND)    <-- required
 *
 * ON THE PI
 *   python coach.py --port /dev/ttyAMA0 --model gemma3:1b --fast
 *
 * SENT TO THE PI:   D|period,acc,angle|period,acc,angle
 *   period  last stroke period, centiseconds  (0 = not rowing)
 *   acc     peak acceleration of that stroke x10
 *   angle   peak tilt of that stroke, degrees from rest
 *
 * RECEIVED FROM THE PI, two digits, rower then fault:
 *   00 both fine     x1 motion    x2 angle    x3 power
 */

#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>       // for esp_wifi_set_channel
#include <string.h>

#define NUM_ROWERS 2
#define ESPNOW_CHANNEL 1

#define PI_RX_PIN 16
#define PI_TX_PIN 17
#define PI_BAUD   115200

uint8_t ROVER1_MAC[] = {0xE8, 0xF6, 0x0A, 0x14, 0x5F, 0x90};
uint8_t ROVER2_MAC[] = {0xE8, 0xF6, 0x0A, 0x14, 0x32, 0x6C};

const unsigned long SEND_INTERVAL = 2000;   // ask the Pi this often
const unsigned long ALIVE_MS      = 4000;   // band counts as present

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
#define STATUS_OUT_OF_SYNC 3
#define STATUS_NOT_FOLLOWED 4    // sent to the leader: your crew is adrift

#define ERROR_NONE         0
#define ERROR_DURATION     1
#define ERROR_ACCELERATION 2
#define ERROR_ANGLE        4
#define ERROR_FOLLOWER     5     // sent to the leader: your follower drifted

#define LED_COMMAND_GREEN  1
#define LED_COMMAND_RED    2

// ---------------------------------------------------------------- state

uint16_t period[3] = {0, 0, 0};     // stroke period, ms
float    acc[3]    = {0, 0, 0};     // peak acceleration of that stroke
float    angle[3]  = {0, 0, 0};     // peak tilt of that stroke, from rest
uint32_t seen[3]  = {0, 0, 0};

unsigned long lastSend = 0;
char piLine[16];
size_t piLen = 0;

// ---------------------------------------------------------------- espnow

uint8_t *macFor(uint8_t r) {
  if (r == 1) return ROVER1_MAC;
  if (r == 2) return ROVER2_MAC;
  return NULL;
}

void addRover(uint8_t r, uint8_t *mac) {
  esp_now_peer_info_t p = {};
  memcpy(p.peer_addr, mac, 6);
  p.channel = ESPNOW_CHANNEL;
  p.encrypt = false;
  Serial.printf("ROVER %u PEER : %s\n", r,
                esp_now_add_peer(&p) == ESP_OK ? "ADDED" : "FAILED");
}

void onRecv(const esp_now_recv_info_t *info, const uint8_t *data, int len) {
  if (len != sizeof(StrokePacket)) return;
  StrokePacket p;
  memcpy(&p, data, sizeof(p));
  if (p.rower < 1 || p.rower > NUM_ROWERS) return;

  int i = p.rower - 1;
  period[i] = p.duration_ms;
  acc[i]    = p.peak_acc;
  angle[i]  = p.angle;
  seen[i]   = millis();
}

void onSent(const wifi_tx_info_t *info, esp_now_send_status_t status) { }

void sendDecision(uint8_t r, uint8_t status, uint8_t error, uint8_t led) {
  uint8_t *mac = macFor(r);
  if (!mac) return;
  DecisionPacket d = { r, status, error, led };
  esp_now_send(mac, (uint8_t *)&d, sizeof(d));
}

// ---------------------------------------------------------------- to the pi

void sendToPi() {
  char msg[96];
  int n = snprintf(msg, sizeof(msg), "D");
  for (int i = 0; i < NUM_ROWERS; i++)
    n += snprintf(msg + n, sizeof(msg) - n, "|%d,%d,%d",
                  period[i] / 10,                  // centiseconds
                  (int)(acc[i] * 10 + 0.5),        // acceleration x10
                  (int)(angle[i] + 0.5));          // degrees

  Serial1.println(msg);
  Serial.print("[to Pi] ");
  Serial.print(msg);

  Serial.print("   bands:");
  uint32_t now = millis();
  for (int i = 0; i < NUM_ROWERS; i++)
    Serial.printf("  R%d %s", i + 1,
                  (seen[i] && now - seen[i] < ALIVE_MS) ? "ok" : "MISSING");
  Serial.println();
}

// ---------------------------------------------------------------- from pi

const char *faultName(int f) {
  if (f == 1) return "MOTION";
  if (f == 2) return "ANGLE";
  if (f == 3) return "POWER";
  return "NONE";
}

uint8_t faultToError(int f) {
  if (f == 2) return ERROR_ANGLE;
  if (f == 3) return ERROR_ACCELERATION;
  return ERROR_DURATION;
}

void applyDecision(int rower, int fault) {
  Serial.println();
  Serial.println("+--------+-------------+------------------------+");

  if (fault == 0) {
    Serial.println("|   1    | LEADING     | GREEN                  |");
    Serial.println("|   2    | FOLLOWING   | GREEN                  |");
    sendDecision(1, STATUS_CORRECT, ERROR_NONE, LED_COMMAND_GREEN);
    sendDecision(2, STATUS_CORRECT, ERROR_NONE, LED_COMMAND_GREEN);
  } else {
    // rower 2 has drifted: he gets the fault, and the leader is told his
    // crew is no longer with him
    Serial.printf("|   1    | NOT FOLLOWED| YELLOW                 |\n");
    Serial.printf("|   2    | %-11s | %-22s |\n",
                  faultName(fault), fault == 2 ? "BLUE" : "RED");
    sendDecision(1, STATUS_NOT_FOLLOWED, ERROR_NONE, LED_COMMAND_RED);
    sendDecision(2, STATUS_OUT_OF_SYNC, faultToError(fault), LED_COMMAND_RED);
  }

  Serial.println("+--------+-------------+------------------------+");
  if (fault != 0)
    Serial.printf("  rower 2 is not following the stroke  (%s)\n",
                  faultName(fault));
  Serial.println();
}

void pumpPi() {
  while (Serial1.available()) {
    char c = Serial1.read();
    if (c == '\n') {
      piLine[piLen] = '\0';
      if (piLen >= 2) {
        int rower = piLine[0] - '0';
        int fault = piLine[1] - '0';
        if (rower >= 0 && rower <= NUM_ROWERS && fault >= 0 && fault <= 3)
          applyDecision(rower, fault);
      }
      piLen = 0;
    } else if (c != '\r' && piLen < sizeof(piLine) - 1) {
      piLine[piLen++] = c;
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
  Serial.println("       CENTRAL  -  simple version");
  Serial.println("========================================");
  Serial.print("Central MAC : ");
  Serial.println(WiFi.macAddress());
  Serial.printf("UART to Pi  : RX=GPIO%d TX=GPIO%d @ %d\n",
                PI_RX_PIN, PI_TX_PIN, PI_BAUD);

  esp_wifi_set_channel(ESPNOW_CHANNEL, WIFI_SECOND_CHAN_NONE);

  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW INIT FAILED");
    while (true) delay(1000);
  }
  esp_now_register_recv_cb(onRecv);
  esp_now_register_send_cb(onSent);

  addRover(1, ROVER1_MAC);
  addRover(2, ROVER2_MAC);

  Serial.printf("rowers in the boat : %d\n", NUM_ROWERS);
  Serial.println("CENTRAL READY\n");
}

void loop() {
  pumpPi();

  unsigned long now = millis();
  if (now - lastSend >= SEND_INTERVAL) {
    lastSend = now;
    sendToPi();                       // always sends, never waits
  }
}
