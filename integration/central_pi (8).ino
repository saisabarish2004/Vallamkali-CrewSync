FQBN: esp32:esp32:esp32
Using board 'esp32' from platform in folder: C:\Users\meera\AppData\Local\Arduino15\packages\esp32\hardware\esp32\3.3.10
Using core 'esp32' from platform in folder: C:\Users\meera\AppData\Local\Arduino15\packages\esp32\hardware\esp32\3.3.10

cmd /c if exist "C:\\Users\\meera\\AppData\\Local\\Temp\\.arduinoIDE-unsaved2026730-1088-158dj95.3ywh\\sketch_aug30f\\partitions.csv" COPY /y "C:\\Users\\meera\\AppData\\Local\\Temp\\.arduinoIDE-unsaved2026730-1088-158dj95.3ywh\\sketch_aug30f\\partitions.csv" "C:\\Users\\meera\\AppData\\Local\\arduino\\sketches\\A72D31F91863EBE3701D0A6938A60BFF\\partitions.csv"
cmd /c if not exist "C:\\Users\\meera\\AppData\\Local\\arduino\\sketches\\A72D31F91863EBE3701D0A6938A60BFF\\partitions.csv" if exist "C:\\Users\\meera\\AppData\\Local\\Arduino15\\packages\\esp32\\hardware\\esp32\\3.3.10\\variants\\esp32\\partitions.csv" COPY "C:\\Users\\meera\\AppData\\Local\\Arduino15\\packages\\esp32\\hardware\\esp32\\3.3.10\\variants\\esp32\\partitions.csv" "C:\\Users\\meera\\AppData\\Local\\arduino\\sketches\\A72D31F91863EBE3701D0A6938A60BFF\\partitions.csv"
cmd /c if not exist "C:\\Users\\meera\\AppData\\Local\\arduino\\sketches\\A72D31F91863EBE3701D0A6938A60BFF\\partitions.csv" COPY "C:\\Users\\meera\\AppData\\Local\\Arduino15\\packages\\esp32\\hardware\\esp32\\3.3.10\\tools\\partitions\\default.csv" "C:\\Users\\meera\\AppData\\Local\\arduino\\sketches\\A72D31F91863EBE3701D0A6938A60BFF\\partitions.csv"
cmd /c IF EXIST "C:\\Users\\meera\\AppData\\Local\\Temp\\.arduinoIDE-unsaved2026730-1088-158dj95.3ywh\\sketch_aug30f\\bootloader.bin" ( COPY /y "C:\\Users\\meera\\AppData\\Local\\Temp\\.arduinoIDE-unsaved2026730-1088-158dj95.3ywh\\sketch_aug30f\\bootloader.bin" "C:\\Users\\meera\\AppData\\Local\\arduino\\sketches\\A72D31F91863EBE3701D0A6938A60BFF\\sketch_aug30f.ino.bootloader.bin" ) ELSE ( IF EXIST "C:\\Users\\meera\\AppData\\Local\\Arduino15\\packages\\esp32\\hardware\\esp32\\3.3.10\\variants\\esp32\\bootloader.bin" ( COPY "C:\\Users\\meera\\AppData\\Local\\Arduino15\\packages\\esp32\\hardware\\esp32\\3.3.10\\variants\\esp32\\bootloader.bin" "C:\\Users\\meera\\AppData\\Local\\arduino\\sketches\\A72D31F91863EBE3701D0A6938A60BFF\\sketch_aug30f.ino.bootloader.bin" ) ELSE ( "C:\\Users\\meera\\AppData\\Local\\Arduino15\\packages\\esp32\\tools\\esptool_py\\5.3.0\\esptool.exe" --chip esp32 elf2image --flash-mode dio --flash-freq 80m --flash-size 4MB -o "C:\\Users\\meera\\AppData\\Local\\arduino\\sketches\\A72D31F91863EBE3701D0A6938A60BFF\\sketch_aug30f.ino.bootloader.bin" "C:\\Users\\meera\\AppData\\Local\\Arduino15\\packages\\esp32\\tools\\esp32-libs\\3.3.10\\bin\\bootloader_qio_80m.elf" ) )
esptool v5.3.0
Creating ESP32 image...
Merged 2 ELF sections.
Successfully created ESP32 image.
cmd /c if exist "C:\\Users\\meera\\AppData\\Local\\Temp\\.arduinoIDE-unsaved2026730-1088-158dj95.3ywh\\sketch_aug30f\\build_opt.h" COPY /y "C:\\Users\\meera\\AppData\\Local\\Temp\\.arduinoIDE-unsaved2026730-1088-158dj95.3ywh\\sketch_aug30f\\build_opt.h" "C:\\Users\\meera\\AppData\\Local\\arduino\\sketches\\A72D31F91863EBE3701D0A6938A60BFF\\build_opt.h"
cmd /c if not exist "C:\\Users\\meera\\AppData\\Local\\arduino\\sketches\\A72D31F91863EBE3701D0A6938A60BFF\\build_opt.h" type nul > "C:\\Users\\meera\\AppData\\Local\\arduino\\sketches\\A72D31F91863EBE3701D0A6938A60BFF\\build_opt.h"
cmd /c type nul > "C:\\Users\\meera\\AppData\\Local\\arduino\\sketches\\A72D31F91863EBE3701D0A6938A60BFF/file_opts"
cmd /c COPY /y "C:\\Users\\meera\\AppData\\Local\\Arduino15\\packages\\esp32\\tools\\esp32-libs\\3.3.10\\sdkconfig" "C:\\Users\\meera\\AppData\\Local\\arduino\\sketches\\A72D31F91863EBE3701D0A6938A60BFF\\sdkconfig"
        1 file(s) copied.
