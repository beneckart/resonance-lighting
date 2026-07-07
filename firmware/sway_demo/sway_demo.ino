// Resonance Sway Demo -- MSA311 accelerometer (STEMMA-QT / Wire1) drives the
// single 4 W SK6812 RGBW point source on GPIO10. Bench tool for the
// motion-reactive lighting idea: the color follows the SWAY (high-passed accel
// delta -- the "wind" signal), the TILT direction, or both, selectable live
// from the built-in web UI, which also draws a bubble-level + sway-pulse
// graphic of the sensor state for verification.
//
// Signal path (50 Hz):
//   accel -> low-pass (~0.4 s)   = gravity vector -> pitch/roll/tilt vs a
//                                  calibrated rest pose (Re-zero on the web UI)
//   accel - gravity (high-pass)  = sway magnitude -> fast-attack / slow-decay
//                                  envelope -> hue + brightness (+W flash)
//
// Wiring: MSA311 on the STEMMA-QT connector = Wire1 (GPIO47/48), which is the
// SHARED charger/gauge bus -- it stays at 100 kHz, never faster
// (POWERFEATHER_NOTES "Wire1 at >100 kHz can OPEN YOUR BATTERY SWITCH").
// RGBW data on GPIO10 / A0, V+ from the switchable 3V3 header rail (GPIO4).
//
// Build/flash (USB): ./build.sh --port /dev/ttyACM1
// Web: http://swaydemo.local/ (mDNS) or the IP in the serial banner (115200).
// OTA: curl -F "firmware=@sway_demo.ino.bin" http://<ip>/update

#include <Arduino.h>
#include <math.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <Update.h>
#include <Wire.h>
#include <Adafruit_NeoPixel.h>
#include <Adafruit_MSA301.h> // library also provides Adafruit_MSA311 (part id 0x13)

#define FW_VERSION "sway-demo-2026-07-06.2"

#ifndef DATA_PIN
#define DATA_PIN 10 // GPIO10 / A0, direct-GPIO LED data (ADR 0018/0022)
#endif
Adafruit_NeoPixel strip(1, DATA_PIN, NEO_GRBW + NEO_KHZ800);
Adafruit_MSA311 msa;

// PowerFeather SDK: rails + telemetry ONLY. Charging stays OFF -- this is a
// bench demo, often cell-less on USB, and enabling charge into a missing
// battery brownout-loops (POWERFEATHER_NOTES). Attach a cell + port the solar
// guard (see led_studio) before enabling charging here.
#include <PowerFeather.h>
using namespace PowerFeather;
#if !defined(POWERFEATHER_BOARD_V2) && !defined(CONFIG_ESP32S3_POWERFEATHER_V2)
#error "Build with -DPOWERFEATHER_BOARD_V2=1 (build.sh passes it) so the SDK targets the V2."
#endif
bool gPfReady = false;

#define EN_3V3_PIN 4 // switchable 3V3 header rail (LED V+), active HIGH; fallback if SDK init fails

#if __has_include("wifi_secrets.h")
#include "wifi_secrets.h"
#define HAVE_SECRETS 1
#else
#define HAVE_SECRETS 0
#endif
#define AP_SSID "ResonanceSway"
#define AP_PASS "resonance"

WebServer server(80);

// ---- Motion state -----------------------------------------------------------
#define SAMPLE_MS 20        // 50 Hz processing (sensor ODR 125 Hz)
#define ALPHA_G 0.06f       // gravity low-pass per sample (~0.4 s time constant)
#define ENV_ATTACK 0.5f     // envelope rise fraction per sample
#define ENV_DECAY 0.96f     // envelope decay per sample (~0.5 s tail)
#define TILT_FULL_DEG 45.0f // tilt that maps to full brightness in tilt mode
#define AZ_DEADBAND_DEG 3.0f // below this tilt the azimuth is noise -> hold hue

