// Resonance Solenoid Demo -- 3 V mini solenoid through the Adafruit MOSFET
// driver board on the PowerFeather, reusing the standard LED header: V+ from
// the switchable 3V3 rail (GPIO4-gated), GND, gate signal on A0/GPIO10.
//
// Design intent (2026-07 noisemaker shootout, candidate B): can the switchable
// 3V3 header rail actually fire a small solenoid for a bamboo strike? The rail
// is not stiff (~2.96-2.97 V at ~290 mA, LOG 2026-07-02) and a solenoid pull-in
// is a hundreds-of-mA-to-~1A pulse, so the open bench questions are: minimum
// pulse width for a reliable strike, rail/MCU stability during the pulse (USB
// vs battery), and whether repeated strikes upset anything. The web dashboard
// gives single strikes, fixed-width test strikes for the min-width sweep,
// bursts, and an auto-repeat mode; strike + failsafe counters track health.
//
// Coil safety: every gate pulse is ended by an esp_timer one-shot AND a loop()
// failsafe deadline; pulse width is hard-clamped (5..300 ms) and a minimum
// coil-rest gap is enforced between strikes. A solenoid left energized on this
// rail is a resistor across the pack -- nothing in here can hold the gate high.
//
// Wiring (Adafruit MOSFET driver, repurposed LED header):
//   3V3 header (GPIO4-gated) -> driver load supply / solenoid +
//   GND                      -> driver ground
//   A0 / GPIO10              -> driver signal (gate) input
//   solenoid across the driver's load output; the board's flyback diode is
//   MANDATORY for a coil load -- check it is populated.
//
// The dashboard "Coil power" button toggles the 3V3 header rail itself -- the
// same software kill-switch the production power policy would use.
//
// Build/flash (USB): ./build.sh --port /dev/ttyACM1
// Web: http://solenoiddemo.local/ or the IP in the serial banner (115200).
// OTA: curl -F "firmware=@solenoid_demo.ino.bin" http://<ip>/update

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <Update.h>
#include <Wire.h>
#include "esp_timer.h"
#include "driver/rtc_io.h" // read back the actual EN_3V3 pad level (SDK RTC-holds it)

#define FW_VERSION "solenoid-demo-2026-07-10.1"

// PowerFeather SDK: rails + telemetry + guarded charging (speaker_demo pattern --
// this unit may carry a cell; charging stays OFF until the gauge reports a
// plausible LFP voltage, and the solar guard handles USB/panel supplies).
#include <PowerFeather.h>
#include "../powerfeather_solar_guard.h"
using namespace PowerFeather;
#if !defined(POWERFEATHER_BOARD_V2) && !defined(CONFIG_ESP32S3_POWERFEATHER_V2)
#error "Build with -DPOWERFEATHER_BOARD_V2=1 (build.sh passes it) so the SDK targets the V2."
#endif
#define SOL_MAINTAIN_V 4.6f // correct for USB; re-tune toward the panel MPP for solar work
bool gPfReady = false;
bool gChargeOn = false;

#define EN_3V3_PIN 4 // switchable 3V3 header rail (solenoid supply), active HIGH

#if __has_include("wifi_secrets.h")
#include "wifi_secrets.h"
#define HAVE_SECRETS 1
#else
#define HAVE_SECRETS 0
#endif
#define AP_SSID "ResonanceSolenoid"
#define AP_PASS "resonance"

WebServer server(80);

// ---- Solenoid gate control ----------------------------------------------------
#ifndef SOLENOID_PIN
#define SOLENOID_PIN 10 // GPIO10 / A0 on the repurposed LED header
#endif
constexpr uint16_t PULSE_MIN_MS = 5;
constexpr uint16_t PULSE_MAX_MS = 300;  // hard cap -- longer is heat, not strike
constexpr uint16_t COIL_REST_MS = 80;   // minimum gap between strikes
constexpr uint16_t BURST_GAP_FLOOR_MS = 150;