Detecting libraries used...
C:\Users\meera\AppData\Local\Arduino15\packages\esp32\tools\esp-x32\2601/bin/xtensa-esp32-elf-g++ -c @C:\Users\meera\AppData\Local\Arduino15\packages\esp32\tools\esp32-libs\3.3.10/flags/cpp_flags -w -Os -Werror=return-type -w -x c++ -E -CC -DF_CPU=240000000L -DARDUINO=10607 -DARDUINO_ESP32_DEV -DARDUINO_ARCH_ESP32 -DARDUINO_BOARD="ESP32_DEV" -DARDUINO_VARIANT="esp32" -DARDUINO_PARTITION_default -DARDUINO_HOST_OS="windows" -DARDUINO_FQBN="esp32:esp32:esp32:UploadSpeed=921600,CPUFreq=240,FlashFreq=80,FlashMode=qio,FlashSize=4M,PartitionScheme=default,DebugLevel=none,PSRAM=disabled,LoopCore=1,EventsCore=1,EraseFlash=none,JTAGAdapter=default,ZigbeeMode=default" -DESP32=ESP32 -DCORE_DEBUG_LEVEL=0 -DARDUINO_RUNNING_CORE=1 -DARDUINO_EVENT_RUNNING_CORE=1 -DARDUINO_USB_CDC_ON_BOOT=0 @C:\Users\meera\AppData\Local\Arduino15\packages\esp32\tools\esp32-libs\3.3.10/flags/defines -IC:\Users\meera\AppData\Local\Temp\.arduinoIDE-unsaved2026730-1088-158dj95.3ywh\sketch_aug30f -iprefix C:\Users\meera\AppData\Local\Arduino15\packages\esp32\tools\esp32-libs\3.3.10/include/ @C:\Users\meera\AppData\Local\Arduino15\packages\esp32\tools\esp32-libs\3.3.10/flags/includes -IC:\Users\meera\AppData\Local\Arduino15\packages\esp32\tools\esp32-libs\3.3.10/qio_qspi/include -IC:\Users\meera\AppData\Local\Arduino15\packages\esp32\hardware\esp32\3.3.10\cores\esp32 -IC:\Users\meera\AppData\Local\Arduino15\packages\esp32\hardware\esp32\3.3.10\variants\esp32 @C:\Users\meera\AppData\Local\arduino\sketches\A72D31F91863EBE3701D0A6938A60BFF/build_opt.h @C:\Users\meera\AppData\Local\arduino\sketches\A72D31F91863EBE3701D0A6938A60BFF/file_opts C:\Users\meera\AppData\Local\arduino\sketches\A72D31F91863EBE3701D0A6938A60BFF\sketch\sketch_aug30f.ino.cpp.merged -o nul -MMD -MF C:\Users\meera\AppData\Local\arduino\sketches\A72D31F91863EBE3701D0A6938A60BFF\sketch\sketch_aug30f.ino.cpp.merged.libsdetect.d
Alternatives for WiFi.h: [WiFi@3.3.10]
ResolveLibrary(WiFi.h)
  -> candidates: [WiFi@3.3.10]
