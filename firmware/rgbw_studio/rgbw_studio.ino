// Resonance RGBW Studio -- interactive aesthetic bench tool for the single
// high-power SK6812 RGBW pixel (Adafruit 5163, 4 W), driven direct-GPIO. Serves a
// phone-friendly web UI for exploring color + temporal modulation through the
// gobo. The RGBW is a POINT source (crisp shadows) with a dedicated white die, so
// unlike the HEX there's no geometry -- the interesting axis is color over time.
//
// Sibling of hex_studio (same UI scaffolding). One pixel, NEO_GRBW.
//
// Wiring: RGBW data on DATA_PIN (default GPIO10 / A0). Power from 3V3 (note: the
// 4 W RGBW is voltage-starved at 3.3 V -- dimmer/non-linear, see ADR 0018) or 5 V
// for full output. GND common.
//
// Build/flash (USB): ./build.sh --port /dev/ttyACM1
// Then open the IP it prints over serial (or http://192.168.4.1 in AP fallback).

#include <Arduino.h>
#include <math.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Adafruit_NeoPixel.h>

#ifndef DATA_PIN
#define DATA_PIN 10 // GPIO10 / A0. Override -DDATA_PIN=16 for D6, etc.
#endif
// Single SK6812 RGBW pixel (GRBW order, 800kHz).
Adafruit_NeoPixel strip(1, DATA_PIN, NEO_GRBW + NEO_KHZ800);

#if __has_include("wifi_secrets.h")
#include "wifi_secrets.h"
#define HAVE_SECRETS 1
#else
#define HAVE_SECRETS 0
#endif
#define AP_SSID "ResonanceRGBW"
#define AP_PASS "resonance"

WebServer server(80);

// ---- State -----------------------------------------------------------------
enum Anim { ANIM_NONE = 0, ANIM_HUE, ANIM_BREATHE, ANIM_CANDLE, ANIM_FADE };
uint8_t gR = 255, gG = 140, gB = 40, gW = 0; // default warm amber, W off
uint8_t gBri = 60;
uint8_t gAnim = ANIM_NONE;
uint8_t gSpeed = 30;
bool gGamma = true;
// Color B (for Fade), RGB only
uint8_t gB2r = 0, gB2g = 120, gB2b = 255;

float animPhase = 0;        // generic phase (hue / breathe / fade)
float candleLevel = 1.0f;   // smoothed candle brightness
float candleTarget = 1.0f;
uint32_t lastFrame = 0;

inline uint8_t gam(uint8_t v) { return gGamma ? Adafruit_NeoPixel::gamma8(v) : v; }

// Apply an RGBW color scaled by global brightness * factor (0..1).
void setRGBW(uint8_t r, uint8_t g, uint8_t b, uint8_t w, float f) {
  float s = (float)gBri / 255.0f * f;
  if (s < 0) s = 0;
  if (s > 1) s = 1;
  strip.setPixelColor(0, gam((uint8_t)(r * s)), gam((uint8_t)(g * s)),
                       gam((uint8_t)(b * s)), gam((uint8_t)(w * s)));
  strip.show();
}

void renderStatic() { setRGBW(gR, gG, gB, gW, 1.0f); }

uint32_t frameMs() {
  uint8_t s = gSpeed < 1 ? 1 : gSpeed;
  return (uint32_t)(400 - (s - 1) * (375.0f / 99.0f));
}

void renderFrame() {
  switch (gAnim) {
    case ANIM_HUE: {
      uint16_t hue = (uint16_t)((uint32_t)animPhase & 0xFFFF);
      uint32_t c = strip.ColorHSV(hue, 255, gBri);
      if (gGamma) c = strip.gamma32(c);
      strip.setPixelColor(0, c); // RGB only; W stays 0
      strip.show();
      animPhase += 256; // ~256 frames per full hue cycle
      break;
    }
    case ANIM_BREATHE: {
      float f = 0.5f + 0.5f * sinf(animPhase);
      setRGBW(gR, gG, gB, gW, f);
      animPhase += 0.15f;
      break;
    }
    case ANIM_CANDLE: {
      // smoothed random-walk flicker of the user's chosen (warm) color
      candleLevel += (candleTarget - candleLevel) * 0.25f;
      if (fabsf(candleTarget - candleLevel) < 0.03f) {
        // pick a new target in [0.45, 1.0]; occasional deeper dip
        uint32_t r = esp_random();
        candleTarget = 0.45f + (float)(r & 0xFFFF) / 65535.0f * 0.55f;
        if ((r & 0x7) == 0) candleTarget *= 0.7f; // rare flutter dip
      }
      setRGBW(gR, gG, gB, gW, candleLevel);
      break;
    }
    case ANIM_FADE: {
      float t = 0.5f + 0.5f * sinf(animPhase); // 0..1 triangle-ish
      uint8_t r = gR + (int)((gB2r - gR) * t);
      uint8_t g = gG + (int)((gB2g - gG) * t);
      uint8_t b = gB + (int)((gB2b - gB) * t);
      uint8_t w = gW + (int)((0 - gW) * t); // fade W out toward color B
      setRGBW(r, g, b, w, 1.0f);
      animPhase += 0.06f;
      break;
    }
    default:
      break;
  }
}