uint16_t gPulseMs = 40;      // default strike width
uint16_t gIntervalMs = 600;  // auto-repeat period
bool gAuto = false;
bool gCoilOn = true;         // requested 3V3 header rail state
volatile bool gGateOn = false;
volatile uint32_t gLastEndMs = 0;   // when the gate last dropped
uint32_t gGateFailsafeMs = 0;       // loop() deadline: force-low if the timer missed
uint32_t gAutoNextMs = 0;
uint8_t gBurstLeft = 0;
uint32_t gBurstNextMs = 0;
uint32_t gStrikes = 0;
uint32_t gBlocked = 0;   // strike requests refused (rest gap / rail off / mid-pulse)
uint32_t gFailsafes = 0; // should stay 0 -- nonzero means the esp_timer path missed
char gLast[40] = "-";
esp_timer_handle_t gPulseTimer = nullptr;

void pulseEnd(void *) { // esp_timer task context
  digitalWrite(SOLENOID_PIN, LOW);
  gGateOn = false;
  gLastEndMs = millis();
}

bool strike(uint16_t ms, const char *why) {
  ms = constrain(ms, PULSE_MIN_MS, PULSE_MAX_MS);
  uint32_t now = millis();
  if (!gCoilOn || gGateOn || now - gLastEndMs < COIL_REST_MS) {
    gBlocked++;
    return false;
  }
  gGateFailsafeMs = now + ms + 50;
  gGateOn = true;
  digitalWrite(SOLENOID_PIN, HIGH);
  esp_timer_stop(gPulseTimer); // no-op if not running
  esp_timer_start_once(gPulseTimer, (uint64_t)ms * 1000ULL);
  gStrikes++;
  snprintf(gLast, sizeof(gLast), "%s %ums", why, ms);
  Serial.printf("strike #%lu: %s\n", (unsigned long)gStrikes, gLast);
  return true;
}

void stopAll() {
  gAuto = false;
  gBurstLeft = 0;
  esp_timer_stop(gPulseTimer);
  digitalWrite(SOLENOID_PIN, LOW);
  if (gGateOn) {
    gGateOn = false;
    gLastEndMs = millis();
  }
}

void burstStart(uint8_t n) {
  gBurstLeft = constrain((int)n, 1, 20);
  gBurstNextMs = millis();
}

uint32_t minIntervalMs() {
  uint32_t m = (uint32_t)gPulseMs + COIL_REST_MS;
  return m < BURST_GAP_FLOOR_MS ? BURST_GAP_FLOOR_MS : m;
}

void burstTick() {
  if (!gBurstLeft) return;
  uint32_t now = millis();
  if ((int32_t)(now - gBurstNextMs) < 0) return;
  if (strike(gPulseMs, "burst")) gBurstLeft--;
  gBurstNextMs = now + minIntervalMs();
}

void autoTick() {
  if (!gAuto) return;
  uint32_t now = millis();
  if ((int32_t)(now - gAutoNextMs) < 0) return;
  strike(gPulseMs, "auto");
  uint32_t iv = gIntervalMs;
  if (iv < minIntervalMs()) iv = minIntervalMs();
  gAutoNextMs = now + iv;
}

void failsafeTick() { // belt-and-suspenders: the coil must never stay energized
  if (gGateOn && (int32_t)(millis() - gGateFailsafeMs) >= 0) {
    digitalWrite(SOLENOID_PIN, LOW);
    gGateOn = false;
    gLastEndMs = millis();
    gFailsafes++;
    Serial.println("FAILSAFE: gate forced low (pulse timer missed)");
  }
}

// The SDK manages EN_3V3 (GPIO4) as an RTC pin with a pad HOLD re-armed after
// every write (Mainboard::_setRTCPin) -- raw pinMode/digitalWrite on GPIO4 does
// NOTHING while the hold is set. Go through Board.enable3V3() when the SDK is
// up and CHECK the result: it try-locks a mutex and can fail silently.
int en3v3Level() {
  return gPfReady ? (int)rtc_gpio_get_level(GPIO_NUM_4) : (int)digitalRead(EN_3V3_PIN);
}

void setCoilPower(bool on) {
  gCoilOn = on;
  if (!on) stopAll(); // rail off: also drop the gate and any queued strikes
  if (gPfReady) {
    Result r = Result::Failure;
    for (int i = 0; i < 5 && r != Result::Ok; i++) {
      r = Board.enable3V3(on);
      if (r != Result::Ok) delay(50); // try-lock miss: retry
    }
    Serial.printf("coil power (3V3 header rail): %s -> SDK %s, GPIO4 pad reads %d\n",
                  on ? "ON" : "OFF", r == Result::Ok ? "Ok" : "FAILED", en3v3Level());
  } else {
    pinMode(EN_3V3_PIN, OUTPUT);
    digitalWrite(EN_3V3_PIN, on ? HIGH : LOW);
    Serial.printf("coil power (3V3 header rail): %s via raw GPIO4 (SDK down)\n",
                  on ? "ON" : "OFF");
  }
}