C:\Users\meera\AppData\Local\Arduino15\packages\esp32\tools\esp-x32\2601/bin/xtensa-esp32-elf-g++ -c @C:\Users\meera\AppData\Local\Arduino15\packages\esp32\tools\esp32-libs\3.3.10/flags/cpp_flags -w -Os -Werror=return-type -w -x c++ -E -CC -DF_CPU=240000000L -DARDUINO=10607 -DARDUINO_ESP32_DEV -DARDUINO_ARCH_ESP32 -DARDUINO_BOARD="ESP32_DEV" -DARDUINO_VARIANT="esp32" -DARDUINO_PARTITION_default -DARDUINO_HOST_OS="windows" -DARDUINO_FQBN="esp32:esp32:esp32:UploadSpeed=921600,CPUFreq=240,FlashFreq=80,FlashMode=qio,FlashSize=4M,PartitionScheme=default,DebugLevel=none,PSRAM=disabled,LoopCore=1,EventsCore=1,EraseFlash=none,JTAGAdapter=default,ZigbeeMode=default" -DESP32=ESP32 -DCORE_DEBUG_LEVEL=0 -DARDUINO_RUNNING_CORE=1 -DARDUINO_EVENT_RUNNING_CORE=1 -DARDUINO_USB_CDC_ON_BOOT=0 @C:\Users\meera\AppData\Local\Arduino15\packages\esp32\tools\esp32-libs\3.3.10/flags/defines -IC:\Users\meera\AppData\Local\Temp\.arduinoIDE-unsaved2026730-1088-158dj95.3ywh\sketch_aug30f -iprefix C:\Users\meera\AppData\Local\Arduino15\packages\esp32\tools\esp32-libs\3.3.10/include/ @C:\Users\meera\AppData\Local\Arduino15\packages\esp32\tools\esp32-libs\3.3.10/flags/includes -IC:\Users\meera\AppData\Local\Arduino15\packages\esp32\tools\esp32-libs\3.3.10/qio_qspi/include -IC:\Users\meera\AppData\Local\Arduino15\packages\esp32\hardware\esp32\3.3.10\cores\esp32 -IC:\Users\meera\AppData\Local\Arduino15\packages\esp32\hardware\esp32\3.3.10\variants\esp32 -IC:\Users\meera\AppData\Local\Arduino15\packages\esp32\hardware\esp32\3.3.10\libraries\WiFi\src @C:\Users\meera\AppData\Local\arduino\sketches\A72D31F91863EBE3701D0A6938A60BFF/build_opt.h @C:\Users\meera\AppData\Local\arduino\sketches\A72D31F91863EBE3701D0A6938A60BFF/file_opts C:\Users\meera\AppData\Local\arduino\sketches\A72D31F91863EBE3701D0A6938A60BFF\sketch\sketch_aug30f.ino.cpp.merged -o nul -MMD -MF C:\Users\meera\AppData\Local\arduino\sketches\A72D31F91863EBE3701D0A6938A60BFF\sketch\sketch_aug30f.ino.cpp.merged.libsdetect.d
Alternatives for Network.h: [Networking@3.3.10]
ResolveLibrary(Network.h)
  -> candidates: [Networking@3.3.10]
