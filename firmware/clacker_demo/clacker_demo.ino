// WiFi dashboard for relay clacker and 8002A speaker bench tests.
// Target board: Adafruit Metro ESP32-S3.

#include <Arduino.h>
#include <ESPmDNS.h>
#include <WebServer.h>
#include <WiFi.h>

#if __has_include("wifi_secrets.h")
#include "wifi_secrets.h"
#endif

#ifndef RES_WIFI_SSID
#define RES_WIFI_SSID "BubbyNet"
#endif

#ifndef RES_WIFI_PASSWORD
#define RES_WIFI_PASSWORD ""
#endif

constexpr uint8_t RELAY_A_PIN = A0;     // Metro A0, GPIO14.
constexpr uint8_t RELAY_B_PIN = A1;     // Metro A1, GPIO15.
constexpr uint8_t SPEAKER_PIN = 5;      // Metro D5, GPIO5.
constexpr uint8_t RELAY_ACTIVE = HIGH;  // Assumes high-trigger relay modules.
constexpr uint8_t RELAY_IDLE = LOW;

constexpr uint16_t MIN_PULSE_MS = 20;
constexpr uint16_t MAX_PULSE_MS = 500;
constexpr uint16_t MIN_CLACK_INTERVAL_MS = 80;
constexpr uint16_t MAX_CLACK_INTERVAL_MS = 4000;
constexpr uint32_t WIFI_RECONNECT_MS = 10000;

struct RelayState {
  uint8_t pin;
  const char *label;
  bool on;
  uint32_t pulseUntilMs;
};

struct Note {
  uint16_t freq;
  uint16_t durationMs;
};

struct SweepState {
  bool active;
  uint16_t startFreq;
  uint16_t endFreq;
  uint16_t durationMs;
  uint16_t stepMs;
  uint32_t startedMs;
  uint32_t nextStepMs;
  const char *name;
};

RelayState relayA{RELAY_A_PIN, "A0", false, 0};
RelayState relayB{RELAY_B_PIN, "A1", false, 0};

bool autoClack = false;
bool nextClackIsA = true;
uint16_t relayPulseMs = 70;
uint16_t clackIntervalMs = 420;
uint32_t nextClackMs = 0;

const Note tuneScale[] = {
    {262, 170}, {294, 170}, {330, 170}, {349, 170}, {392, 170},
    {440, 170}, {494, 170}, {523, 260}, {0, 160},   {523, 170},
    {494, 170}, {440, 170}, {392, 170}, {349, 170}, {330, 170},
    {294, 170}, {262, 300},
};

const Note tuneBeacon[] = {
    {880, 90}, {0, 50}, {1175, 90}, {0, 160}, {880, 90}, {0, 50},
    {1175, 90}, {0, 420},
};

const Note tuneChime[] = {
    {659, 120}, {0, 35},  {784, 140}, {0, 35},  {988, 180},
    {0, 180},   {784, 80}, {880, 120}, {0, 45},  {1175, 260},
};

const Note tuneSweepUp[] = {
    {120, 120},  {151, 120},  {190, 120},  {239, 120},  {301, 120},
    {379, 120},  {477, 120},  {601, 120},  {757, 120},  {953, 120},
    {1200, 120}, {1511, 120}, {1903, 120}, {2397, 160},
};

const Note tuneSweepDown[] = {
    {2397, 120}, {1903, 120}, {1511, 120}, {1200, 120}, {953, 120},
    {757, 120},  {601, 120},  {477, 120},  {379, 120},  {301, 120},
    {239, 120},  {190, 120},  {151, 120},  {120, 160},
};

const Note tuneLaser[] = {
    {180, 55},  {240, 55},  {320, 55},  {427, 55},  {569, 55},
    {759, 55},  {1012, 55}, {1350, 55}, {1800, 55}, {2400, 55},
    {3200, 90}, {0, 45},    {3200, 55}, {1800, 55}, {900, 90},
};