// ---- Battery stats cache (speaker_demo/sway_demo pattern) ---------------------
float gBatV = 0, gBatMa = 0, gSupV = 0, gSupMa = 0;
uint8_t gSoc = 0;
bool gSupGood = false;

void batteryTick() {
  static uint32_t nextMs = 0;
  static uint8_t idx = 0;
  if (!gPfReady || millis() < nextMs) return;
  nextMs = millis() + 800;
  switch (idx++ % 5) {
    case 0: Board.getBatteryVoltage(gBatV); break;
    case 1: Board.getBatteryCurrent(gBatMa); break;
    case 2: Board.getBatteryCharge(gSoc); break;
    case 3: Board.getSupplyVoltage(gSupV); break;
    case 4: Board.checkSupplyGood(gSupGood); break;
  }
}

// One-shot guarded charge-enable (speaker_demo/presence_bench pattern).
void chargeTick() {
  static bool done = false;
  if (done || !gPfReady || millis() < 6000) return;
  if (gBatV < 0.1f) {
    if (millis() > 60000) {
      done = true;
      Serial.println("no battery reading after 60 s -> charging stays OFF");
    }
    return;
  }
  done = true;
  if (gBatV > 2.5f && gBatV < 4.4f) {
    Board.setBatteryChargingMaxCurrent(500);
    Board.enableBatteryCharging(true);
    gChargeOn = true;
    pfSolarGuardInit("solenoid_demo", SOL_MAINTAIN_V, true);
    Serial.printf("battery %.2fV present -> charging ON (500 mA, LFP 3.65 V ceiling)\n", gBatV);
  } else {
    Serial.printf("battery %.2fV implausible -> charging stays OFF\n", gBatV);
  }
}

void solarGuardTick() {
  if (!gPfReady || !gChargeOn) return;
  static uint32_t lastMs = 0;
  uint32_t now = millis();
  if (now - lastMs < 2000) return;
  lastMs = now;
  float sv = 0.0f, sma = 0.0f;
  bool good = false;
  if (Board.getSupplyVoltage(sv) != Result::Ok) return;
  if (Board.getSupplyCurrent(sma) != Result::Ok) return;
  if (Board.checkSupplyGood(good) != Result::Ok) return;
  gSupV = sv;
  gSupMa = sma;
  gSupGood = good;
  pfSolarGuardTick("solenoid_demo", sv, sma, good, SOL_MAINTAIN_V, true);
}