C:\Users\meera\AppData\Local\Arduino15\packages\esp32\tools\esp-x32\2601/bin/xtensa-esp32-elf-g++ -c @C:\Users\meera\AppData\Local\Arduino15\packages\esp32\tools\esp32-libs\3.3.10/flags/cpp_flags -w -Os -Werror=return-type -w -x c++ -E -CC -DF_CPU=240000000L -DARDUINO=10607 -DARDUINO_ESP32_DEV -DARDUINO_ARCH_ESP32 -DARDUINO_BOARD="ESP32_DEV" -DARDUINO_VARIANT="esp32" -DARDUINO_PARTITION_default -DARDUINO_HOST_OS="windows" -DARDUINO_FQBN="esp32:esp32:esp32:UploadSpeed=921600,CPUFreq=240,FlashFreq=80,FlashMode=qio,FlashSize=4M,PartitionScheme=default,DebugLevel=none,PSRAM=disabled,LoopCore=1,EventsCore=1,EraseFlash=none,JTAGAdapter=default,ZigbeeMode=default" -DESP32=ESP32 -DCORE_DEBUG_LEVEL=0 -DARDUINO_RUNNING_CORE=1 -DARDUINO_EVENT_RUNNING_CORE=1 -DARDUINO_USB_CDC_ON_BOOT=0 @C:\Users\meera\AppData\Local\Arduino15\packages\esp32\tools\esp32-libs\3.3.10/flags/defines -IC:\Users\meera\AppData\Local\Temp\.arduinoIDE-unsaved2026730-1088-158dj95.3ywh\sketch_aug30f -iprefix C:\Users\meera\AppData\Local\Arduino15\packages\esp32\tools\esp32-libs\3.3.10/include/ @C:\Users\meera\AppData\Local\Arduino15\packages\esp32\tools\esp32-libs\3.3.10/flags/includes -IC:\Users\meera\AppData\Local\Arduino15\packages\esp32\tools\esp32-libs\3.3.10/qio_qspi/include -IC:\Users\meera\AppData\Local\Arduino15\packages\esp32\hardware\esp32\3.3.10\cores\esp32 -IC:\Users\meera\AppData\Local\Arduino15\packages\esp32\hardware\esp32\3.3.10\variants\esp32 -IC:\Users\meera\AppData\Local\Arduino15\packages\esp32\hardware\esp32\3.3.10\libraries\WiFi\src -IC:\Users\meera\AppData\Local\Arduino15\packages\esp32\hardware\esp32\3.3.10\libraries\Network\src @C:\Users\meera\AppData\Local\arduino\sketches\A72D31F91863EBE3701D0A6938A60BFF/build_opt.h @C:\Users\meera\AppData\Local\arduino\sketches\A72D31F91863EBE3701D0A6938A60BFF/file_opts C:\Users\meera\AppData\Local\arduino\sketches\A72D31F91863EBE3701D0A6938A60BFF\sketch\sketch_aug30f.ino.cpp.merged -o nul -MMD -MF C:\Users\meera\AppData\Local\arduino\sketches\A72D31F91863EBE3701D0A6938A60BFF\sketch\sketch_aug30f.ino.cpp.merged.libsdetect.d
Using cached library dependencies for file: C:\Users\meera\AppData\Local\Arduino15\packages\esp32\hardware\esp32\3.3.10\libraries\WiFi\src\AP.cpp
Using cached library dependencies for file: C:\Users\meera\AppData\Local\Arduino15\packages\esp32\hardware\esp32\3.3.10\libraries\WiFi\src\STA.cpp
Using cached library dependencies for file: C:\Users\meera\AppData\Local\Arduino15\packages\esp32\hardware\esp32\3.3.10\libraries\WiFi\src\WiFi.cpp
Using cached library dependencies for file: C:\Users\meera\AppData\Local\Arduino15\packages\esp32\hardware\esp32\3.3.10\libraries\WiFi\src\WiFiAP.cpp
Using cached library dependencies for file: C:\Users\meera\AppData\Local\Arduino15\packages\esp32\hardware\esp32\3.3.10\libraries\WiFi\src\WiFiGeneric.cpp
Using cached library dependencies for file: C:\Users\meera\AppData\Local\Arduino15\packages\esp32\hardware\esp32\3.3.10\libraries\WiFi\src\WiFiMulti.cpp
Using cached library dependencies for file: C:\Users\meera\AppData\Local\Arduino15\packages\esp32\hardware\esp32\3.3.10\libraries\WiFi\src\WiFiSTA.cpp
Using cached library dependencies for file: C:\Users\meera\AppData\Local\Arduino15\packages\esp32\hardware\esp32\3.3.10\libraries\WiFi\src\WiFiScan.cpp
Using cached library dependencies for file: C:\Users\meera\AppData\Local\Arduino15\packages\esp32\hardware\esp32\3.3.10\libraries\Network\src\NetworkClient.cpp
Using cached library dependencies for file: C:\Users\meera\AppData\Local\Arduino15\packages\esp32\hardware\esp32\3.3.10\libraries\Network\src\NetworkEvents.cpp
Using cached library dependencies for file: C:\Users\meera\AppData\Local\Arduino15\packages\esp32\hardware\esp32\3.3.10\libraries\Network\src\NetworkInterface.cpp
Using cached library dependencies for file: C:\Users\meera\AppData\Local\Arduino15\packages\esp32\hardware\esp32\3.3.10\libraries\Network\src\NetworkManager.cpp
Using cached library dependencies for file: C:\Users\meera\AppData\Local\Arduino15\packages\esp32\hardware\esp32\3.3.10\libraries\Network\src\NetworkServer.cpp
Using cached library dependencies for file: C:\Users\meera\AppData\Local\Arduino15\packages\esp32\hardware\esp32\3.3.10\libraries\Network\src\NetworkUdp.cpp
Generating function prototypes...
C:\Users\meera\AppData\Local\Arduino15\packages\esp32\tools\esp-x32\2601/bin/xtensa-esp32-elf-g++ -c @C:\Users\meera\AppData\Local\Arduino15\packages\esp32\tools\esp32-libs\3.3.10/flags/cpp_flags -w -Os -Werror=return-type -w -x c++ -E -CC -DF_CPU=240000000L -DARDUINO=10607 -DARDUINO_ESP32_DEV -DARDUINO_ARCH_ESP32 -DARDUINO_BOARD="ESP32_DEV" -DARDUINO_VARIANT="esp32" -DARDUINO_PARTITION_default -DARDUINO_HOST_OS="windows" -DARDUINO_FQBN="esp32:esp32:esp32:UploadSpeed=921600,CPUFreq=240,FlashFreq=80,FlashMode=qio,FlashSize=4M,PartitionScheme=default,DebugLevel=none,PSRAM=disabled,LoopCore=1,EventsCore=1,EraseFlash=none,JTAGAdapter=default,ZigbeeMode=default" -DESP32=ESP32 -DCORE_DEBUG_LEVEL=0 -DARDUINO_RUNNING_CORE=1 -DARDUINO_EVENT_RUNNING_CORE=1 -DARDUINO_USB_CDC_ON_BOOT=0 @C:\Users\meera\AppData\Local\Arduino15\packages\esp32\tools\esp32-libs\3.3.10/flags/defines -IC:\Users\meera\AppData\Local\Temp\.arduinoIDE-unsaved2026730-1088-158dj95.3ywh\sketch_aug30f -iprefix C:\Users\meera\AppData\Local\Arduino15\packages\esp32\tools\esp32-libs\3.3.10/include/ @C:\Users\meera\AppData\Local\Arduino15\packages\esp32\tools\esp32-libs\3.3.10/flags/includes -IC:\Users\meera\AppData\Local\Arduino15\packages\esp32\tools\esp32-libs\3.3.10/qio_qspi/include -IC:\Users\meera\AppData\Local\Arduino15\packages\esp32\hardware\esp32\3.3.10\cores\esp32 -IC:\Users\meera\AppData\Local\Arduino15\packages\esp32\hardware\esp32\3.3.10\variants\esp32 -IC:\Users\meera\AppData\Local\Arduino15\packages\esp32\hardware\esp32\3.3.10\libraries\WiFi\src -IC:\Users\meera\AppData\Local\Arduino15\packages\esp32\hardware\esp32\3.3.10\libraries\Network\src @C:\Users\meera\AppData\Local\arduino\sketches\A72D31F91863EBE3701D0A6938A60BFF/build_opt.h @C:\Users\meera\AppData\Local\arduino\sketches\A72D31F91863EBE3701D0A6938A60BFF/file_opts C:\Users\meera\AppData\Local\arduino\sketches\A72D31F91863EBE3701D0A6938A60BFF\sketch\sketch_aug30f.ino.cpp.merged -o C:\Users\meera\AppData\Local\Temp\466671950\sketch_merged.cpp
C:\Users\meera\AppData\Local\Arduino15\packages\builtin\tools\ctags\5.8-arduino11/ctags -u --language-force=c++ -f - --c++-kinds=svpf --fields=KSTtzns --line-directives C:\Users\meera\AppData\Local\Temp\466671950\sketch_merged.cpp