bool gMsaOk = false;
float gAx = 0, gAy = 0, gAz = 0;    // last raw sample (g)
float gGx = 0, gGy = 0, gGz = 1;    // gravity low-pass (g)
bool gGravSeeded = false;
float gSwayInst = 0, gEnv = 0;      // high-pass magnitude + envelope (g)
float gPitchRaw = 0, gRollRaw = 0;  // from gravity, absolute (deg)
float gPitch0 = 0, gRoll0 = 0;      // calibrated rest pose (deg)
float gRestX = 0, gRestY = 0, gRestZ = 1; // calibrated rest gravity unit vector
float gPitch = 0, gRoll = 0;        // deltas vs rest (deg) -- the bubble position
float gTiltDeg = 0, gAzDeg = 0;     // tilt magnitude + direction vs rest
bool gCalPending = true;            // auto re-zero shortly after boot
uint32_t gCalDueMs = 0;

// ---- Color mapping ----------------------------------------------------------
enum Mode { MODE_SWAY = 0, MODE_TILT = 1, MODE_BOTH = 2 };
uint8_t gMode = MODE_SWAY;
uint8_t gSens = 50;       // 1..100 -> full-scale sway 1.5 g .. 0.03 g (exponential)
float gFullScaleG = 0.22f;
uint8_t gBase = 60;       // brightness at rest (30 fell into the gamma dead-zone: rgbw=1,0,0)
bool gGamma = true;
bool gLedOn = true;
// Calm amber (hue16 ~4000) sweeping to violet (~48000) as sway rises.
#define HUE_CALM 4000
#define HUE_SPAN 44000
uint16_t gHue = HUE_CALM; // last rendered hue (held through the azimuth deadband)
uint8_t gOutR = 0, gOutG = 0, gOutB = 0, gOutW = 0; // what the LED is showing

void applySens() { gFullScaleG = 1.5f * powf(0.02f, (gSens - 1) / 99.0f); }

// ---- Sensor -----------------------------------------------------------------
bool msaInit() {
  if (!msa.begin(MSA311_I2CADDR_DEFAULT, &Wire1)) return false;
  // begin() defaults to 500 Hz ODR / 250 Hz BW; calmer settings for a 50 Hz loop.
  msa.setRange(MSA301_RANGE_4_G); // sway spikes clear 1 g easily
  msa.setDataRate(MSA301_DATARATE_125_HZ);
  msa.setBandwidth(MSA301_BANDWIDTH_62_5_HZ);
  msa.setPowerMode(MSA301_NORMALMODE);
  return true;
}

void msaRetryTick() { // hot-plug friendly: keep probing if the sensor is missing
  static uint32_t nextMs = 0;
  if (gMsaOk || millis() < nextMs) return;
  nextMs = millis() + 5000;
  gMsaOk = msaInit();
  if (gMsaOk) {
    Serial.println("MSA311 found (late)");
    gGravSeeded = false;
    gCalPending = true;
    gCalDueMs = millis() + 1500;
  }
}

void captureRest() {
  float gm = sqrtf(gGx * gGx + gGy * gGy + gGz * gGz);
  if (gm < 0.05f) return;
  gRestX = gGx / gm;
  gRestY = gGy / gm;
  gRestZ = gGz / gm;
  gPitch0 = gPitchRaw;
  gRoll0 = gRollRaw;
  gCalPending = false;
  Serial.printf("rest pose captured: pitch0=%.1f roll0=%.1f g=[%.2f %.2f %.2f]\n",
                gPitch0, gRoll0, gRestX, gRestY, gRestZ);
}

// ---- LED rendering ----------------------------------------------------------
void showRGBW(uint32_t rgb, uint8_t w) {
  if (gGamma) {
    rgb = Adafruit_NeoPixel::gamma32(rgb);
    w = Adafruit_NeoPixel::gamma8(w);
  }
  gOutR = (rgb >> 16) & 0xFF;
  gOutG = (rgb >> 8) & 0xFF;
  gOutB = rgb & 0xFF;
  gOutW = w;
  strip.setPixelColor(0, gOutR, gOutG, gOutB, gOutW);
  strip.show();
}

void renderMissing() { // sensor absent: slow red breathe so the bench state is obvious
  float f = 0.5f + 0.5f * sinf(millis() * 0.003f);
  showRGBW(Adafruit_NeoPixel::Color((uint8_t)(30 + 60 * f), 0, 0), 0);
}