// ---- Web UI -------------------------------------------------------------------
const char PAGE[] PROGMEM = R"HTML(<!doctype html><html><head>
<meta name=viewport content="width=device-width,initial-scale=1">
<title>Solenoid Demo</title>
<style>
 body{font-family:system-ui,sans-serif;background:#111;color:#eee;margin:0;padding:14px;max-width:520px}
 h2{margin:.2em 0}
 .row{margin:10px 0}
 label{display:block;font-size:13px;color:#aaa;margin-bottom:3px}
 input[type=range]{width:100%;height:30px}
 .btns{display:flex;flex-wrap:wrap;gap:6px}
 button{flex:1 1 auto;min-width:58px;padding:14px 8px;font-size:14px;border:0;border-radius:8px;background:#333;color:#eee}
 button.on{background:#0a7;color:#fff}
 button.big{background:#264;font-weight:bold;font-size:17px;padding:18px 8px}
 #vals{font-family:monospace;font-size:13px;background:#000;padding:8px;border-radius:6px;white-space:pre;color:#6f6;overflow-x:auto}
 hr{border:0;border-top:1px solid #333;margin:14px 0}
</style></head><body>
<h2>Solenoid Demo <span style="font-size:12px;color:#888" id=fw></span></h2>
<div class=row><div class=btns>
 <button class=big onclick="fetch('/strike')">STRIKE</button>
</div></div>
<div class=row><label>Fixed-width test strike (min reliable width sweep)</label><div class=btns>
 <button onclick="sw(10)">10ms</button>
 <button onclick="sw(15)">15ms</button>
 <button onclick="sw(20)">20ms</button>
 <button onclick="sw(30)">30ms</button>
 <button onclick="sw(50)">50ms</button>
 <button onclick="sw(80)">80ms</button>
 <button onclick="sw(120)">120ms</button>
</div></div>
<div class=row><label>Patterns</label><div class=btns>
 <button onclick="fetch('/burst?n=2')">Double</button>
 <button onclick="fetch('/burst?n=5')">Burst x5</button>
 <button onclick="fetch('/burst?n=10')">Burst x10</button>
 <button id=auto onclick="toggleAuto()">Auto: off</button>
</div></div>
<div class=row><label>Pulse width <span id=pulsel></span></label>
 <input type=range id=pulse min=5 max=300 value=40 oninput="ch('pulse',this.value)"></div>
<div class=row><label>Auto interval <span id=intervall></span></label>
 <input type=range id=interval min=150 max=4000 step=50 value=600 oninput="ch('interval',this.value)"></div>
<hr>
<div class=row><div class=btns>
 <button id=coil onclick="toggleCoil()">Coil power: on</button>
 <button onclick="fetch('/stop')">Stop / all off</button>
</div></div>
<div class=row><div id=vals>...</div></div>
<div class=row><label>Battery</label><div id=bat>...</div></div>
<script>
let st={pulse:40,interval:600,auto:0,coil:1};
function send(q){fetch('/set?'+q);}
function ch(k,v){st[k]=+v;send(k+'='+v);syncLabels();}
function sw(ms){fetch('/strike?ms='+ms);}
function toggleAuto(){st.auto^=1;send('auto='+st.auto);autoBtn();}
function autoBtn(){let e=document.getElementById('auto');
 e.textContent='Auto: '+(st.auto?'on':'off');e.className=st.auto?'on':'';}
function toggleCoil(){st.coil^=1;send('coil='+st.coil);coilBtn();}
function coilBtn(){let e=document.getElementById('coil');
 e.textContent='Coil power: '+(st.coil?'on':'off');e.className=st.coil?'on':'';}
function syncLabels(){
 pulsel.textContent=st.pulse+' ms';
 intervall.textContent=st.interval+' ms';}
function tick(){fetch('/state').then(r=>r.json()).then(s=>{
 document.getElementById('fw').textContent=s.fw;
 if(document.activeElement.type!='range'){
  st.pulse=s.pulse;st.interval=s.interval;
  pulse.value=s.pulse;interval.value=s.interval;}
 st.auto=s.auto;st.coil=s.coil;autoBtn();coilBtn();syncLabels();
 vals.textContent='last    '+s.last+'\nstrikes '+s.strikes+'   blocked '+s.blocked+
  '   failsafe '+s.failsafes+'\ngate '+(s.gate?'HIGH':'low')+'   en3v3 '+s.en3v3+
  '   rssi '+s.rssi+' dBm';
 let bat=document.getElementById('bat');
 if(!s.pf){bat.textContent='no battery data (SDK init failed)';}
 else{let act=s.ma>30?('charging +'+s.ma+'mA'):(s.ma<-30?('discharging '+s.ma+'mA'):'idle ~'+s.ma+'mA');
  bat.textContent='SOC '+s.soc+'%  '+s.bv.toFixed(3)+'V  '+act+
   (s.sgood?('  |  supply '+s.sv.toFixed(2)+'V ok'):'  |  on battery')+
   (s.chg?'':'  |  charger disabled');}
 setTimeout(tick,600);}).catch(()=>setTimeout(tick,1200));}
syncLabels();tick();
</script></body></html>)HTML";

void handleStrike() {
  uint16_t ms = gPulseMs;
  const char *why = "web";
  if (server.hasArg("ms")) {
    ms = constrain(server.arg("ms").toInt(), (long)PULSE_MIN_MS, (long)PULSE_MAX_MS);
    why = "test";
  }
  const bool ok = strike(ms, why);
  server.send(ok ? 200 : 409, "text/plain", ok ? "ok" : "blocked");
}

void handleBurst() {
  burstStart(server.hasArg("n") ? server.arg("n").toInt() : 5);
  server.send(200, "text/plain", "ok");
}

void handleSet() {
  if (server.hasArg("pulse"))
    gPulseMs = constrain(server.arg("pulse").toInt(), (long)PULSE_MIN_MS, (long)PULSE_MAX_MS);
  if (server.hasArg("interval"))
    gIntervalMs = constrain(server.arg("interval").toInt(), 150L, 4000L);
  if (server.hasArg("auto")) {
    gAuto = server.arg("auto").toInt() != 0;
    gAutoNextMs = millis();
  }
  if (server.hasArg("coil")) setCoilPower(server.arg("coil").toInt() != 0);
  server.send(200, "text/plain", "ok");
}

void handleState() {
  static char buf[512];
  snprintf(buf, sizeof(buf),
           "{\"fw\":\"%s\",\"pulse\":%u,\"interval\":%u,\"auto\":%d,\"coil\":%d,"
           "\"gate\":%d,\"en3v3\":%d,\"strikes\":%lu,\"blocked\":%lu,\"failsafes\":%lu,"
           "\"last\":\"%s\",\"rssi\":%d,\"mac\":\"%s\","
           "\"pf\":%d,\"chg\":%d,\"bv\":%.3f,\"ma\":%.0f,\"soc\":%u,\"sv\":%.2f,\"sgood\":%d}",
           FW_VERSION, gPulseMs, gIntervalMs, gAuto ? 1 : 0, gCoilOn ? 1 : 0,
           gGateOn ? 1 : 0, en3v3Level(), (unsigned long)gStrikes,
           (unsigned long)gBlocked, (unsigned long)gFailsafes, gLast, (int)WiFi.RSSI(),
           WiFi.macAddress().c_str(),
           gPfReady ? 1 : 0, gChargeOn ? 1 : 0, gBatV, gBatMa, gSoc, gSupV,
           gSupGood ? 1 : 0);
  server.send(200, "application/json", buf);
}

void setupWifi() {
#if HAVE_SECRETS
  WiFi.mode(WIFI_AP_STA);
  WiFi.setHostname("solenoiddemo");
  WiFi.begin(RES_WIFI_SSID, RES_WIFI_PASSWORD);
  Serial.print("WiFi connecting");
  uint32_t t0 = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - t0 < 12000) {
    delay(300);
    Serial.print(".");
  }
  Serial.println();
  if (WiFi.status() == WL_CONNECTED) {
    bool apOk = WiFi.softAP(AP_SSID, AP_PASS, WiFi.channel());
    Serial.print("Solenoid Demo STA at http://");
    Serial.println(WiFi.localIP());
    if (apOk) {
      Serial.print("Solenoid Demo AP '" AP_SSID "' -> http://");
      Serial.println(WiFi.softAPIP());
    }
    return;
  }
  Serial.println("station failed; starting AP fallback");
#endif
  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_SSID, AP_PASS);
  Serial.print("AP '" AP_SSID "' -> http://");
  Serial.println(WiFi.softAPIP());
}

void setup() {
  // Gate low FIRST -- before the SDK powers the 3V3 rail up, so the solenoid
  // cannot fire (or latch on) during boot.
  pinMode(SOLENOID_PIN, OUTPUT);
  digitalWrite(SOLENOID_PIN, LOW);

  Serial.begin(115200);
  delay(200);
  Serial.println("Solenoid Demo " FW_VERSION);

  const esp_timer_create_args_t targs = {
      .callback = &pulseEnd, .arg = nullptr, .dispatch_method = ESP_TIMER_TASK,
      .name = "sol_pulse", .skip_unhandled_events = true};
  esp_timer_create(&targs, &gPulseTimer);

  Result pf = Result::Failure;
  for (int i = 0; i < 4 && pf != Result::Ok; i++) {
    pf = Board.init(2000, Mainboard::BatteryType::Generic_LFP);
    if (pf != Result::Ok) delay(250);
  }
  if (pf == Result::Ok) {
    gPfReady = true;
    Board.enableBatteryCharging(false); // chargeTick() enables it once the gauge warms up
    Board.setSupplyMaintainVoltage(SOL_MAINTAIN_V);
    Board.enableVSQT(false); // no STEMMA-QT devices on this bench
    Result r3 = Result::Failure;
    for (int i = 0; i < 5 && r3 != Result::Ok; i++) {
      r3 = Board.enable3V3(true); // solenoid supply rail; try-locks, so verify + retry
      if (r3 != Result::Ok) delay(50);
    }
    Serial.printf("PowerFeather SDK Ok: VSQT off, charging OFF (bench); 3V3 enable %s, "
                  "GPIO4 pad reads %d\n",
                  r3 == Result::Ok ? "Ok" : "FAILED", (int)rtc_gpio_get_level(GPIO_NUM_4));
    // Clear a latched BQ EN_HIZ (REG0x16 bit 4) left by a prior image -- otherwise
    // the board can silently drain its cell while on USB (presence_bench pattern).
    uint8_t r16 = 0;
    Wire1.beginTransmission(0x6A);
    Wire1.write(0x16);
    if (Wire1.endTransmission(false) == 0 && Wire1.requestFrom(0x6A, 1) == 1) {
      r16 = Wire1.read();
      if (r16 & (1 << 4)) {
        Wire1.beginTransmission(0x6A);
        Wire1.write(0x16);
        Wire1.write(r16 & ~(1 << 4));
        Serial.printf("BQ EN_HIZ was latched (REG0x16=0x%02X) -> cleared %s\n", r16,
                      Wire1.endTransmission() == 0 ? "ok" : "FAILED");
      }
    }
  } else {
    Serial.println("WARNING: Board.init failed -- enabling 3V3 via GPIO4 manually");
    Wire1.begin(47, 48, 100000);
    // Raw fallback ONLY when the SDK is down: with the SDK up, GPIO4 is an
    // RTC-held pad and pinMode/digitalWrite are silently ignored (and pinMode
    // would remux the pad away from the SDK's RTC config).
    pinMode(EN_3V3_PIN, OUTPUT);
    digitalWrite(EN_3V3_PIN, HIGH);
  }
  delay(100); // rail settle
  Wire1.setClock(100000); // shared charger/gauge bus: 100 kHz, NEVER faster

  strike(gPulseMs, "boot"); // boot click: proves the whole gate->coil path

  setupWifi();
  if (MDNS.begin("solenoiddemo")) { // http://solenoiddemo.local/
    MDNS.addService("http", "tcp", 80);
    Serial.println("mDNS: http://solenoiddemo.local/");
  } else {
    Serial.println("mDNS start failed (use the IP)");
  }
  server.on("/", []() { server.send_P(200, "text/html", PAGE); });
  server.on("/strike", handleStrike);
  server.on("/burst", handleBurst);
  server.on("/stop", []() { stopAll(); server.send(200, "text/plain", "ok"); });
  server.on("/set", handleSet);
  server.on("/state", handleState);
  server.on("/update", HTTP_GET, []() {
    server.send(200, "text/html",
                "<form method=POST action=/update enctype=multipart/form-data>"
                "<input type=file name=firmware><input type=submit value=Flash></form>");
  });
  server.on(
      "/update", HTTP_POST,
      []() {
        bool ok = !Update.hasError();
        server.send(ok ? 200 : 500, "text/plain",
                    ok ? "Update complete. Rebooting.\n" : "Update failed.\n");
        delay(500);
        if (ok) ESP.restart();
      },
      []() {
        HTTPUpload &upload = server.upload();
        if (upload.status == UPLOAD_FILE_START) {
          Serial.printf("OTA upload start: %s\n", upload.filename.c_str());
          stopAll(); // gate low + no queued strikes while flash writes stall loop()
          if (!Update.begin(UPDATE_SIZE_UNKNOWN)) Update.printError(Serial);
        } else if (upload.status == UPLOAD_FILE_WRITE) {
          if (Update.write(upload.buf, upload.currentSize) != upload.currentSize)
            Update.printError(Serial);
        } else if (upload.status == UPLOAD_FILE_END) {
          if (Update.end(true)) Serial.printf("OTA upload done: %u bytes\n", upload.totalSize);
          else Update.printError(Serial);
        }
      });
  server.begin();
  Serial.printf("Solenoid Demo ready: gate on GPIO%d, pulse %u ms\n", SOLENOID_PIN, gPulseMs);
}

void loop() {
  server.handleClient();
  failsafeTick();
  burstTick();
  autoTick();
  batteryTick();
  chargeTick();
  solarGuardTick();
}