Compiling sketch...
"C:\\Users\\meera\\AppData\\Local\\Arduino15\\packages\\esp32\\tools\\esp-x32\\2601/bin/xtensa-esp32-elf-g++" -MMD -c "@C:\\Users\\meera\\AppData\\Local\\Arduino15\\packages\\esp32\\tools\\esp32-libs\\3.3.10/flags/cpp_flags" -w -Os -Werror=return-type -DF_CPU=240000000L -DARDUINO=10607 -DARDUINO_ESP32_DEV -DARDUINO_ARCH_ESP32 "-DARDUINO_BOARD=\"ESP32_DEV\"" "-DARDUINO_VARIANT=\"esp32\"" -DARDUINO_PARTITION_default "-DARDUINO_HOST_OS=\"windows\"" "-DARDUINO_FQBN=\"esp32:esp32:esp32:UploadSpeed=921600,CPUFreq=240,FlashFreq=80,FlashMode=qio,FlashSize=4M,PartitionScheme=default,DebugLevel=none,PSRAM=disabled,LoopCore=1,EventsCore=1,EraseFlash=none,JTAGAdapter=default,ZigbeeMode=default\"" -DESP32=ESP32 -DCORE_DEBUG_LEVEL=0 -DARDUINO_RUNNING_CORE=1 -DARDUINO_EVENT_RUNNING_CORE=1 -DARDUINO_USB_CDC_ON_BOOT=0 "@C:\\Users\\meera\\AppData\\Local\\Arduino15\\packages\\esp32\\tools\\esp32-libs\\3.3.10/flags/defines" "-IC:\\Users\\meera\\AppData\\Local\\Temp\\.arduinoIDE-unsaved2026730-1088-158dj95.3ywh\\sketch_aug30f" -iprefix "C:\\Users\\meera\\AppData\\Local\\Arduino15\\packages\\esp32\\tools\\esp32-libs\\3.3.10/include/" "@C:\\Users\\meera\\AppData\\Local\\Arduino15\\packages\\esp32\\tools\\esp32-libs\\3.3.10/flags/includes" "-IC:\\Users\\meera\\AppData\\Local\\Arduino15\\packages\\esp32\\tools\\esp32-libs\\3.3.10/qio_qspi/include" "-IC:\\Users\\meera\\AppData\\Local\\Arduino15\\packages\\esp32\\hardware\\esp32\\3.3.10\\cores\\esp32" "-IC:\\Users\\meera\\AppData\\Local\\Arduino15\\packages\\esp32\\hardware\\esp32\\3.3.10\\variants\\esp32" "-IC:\\Users\\meera\\AppData\\Local\\Arduino15\\packages\\esp32\\hardware\\esp32\\3.3.10\\libraries\\WiFi\\src" "-IC:\\Users\\meera\\AppData\\Local\\Arduino15\\packages\\esp32\\hardware\\esp32\\3.3.10\\libraries\\Network\\src" "@C:\\Users\\meera\\AppData\\Local\\arduino\\sketches\\A72D31F91863EBE3701D0A6938A60BFF/build_opt.h" "@C:\\Users\\meera\\AppData\\Local\\arduino\\sketches\\A72D31F91863EBE3701D0A6938A60BFF/file_opts" "C:\\Users\\meera\\AppData\\Local\\arduino\\sketches\\A72D31F91863EBE3701D0A6938A60BFF\\sketch\\sketch_aug30f.ino.cpp" -o "C:\\Users\\meera\\AppData\\Local\\arduino\\sketches\\A72D31F91863EBE3701D0A6938A60BFF\\sketch\\sketch_aug30f.ino.cpp.o"
C:\Users\meera\AppData\Local\Temp\.arduinoIDE-unsaved2026730-1088-158dj95.3ywh\sketch_aug30f\sketch_aug30f.ino: In function 'void setup()':
C:\Users\meera\AppData\Local\Temp\.arduinoIDE-unsaved2026730-1088-158dj95.3ywh\sketch_aug30f\sketch_aug30f.ino:212:3: error: 'esp_wifi_set_channel' was not declared in this scope; did you mean 'esp_etm_new_channel'?
  212 |   esp_wifi_set_channel(ESPNOW_CHANNEL, WIFI_SECOND_CHAN_NONE);
      |   ^~~~~~~~~~~~~~~~~~~~
      |   esp_etm_new_channel
