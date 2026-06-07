// Resonance HEX Studio -- interactive aesthetic bench tool for the SK6812 "HEX"
// 37-pixel board, driven direct-GPIO (off the I2C bus). Serves a phone-friendly
// web UI over WiFi: brightness + R/G/B sliders, a shape selector (center / inner
// ring / two rings / all), and animations (spiral, single-pixel orbit, breathe,
// twinkle). Purpose-built for dialing in the look through the gobo/filter -- the
// page reads back the exact current settings so a good-looking combo can be
// recorded precisely.
//
// This is a STANDALONE sketch, intentionally separate from power_bench (which is
// brownout/telemetry scaffolding). It only drives LEDs + serves the UI.
//
// Wiring: HEX data on DATA_PIN (default GPIO10 / A0 -- the validated direct-GPIO
// header on board 2). Power the HEX from 3V3 (dim, safe) or 5V (bright) per the
// LED-power discussion; GND common. Change DATA_PIN below if you wired elsewhere.
//
// Build/flash (USB): ./build.sh --port /dev/ttyACM0
// Then open the IP it prints over serial (or http://192.168.4.1 in AP fallback).

#include <Arduino.h>
#include <math.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Adafruit_NeoPixel.h>

// ---- Hardware config -------------------------------------------------------
#ifndef DATA_PIN
#define DATA_PIN 10 // GPIO10 / A0. Override -DDATA_PIN=16 to use D6, etc.
#endif
#define NUMPIXELS 37
#define CENTER 18
// SK6812/WS2812 HEX is RGB (GRB order, 800kHz). Not RGBW.
Adafruit_NeoPixel strip(NUMPIXELS, DATA_PIN, NEO_GRB + NEO_KHZ800);

// ---- WiFi ------------------------------------------------------------------
#if __has_include("wifi_secrets.h")
#include "wifi_secrets.h"
#define HAVE_SECRETS 1
#else
#define HAVE_SECRETS 0
#endif
#define AP_SSID "ResonanceHEX"
#define AP_PASS "resonance"

WebServer server(80);

// ---- HEX37 geometry --------------------------------------------------------
// Hexagon stored as 7 rows of 4,5,6,7,6,5,4 pixels (center row index 3, center
// pixel 18). We compute each pixel's (x,y), its ring (0..3 = hex distance from
// center, via rounded Euclidean radius), and its angle -- then derive a spiral
// order (ring-by-ring, by angle = an outward spiral) and per-ring angle-sorted
// member lists (for the orbit animation).
const uint8_t ROW_COUNT[7] = {4, 5, 6, 7, 6, 5, 4};
uint8_t ringOf[NUMPIXELS];
float pxAngle[NUMPIXELS];
uint8_t spiralOrder[NUMPIXELS];
uint8_t ringMembers[4][18]; // angle-sorted; ring k has ringSize[k] entries
uint8_t ringSize[4] = {0, 0, 0, 0};
float gX[NUMPIXELS], gY[NUMPIXELS]; // pixel coords (used by Split mode)

uint8_t nearestPixel(float x, float y) {
  uint8_t best = 0;
  float bd = 1e9f;
  for (uint8_t i = 0; i < NUMPIXELS; i++) {
    float dx = gX[i] - x, dy = gY[i] - y, d = dx * dx + dy * dy;
    if (d < bd) { bd = d; best = i; }
  }
  return best;
}