// Monophonic reduction of the famous opening triplet texture. The MIDI source uses
// 120 ticks/quarter and starts around 50 BPM, so each arpeggio note is about 400 ms.
// When the high melody enters, a one-voice buzzer cannot hold that note over the
// triplets, so the first G#4 entrance is exaggerated as separate long hits.
const Note tuneMoonlight[] = {
    {208, 400}, {277, 400}, {330, 400},
    {208, 400}, {277, 400}, {330, 400},
    {208, 400}, {277, 400}, {330, 400},
    {208, 400}, {277, 400}, {330, 400},
    {208, 400}, {277, 400}, {330, 400},
    {208, 400}, {277, 400}, {330, 400},
    {208, 400}, {277, 400}, {330, 400},
    {208, 400}, {277, 400}, {330, 400},
    {220, 400}, {277, 400}, {330, 400},
    {220, 400}, {277, 400}, {330, 400},
    {220, 400}, {294, 400}, {370, 400},
    {220, 400}, {294, 400}, {370, 400},
    {208, 400}, {262, 400}, {370, 400},
    {208, 400}, {277, 400}, {330, 400},
    {208, 400}, {277, 400}, {311, 400},
    {185, 400}, {262, 400}, {311, 400},
    {208, 400}, {277, 400}, {330, 400},
    {208, 400}, {277, 400}, {330, 400},
    {415, 760}, {0, 80}, {415, 220}, {415, 1120},
    {208, 400}, {311, 400}, {370, 400},
    {208, 400}, {311, 400}, {370, 400},
    {415, 760}, {0, 80}, {415, 220}, {415, 1280},
};

const Note *activeTune = nullptr;
size_t activeTuneLen = 0;
size_t activeTuneIndex = 0;
uint32_t nextNoteMs = 0;
uint32_t noteOffMs = 0;
uint32_t manualToneOffMs = 0;
const char *activeTuneName = "none";
SweepState activeSweep{false, 0, 0, 0, 0, 0, 0, "none"};
bool speakerAttached = false;

WebServer server(80);
uint32_t lastWifiReconnectMs = 0;