Using library WiFi at version 3.3.10 in folder: C:\Users\meera\AppData\Local\Arduino15\packages\esp32\hardware\esp32\3.3.10\libraries\WiFi 
Using library Networking at version 3.3.10 in folder: C:\Users\meera\AppData\Local\Arduino15\packages\esp32\hardware\esp32\3.3.10\libraries\Network 
exit status 1

Compilation error: 'esp_wifi_set_channel' was not declared in this scope; did you mean 'esp_etm_new_channel'?

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
 * SENT TO THE PI:   D|acc,gyro,angle|acc,gyro,angle
 *   acc   peak acceleration x10
 *   gyro  peak rotation, deg/s
 *   angle tilt, degrees
 *
 * RECEIVED FROM THE PI, two digits, rower then fault:
 *   00 both fine     x1 motion    x2 angle    x3 power
 */

#include <WiFi.h>
#include <esp_now.h>
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

#define ERROR_NONE         0
#define ERROR_DURATION     1
#define ERROR_ACCELERATION 2
#define ERROR_ANGLE        4

#define LED_COMMAND_GREEN  1
#define LED_COMMAND_RED    2

// ---------------------------------------------------------------- state

float    acc[3]   = {0, 0, 0};
float    gyro[3]  = {0, 0, 0};
float    angle[3] = {42, 42, 42};
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
  acc[i]   = p.peak_acc;
  gyro[i]  = p.gyro_peak;
  angle[i] = p.angle;
  seen[i]  = millis();
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
                  (int)(acc[i] * 10 + 0.5), (int)(gyro[i] + 0.5),
                  (int)(angle[i] + 0.5));

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
  Serial.println("+--------+---------+------------+");
  for (int r = 1; r <= NUM_ROWERS; r++) {
    if (r == rower && fault != 0) {
      const char *fb = (fault == 2) ? "BLUE" : "RED";
      Serial.printf("|   %d    | %-7s | %-10s |\n", r, faultName(fault), fb);
      sendDecision(r, STATUS_OUT_OF_SYNC, faultToError(fault), LED_COMMAND_RED);
    } else {
      Serial.printf("|   %d    | IN SYNC | GREEN      |\n", r);
      sendDecision(r, STATUS_CORRECT, ERROR_NONE, LED_COMMAND_GREEN);
    }
  }
  Serial.println("+--------+---------+------------+");
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