void buildGeometry() {
  uint8_t idx = 0;
  for (uint8_t r = 0; r < 7; r++) {
    uint8_t n = ROW_COUNT[r];
    float y = (3.0f - (float)r) * 0.8660254f; // row spacing = sqrt(3)/2
    for (uint8_t j = 0; j < n; j++) {
      float x = (float)j - (float)(n - 1) / 2.0f;
      gX[idx] = x;
      gY[idx] = y;
      float d = sqrtf(x * x + y * y);
      uint8_t ring = (uint8_t)lroundf(d);
      if (ring > 3) ring = 3;
      ringOf[idx] = ring;
      pxAngle[idx] = atan2f(y, x);
      idx++;
    }
  }
  // Spiral order: stable-sort all pixels by (ring, angle) via insertion sort.
  for (uint8_t i = 0; i < NUMPIXELS; i++) spiralOrder[i] = i;
  for (uint8_t i = 1; i < NUMPIXELS; i++) {
    uint8_t key = spiralOrder[i];
    int8_t k = i - 1;
    while (k >= 0) {
      uint8_t a = spiralOrder[k];
      bool greater = (ringOf[a] > ringOf[key]) ||
                     (ringOf[a] == ringOf[key] && pxAngle[a] > pxAngle[key]);
      if (!greater) break;
      spiralOrder[k + 1] = spiralOrder[k];
      k--;
    }
    spiralOrder[k + 1] = key;
  }
  // Per-ring angle-sorted members (for orbit).
  for (uint8_t i = 0; i < NUMPIXELS; i++) {
    uint8_t r = ringOf[i];
    ringMembers[r][ringSize[r]++] = i;
  }
  for (uint8_t r = 0; r < 4; r++) {
    for (uint8_t i = 1; i < ringSize[r]; i++) {
      uint8_t key = ringMembers[r][i];
      int8_t k = i - 1;
      while (k >= 0 && pxAngle[ringMembers[r][k]] > pxAngle[key]) {
        ringMembers[r][k + 1] = ringMembers[r][k];
        k--;
      }
      ringMembers[r][k + 1] = key;
    }
  }
}

// ---- State -----------------------------------------------------------------
enum Anim {
  ANIM_NONE = 0,
  ANIM_SPIRAL,
  ANIM_ORBIT,
  ANIM_BREATHE,
  ANIM_TWINKLE,
  ANIM_SPLIT // static: pure R/G/B on separate pixels for wide color fringing
};
uint8_t gR = 255, gG = 140, gB = 40; // default warm amber
uint8_t gBri = 40;                   // 0..255, default low (ambient)
uint8_t gShape = 1;                  // 0 center, 1 +ring1, 2 +ring1+2, 3 all
uint8_t gAnim = ANIM_NONE;
uint8_t gSpeed = 30;   // 1..100 (higher = faster)
uint8_t gTrail = 3;    // trailing pixels for spiral/orbit
uint8_t gOrbitRing = 1; // 1..3
bool gGamma = true;
bool gFrozen = false;   // freeze animation on current frame (for parking shadow)
// Split mode (color-channel fringing): pure R/G/B on three pixels arranged as a
// triad around an anchor; spread = separation (fringe width), angle = orientation.
float gSpread = 1.2f;     // hex-units between the R/G/B channel pixels
float gFringeAngle = 0;   // radians, triad orientation
uint8_t gAnchor = CENTER; // triad center pixel (moved by Step+)
uint32_t anchorStep = 0;

uint32_t animPos = 0;     // step index into spiralOrder / ring members
float breathePhase = 0;   // radians
uint32_t lastFrame = 0;
uint8_t lastLit = CENTER; // last single lit pixel (for readback)

// gamma table (Adafruit's standard 2.8 curve via NeoPixel::gamma8)
inline uint8_t gam(uint8_t v) { return gGamma ? Adafruit_NeoPixel::gamma8(v) : v; }

// Set pixel i to the global color scaled by overall brightness * factor (0..1).
void setPx(uint16_t i, float factor) {
  if (factor < 0) factor = 0;
  if (factor > 1) factor = 1;
  float s = (float)gBri / 255.0f * factor;
  strip.setPixelColor(i, gam((uint8_t)(gR * s)), gam((uint8_t)(gG * s)),
                       gam((uint8_t)(gB * s)));
}

bool inShape(uint8_t i) {
  uint8_t r = ringOf[i];
  switch (gShape) {
    case 0: return r == 0;
    case 1: return r <= 1;
    case 2: return r <= 2;
    default: return true;
  }
}