// ---- Web UI ----------------------------------------------------------------
const char PAGE[] PROGMEM = R"HTML(<!doctype html><html><head>
<meta name=viewport content="width=device-width,initial-scale=1">
<title>RGBW Studio</title>
<style>
 body{font-family:system-ui,sans-serif;background:#111;color:#eee;margin:0;padding:14px;max-width:520px}
 h2{margin:.2em 0}
 .row{margin:10px 0}
 label{display:block;font-size:13px;color:#aaa;margin-bottom:3px}
 input[type=range]{width:100%;height:30px}
 .btns{display:flex;flex-wrap:wrap;gap:6px}
 button{flex:1 1 auto;min-width:64px;padding:11px 8px;font-size:14px;border:0;border-radius:8px;background:#333;color:#eee}
 button.on{background:#0a7;color:#fff}
 #rb{font-family:monospace;font-size:13px;background:#000;padding:8px;border-radius:6px;white-space:pre-wrap;color:#6f6}
 .sw{display:inline-block;width:22px;height:22px;border-radius:5px;vertical-align:middle;border:1px solid #555}
</style></head><body>
<h2>RGBW Studio</h2>

<div class=row><label>Color (RGB) <span id=sw class=sw></span></label>
 <input type=color id=col value="#ff8c28" oninput="setCol(this.value)"></div>
<div class=row><label>R <span id=rl></span></label><input type=range id=r min=0 max=255 value=255 oninput="ch('r',this.value)"></div>
<div class=row><label>G <span id=gl></span></label><input type=range id=g min=0 max=255 value=140 oninput="ch('g',this.value)"></div>
<div class=row><label>B <span id=bl></span></label><input type=range id=b min=0 max=255 value=40 oninput="ch('b',this.value)"></div>
<div class=row><label>W (white die) <span id=wl></span></label><input type=range id=w min=0 max=255 value=0 oninput="ch('w',this.value)"></div>
<div class=row><label>Brightness <span id=bril></span></label><input type=range id=bri min=0 max=255 value=60 oninput="ch('bri',this.value)"></div>

<div class=row><label>White / warmth presets</label><div class=btns>
 <button onclick="preset('wonly')">W only</button>
 <button onclick="preset('rgbw')">RGB white</button>
 <button onclick="preset('full')">RGBW full</button>
 <button onclick="preset('candle')">Warm amber</button>
</div></div>
<div class=row><label>Warmth crossfade (RGB white &harr; W) <span id=warl></span></label>
 <input type=range id=war min=0 max=100 value=0 oninput="warmth(this.value)"></div>

<div class=row><label>Animation</label><div class=btns>
 <button id=an0 onclick="anim(0)">Static</button>
 <button id=an1 onclick="anim(1)">Hue cycle</button>
 <button id=an2 onclick="anim(2)">Breathe</button>
 <button id=an3 onclick="anim(3)">Candle</button>
 <button id=an4 onclick="anim(4)">Fade</button>
</div></div>
<div class=row><label>Speed <span id=spl></span></label><input type=range id=sp min=1 max=100 value=30 oninput="ch('speed',this.value)"></div>
<div class=row><label>Color B (for Fade)</label>
 <input type=color id=colb value="#0078ff" oninput="setColB(this.value)"></div>

<div class=row><div class=btns>
 <button id=gam onclick="toggleGamma()">Gamma: on</button>
 <button onclick="send('off=1')">All off</button>
</div></div>

<div class=row><label>Current settings (read off when it looks good)</label>
 <div id=rb>...</div></div>

<script>
let st={r:255,g:140,b:40,w:0,bri:60,anim:0,speed:30,gamma:1,b2r:0,b2g:120,b2b:255};
function send(q){fetch('/set?'+q);}
function ch(k,v){v=+v;st[k]=v;send(k+'='+v);syncLabels();}
function anim(n){st.anim=n;send('anim='+n);hl('an',n,5);}
function hx(v){return ('0'+(+v).toString(16)).slice(-2);}
function setCol(hex){let r=parseInt(hex.substr(1,2),16),g=parseInt(hex.substr(3,2),16),b=parseInt(hex.substr(5,2),16);
 st.r=r;st.g=g;st.b=b;document.getElementById('r').value=r;document.getElementById('g').value=g;document.getElementById('b').value=b;
 send('r='+r+'&g='+g+'&b='+b);syncLabels();}
function setColB(hex){st.b2r=parseInt(hex.substr(1,2),16);st.b2g=parseInt(hex.substr(3,2),16);st.b2b=parseInt(hex.substr(5,2),16);
 send('b2r='+st.b2r+'&b2g='+st.b2g+'&b2b='+st.b2b);}
function setRGBW(r,g,b,w){st.r=r;st.g=g;st.b=b;st.w=w;
 for(const k of ['r','g','b','w'])document.getElementById(k).value=st[k];
 send('r='+r+'&g='+g+'&b='+b+'&w='+w);syncLabels();}
function preset(p){if(p=='wonly')setRGBW(0,0,0,255);
 else if(p=='rgbw')setRGBW(255,255,255,0);
 else if(p=='full')setRGBW(255,255,255,255);
 else if(p=='candle')setRGBW(255,120,25,40);}
function warmth(v){let f=v/100;document.getElementById('warl').textContent=v+'%';
 let c=Math.round(255*(1-f)),wv=Math.round(255*f);setRGBW(c,c,c,wv);}
function toggleGamma(){st.gamma^=1;send('gamma='+st.gamma);let e=document.getElementById('gam');e.textContent='Gamma: '+(st.gamma?'on':'off');e.className=st.gamma?'on':'';}
function hl(p,n,cnt){for(let i=0;i<cnt;i++)document.getElementById(p+i).className=(i==n?'on':'');}
function syncLabels(){rl.textContent=st.r;gl.textContent=st.g;bl.textContent=st.b;wl.textContent=st.w;bril.textContent=st.bri;spl.textContent=st.speed;
 let c='#'+hx(st.r)+hx(st.g)+hx(st.b);sw.style.background=c;col.value=c;}
function refresh(){fetch('/state').then(r=>r.json()).then(s=>{
 rb.textContent='rgbw='+s.r+','+s.g+','+s.b+','+s.w+'  rgb=#'+hx(s.r)+hx(s.g)+hx(s.b)+
  '\nbri='+s.bri+'  gamma='+(s.gamma?'on':'off')+
  '\nanim='+['static','hue','breathe','candle','fade'][s.anim]+'  speed='+s.speed+
  '\ncolorB=#'+hx(s.b2r)+hx(s.b2g)+hx(s.b2b);});}
hl('an',0,5);syncLabels();setInterval(refresh,600);refresh();
</script></body></html>)HTML";

void handleSet() {
  if (server.hasArg("r")) gR = server.arg("r").toInt();
  if (server.hasArg("g")) gG = server.arg("g").toInt();
  if (server.hasArg("b")) gB = server.arg("b").toInt();
  if (server.hasArg("w")) gW = server.arg("w").toInt();
  if (server.hasArg("bri")) gBri = server.arg("bri").toInt();
  if (server.hasArg("speed")) gSpeed = server.arg("speed").toInt();
  if (server.hasArg("gamma")) gGamma = server.arg("gamma").toInt() != 0;
  if (server.hasArg("b2r")) gB2r = server.arg("b2r").toInt();
  if (server.hasArg("b2g")) gB2g = server.arg("b2g").toInt();
  if (server.hasArg("b2b")) gB2b = server.arg("b2b").toInt();
  if (server.hasArg("off")) {
    gAnim = ANIM_NONE;
    strip.clear();
    strip.show();
  }
  if (server.hasArg("anim")) {
    gAnim = server.arg("anim").toInt();
    animPhase = 0;
  }
  if (gAnim == ANIM_NONE) renderStatic();
  server.send(200, "text/plain", "ok");
}

void handleState() {
  char buf[200];
  snprintf(buf, sizeof(buf),
           "{\"r\":%u,\"g\":%u,\"b\":%u,\"w\":%u,\"bri\":%u,\"anim\":%u,"
           "\"speed\":%u,\"gamma\":%u,\"b2r\":%u,\"b2g\":%u,\"b2b\":%u}",
           gR, gG, gB, gW, gBri, gAnim, gSpeed, gGamma ? 1 : 0, gB2r, gB2g,
           gB2b);
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
    Serial.print("RGBW Studio at http://");
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
  strip.begin();
  strip.setBrightness(255); // we scale manually for fine low-end control
  strip.clear();
  strip.show();
  setupWifi();
  server.on("/", []() { server.send_P(200, "text/html", PAGE); });
  server.on("/set", handleSet);
  server.on("/state", handleState);
  server.begin();
  Serial.printf("RGBW Studio ready on GPIO%d\n", DATA_PIN);
  renderStatic();
}

void loop() {
  server.handleClient();
  if (gAnim != ANIM_NONE && millis() - lastFrame >= frameMs()) {
    lastFrame = millis();
    renderFrame();
  }
}