void renderLed() {
  if (!gLedOn) {
    if (gOutR | gOutG | gOutB | gOutW) showRGBW(0, 0);
    return;
  }
  float envN = gEnv / gFullScaleG;
  if (envN > 1) envN = 1;
  float tiltN = gTiltDeg / TILT_FULL_DEG;
  if (tiltN > 1) tiltN = 1;
  uint16_t azHue = (uint16_t)((gAzDeg + 180.0f) / 360.0f * 65535.0f);

  float lift = 0; // 0..1 brightness above the resting base
  switch (gMode) {
    case MODE_SWAY: // hue sweeps amber -> violet with sway energy
      gHue = HUE_CALM + (uint16_t)(envN * (float)HUE_SPAN);
      lift = envN;
      break;
    case MODE_TILT: // hue = which way it leans, brightness = how far
      if (gTiltDeg >= AZ_DEADBAND_DEG) gHue = azHue;
      lift = tiltN;
      break;
    default: // BOTH: tilt steers the hue, sway pumps the brightness
      if (gTiltDeg >= AZ_DEADBAND_DEG) gHue = azHue;
      lift = envN > tiltN * 0.5f ? envN : tiltN * 0.5f;
      break;
  }
  uint8_t val = gBase + (uint8_t)(lift * (float)(255 - gBase));
  // White die: a strong sway spike (top 20% of scale) flashes the W channel.
  uint8_t w = envN > 0.8f ? (uint8_t)((envN - 0.8f) * 5.0f * 255.0f) : 0;
  showRGBW(Adafruit_NeoPixel::ColorHSV(gHue, 255, val), w);
}

void sensorTick() {
  static uint32_t lastMs = 0;
  uint32_t now = millis();
  if (now - lastMs < SAMPLE_MS) return;
  lastMs = now;
  if (!gMsaOk) {
    renderMissing();
    return;
  }
  msa.read();
  gAx = msa.x_g;
  gAy = msa.y_g;
  gAz = msa.z_g;
  if (!gGravSeeded) {
    gGx = gAx;
    gGy = gAy;
    gGz = gAz;
    gGravSeeded = true;
  }
  gGx += ALPHA_G * (gAx - gGx);
  gGy += ALPHA_G * (gAy - gGy);
  gGz += ALPHA_G * (gAz - gGz);

  // Sway = what's left after gravity is removed (the delta signal).
  float hx = gAx - gGx, hy = gAy - gGy, hz = gAz - gGz;
  gSwayInst = sqrtf(hx * hx + hy * hy + hz * hz);
  if (gSwayInst > gEnv) gEnv += ENV_ATTACK * (gSwayInst - gEnv);
  else gEnv *= ENV_DECAY;

  // Tilt from the smoothed gravity vector.
  float gm = sqrtf(gGx * gGx + gGy * gGy + gGz * gGz);
  if (gm > 0.05f) {
    gPitchRaw = atan2f(-gGx, sqrtf(gGy * gGy + gGz * gGz)) * 57.29578f;
    gRollRaw = atan2f(gGy, gGz) * 57.29578f;
    gPitch = gPitchRaw - gPitch0;
    gRoll = gRollRaw - gRoll0;
    float dot = (gGx * gRestX + gGy * gRestY + gGz * gRestZ) / gm;
    if (dot > 1) dot = 1;
    if (dot < -1) dot = -1;
    gTiltDeg = acosf(dot) * 57.29578f;
    gAzDeg = atan2f(gRoll, gPitch) * 57.29578f;
  }
  if (gCalPending && now >= gCalDueMs && gGravSeeded) captureRest();
  renderLed();
}

// ---- Battery stats cache (led_studio pattern: one short SDK read per 800 ms) --
float gBatV = 0, gBatMa = 0, gSupV = 0;
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