void renderStatic() {
  for (uint16_t i = 0; i < NUMPIXELS; i++) setPx(i, inShape(i) ? 1.0f : 0.0f);
  strip.show();
}

// Split mode: pure R, G, B on three pixels at 120 deg around the anchor, radius =
// spread. Forces pure channels (ignores the RGB sliders) -- the point is to throw
// separated color fringes through the gobo. If two channels snap to the same
// pixel they add (max per channel). Static: rendered on change, not time-stepped.
void renderSplit() {
  uint8_t R[NUMPIXELS] = {0}, G[NUMPIXELS] = {0}, B[NUMPIXELS] = {0};
  const float T = 2.0943951f; // 120 deg
  float ax = gX[gAnchor], ay = gY[gAnchor];
  uint8_t pr = nearestPixel(ax + gSpread * cosf(gFringeAngle),
                            ay + gSpread * sinf(gFringeAngle));
  uint8_t pg = nearestPixel(ax + gSpread * cosf(gFringeAngle + T),
                            ay + gSpread * sinf(gFringeAngle + T));
  uint8_t pb = nearestPixel(ax + gSpread * cosf(gFringeAngle + 2 * T),
                            ay + gSpread * sinf(gFringeAngle + 2 * T));
  uint8_t v = (uint8_t)(255.0f * (float)gBri / 255.0f);
  R[pr] = max(R[pr], v);
  G[pg] = max(G[pg], v);
  B[pb] = max(B[pb], v);
  for (uint16_t i = 0; i < NUMPIXELS; i++)
    strip.setPixelColor(i, gam(R[i]), gam(G[i]), gam(B[i]));
  strip.show();
  lastLit = pr;
}

// frame interval in ms from speed (1..100) -> ~400ms (slow) .. ~25ms (fast)
uint32_t frameMs() {
  uint8_t s = gSpeed < 1 ? 1 : gSpeed;
  return (uint32_t)(400 - (s - 1) * (375.0f / 99.0f));
}

void renderFrame() {
  switch (gAnim) {
    case ANIM_SPIRAL:
    case ANIM_ORBIT: {
      const uint8_t *order;
      uint16_t n;
      if (gAnim == ANIM_SPIRAL) {
        order = spiralOrder;
        n = NUMPIXELS;
      } else {
        uint8_t r = gOrbitRing < 1 ? 1 : (gOrbitRing > 3 ? 3 : gOrbitRing);
        order = ringMembers[r];
        n = ringSize[r];
      }
      for (uint16_t i = 0; i < NUMPIXELS; i++) strip.setPixelColor(i, 0);
      uint8_t trail = gTrail;
      for (int t = 0; t <= trail; t++) {
        int p = ((int)(animPos % n) - t);
        p = ((p % (int)n) + (int)n) % (int)n;
        float f = 1.0f - (float)t / (float)(trail + 1);
        setPx(order[p], f);
        if (t == 0) lastLit = order[p];
      }
      strip.show();
      if (!gFrozen) animPos++;
      break;
    }
    case ANIM_BREATHE: {
      float f = 0.5f + 0.5f * sinf(breathePhase);
      for (uint16_t i = 0; i < NUMPIXELS; i++) setPx(i, inShape(i) ? f : 0.0f);
      strip.show();
      if (!gFrozen) breathePhase += 0.15f;
      break;
    }
    case ANIM_TWINKLE: {
      // gentle decay + occasional new spark within the active shape
      for (uint16_t i = 0; i < NUMPIXELS; i++) {
        uint32_t c = strip.getPixelColor(i);
        uint8_t r = (c >> 16) & 0xFF, g = (c >> 8) & 0xFF, b = c & 0xFF;
        strip.setPixelColor(i, r * 7 / 8, g * 7 / 8, b * 7 / 8);
      }
      if (!gFrozen && (esp_random() & 0x3) == 0) {
        // pick a random in-shape pixel
        uint8_t tries = 0, i;
        do { i = esp_random() % NUMPIXELS; } while (!inShape(i) && ++tries < 20);
        if (inShape(i)) { lastLit = i; setPx(i, 1.0f); }
      }
      strip.show();
      break;
    }
    default:
      break;
  }
}

