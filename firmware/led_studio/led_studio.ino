// Resonance LED Studio -- merged interactive aesthetic bench tool. Drives EITHER
// the SK6812 "HEX" 37px RGB grid OR the single 4 W SK6812 RGBW point source on the
// SAME data pin (default GPIO10 / A0), with a UI toggle to hot-swap between them
// (reconfigures the NeoPixel type/length at runtime -- no reflash). Supersedes the
// separate hex_studio + rgbw_studio sketches.
//
// Workflow: blank the LEDs (All off), physically swap the module on the JST, then
// flip the mode toggle to match. Mismatched mode is harmless (both are SK6812) --
// worst case is wrong colors until refreshed; the firmware blanks on every switch.
//
// Build/flash (USB): ./build.sh --port /dev/ttyACM1   (override pin with --pin N)

#include <Arduino.h>
#include <math.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <Update.h>
#include <Adafruit_NeoPixel.h>

#ifndef DATA_PIN
#define DATA_PIN 10 // GPIO10 / A0
#endif
#define NUMPIXELS 37 // HEX max; RGBW mode uses length 1
#define CENTER 18
// Constructed for HEX (RGB) by default; switched to RGBW at runtime via applyMode().
Adafruit_NeoPixel strip(NUMPIXELS, DATA_PIN, NEO_GRB + NEO_KHZ800);

// PowerFeather SDK: we init it ONLY to program the charger's LFP profile (3.65 V
// ceiling) -- without this the BQ25628E runs its 4.2 V Li-ion default, which
// OVERCHARGES an attached LFP whenever USB/panel power is present (see
// POWERFEATHER_NOTES "chemistry flash order"). Added 2026-06-11 so studio sessions
// are safe with USB + cell simultaneously (and the cell charges correctly).
#include <PowerFeather.h>
#include "../powerfeather_solar_guard.h"
using namespace PowerFeather;
#if !defined(POWERFEATHER_BOARD_V2) && !defined(CONFIG_ESP32S3_POWERFEATHER_V2)
#error "Build with -DPOWERFEATHER_BOARD_V2=1 (build.sh passes it) so the SDK targets the V2."
#endif
#define STUDIO_BATTERY_MAH 2000
#define STUDIO_BATTERY_TYPE Mainboard::BatteryType::Generic_LFP
#define STUDIO_CHARGE_MA 500.0f
#define STUDIO_MAINTAIN_V 4.6f
bool gPfReady = false; // SDK up -> /state carries battery stats (SOC matters for sag/brightness)

// PowerFeather V2: the switchable 3V3 header rail (powers the LED) is gated by GPIO4
// (EN_3V3, active HIGH). Kept as a fallback in case Board.init() fails -- the rail
// must be on either way. See firmware/POWERFEATHER_NOTES.md.
#define EN_3V3_PIN 4

#if __has_include("wifi_secrets.h")
#include "wifi_secrets.h"
#define HAVE_SECRETS 1
#else
#define HAVE_SECRETS 0
#endif
#define AP_SSID "ResonanceLED"
#define AP_PASS "resonance"

WebServer server(80);

// ---- Modes & animations ----------------------------------------------------
enum Mode { MODE_HEX = 0, MODE_RGBW = 1, MODE_RGB = 2 };
// HEX anims:      0 static, 1 spiral, 2 orbit, 3 breathe, 4 twinkle  (+ Split modifier)
// RGBW/RGB anims: 0 static, 1 hue,    2 breathe, 3 candle, 4 fade
// MODE_RGB is a single high-power RGB pixel (no white die): same render path as
// RGBW but a 3-byte strip -- the W component is simply ignored by the library.
uint8_t gMode = MODE_HEX;
uint8_t gAnim = 0; // index within the current mode's animation set

// ---- Shared color state ----------------------------------------------------
uint8_t gR = 255, gG = 140, gB = 40, gW = 0; // warm amber default; W = RGBW only
uint8_t gBri = 40;
uint8_t gSpeed = 30;
bool gGamma = true;

// ---- HEX state -------------------------------------------------------------
uint8_t gShape = 1;     // 0 center, 1 +ring1, 2 +ring1+2, 3 all
uint8_t gTrail = 3;
uint8_t gOrbitRing = 1; // 1..3
bool gFrozen = false;
uint8_t gSplit = 0; // Split-RGB style: 0 off, 1 triad (local offset), 2 rotate (120 deg about center)
uint32_t hexAnimPos = 0;
float hexBreathePhase = 0;
uint8_t lastLit = CENTER;
// Split (HEX): pure R/G/B triad for wide color fringing
float gSpread = 1.2f;
float gFringeAngle = 0;
uint8_t gAnchor = CENTER;
uint32_t anchorStep = 0;