// ---- Web UI -----------------------------------------------------------------
const char PAGE[] PROGMEM = R"HTML(<!doctype html><html><head>
<meta name=viewport content="width=device-width,initial-scale=1">
<title>Sway Demo</title>
<style>
 body{font-family:system-ui,sans-serif;background:#111;color:#eee;margin:0;padding:14px;max-width:520px}
 h2{margin:.2em 0}
 .row{margin:10px 0}
 label{display:block;font-size:13px;color:#aaa;margin-bottom:3px}
 input[type=range]{width:100%;height:30px}
 .btns{display:flex;flex-wrap:wrap;gap:6px}
 button{flex:1 1 auto;min-width:64px;padding:11px 8px;font-size:14px;border:0;border-radius:8px;background:#333;color:#eee}
 button.on{background:#0a7;color:#fff}
 canvas{background:#000;border-radius:10px;display:block;margin:0 auto}
 #vals{font-family:monospace;font-size:13px;background:#000;padding:8px;border-radius:6px;white-space:pre;color:#6f6;overflow-x:auto}
 #warn{color:#f66;font-weight:bold}
 hr{border:0;border-top:1px solid #333;margin:14px 0}
</style></head><body>
<h2>Sway Demo <span style="font-size:12px;color:#888" id=fw></span></h2>
<div id=warn></div>

<div class=row><canvas id=lvl width=300 height=300></canvas></div>
<div class=row><label>Sway envelope (last 30 s, line = full scale)</label>
 <canvas id=spark width=300 height=60></canvas></div>
<div class=row><div id=vals>...</div></div>

<hr>
<div class=row><label>Color source</label><div class=btns>
 <button id=m0 onclick="mode(0)">Sway</button>
 <button id=m1 onclick="mode(1)">Tilt</button>
 <button id=m2 onclick="mode(2)">Both</button>
</div></div>
<div class=row><label>Sway sensitivity <span id=sensl></span></label>
 <input type=range id=sens min=1 max=100 value=50 oninput="ch('sens',this.value)"></div>
<div class=row><label>Base brightness <span id=basel></span></label>
 <input type=range id=base min=0 max=255 value=60 oninput="ch('base',this.value)"></div>
<div class=row><div class=btns>
 <button onclick="send('cal=1')">Re-zero tilt</button>
 <button id=led onclick="toggleLed()">LED: on</button>
 <button id=gam onclick="toggleGamma()">Gamma: on</button>
</div></div>
<div class=row><label>Battery</label><div id=bat>...</div></div>

<script>
let st={mode:0,sens:50,base:60,led:1,gamma:1};
let hist=[],trail=[];
function send(q){fetch('/set?'+q);}
function ch(k,v){st[k]=+v;send(k+'='+v);syncLabels();}
function mode(m){st.mode=m;send('mode='+m);hl(m);}
function hl(m){for(let i=0;i<3;i++)document.getElementById('m'+i).className=(i==m?'on':'');}
function toggleLed(){st.led^=1;send('led='+st.led);let e=document.getElementById('led');
 e.textContent='LED: '+(st.led?'on':'off');e.className=st.led?'on':'';}
function toggleGamma(){st.gamma^=1;send('gamma='+st.gamma);let e=document.getElementById('gam');
 e.textContent='Gamma: '+(st.gamma?'on':'off');e.className=st.gamma?'on':'';}
function syncLabels(){
 sensl.textContent=st.sens+' (full scale '+(1.5*Math.pow(0.02,(st.sens-1)/99)).toFixed(2)+' g)';
 basel.textContent=st.base;}
function drawLevel(s){
 const c=document.getElementById('lvl'),x=c.getContext('2d'),R=130,cx=150,cy=150;
 const col='rgb('+s.r+','+s.g+','+s.b+')';
 x.clearRect(0,0,300,300);
 x.strokeStyle='#333';x.lineWidth=1;
 for(const d of [10,20,30,45]){x.beginPath();x.arc(cx,cy,d/45*R,0,7);x.stroke();}
 x.beginPath();x.moveTo(cx-R,cy);x.lineTo(cx+R,cy);x.moveTo(cx,cy-R);x.lineTo(cx,cy+R);x.stroke();
 x.fillStyle='#666';x.font='11px monospace';
 x.fillText('+pitch',cx+4,cy-R+10);x.fillText('+roll',cx+R-34,cy-4);
 if(s.envn>0.01){ // sway pulse ring
  x.strokeStyle=col;x.globalAlpha=0.9;x.lineWidth=4;
  x.beginPath();x.arc(cx,cy,Math.max(6,s.envn*R),0,7);x.stroke();x.globalAlpha=1;}
 const bx=cx+Math.max(-1,Math.min(1,s.roll/45))*R,
       by=cy-Math.max(-1,Math.min(1,s.pitch/45))*R;
 trail.push([bx,by]);if(trail.length>40)trail.shift();
 for(let i=0;i<trail.length;i++){x.globalAlpha=i/trail.length*0.5;
  x.fillStyle=col;x.beginPath();x.arc(trail[i][0],trail[i][1],3,0,7);x.fill();}
 x.globalAlpha=1;x.fillStyle=col;x.strokeStyle='#fff';x.lineWidth=2;
 x.beginPath();x.arc(bx,by,12,0,7);x.fill();x.stroke();
 if(s.w>0){x.fillStyle='rgba(255,255,255,'+(s.w/255)+')';
  x.beginPath();x.arc(bx,by,6,0,7);x.fill();}
}
function drawSpark(s){
 hist.push(s.env/s.fs);if(hist.length>300)hist.shift();
 const c=document.getElementById('spark'),x=c.getContext('2d'),h=60;
 x.clearRect(0,0,300,60);
 x.strokeStyle='#555';x.beginPath();x.moveTo(0,2);x.lineTo(300,2);x.stroke(); // full scale
 x.strokeStyle='#0a7';x.lineWidth=1.5;x.beginPath();
 for(let i=0;i<hist.length;i++){const y=h-Math.min(1,hist[i])*(h-4);
  i?x.lineTo(i,y):x.moveTo(i,y);}
 x.stroke();
}
function tick(){fetch('/state').then(r=>r.json()).then(s=>{
 document.getElementById('fw').textContent=s.fw;
 document.getElementById('warn').textContent=s.msa?'':'MSA311 NOT FOUND -- check the STEMMA cable (retrying every 5 s)';
 if(s.msa){drawLevel(s);drawSpark(s);}
 vals.textContent=
  'accel  ['+s.ax.toFixed(3)+' '+s.ay.toFixed(3)+' '+s.az.toFixed(3)+'] g\n'+
  'tilt   '+s.tilt.toFixed(1)+' deg   az '+s.azdeg.toFixed(0)+' deg   pitch '+s.pitch.toFixed(1)+'  roll '+s.roll.toFixed(1)+'\n'+
  'sway   '+s.sway.toFixed(3)+' g   env '+s.env.toFixed(3)+' g  ('+(100*s.envn).toFixed(0)+'% of '+s.fs.toFixed(2)+' g)\n'+
  'LED    rgbw='+s.r+','+s.g+','+s.b+','+s.w;
 let bat=document.getElementById('bat');
 if(!s.pf){bat.textContent='no battery data (SDK init failed)';}
 else{let act=s.ma>30?('charging +'+s.ma+'mA'):(s.ma<-30?('discharging '+s.ma+'mA'):'idle ~'+s.ma+'mA');
  bat.textContent='SOC '+s.soc+'%  '+s.bv.toFixed(3)+'V  '+act+
   (s.sgood?('  |  supply '+s.sv.toFixed(2)+'V ok'):'  |  on battery');}
 setTimeout(tick,100);}).catch(()=>setTimeout(tick,600));}
hl(0);syncLabels();tick();
</script></body></html>)HTML";

void handleSet() {
  if (server.hasArg("mode")) gMode = constrain(server.arg("mode").toInt(), 0, 2);
  if (server.hasArg("sens")) {
    gSens = constrain(server.arg("sens").toInt(), 1, 100);
    applySens();
  }
  if (server.hasArg("base")) gBase = constrain(server.arg("base").toInt(), 0, 255);
  if (server.hasArg("led")) gLedOn = server.arg("led").toInt() != 0;
  if (server.hasArg("gamma")) gGamma = server.arg("gamma").toInt() != 0;
  if (server.hasArg("cal")) {
    gCalPending = true;
    gCalDueMs = 0; // capture on the next sample
  }
  server.send(200, "text/plain", "ok");
}

void handleState() {
  float envN = gEnv / gFullScaleG;
  if (envN > 1) envN = 1;
  char buf[640];
  snprintf(buf, sizeof(buf),
           "{\"fw\":\"%s\",\"msa\":%d,"
           "\"ax\":%.3f,\"ay\":%.3f,\"az\":%.3f,"
           "\"pitch\":%.1f,\"roll\":%.1f,\"tilt\":%.1f,\"azdeg\":%.0f,"
           "\"sway\":%.3f,\"env\":%.3f,\"envn\":%.3f,\"fs\":%.3f,"
           "\"r\":%u,\"g\":%u,\"b\":%u,\"w\":%u,"
           "\"mode\":%u,\"sens\":%u,\"base\":%u,\"led\":%d,\"gamma\":%d,"
           "\"pf\":%d,\"bv\":%.3f,\"ma\":%.0f,\"soc\":%u,\"sv\":%.2f,\"sgood\":%d}",
           FW_VERSION, gMsaOk ? 1 : 0, gAx, gAy, gAz, gPitch, gRoll, gTiltDeg,
           gAzDeg, gSwayInst, gEnv, envN, gFullScaleG, gOutR, gOutG, gOutB,
           gOutW, gMode, gSens, gBase, gLedOn ? 1 : 0, gGamma ? 1 : 0,
           gPfReady ? 1 : 0, gBatV, gBatMa, gSoc, gSupV, gSupGood ? 1 : 0);
  server.send(200, "application/json", buf);
}

void setupWifi() {
#if HAVE_SECRETS
  WiFi.mode(WIFI_AP_STA);
  WiFi.setHostname("swaydemo");
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
    Serial.print("Sway Demo STA at http://");
    Serial.println(WiFi.localIP());
    if (apOk) {
      Serial.print("Sway Demo AP '" AP_SSID "' -> http://");
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
  Serial.begin(115200);
  delay(200);
  Serial.println("Sway Demo " FW_VERSION);

  // SDK init for rails + telemetry only (charging OFF -- see note at the include).
  Result pf = Result::Failure;
  for (int i = 0; i < 4 && pf != Result::Ok; i++) {
    pf = Board.init(2000, Mainboard::BatteryType::Generic_LFP);
    if (pf != Result::Ok) delay(250);
  }
  if (pf == Result::Ok) {
    gPfReady = true;
    Board.enableBatteryCharging(false);
    // Power-CYCLE the sensor rail so the MSA311 gets a fresh POR (VSQT persists
    // across ESP reboots; presence_bench learned this the hard way).
    Board.enableVSQT(false);
    Board.enable3V3(true); // LED rail
    delay(600);
    Board.enableVSQT(true);
    Serial.println("PowerFeather SDK Ok: VSQT power-cycled, 3V3 on, charging OFF (bench)");
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
    Serial.println("WARNING: Board.init failed -- VSQT may be unpowered (MSA311 will "
                   "read missing); enabling 3V3 + Wire1 manually");
    Wire1.begin(47, 48, 100000); // STEMMA bus pins, manual bring-up
  }
  pinMode(EN_3V3_PIN, OUTPUT);
  digitalWrite(EN_3V3_PIN, HIGH); // LED rail fallback/no-op
  delay(100);                     // rail settle before first probe
  Wire1.setClock(100000); // shared charger/gauge bus: 100 kHz, NEVER faster

  strip.begin();
  strip.setBrightness(255);
  strip.clear();
  strip.show();

  applySens();
  gMsaOk = msaInit();
  Serial.printf("MSA311 on Wire1 (GPIO47/48 @ 100 kHz): %s\n",
                gMsaOk ? "OK" : "NOT FOUND (will retry every 5 s)");
  gCalDueMs = millis() + 1500; // auto re-zero once the gravity filter settles

  setupWifi();
  if (MDNS.begin("swaydemo")) { // http://swaydemo.local/
    MDNS.addService("http", "tcp", 80);
    Serial.println("mDNS: http://swaydemo.local/");
  } else {
    Serial.println("mDNS start failed (use the IP)");
  }
  server.on("/", []() { server.send_P(200, "text/html", PAGE); });
  server.on("/set", handleSet);
  server.on("/state", handleState);
  // Standard OTA (led_studio/net_bench pattern) so tweaks never need the tether:
  //   curl -F "firmware=@sway_demo.ino.bin" http://<ip>/update
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
  Serial.printf("Sway Demo ready: LED on GPIO%d, mode=sway, full scale %.2f g\n",
                DATA_PIN, gFullScaleG);
}

void loop() {
  server.handleClient();
  msaRetryTick();
  sensorTick();
  batteryTick();
}