// ---- Web UI ----------------------------------------------------------------
const char PAGE[] PROGMEM = R"HTML(<!doctype html><html><head>
<meta name=viewport content="width=device-width,initial-scale=1">
<title>HEX Studio</title>
<style>
 body{font-family:system-ui,sans-serif;background:#111;color:#eee;margin:0;padding:14px;max-width:520px}
 h2{margin:.2em 0}
 .row{margin:10px 0}
 label{display:block;font-size:13px;color:#aaa;margin-bottom:3px}
 input[type=range]{width:100%;height:30px}
 .btns{display:flex;flex-wrap:wrap;gap:6px}
 button{flex:1 1 auto;min-width:70px;padding:11px 8px;font-size:14px;border:0;border-radius:8px;background:#333;color:#eee}
 button.on{background:#0a7;color:#fff}
 #rb{font-family:monospace;font-size:13px;background:#000;padding:8px;border-radius:6px;white-space:pre-wrap;color:#6f6}
 .sw{display:inline-block;width:22px;height:22px;border-radius:5px;vertical-align:middle;border:1px solid #555}
</style></head><body>
<h2>HEX Studio</h2>

<div class=row><label>Color <span id=sw class=sw></span></label>
 <input type=color id=col value="#ff8c28" oninput="setCol(this.value)"></div>
<div class=row><label>R <span id=rl></span></label><input type=range id=r min=0 max=255 value=255 oninput="ch('r',this.value)"></div>
<div class=row><label>G <span id=gl></span></label><input type=range id=g min=0 max=255 value=140 oninput="ch('g',this.value)"></div>
<div class=row><label>B <span id=bl></span></label><input type=range id=b min=0 max=255 value=40 oninput="ch('b',this.value)"></div>
<div class=row><label>Brightness <span id=bril></span></label><input type=range id=bri min=0 max=255 value=40 oninput="ch('bri',this.value)"></div>

<div class=row><label>Shape</label><div class=btns>
 <button id=sh0 onclick="shape(0)">Center</button>
 <button id=sh1 onclick="shape(1)">+Inner ring</button>
 <button id=sh2 onclick="shape(2)">+Two rings</button>
 <button id=sh3 onclick="shape(3)">All</button>
</div></div>

<div class=row><label>Animation</label><div class=btns>
 <button id=an0 onclick="anim(0)">Static</button>
 <button id=an1 onclick="anim(1)">Spiral</button>
 <button id=an2 onclick="anim(2)">Orbit</button>
 <button id=an3 onclick="anim(3)">Breathe</button>
 <button id=an4 onclick="anim(4)">Twinkle</button>
 <button id=an5 onclick="anim(5)">Split RGB</button>
</div></div>

<div class=row><label>Speed <span id=spl></span></label><input type=range id=sp min=1 max=100 value=30 oninput="ch('speed',this.value)"></div>
<div class=row><label>Trail (spiral/orbit) <span id=trl></span></label><input type=range id=tr min=0 max=10 value=3 oninput="ch('trail',this.value)"></div>
<div class=row><label>Orbit ring</label><div class=btns>
 <button id=or1 onclick="ch('ring',1)">1</button>
 <button id=or2 onclick="ch('ring',2)">2</button>
 <button id=or3 onclick="ch('ring',3)">3</button>
</div></div>
<div class=row><label>Fringe spread (Split) <span id=fsl></span></label><input type=range id=fs min=0 max=30 value=12 oninput="ch('spread',this.value)"></div>
<div class=row><label>Fringe rotate (Split) <span id=frl2></span></label><input type=range id=fr2 min=0 max=360 value=0 oninput="ch('rotate',this.value)"></div>

<div class=row><div class=btns>
 <button id=frz onclick="toggleFreeze()">Freeze</button>
 <button onclick="send('step=1')">Step +</button>
 <button id=gam onclick="toggleGamma()">Gamma: on</button>
 <button onclick="send('off=1')">All off</button>
</div></div>

<div class=row><label>Current settings (read off when it looks good)</label>
 <div id=rb>...</div></div>

<script>
let st={r:255,g:140,b:40,bri:40,shape:1,anim:0,speed:30,trail:3,ring:1,gamma:1,frozen:0,lit:18,spread:12,rotate:0,anchor:18};
function send(q){fetch('/set?'+q);}
function ch(k,v){v=+v;st[k]=v;send(k+'='+v);syncLabels();}
function shape(n){st.shape=n;send('shape='+n);hl('sh',n,4);}
function anim(n){st.anim=n;send('anim='+n);hl('an',n,6);}
function setCol(hex){let r=parseInt(hex.substr(1,2),16),g=parseInt(hex.substr(3,2),16),b=parseInt(hex.substr(5,2),16);
 st.r=r;st.g=g;st.b=b;document.getElementById('r').value=r;document.getElementById('g').value=g;document.getElementById('b').value=b;
 send('r='+r+'&g='+g+'&b='+b);syncLabels();}
function toggleFreeze(){st.frozen^=1;send('freeze='+st.frozen);document.getElementById('frz').className=st.frozen?'on':'';}
function toggleGamma(){st.gamma^=1;send('gamma='+st.gamma);let e=document.getElementById('gam');e.textContent='Gamma: '+(st.gamma?'on':'off');e.className=st.gamma?'on':'';}
function hl(p,n,cnt){for(let i=0;i<cnt;i++)document.getElementById(p+i).className=(i==n?'on':'');}
function hx(v){return ('0'+(+v).toString(16)).slice(-2);}
function syncLabels(){rl.textContent=st.r;gl.textContent=st.g;bl.textContent=st.b;bril.textContent=st.bri;
 spl.textContent=st.speed;trl.textContent=st.trail;fsl.textContent=(st.spread/10).toFixed(1);frl2.textContent=st.rotate;
 let c='#'+hx(st.r)+hx(st.g)+hx(st.b);sw.style.background=c;col.value=c;}
function refresh(){fetch('/state').then(r=>r.json()).then(s=>{st.lit=s.lit;st.anchor=s.anchor;
 rb.textContent='rgb='+st.r+','+st.g+','+st.b+'  hex=#'+hx(st.r)+hx(st.g)+hx(st.b)+
  '\nbri='+st.bri+'  gamma='+(st.gamma?'on':'off')+
  '\nshape='+['center','+ring1','+ring2','all'][st.shape]+
  '  anim='+['static','spiral','orbit','breathe','twinkle','split'][st.anim]+
  '\nspeed='+st.speed+'  trail='+st.trail+'  orbitRing='+st.ring+
  '\nlit pixel='+st.lit+
  (st.anim==5?'\nsplit: anchor='+st.anchor+'  spread='+(st.spread/10).toFixed(1)+'  rotate='+st.rotate+'deg':'');});}
hl('sh',1,4);hl('an',0,5);syncLabels();setInterval(refresh,600);refresh();
</script></body></html>)HTML";

void applyAfterSet() {
  if (gAnim == ANIM_NONE)
    renderStatic();
  else if (gAnim == ANIM_SPLIT)
    renderSplit(); // static; redraw on every edit
  else if (gFrozen)
    renderFrame(); // redraw current (frozen) frame so edits show immediately
}

void handleSet() {
  if (server.hasArg("r")) gR = server.arg("r").toInt();
  if (server.hasArg("g")) gG = server.arg("g").toInt();
  if (server.hasArg("b")) gB = server.arg("b").toInt();
  if (server.hasArg("bri")) gBri = server.arg("bri").toInt();
  if (server.hasArg("shape")) gShape = server.arg("shape").toInt();
  if (server.hasArg("speed")) gSpeed = server.arg("speed").toInt();
  if (server.hasArg("trail")) gTrail = server.arg("trail").toInt();
  if (server.hasArg("ring")) gOrbitRing = server.arg("ring").toInt();
  if (server.hasArg("gamma")) gGamma = server.arg("gamma").toInt() != 0;
  if (server.hasArg("freeze")) gFrozen = server.arg("freeze").toInt() != 0;
  if (server.hasArg("spread")) gSpread = server.arg("spread").toInt() / 10.0f; // 0..30 -> 0..3.0
  if (server.hasArg("rotate")) gFringeAngle = server.arg("rotate").toInt() * 0.0174533f; // deg->rad
  if (server.hasArg("step")) {
    if (gAnim == ANIM_SPIRAL || gAnim == ANIM_ORBIT)
      animPos++; // advance one frame; applyAfterSet() redraws if frozen
    else if (gAnim == ANIM_SPLIT)
      gAnchor = spiralOrder[(++anchorStep) % NUMPIXELS]; // walk the triad around
  }
  if (server.hasArg("off")) {
    gAnim = ANIM_NONE;
    strip.clear();
    strip.show();
  }
  if (server.hasArg("anim")) {
    gAnim = server.arg("anim").toInt();
    animPos = 0;
    breathePhase = 0;
    if (gAnim != ANIM_NONE) strip.clear();
  }
  applyAfterSet();
  server.send(200, "text/plain", "ok");
}

void handleState() {
  char buf[220];
  snprintf(buf, sizeof(buf),
           "{\"r\":%u,\"g\":%u,\"b\":%u,\"bri\":%u,\"shape\":%u,\"anim\":%u,"
           "\"speed\":%u,\"trail\":%u,\"ring\":%u,\"gamma\":%u,\"lit\":%u,"
           "\"anchor\":%u}",
           gR, gG, gB, gBri, gShape, gAnim, gSpeed, gTrail, gOrbitRing,
           gGamma ? 1 : 0, lastLit, gAnchor);
  server.send(200, "application/json", buf);
}

void setupWifi() {
  WiFi.mode(WIFI_STA);
#if HAVE_SECRETS
  WiFi.begin(RES_WIFI_SSID, RES_WIFI_PASSWORD);
  Serial.print("WiFi connecting");
  uint32_t t0 = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - t0 < 12000) {
    delay(300);
    Serial.print(".");
  }
  Serial.println();
  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("HEX Studio at http://");
    Serial.println(WiFi.localIP());
    return;
  }
  Serial.println("station failed; starting AP fallback");
#endif
  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_SSID, AP_PASS);
  Serial.print("AP '" AP_SSID "' -> http://");
  Serial.println(WiFi.softAPIP());
}

// PowerFeather V2: the switchable 3V3 header rail (powers the LED + STEMMA) is
// gated by GPIO4 (EN_3V3). The SDK enables it in Board.init(); we don't run the
// SDK, so drive it high ourselves or the 3V3 header reads 0 V.
#define EN_3V3_PIN 4

void setup() {
  Serial.begin(115200);
  delay(200);
  pinMode(EN_3V3_PIN, OUTPUT);
  digitalWrite(EN_3V3_PIN, HIGH); // enable the switchable 3V3 header rail
  delay(20);
  buildGeometry();
  strip.begin();
  strip.setBrightness(255); // we scale manually for fine low-end control
  strip.clear();
  strip.show();
  setupWifi();
  server.on("/", []() { server.send_P(200, "text/html", PAGE); });
  server.on("/set", handleSet);
  server.on("/state", handleState);
  server.begin();
  Serial.printf("HEX37 geometry: ring sizes %u/%u/%u/%u\n", ringSize[0],
                ringSize[1], ringSize[2], ringSize[3]);
  renderStatic();
}

void loop() {
  server.handleClient();
  if (gAnim != ANIM_NONE && gAnim != ANIM_SPLIT && !gFrozen &&
      millis() - lastFrame >= frameMs()) {
    lastFrame = millis();
    renderFrame();
  }
}