const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!doctype html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Lantern noise bench</title>
<style>
:root{color-scheme:dark;--bg:#171a1f;--panel:#222832;--panel2:#29303a;--text:#eef2f6;--muted:#aab4c0;--line:#3a4654;--green:#36c278;--amber:#e5a93d;--red:#ee6b5d;--blue:#63a6f4}
*{box-sizing:border-box}body{margin:0;background:var(--bg);color:var(--text);font:16px/1.4 system-ui,-apple-system,Segoe UI,sans-serif}
main{max-width:860px;margin:0 auto;padding:18px}header{display:flex;align-items:flex-end;justify-content:space-between;gap:12px;margin-bottom:14px}
h1{font-size:24px;line-height:1.1;margin:0}#status{color:var(--muted);font-size:13px;text-align:right}
section{background:var(--panel);border:1px solid var(--line);border-radius:8px;padding:14px;margin:12px 0}
h2{font-size:16px;margin:0 0 12px}.row{display:flex;gap:10px;flex-wrap:wrap;align-items:center}.grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(180px,1fr));gap:10px}
button{min-height:42px;border:1px solid var(--line);border-radius:7px;background:var(--panel2);color:var(--text);font:inherit;font-weight:650;padding:9px 13px;cursor:pointer}
button:hover{border-color:var(--blue)}button.primary{background:#21472f;border-color:#2d8051}button.warn{background:#4a3520;border-color:#8d641e}button.danger{background:#4d2927;border-color:#924039}
label{display:grid;gap:6px;color:var(--muted);font-size:13px}input[type=range]{width:100%;accent-color:var(--blue)}output{color:var(--text);font-variant-numeric:tabular-nums}
.pill{display:inline-flex;align-items:center;gap:6px;border:1px solid var(--line);background:var(--panel2);border-radius:999px;padding:5px 9px;color:var(--muted);font-size:13px}
.on{color:var(--green)}.off{color:var(--muted)}.split{display:grid;grid-template-columns:1fr 1fr;gap:10px}@media(max-width:620px){header{display:block}#status{text-align:left;margin-top:8px}.split{grid-template-columns:1fr}}
</style>
</head>
<body>
<main>
<header>
  <h1>Lantern noise bench</h1>
  <div id="status">connecting...</div>
</header>

<section>
  <h2>Relays</h2>
  <div class="grid">
    <button class="primary" onclick="pulse('a')">A0 click</button>
    <button class="primary" onclick="pulse('b')">A1 click</button>
    <button onclick="pulse('both')">Both click</button>
    <button class="danger" onclick="allOff()">All off</button>
  </div>
  <div class="row" style="margin-top:12px">
    <span class="pill">A0 <span id="relayA" class="off">off</span></span>
    <span class="pill">A1 <span id="relayB" class="off">off</span></span>
    <span class="pill">auto <span id="autoState" class="off">off</span></span>
  </div>
</section>

<section>
  <h2>Auto clack</h2>
  <div class="split">
    <label>Gap between clicks <input id="interval" type="range" min="80" max="2200" step="20" value="420" oninput="settingsChanged()" onchange="pushSettingsNow()"><output id="intervalOut">420 ms</output></label>
    <label>Relay pulse width <input id="pulseMs" type="range" min="20" max="250" step="5" value="70" oninput="settingsChanged()" onchange="pushSettingsNow()"><output id="pulseOut">70 ms</output></label>
  </div>
  <div class="row" style="margin-top:12px">
    <button class="warn" onclick="setClack(true)">Start A/B</button>
    <button onclick="setClack(false)">Stop</button>
  </div>
</section>

<section>
  <h2>Speaker</h2>
  <div class="grid">
    <button class="primary" onclick="beep(440,180)">440 Hz</button>
    <button class="primary" onclick="beep(880,160)">880 Hz</button>
    <button onclick="tune('scale')">Scale</button>
    <button onclick="tune('beacon')">Beacon</button>
    <button onclick="tune('chime')">Chime</button>
    <button onclick="tune('moonlight')">Moonlight</button>
    <button onclick="sweep('up')">Sweep up</button>
    <button onclick="sweep('down')">Sweep down</button>
    <button onclick="sweep('laser')">Laser sweep</button>
    <button class="danger" onclick="tune('stop')">Mute</button>
  </div>
  <div class="row" style="margin-top:12px">
    <span class="pill">pin <span>D5 / GPIO5</span></span>
    <span class="pill">tune <span id="tuneName">none</span></span>
  </div>
</section>
</main>

<script>
const $=id=>document.getElementById(id);
let settingsTimer=0;
let slidersDirtyUntil=0;
function syncLabels(){
  $('intervalOut').textContent=$('interval').value+' ms';
  $('pulseOut').textContent=$('pulseMs').value+' ms';
}
function settingsChanged(){
  syncLabels();
  slidersDirtyUntil=Date.now()+1500;
  clearTimeout(settingsTimer);
  settingsTimer=setTimeout(pushSettingsNow,150);
}
async function pushSettingsNow(){
  clearTimeout(settingsTimer);
  slidersDirtyUntil=Date.now()+1500;
  try{
    const path=`/api/settings?interval=${$('interval').value}&pulse=${$('pulseMs').value}`;
    const r=await fetch(path,{cache:'no-store'});
    if(!r.ok) throw new Error(await r.text());
    applyState(await r.json(),false);
    slidersDirtyUntil=Date.now()+300;
  }catch(e){$('status').textContent='settings save failed'}
}
async function api(path){
  const r=await fetch(path,{cache:'no-store'});
  if(!r.ok) throw new Error(await r.text());
  await refresh();
}
function pulse(which){api(`/api/pulse?which=${which}&ms=${$('pulseMs').value}`)}
function allOff(){api('/api/alloff')}
function setClack(on){api(`/api/clack?on=${on?1:0}&interval=${$('interval').value}&pulse=${$('pulseMs').value}`)}
function beep(freq,ms){api(`/api/beep?freq=${freq}&ms=${ms}`)}
function tune(id){api(`/api/tune?id=${id}`)}
function sweep(id){api(`/api/sweep?id=${id}`)}
function setFlag(id,on){const e=$(id);e.textContent=on?'on':'off';e.className=on?'on':'off'}
function applyState(s,allowSliderUpdate=true){
  setFlag('relayA',s.relay_a);
  setFlag('relayB',s.relay_b);
  setFlag('autoState',s.auto_clack);
  if(allowSliderUpdate && Date.now()>slidersDirtyUntil){
    $('interval').value=s.clack_interval_ms;
    $('pulseMs').value=s.relay_pulse_ms;
    syncLabels();
  }
  $('tuneName').textContent=s.tune;
  $('status').textContent=`${s.ip}  RSSI ${s.rssi_dbm} dBm`;
}
async function refresh(){
  try{
    const s=await (await fetch('/api/state',{cache:'no-store'})).json();
    applyState(s);
  }catch(e){$('status').textContent='offline'}
}
syncLabels();
refresh();
setInterval(refresh,1000);
</script>
</body>
</html>
)rawliteral";

uint16_t clampU16(uint16_t value, uint16_t lo, uint16_t hi) {
  if (value < lo) {
    return lo;
  }
  if (value > hi) {
    return hi;
  }
  return value;
}

void setRelay(RelayState &relay, bool on) {
  relay.on = on;
  digitalWrite(relay.pin, on ? RELAY_ACTIVE : RELAY_IDLE);
}

void allRelaysOff() {
  relayA.pulseUntilMs = 0;
  relayB.pulseUntilMs = 0;
  setRelay(relayA, false);
  setRelay(relayB, false);
}

void pulseRelay(RelayState &relay, uint16_t durationMs) {
  durationMs = clampU16(durationMs, MIN_PULSE_MS, MAX_PULSE_MS);
  setRelay(relay, true);
  relay.pulseUntilMs = millis() + durationMs;
}

void serviceRelayPulse(RelayState &relay, uint32_t nowMs) {
  if (relay.pulseUntilMs != 0 && static_cast<int32_t>(nowMs - relay.pulseUntilMs) >= 0) {
    relay.pulseUntilMs = 0;
    setRelay(relay, false);
  }
}

void serviceAutoClack(uint32_t nowMs) {
  if (!autoClack || static_cast<int32_t>(nowMs - nextClackMs) < 0) {
    return;
  }

  pulseRelay(nextClackIsA ? relayA : relayB, relayPulseMs);
  nextClackIsA = !nextClackIsA;
  nextClackMs = nowMs + clackIntervalMs;
}

void stopTune() {
  activeTune = nullptr;
  activeTuneLen = 0;
  activeTuneIndex = 0;
  nextNoteMs = 0;
  noteOffMs = 0;
  manualToneOffMs = 0;
  activeSweep.active = false;
  activeTuneName = "none";
  if (speakerAttached) {
    ledcWriteTone(SPEAKER_PIN, 0);
    ledcDetach(SPEAKER_PIN);
    speakerAttached = false;
  }
  digitalWrite(SPEAKER_PIN, LOW);
}

void speakerTone(uint16_t freq) {
  freq = clampU16(freq, 60, 6000);
  if (!speakerAttached) {
    speakerAttached = ledcAttach(SPEAKER_PIN, freq, 10);
  }
  if (speakerAttached) {
    ledcWriteTone(SPEAKER_PIN, freq);
  }
}

void speakerQuiet() {
  if (speakerAttached) {
    ledcWriteTone(SPEAKER_PIN, 0);
  }
  digitalWrite(SPEAKER_PIN, LOW);
}

void startTune(const char *name, const Note *notes, size_t len) {
  activeSweep.active = false;
  speakerQuiet();
  activeTune = notes;
  activeTuneLen = len;
  activeTuneIndex = 0;
  nextNoteMs = 0;
  noteOffMs = 0;
  manualToneOffMs = 0;
  activeTuneName = name;
}

void startSweep(const char *name, uint16_t startFreq, uint16_t endFreq, uint16_t durationMs) {
  stopTune();
  activeSweep.active = true;
  activeSweep.startFreq = clampU16(startFreq, 60, 6000);
  activeSweep.endFreq = clampU16(endFreq, 60, 6000);
  activeSweep.durationMs = clampU16(durationMs, 100, 8000);
  activeSweep.stepMs = 25;
  activeSweep.startedMs = millis();
  activeSweep.nextStepMs = 0;
  activeSweep.name = name;
  activeTuneName = name;
}

void playManualTone(uint16_t freq, uint16_t durationMs) {
  stopTune();
  freq = clampU16(freq, 60, 6000);
  durationMs = clampU16(durationMs, 20, 2000);
  speakerTone(freq);
  manualToneOffMs = millis() + durationMs + 10;
  activeTuneName = "beep";
}

void serviceSpeaker(uint32_t nowMs) {
  if (noteOffMs != 0 && static_cast<int32_t>(nowMs - noteOffMs) >= 0) {
    noteOffMs = 0;
    speakerQuiet();
  }

  if (activeSweep.active) {
    const uint32_t elapsedMs = nowMs - activeSweep.startedMs;
    if (elapsedMs >= activeSweep.durationMs) {
      stopTune();
      return;
    }
    if (static_cast<int32_t>(nowMs - activeSweep.nextStepMs) >= 0) {
      const int32_t range = static_cast<int32_t>(activeSweep.endFreq) -
                            static_cast<int32_t>(activeSweep.startFreq);
      const int32_t freq = static_cast<int32_t>(activeSweep.startFreq) +
                           (range * static_cast<int32_t>(elapsedMs)) /
                               static_cast<int32_t>(activeSweep.durationMs);
      speakerTone(clampU16(static_cast<uint16_t>(freq), 60, 6000));
      activeSweep.nextStepMs = nowMs + activeSweep.stepMs;
    }
    return;
  }

  if (activeTune == nullptr) {
    if (manualToneOffMs != 0 && static_cast<int32_t>(nowMs - manualToneOffMs) >= 0) {
      manualToneOffMs = 0;
      activeTuneName = "none";
      stopTune();
    }
    return;
  }

  if (static_cast<int32_t>(nowMs - nextNoteMs) < 0) {
    return;
  }

  if (activeTuneIndex >= activeTuneLen) {
    stopTune();
    return;
  }

  const Note note = activeTune[activeTuneIndex++];
  if (note.freq == 0) {
    speakerQuiet();
    noteOffMs = 0;
  } else {
    const uint16_t toneMs = note.durationMs > 20 ? note.durationMs - 20 : note.durationMs;
    speakerTone(note.freq);
    noteOffMs = nowMs + toneMs;
  }
  nextNoteMs = nowMs + note.durationMs;
}

String jsonState() {
  String out;
  out.reserve(220);
  out += "{";
  out += "\"relay_a\":";
  out += relayA.on ? "true" : "false";
  out += ",\"relay_b\":";
  out += relayB.on ? "true" : "false";
  out += ",\"auto_clack\":";
  out += autoClack ? "true" : "false";
  out += ",\"relay_pulse_ms\":";
  out += relayPulseMs;
  out += ",\"clack_interval_ms\":";
  out += clackIntervalMs;
  out += ",\"tune\":\"";
  out += activeTuneName;
  out += "\",\"ip\":\"";
  out += WiFi.localIP().toString();
  out += "\",\"rssi_dbm\":";
  out += WiFi.isConnected() ? WiFi.RSSI() : 0;
  out += "}";
  return out;
}

uint16_t argU16(const char *name, uint16_t fallback, uint16_t lo, uint16_t hi) {
  if (!server.hasArg(name)) {
    return fallback;
  }
  const long value = server.arg(name).toInt();
  if (value <= 0) {
    return fallback;
  }
  return clampU16(static_cast<uint16_t>(value), lo, hi);
}

void sendNoStore() {
  server.sendHeader("Cache-Control", "no-store, max-age=0");
}

void sendState() {
  sendNoStore();
  server.send(200, "application/json", jsonState());
}

void handlePulse() {
  const String which = server.arg("which");
  const uint16_t durationMs = argU16("ms", relayPulseMs, MIN_PULSE_MS, MAX_PULSE_MS);
  relayPulseMs = durationMs;

  if (which == "a") {
    pulseRelay(relayA, durationMs);
  } else if (which == "b") {
    pulseRelay(relayB, durationMs);
  } else if (which == "both") {
    pulseRelay(relayA, durationMs);
    pulseRelay(relayB, durationMs);
  } else {
    server.send(400, "text/plain", "which must be a, b, or both");
    return;
  }
  sendState();
}

void handleAllOff() {
  autoClack = false;
  allRelaysOff();
  stopTune();
  sendState();
}

void handleSettings() {
  relayPulseMs = argU16("pulse", relayPulseMs, MIN_PULSE_MS, MAX_PULSE_MS);
  clackIntervalMs = argU16("interval", clackIntervalMs, MIN_CLACK_INTERVAL_MS, MAX_CLACK_INTERVAL_MS);
  sendState();
}

void handleClack() {
  relayPulseMs = argU16("pulse", relayPulseMs, MIN_PULSE_MS, MAX_PULSE_MS);
  clackIntervalMs = argU16("interval", clackIntervalMs, MIN_CLACK_INTERVAL_MS, MAX_CLACK_INTERVAL_MS);
  if (server.hasArg("on")) {
    autoClack = server.arg("on").toInt() != 0;
  } else {
    autoClack = !autoClack;
  }
  nextClackIsA = true;
  nextClackMs = millis();
  sendState();
}

void handleBeep() {
  const uint16_t freq = argU16("freq", 880, 60, 6000);
  const uint16_t durationMs = argU16("ms", 160, 20, 2000);
  playManualTone(freq, durationMs);
  sendState();
}

void handleTune() {
  const String id = server.arg("id");
  if (id == "scale") {
    startTune("scale", tuneScale, sizeof(tuneScale) / sizeof(tuneScale[0]));
  } else if (id == "beacon") {
    startTune("beacon", tuneBeacon, sizeof(tuneBeacon) / sizeof(tuneBeacon[0]));
  } else if (id == "chime") {
    startTune("chime", tuneChime, sizeof(tuneChime) / sizeof(tuneChime[0]));
  } else if (id == "moonlight") {
    startTune("moonlight", tuneMoonlight, sizeof(tuneMoonlight) / sizeof(tuneMoonlight[0]));
  } else if (id == "stop") {
    stopTune();
  } else {
    server.send(400, "text/plain", "id must be scale, beacon, chime, moonlight, or stop");
    return;
  }
  sendState();
}

void handleSweep() {
  const String id = server.arg("id");
  if (id == "up") {
    startTune("sweep up", tuneSweepUp, sizeof(tuneSweepUp) / sizeof(tuneSweepUp[0]));
  } else if (id == "down") {
    startTune("sweep down", tuneSweepDown, sizeof(tuneSweepDown) / sizeof(tuneSweepDown[0]));
  } else if (id == "laser") {
    startTune("laser", tuneLaser, sizeof(tuneLaser) / sizeof(tuneLaser[0]));
  } else {
    server.send(400, "text/plain", "id must be up, down, or laser");
    return;
  }
  sendState();
}

void handleNotFound() {
  server.send(404, "text/plain", "not found");
}

void startServer() {
  server.on("/", HTTP_GET, []() {
    sendNoStore();
    server.send_P(200, "text/html", INDEX_HTML);
  });
  server.on("/api/state", HTTP_GET, sendState);
  server.on("/api/pulse", HTTP_GET, handlePulse);
  server.on("/api/alloff", HTTP_GET, handleAllOff);
  server.on("/api/settings", HTTP_GET, handleSettings);
  server.on("/api/clack", HTTP_GET, handleClack);
  server.on("/api/beep", HTTP_GET, handleBeep);
  server.on("/api/tune", HTTP_GET, handleTune);
  server.on("/api/sweep", HTTP_GET, handleSweep);
  server.onNotFound(handleNotFound);
  server.begin();
}

void connectWifi() {
  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);
  WiFi.begin(RES_WIFI_SSID, RES_WIFI_PASSWORD);

  Serial.print("WiFi: connecting to ");
  Serial.println(RES_WIFI_SSID);
  const uint32_t startMs = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startMs < 20000) {
    delay(250);
    Serial.print(".");
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("WiFi: connected, IP ");
    Serial.println(WiFi.localIP());
    if (MDNS.begin("clacker")) {
      MDNS.addService("http", "tcp", 80);
      Serial.println("mDNS: http://clacker.local/");
    }
  } else {
    Serial.println("WiFi: not connected yet; dashboard will appear after reconnect");
  }
}

void setup() {
  Serial.begin(115200);
  delay(1500);

  pinMode(relayA.pin, OUTPUT);
  pinMode(relayB.pin, OUTPUT);
  pinMode(SPEAKER_PIN, OUTPUT);
  allRelaysOff();
  stopTune();

  Serial.println();
  Serial.println("clacker_demo dashboard");
  Serial.println("Relays: A0/GPIO14 and A1/GPIO15, high-trigger");
  Serial.println("Speaker signal: Metro D5/GPIO5");

  connectWifi();
  startServer();
}

void loop() {
  const uint32_t nowMs = millis();

  server.handleClient();
  serviceRelayPulse(relayA, nowMs);
  serviceRelayPulse(relayB, nowMs);
  serviceAutoClack(nowMs);
  serviceSpeaker(nowMs);

  if (WiFi.status() != WL_CONNECTED && nowMs - lastWifiReconnectMs >= WIFI_RECONNECT_MS) {
    lastWifiReconnectMs = nowMs;
    WiFi.reconnect();
  }
}