// ---- RGBW state ------------------------------------------------------------
uint8_t gB2r = 0, gB2g = 120, gB2b = 255; // Color B for Fade
float rgbwPhase = 0;
float candleLevel = 1.0f, candleTarget = 1.0f;

uint32_t lastFrame = 0;

// ---- HEX geometry (7 rows 4-5-6-7-6-5-4, center=18) ------------------------
const uint8_t ROW_COUNT[7] = {4, 5, 6, 7, 6, 5, 4};
uint8_t ringOf[NUMPIXELS];
float pxAngle[NUMPIXELS];
uint8_t spiralOrder[NUMPIXELS];
uint8_t ringMembers[4][18];
uint8_t ringSize[4] = {0, 0, 0, 0};
float gX[NUMPIXELS], gY[NUMPIXELS];

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
    float y = (3.0f - (float)r) * 0.8660254f;
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
  for (uint8_t i = 0; i < NUMPIXELS; i++)
    ringMembers[ringOf[i]][ringSize[ringOf[i]]++] = i;
  for (uint8_t r = 0; r < 4; r++)
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

inline uint8_t gam(uint8_t v) { return gGamma ? Adafruit_NeoPixel::gamma8(v) : v; }

// ---- HEX rendering ---------------------------------------------------------
void setPxHex(uint16_t i, float factor) {
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

// Split the base point into pure R/G/B across three pixels (scaled by factor,
// max-combining on overlap), by the current style:
//  - gSplit==1 (triad):  R/G/B on a small triangle OFFSET from baseIdx by `spread`
//    at 120 deg apart (orientation = `rotate`). A local color-fringe cluster.
//  - gSplit==2 (rotate): R at baseIdx itself, G/B at baseIdx ROTATED 120/240 deg
//    about the grid center (same radius). A 3-fold rotationally-symmetric split.
void splitInto(uint8_t baseIdx, float factor, uint8_t *R, uint8_t *G, uint8_t *B) {
  const float T = 2.0943951f; // 120 deg
  if (factor < 0) factor = 0;
  if (factor > 1) factor = 1;
  uint8_t v = (uint8_t)((float)gBri * factor);
  float ax = gX[baseIdx], ay = gY[baseIdx];
  uint8_t pr, pg, pb;
  if (gSplit == 2) { // rotate the point 120/240 deg about the center
    float rad = sqrtf(ax * ax + ay * ay), th = atan2f(ay, ax);
    pr = baseIdx;
    pg = nearestPixel(rad * cosf(th + T), rad * sinf(th + T));
    pb = nearestPixel(rad * cosf(th + 2 * T), rad * sinf(th + 2 * T));
  } else { // local triad offset by `spread`
    pr = nearestPixel(ax + gSpread * cosf(gFringeAngle),
                      ay + gSpread * sinf(gFringeAngle));
    pg = nearestPixel(ax + gSpread * cosf(gFringeAngle + T),
                      ay + gSpread * sinf(gFringeAngle + T));
    pb = nearestPixel(ax + gSpread * cosf(gFringeAngle + 2 * T),
                      ay + gSpread * sinf(gFringeAngle + 2 * T));
  }
  R[pr] = max(R[pr], v);
  G[pg] = max(G[pg], v);
  B[pb] = max(B[pb], v);
}

void showSplit(uint8_t *R, uint8_t *G, uint8_t *B) {
  for (uint16_t i = 0; i < NUMPIXELS; i++)
    strip.setPixelColor(i, gam(R[i]), gam(G[i]), gam(B[i]));
  strip.show();
}

void renderStaticHex() {
  if (gSplit) { // static split at the anchor
    uint8_t R[NUMPIXELS] = {0}, G[NUMPIXELS] = {0}, B[NUMPIXELS] = {0};
    splitInto(gAnchor, 1.0f, R, G, B);
    lastLit = gAnchor;
    showSplit(R, G, B);
    return;
  }
  for (uint16_t i = 0; i < NUMPIXELS; i++) setPxHex(i, inShape(i) ? 1.0f : 0.0f);
  strip.show();
}

// Map a monotonic step counter to a position along the path. Orbit (anim 2) wraps
// seamlessly around the ring; Spiral (anim 1) ping-pongs (0..n-1..1..0) so it
// reverses at the ends instead of jumping from the outer tip back to the center.
int pathIndex(long step, int n) {
  if (n <= 1) return 0;
  if (gAnim == 2) return (int)(((step % n) + n) % n); // orbit: seamless ring wrap
  long period = 2 * (n - 1);                          // spiral: ping-pong
  long m = ((step % period) + period) % period;
  return (int)(m < n ? m : period - m);
}

void renderFrameHex() {
  switch (gAnim) {
    case 1:
    case 2: {
      const uint8_t *order;
      uint16_t n;
      if (gAnim == 1) { order = spiralOrder; n = NUMPIXELS; }
      else {
        uint8_t r = gOrbitRing < 1 ? 1 : (gOrbitRing > 3 ? 3 : gOrbitRing);
        order = ringMembers[r];
        n = ringSize[r];
      }
      if (gSplit) { // a moving split at the head + each trail step
        uint8_t R[NUMPIXELS] = {0}, G[NUMPIXELS] = {0}, B[NUMPIXELS] = {0};
        for (int t = 0; t <= gTrail; t++) {
          int p = pathIndex((long)hexAnimPos - t, n);
          float f = 1.0f - (float)t / (float)(gTrail + 1);
          splitInto(order[p], f, R, G, B);
          if (t == 0) lastLit = order[p];
        }
        showSplit(R, G, B);
      } else {
        for (uint16_t i = 0; i < NUMPIXELS; i++) strip.setPixelColor(i, 0);
        for (int t = 0; t <= gTrail; t++) {
          int p = pathIndex((long)hexAnimPos - t, n);
          float f = 1.0f - (float)t / (float)(gTrail + 1);
          setPxHex(order[p], f);
          if (t == 0) lastLit = order[p];
        }
        strip.show();
      }
      if (!gFrozen) hexAnimPos++;
      break;
    }
    case 3: {
      float f = 0.5f + 0.5f * sinf(hexBreathePhase);
      if (gSplit) {
        uint8_t R[NUMPIXELS] = {0}, G[NUMPIXELS] = {0}, B[NUMPIXELS] = {0};
        splitInto(gAnchor, f, R, G, B);
        lastLit = gAnchor;
        showSplit(R, G, B);
      } else {
        for (uint16_t i = 0; i < NUMPIXELS; i++) setPxHex(i, inShape(i) ? f : 0.0f);
        strip.show();
      }
      if (!gFrozen) hexBreathePhase += 0.15f;
      break;
    }
    case 4: {
      for (uint16_t i = 0; i < NUMPIXELS; i++) {
        uint32_t c = strip.getPixelColor(i);
        uint8_t r = (c >> 16) & 0xFF, g = (c >> 8) & 0xFF, b = c & 0xFF;
        strip.setPixelColor(i, r * 7 / 8, g * 7 / 8, b * 7 / 8);
      }
      if (!gFrozen && (esp_random() & 0x3) == 0) {
        uint8_t tries = 0, i;
        do { i = esp_random() % NUMPIXELS; } while (!inShape(i) && ++tries < 20);
        if (inShape(i)) { lastLit = i; setPxHex(i, 1.0f); }
      }
      strip.show();
      break;
    }
    default:
      break;
  }
}

// ---- RGBW rendering --------------------------------------------------------
void setRGBWpix(uint8_t r, uint8_t g, uint8_t b, uint8_t w, float f) {
  float s = (float)gBri / 255.0f * f;
  if (s < 0) s = 0;
  if (s > 1) s = 1;
  strip.setPixelColor(0, gam((uint8_t)(r * s)), gam((uint8_t)(g * s)),
                       gam((uint8_t)(b * s)), gam((uint8_t)(w * s)));
  strip.show();
}

void renderStaticRGBW() { setRGBWpix(gR, gG, gB, gW, 1.0f); }

void renderFrameRGBW() {
  switch (gAnim) {
    case 1: { // hue cycle
      uint16_t hue = (uint16_t)((uint32_t)rgbwPhase & 0xFFFF);
      uint32_t c = strip.ColorHSV(hue, 255, gBri);
      if (gGamma) c = strip.gamma32(c);
      strip.setPixelColor(0, c);
      strip.show();
      rgbwPhase += 256;
      break;
    }
    case 2: { // breathe
      float f = 0.5f + 0.5f * sinf(rgbwPhase);
      setRGBWpix(gR, gG, gB, gW, f);
      rgbwPhase += 0.15f;
      break;
    }
    case 3: { // candle
      candleLevel += (candleTarget - candleLevel) * 0.25f;
      if (fabsf(candleTarget - candleLevel) < 0.03f) {
        uint32_t r = esp_random();
        candleTarget = 0.45f + (float)(r & 0xFFFF) / 65535.0f * 0.55f;
        if ((r & 0x7) == 0) candleTarget *= 0.7f;
      }
      setRGBWpix(gR, gG, gB, gW, candleLevel);
      break;
    }
    case 4: { // fade current <-> color B
      float t = 0.5f + 0.5f * sinf(rgbwPhase);
      uint8_t r = gR + (int)((gB2r - gR) * t);
      uint8_t g = gG + (int)((gB2g - gG) * t);
      uint8_t b = gB + (int)((gB2b - gB) * t);
      uint8_t w = gW + (int)((0 - gW) * t);
      setRGBWpix(r, g, b, w, 1.0f);
      rgbwPhase += 0.06f;
      break;
    }
    default:
      break;
  }
}

// ---- Mode + dispatch -------------------------------------------------------
void applyMode() {
  strip.clear();
  strip.show();
  if (gMode == MODE_RGBW) {
    // RGBW, not GRBW: slot-tested on the production 4 W module 2026-07-11
    // (led_sol_bench /raw) -- GRBW had R/G silently swapped in every prior
    // studio session. MODE_RGB below is a different module and is UNVERIFIED.
    strip.updateType(NEO_RGBW + NEO_KHZ800);
    strip.updateLength(1);
  } else if (gMode == MODE_RGB) {
    strip.updateType(NEO_GRB + NEO_KHZ800);
    strip.updateLength(1);
  } else {
    strip.updateType(NEO_GRB + NEO_KHZ800);
    strip.updateLength(NUMPIXELS);
  }
  strip.clear();
  strip.show();
  gAnim = 0;
  hexAnimPos = 0;
  hexBreathePhase = 0;
  rgbwPhase = 0;
  candleLevel = candleTarget = 1.0f;
}

bool isAnimating() {
  if (gMode == MODE_HEX) return gAnim >= 1 && gAnim <= 4 && !gFrozen;
  return gAnim >= 1 && gAnim <= 4;
}

void renderStatic() {
  if (gMode == MODE_HEX)
    renderStaticHex();
  else
    renderStaticRGBW();
}

void renderFrame() {
  if (gMode == MODE_HEX)
    renderFrameHex();
  else
    renderFrameRGBW();
}

void applyAfterSet() {
  if (gMode == MODE_HEX) {
    if (gAnim == 0) renderStaticHex();
    else if (gFrozen) renderFrameHex();
  } else {
    if (gAnim == 0) renderStaticRGBW();
  }
}

// ---- Web UI ----------------------------------------------------------------
const char PAGE[] PROGMEM = R"HTML(<!doctype html><html><head>
<meta name=viewport content="width=device-width,initial-scale=1">
<title>LED Studio</title>
<style>
 body{font-family:system-ui,sans-serif;background:#111;color:#eee;margin:0;padding:14px;max-width:520px}
 h2{margin:.2em 0}
 .row{margin:10px 0}
 label{display:block;font-size:13px;color:#aaa;margin-bottom:3px}
 input[type=range]{width:100%;height:30px}
 .btns{display:flex;flex-wrap:wrap;gap:6px}
 button{flex:1 1 auto;min-width:64px;padding:11px 8px;font-size:14px;border:0;border-radius:8px;background:#333;color:#eee}
 button.on{background:#0a7;color:#fff}
 .mode button{padding:13px;font-size:15px}
 #rb{font-family:monospace;font-size:13px;background:#000;padding:8px;border-radius:6px;white-space:pre-wrap;color:#6f6}
 .sw{display:inline-block;width:22px;height:22px;border-radius:5px;vertical-align:middle;border:1px solid #555}
 .hide{display:none}
 hr{border:0;border-top:1px solid #333;margin:14px 0}
</style></head><body>
<h2>LED Studio</h2>

<div class=row><label>Module (blank LEDs + swap before toggling)</label><div class="btns mode">
 <button id=md0 onclick="mode(0)">HEX grid (37px)</button>
 <button id=md1 onclick="mode(1)">RGBW point</button>
 <button id=md2 onclick="mode(2)">RGB point</button>
</div></div>
<hr>

<div class=row><label>Color (RGB) <span id=sw class=sw></span></label>
 <input type=color id=col value="#ff8c28" oninput="setCol(this.value)"></div>
<div class=row><label>R <span id=rl></span></label><input type=range id=r min=0 max=255 value=255 oninput="ch('r',this.value)"></div>
<div class=row><label>G <span id=gl></span></label><input type=range id=g min=0 max=255 value=140 oninput="ch('g',this.value)"></div>
<div class=row><label>B <span id=bl></span></label><input type=range id=b min=0 max=255 value=40 oninput="ch('b',this.value)"></div>
<div class=row id=wrow><label>W (white die, RGBW) <span id=wl></span></label><input type=range id=w min=0 max=255 value=0 oninput="ch('w',this.value)"></div>
<div class=row><label>Brightness <span id=bril></span></label><input type=range id=bri min=0 max=255 value=40 oninput="ch('bri',this.value)"></div>
<div class=row><label>Speed <span id=spl></span></label><input type=range id=sp min=1 max=100 value=30 oninput="ch('speed',this.value)"></div>

<!-- HEX-only controls -->
<div id=hexUI>
<hr>
<div class=row><label>Shape</label><div class=btns>
 <button id=sh0 onclick="shape(0)">Center</button>
 <button id=sh1 onclick="shape(1)">+Inner ring</button>
 <button id=sh2 onclick="shape(2)">+Two rings</button>
 <button id=sh3 onclick="shape(3)">All</button>
</div></div>
<div class=row><label>Animation</label><div class=btns>
 <button id=ah0 onclick="anim(0)">Static</button>
 <button id=ah1 onclick="anim(1)">Spiral</button>
 <button id=ah2 onclick="anim(2)">Orbit</button>
 <button id=ah3 onclick="anim(3)">Breathe</button>
 <button id=ah4 onclick="anim(4)">Twinkle</button>
</div></div>
<div class=row><label>Split RGB (applies to Static / Spiral / Orbit / Breathe)</label><div class=btns>
 <button id=sp0 onclick="splitMode(0)">Off</button>
 <button id=sp1 onclick="splitMode(1)">Triad</button>
 <button id=sp2 onclick="splitMode(2)">Rotate 120&deg;</button>
</div><label style="margin-top:4px">Triad = local R/G/B offset cluster (use Fringe spread/rotate). Rotate = R at the point, G/B the same point rotated 120/240&deg; about the grid center.</label></div>
<div class=row><label>Trail (spiral/orbit) <span id=trl></span></label><input type=range id=tr min=0 max=10 value=3 oninput="ch('trail',this.value)"></div>
<div class=row><label>Orbit ring</label><div class=btns>
 <button onclick="ch('ring',1)">1</button><button onclick="ch('ring',2)">2</button><button onclick="ch('ring',3)">3</button>
</div></div>
<div class=row><label>Fringe spread (Split) <span id=fsl></span></label><input type=range id=fs min=0 max=30 value=12 oninput="ch('spread',this.value)"></div>
<div class=row><label>Fringe rotate (Split) <span id=frl2></span></label><input type=range id=fr2 min=0 max=360 value=0 oninput="ch('rotate',this.value)"></div>
<div class=row><div class=btns>
 <button id=frz onclick="toggleFreeze()">Freeze</button>
 <button onclick="send('step=1')">Step +</button>
</div></div>
</div>

<!-- RGBW-only controls -->
<div id=rgbwUI class=hide>
<hr>
<div id=whiteBlock>
<div class=row><label>White / warmth presets</label><div class=btns>
 <button onclick="preset('wonly')">W only</button>
 <button onclick="preset('rgbw')">RGB white</button>
 <button onclick="preset('full')">RGBW full</button>
 <button onclick="preset('candle')">Warm amber</button>
</div></div>
<div class=row><label>Warmth crossfade (RGB white &harr; W) <span id=warl></span></label>
 <input type=range id=war min=0 max=100 value=0 oninput="warmth(this.value)"></div>
</div>
<div class=row><label>Animation</label><div class=btns>
 <button id=ar0 onclick="anim(0)">Static</button>
 <button id=ar1 onclick="anim(1)">Hue cycle</button>
 <button id=ar2 onclick="anim(2)">Breathe</button>
 <button id=ar3 onclick="anim(3)">Candle</button>
 <button id=ar4 onclick="anim(4)">Fade</button>
</div></div>
<div class=row><label>Color B (for Fade)</label>
 <input type=color id=colb value="#0078ff" oninput="setColB(this.value)"></div>
</div>

<hr>
<div class=row><div class=btns>
 <button id=gam onclick="toggleGamma()">Gamma: on</button>
 <button onclick="send('off=1')">All off</button>
</div></div>
<div class=row><label>Current settings</label><div id=rb>...</div></div>
<div class=row><label>Battery</label><div id=bat>...</div></div>

<script>
let st={mode:0,r:255,g:140,b:40,w:0,bri:40,speed:30,anim:0,gamma:1,shape:1,trail:3,ring:1,
 spread:12,rotate:0,b2r:0,b2g:120,b2b:255,frozen:0,split:0,lit:18,anchor:18};
function send(q){fetch('/set?'+q);}
function ch(k,v){v=+v;st[k]=v;send(k+'='+v);syncLabels();}
function hx(v){return ('0'+(+v).toString(16)).slice(-2);}
function hl(p,n,cnt){for(let i=0;i<cnt;i++){let e=document.getElementById(p+i);if(e)e.className=(i==n?'on':'');}}
function applyModeUI(m){
 hl('md',m,3);
 document.getElementById('hexUI').className=(m==0?'':'hide');
 document.getElementById('rgbwUI').className=((m==1||m==2)?'':'hide');
 document.getElementById('whiteBlock').style.display=(m==1?'':'none'); // white die = RGBW only
 document.getElementById('wrow').style.display=(m==1?'':'none');
}
function mode(m){st.mode=m;st.anim=0;send('mode='+m);applyModeUI(m);hl('ah',0,5);hl('ar',0,5);}
function anim(n){st.anim=n;send('anim='+n);hl(st.mode==0?'ah':'ar',n,5);}
function splitMode(n){st.split=n;send('split='+n);hl('sp',n,3);}
function shape(n){st.shape=n;send('shape='+n);hl('sh',n,4);}
function setCol(hex){let r=parseInt(hex.substr(1,2),16),g=parseInt(hex.substr(3,2),16),b=parseInt(hex.substr(5,2),16);
 st.r=r;st.g=g;st.b=b;document.getElementById('r').value=r;document.getElementById('g').value=g;document.getElementById('b').value=b;
 send('r='+r+'&g='+g+'&b='+b);syncLabels();}
function setColB(hex){st.b2r=parseInt(hex.substr(1,2),16);st.b2g=parseInt(hex.substr(3,2),16);st.b2b=parseInt(hex.substr(5,2),16);
 send('b2r='+st.b2r+'&b2g='+st.b2g+'&b2b='+st.b2b);}
function setRGBW(r,g,b,w){st.r=r;st.g=g;st.b=b;st.w=w;
 for(const k of ['r','g','b','w'])document.getElementById(k).value=st[k];
 send('r='+r+'&g='+g+'&b='+b+'&w='+w);syncLabels();}
function preset(p){if(p=='wonly')setRGBW(0,0,0,255);else if(p=='rgbw')setRGBW(255,255,255,0);
 else if(p=='full')setRGBW(255,255,255,255);else if(p=='candle')setRGBW(255,120,25,40);}
function warmth(v){let f=v/100;document.getElementById('warl').textContent=v+'%';
 let c=Math.round(255*(1-f)),wv=Math.round(255*f);setRGBW(c,c,c,wv);}
function toggleFreeze(){st.frozen^=1;send('freeze='+st.frozen);document.getElementById('frz').className=st.frozen?'on':'';}
function toggleGamma(){st.gamma^=1;send('gamma='+st.gamma);let e=document.getElementById('gam');e.textContent='Gamma: '+(st.gamma?'on':'off');e.className=st.gamma?'on':'';}
function syncLabels(){rl.textContent=st.r;gl.textContent=st.g;bl.textContent=st.b;wl.textContent=st.w;bril.textContent=st.bri;
 spl.textContent=st.speed;trl.textContent=st.trail;fsl.textContent=(st.spread/10).toFixed(1);frl2.textContent=st.rotate;
 let c='#'+hx(st.r)+hx(st.g)+hx(st.b);sw.style.background=c;col.value=c;}
function refresh(){fetch('/state').then(r=>r.json()).then(s=>{st.lit=s.lit;st.anchor=s.anchor;
 let an=st.mode==0?['static','spiral','orbit','breathe','twinkle','split'][s.anim]
                  :['static','hue','breathe','candle','fade'][s.anim];
 rb.textContent='mode='+['HEX','RGBW','RGB'][st.mode]+'  anim='+an+
  '\nrgb'+(st.mode==1?'w':'')+'='+s.r+','+s.g+','+s.b+(st.mode==1?','+s.w:'')+'  hex=#'+hx(s.r)+hx(s.g)+hx(s.b)+
  '\nbri='+s.bri+'  gamma='+(s.gamma?'on':'off')+'  speed='+s.speed+
  (st.mode==0?'\nshape='+['center','+ring1','+ring2','all'][s.shape]+'  lit='+s.lit+
    (s.split?'  split='+['off','triad','rotate'][s.split]+'[anchor='+s.anchor+(s.split==1?' spread='+(st.spread/10).toFixed(1)+' rot='+st.rotate:'')+']':''):
    '\ncolorB=#'+hx(st.b2r)+hx(st.b2g)+hx(st.b2b));
 let bat=document.getElementById('bat');
 if(!s.pf){bat.textContent='no battery data (SDK init failed)';}
 else{let act=s.ma>30?('charging +'+s.ma+'mA'):(s.ma<-30?('discharging '+s.ma+'mA'):'idle ~'+s.ma+'mA');
  bat.textContent='SOC '+s.soc+'%  '+s.bv.toFixed(3)+'V  '+act+
   (s.sgood?('  |  supply '+s.sv.toFixed(2)+'V ok'):'  |  on battery');}});}
applyModeUI(0);hl('sh',1,4);hl('ah',0,5);hl('sp',0,3);syncLabels();setInterval(refresh,600);refresh();
</script></body></html>)HTML";

void handleSet() {
  if (server.hasArg("r")) gR = server.arg("r").toInt();
  if (server.hasArg("g")) gG = server.arg("g").toInt();
  if (server.hasArg("b")) gB = server.arg("b").toInt();
  if (server.hasArg("w")) gW = server.arg("w").toInt();
  if (server.hasArg("bri")) gBri = server.arg("bri").toInt();
  if (server.hasArg("speed")) gSpeed = server.arg("speed").toInt();
  if (server.hasArg("gamma")) gGamma = server.arg("gamma").toInt() != 0;
  if (server.hasArg("shape")) gShape = server.arg("shape").toInt();
  if (server.hasArg("trail")) gTrail = server.arg("trail").toInt();
  if (server.hasArg("ring")) gOrbitRing = server.arg("ring").toInt();
  if (server.hasArg("freeze")) gFrozen = server.arg("freeze").toInt() != 0;
  if (server.hasArg("spread")) gSpread = server.arg("spread").toInt() / 10.0f;
  if (server.hasArg("rotate")) gFringeAngle = server.arg("rotate").toInt() * 0.0174533f;
  if (server.hasArg("b2r")) gB2r = server.arg("b2r").toInt();
  if (server.hasArg("b2g")) gB2g = server.arg("b2g").toInt();
  if (server.hasArg("b2b")) gB2b = server.arg("b2b").toInt();
  if (server.hasArg("split")) gSplit = constrain(server.arg("split").toInt(), 0, 2);
  if (server.hasArg("step")) {
    if (gMode == MODE_HEX && (gAnim == 1 || gAnim == 2)) hexAnimPos++;
    else if (gMode == MODE_HEX && gAnim == 0 && gSplit)
      gAnchor = spiralOrder[(++anchorStep) % NUMPIXELS]; // walk the static triad
  }
  if (server.hasArg("mode")) {
    int m = server.arg("mode").toInt();
    gMode = (m == 1) ? MODE_RGBW : (m == 2) ? MODE_RGB : MODE_HEX;
    applyMode();
  }
  if (server.hasArg("off")) {
    gAnim = 0;
    strip.clear();
    strip.show();
  }
  if (server.hasArg("anim")) {
    gAnim = server.arg("anim").toInt();
    hexAnimPos = 0;
    hexBreathePhase = 0;
    rgbwPhase = 0;
    strip.clear();
  }
  applyAfterSet();
  server.send(200, "text/plain", "ok");
}

// Battery stats CACHE: live SDK reads in handleState stalled the animation loop
// (charger reads trigger ADC one-shots = tens of ms, x5 fields, every 600 ms poll).
// Instead loop() refreshes ONE field per ~800 ms round-robin (a single short I2C
// transaction per frame at worst) and /state serves the cache instantly.
float gBatV = 0, gBatMa = 0, gSupV = 0, gSupMa = 0;
uint8_t gSoc = 0;
bool gSupGood = false;

void batteryTick() {
  static uint32_t nextMs = 0;
  static uint8_t idx = 0;
  if (!gPfReady || millis() < nextMs) return;
  nextMs = millis() + 800;
  switch (idx++ % 6) {
  case 0: Board.getBatteryVoltage(gBatV); break;
  case 1: Board.getBatteryCurrent(gBatMa); break;
  case 2: Board.getBatteryCharge(gSoc); break;
  case 3: Board.getSupplyVoltage(gSupV); break;
  case 4: Board.getSupplyCurrent(gSupMa); break;
  case 5: Board.checkSupplyGood(gSupGood); break;
  }
}

void solarGuardTick() {
  if (!gPfReady) return;
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
  pfSolarGuardTick("led_studio", sv, sma, good, STUDIO_MAINTAIN_V, true);
}

void handleState() {
  float bv = gBatV, ma = gBatMa, sv = gSupV, sma = gSupMa;
  uint8_t soc = gSoc;
  bool sgood = gSupGood;
  char buf[420];
  snprintf(buf, sizeof(buf),
           "{\"mode\":%u,\"anim\":%u,\"r\":%u,\"g\":%u,\"b\":%u,\"w\":%u,\"bri\":%u,"
           "\"speed\":%u,\"gamma\":%u,\"shape\":%u,\"lit\":%u,\"anchor\":%u,\"split\":%u,"
           "\"pf\":%d,\"bv\":%.3f,\"ma\":%.0f,\"soc\":%u,\"sv\":%.2f,\"sma\":%.0f,\"sgood\":%d}",
           gMode, gAnim, gR, gG, gB, gW, gBri, gSpeed, gGamma ? 1 : 0, gShape,
           lastLit, gAnchor, gSplit, gPfReady ? 1 : 0, bv, ma, soc, sv, sma,
           sgood ? 1 : 0);
  server.send(200, "application/json", buf);
}

void setupWifi() {
#if HAVE_SECRETS
  WiFi.mode(WIFI_AP_STA);
  WiFi.setHostname("ledstudio");
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
    Serial.print("LED Studio STA at http://");
    Serial.println(WiFi.localIP());
    if (apOk) {
      Serial.print("LED Studio AP '" AP_SSID "' -> http://");
      Serial.println(WiFi.softAPIP());
    } else {
      Serial.println("LED Studio AP start failed");
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
  // LFP-safe charger config (see note at the SDK include). Retry a few times like
  // the bench firmwares; fall back to the manual rail-enable if init won't come up.
  Result pf = Result::Failure;
  for (int i = 0; i < 4 && pf != Result::Ok; i++) {
    pf = Board.init((uint16_t)STUDIO_BATTERY_MAH, STUDIO_BATTERY_TYPE);
    if (pf != Result::Ok) delay(250);
  }
  if (pf == Result::Ok) {
    gPfReady = true;
    Board.setSupplyMaintainVoltage(STUDIO_MAINTAIN_V);
    Board.setBatteryChargingMaxCurrent(STUDIO_CHARGE_MA); // gentle USB-friendly charge
    Board.enableBatteryCharging(true);
    pfSolarGuardInit("led_studio", STUDIO_MAINTAIN_V, true);
    Board.enable3V3(true); // LED rail (SDK path)
    Serial.println("PowerFeather SDK Ok: LFP charger on (500 mA, maintain 4.6 V), 3V3 on");
  } else {
    Serial.println("WARNING: Board.init failed -- charger UNCONFIGURED (do NOT attach a cell "
                   "while on USB); enabling 3V3 rail manually");
  }
  pinMode(EN_3V3_PIN, OUTPUT);
  digitalWrite(EN_3V3_PIN, HIGH); // enable the switchable 3V3 header rail (fallback/no-op)
  delay(20);
  buildGeometry();
  strip.begin();
  strip.setBrightness(255);
  strip.clear();
  strip.show();
  setupWifi();
  if (MDNS.begin("ledstudio")) { // http://ledstudio.local/ -- works on STA and the AP
    MDNS.addService("http", "tcp", 80);
    Serial.println("mDNS: http://ledstudio.local/");
  } else {
    Serial.println("mDNS start failed (use the IP)");
  }
  server.on("/", []() { server.send_P(200, "text/html", PAGE); });
  server.on("/set", handleSet);
  server.on("/state", handleState);
  // Standard OTA (same handler as power_bench) so studio tweaks never need a tether:
  //   curl -F "firmware=@led_studio.ino.bin" http://<ip>/update   (or the GET form)
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
  Serial.printf("LED Studio ready (GPIO%d). HEX ring sizes %u/%u/%u/%u\n", DATA_PIN,
                ringSize[0], ringSize[1], ringSize[2], ringSize[3]);
  renderStatic();
}

void loop() {
  server.handleClient();
  batteryTick();
  solarGuardTick();
  if (isAnimating() && millis() - lastFrame >= (uint32_t)(400 - (gSpeed - 1) * (375.0f / 99.0f))) {
    lastFrame = millis();
    renderFrame();
  }
}
