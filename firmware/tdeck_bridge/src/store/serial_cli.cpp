#include "serial_cli.h"

#include <Arduino.h>
#include <string.h>

#include <ctype.h>

#include "../hal/hal_board.h"
#include "../hal/hal_display.h"
#include "../net/census_svc.h"
#include "../net/claude_client.h"
#include "../net/espnow_link.h"
#include "../net/mesh_tx.h"
#include "../net/nb_emit.h"
#include "../net/net_mgr.h"
#include "fixture/src/core/fixture_context.h"
#include "store.h"

static void printHelp() {
  Serial.println(
      "commands:\n"
      "  show                     settings (api key redacted)\n"
      "  set wifi <ssid> <psk>    store credentials (ssid must be one token)\n"
      "  set key <api-key>        store Anthropic API key\n"
      "  set model <model-id>     default claude-sonnet-5\n"
      "  set channel <1-13>       mesh channel (fleet = 11)\n"
      "  set display day|night    display mode (reboot applies theme)\n"
      "  set bl <0-255>           night-mode backlight level\n"
      "  wifi retry|off           re-attempt association / mesh-only\n"
      "  peers                    census table\n"
      "  emit on|off              1 Hz nb-master/nb-peer dashboard lines\n"
      "quick commands (mesh, WAN-down safe):\n"
      "  i<ID>[:secs]             identify one fixture (blink), e.g. i9E5AB8:10\n"
      "  I                        identify ALL for 8 s\n"
      "  A<ID>[:secs]             exact-target CA lease; 0 releases (default 180 s)\n"
      "  K<ID>:<ms>               solenoid strike, 5-300 ms (never broadcast)\n"
      "  U<ID>                    exact-target OTA maintenance for 35 s\n"
      "  F<ID>:<0|1>:<0|1>        exact profile commission/field + persist bit\n"
      "  uB<job8>:<seconds>        begin/replace an OTA gather roster\n"
      "  uA<job8>:<ID>             add one exact target to that roster\n"
      "  uF<job8>                  freeze roster before any upload\n"
      "  uS<job8>                  print structured roster status\n"
      "  B[secs]                  fleet dark lease (default 600 s)\n"
      "  b                        release fleet lease (back to autonomous)\n"
      "  t                        one-line JSON telemetry\n"
      "misc: probe | mem | reboot | help");
}

static bool parseHexId(const char *s, uint8_t out[3]) {
  for (int i = 0; i < 6; ++i) {
    char ch = s[i];
    if (!isxdigit((unsigned char)ch)) return false;
  }
  for (int i = 0; i < 3; ++i) {
    char b[3] = {s[i * 2], s[i * 2 + 1], 0};
    out[i] = (uint8_t)strtoul(b, nullptr, 16);
  }
  return true;
}

static bool parseHex32(const char *s, uint32_t &out) {
  char copy[9] = {};
  for (int i = 0; i < 8; ++i) {
    if (!isxdigit((unsigned char)s[i])) return false;
    copy[i] = s[i];
  }
  out = (uint32_t)strtoul(copy, nullptr, 16);
  return out != 0;
}

static bool parseDecimalRange(const char *s, int low, int high, int &out) {
  if (!s || !*s) return false;
  char *end = nullptr;
  long value = strtol(s, &end, 10);
  if (!end || *end != 0 || value < low || value > high) return false;
  out = (int)value;
  return true;
}

// Single-token mesh quick commands (bench-alphabet compatible shapes carried
// as lines). Returns true if the token was one of them.
static bool handleQuickCommand(const char *tok) {
  static const uint8_t kAll[3] = {0, 0, 0};
  if (tok[0] == 'I' && tok[1] == 0) {
    meshIdentify(kAll, 8);
    Serial.println("identify ALL 8s");
    return true;
  }
  if (tok[0] == 'i' && tok[1] != 0) {
    uint8_t id[3];
    if (!parseHexId(tok + 1, id)) { Serial.println("i<6-hex-ID>[:secs]"); return true; }
    int secs = 8;
    const char *colon = strchr(tok + 7, ':');
    if (colon) secs = atoi(colon + 1);
    if (secs < 1) secs = 1;
    if (secs > 255) secs = 255;
    meshIdentify(id, (uint8_t)secs);
    Serial.printf("identify %.6s %ds\n", tok + 1, secs);
    return true;
  }
  if (tok[0] == 'A' && tok[1] != 0) {
    uint8_t id[3] = {};
    size_t len = strlen(tok);
    const char *colon = len > 7 && tok[7] == ':' ? tok + 7 : nullptr;
    if ((len != 7 && !colon) || !parseHexId(tok + 1, id) ||
        memcmp(id, kAll, sizeof(id)) == 0) {
      Serial.println("A<6-hex-ID>[:0-900-secs] (exact target required)");
      return true;
    }
    int secs = 180;
    if (colon && !parseDecimalRange(colon + 1, 0, 900, secs)) {
      Serial.println("A<6-hex-ID>[:0-900-secs] (0 releases)");
      return true;
    }
    if (!meshProgramLease(id, 1 /* PROG_GH_CA */, (uint16_t)secs,
                          0x01 /* hard cut */, nullptr)) {
      Serial.println("CA lease refused: exact nonzero target required");
      return true;
    }
    if (secs)
      Serial.printf("CA %.6s %ds lease\n", tok + 1, secs);
    else
      Serial.printf("CA %.6s released\n", tok + 1);
    return true;
  }
  if (tok[0] == 'K' && tok[1] != 0) {
    uint8_t id[3];
    const char *colon = strchr(tok, ':');
    if (!parseHexId(tok + 1, id) || !colon) { Serial.println("K<6-hex-ID>:<ms>"); return true; }
    int ms = atoi(colon + 1);
    if (meshStrike(id, (uint16_t)ms))
      Serial.printf("strike %.6s %dms (clamped 5-300; fixture gates apply)\n",
                    tok + 1, ms);
    else
      Serial.println("strike refused: needs a real target id");
    return true;
  }
  if (strncmp(tok, "uB", 2) == 0) {
    uint32_t jobId = 0;
    int durationS = 0;
    if (strlen(tok) < 12 || tok[10] != ':' ||
        !parseHex32(tok + 2, jobId) ||
        !parseDecimalRange(tok + 11, 1, 3600, durationS)) {
      Serial.println("uB<8-hex-job>:<1-3600-seconds>");
      return true;
    }
    if (meshMaintenanceBegin(jobId, (uint16_t)durationS))
      Serial.printf("maintenance gather %08lX begun for %ds\n",
                    (unsigned long)jobId, durationS);
    else
      Serial.println("maintenance gather begin refused");
    meshMaintenancePrintStatus();
    return true;
  }
  if (strncmp(tok, "uA", 2) == 0) {
    uint32_t jobId = 0;
    uint8_t id[3] = {};
    if (strlen(tok) != 17 || tok[10] != ':' ||
        !parseHex32(tok + 2, jobId) || !parseHexId(tok + 11, id)) {
      Serial.println("uA<8-hex-job>:<6-hex-ID>");
      return true;
    }
    if (!meshMaintenanceAdd(jobId, id))
      Serial.println("maintenance target add refused (job/state/capacity)");
    meshMaintenancePrintStatus();
    return true;
  }
  if (strncmp(tok, "uF", 2) == 0 || strncmp(tok, "uS", 2) == 0) {
    uint32_t jobId = 0;
    bool freeze = tok[1] == 'F';
    if (strlen(tok) != 10 || !parseHex32(tok + 2, jobId)) {
      Serial.println(freeze ? "uF<8-hex-job>" : "uS<8-hex-job>");
      return true;
    }
    if (freeze && !meshMaintenanceFreeze(jobId))
      Serial.println("maintenance freeze refused (job/state mismatch)");
    meshMaintenancePrintStatus();
    return true;
  }
  if (tok[0] == 'U') {
    uint8_t id[3];
    if (strlen(tok) != 7 || !parseHexId(tok + 1, id)) {
      Serial.println("U<6-hex-ID> (exact target required)");
      return true;
    }
    if (meshEnterMaintenance(id))
      Serial.printf("sustained TARGET_ENTER_MAINT %.6s 35s (nonblocking)\n",
                    tok + 1);
    else
      Serial.println("maintenance refused: invalid target or campaign active");
    return true;
  }
  if (tok[0] == 'F') {
    uint8_t id[3] = {};
    if (strlen(tok) != 11 || tok[7] != ':' || tok[9] != ':' ||
        !parseHexId(tok + 1, id) ||
        (tok[8] != '0' && tok[8] != '1') ||
        (tok[10] != '0' && tok[10] != '1')) {
      Serial.println("F<6-hex-ID>:<0|1>:<0|1> (profile, persist)");
      return true;
    }
    uint8_t profile = (uint8_t)(tok[8] - '0');
    bool persist = tok[10] == '1';
    if (meshProfile(id, profile, persist))
      Serial.printf("profile %.6s=%s persist=%u\n", tok + 1,
                    profile == PROFILE_DEV ? "commission" : "field",
                    persist ? 1U : 0U);
    else
      Serial.println("profile refused: exact nonzero target required");
    return true;
  }
  if (tok[0] == 'B') {
    int secs = tok[1] ? atoi(tok + 1) : 600;
    if (secs < 1) secs = 600;
    if (secs > 65535) secs = 65535;
    meshProgramLease(kAll, 4 /*PROG_COMMISSION_DARK*/, (uint16_t)secs,
                     0x01 /*hard cut*/, nullptr);
    Serial.printf("fleet dark lease %ds (self-expires)\n", secs);
    return true;
  }
  if (tok[0] == 'b' && tok[1] == 0) {
    meshProgramLease(kAll, 4, 0, 0x01, nullptr);  // lease 0 = explicit release
    Serial.println("fleet lease released -> autonomous");
    return true;
  }
  if (tok[0] == 't' && tok[1] == 0) {
    uint32_t now = millis();
    Serial.printf(
        "{\"bridge\":\"tdeck\",\"channel\":%u,\"peers\":%d,\"live\":%d,"
        "\"queue_drops\":%lu}\n",
        settings().channel, census().seenCount(), census().liveCount(now),
        (unsigned long)censusRingDrops());
    return true;
  }
  return false;
}

static void printShow() {
  const Settings &s = settings();
  char keyMask[20] = "(unset)";
  if (s.apiKey[0]) snprintf(keyMask, sizeof(keyMask), "%.10s...", s.apiKey);
  Serial.printf("ssid=%s psk=%s key=%s model=%s ch=%u display=%s night_bl=%u\n",
                s.ssid[0] ? s.ssid : "(unset)", s.psk[0] ? "(set)" : "(unset)",
                keyMask, s.model, s.channel, s.dayMode ? "day" : "night",
                s.backlight);
  Serial.printf("net=%s ip=%s ap_ch=%u sntp=%d mesh_up=%d mesh_frames=%lu\n",
                netStateName(), netIp(), netApChannel(), netSntpSynced() ? 1 : 0,
                espnowUp() ? 1 : 0, (unsigned long)espnowStats().frames);
  storePrintActions();
}

static void printMem() {
  Serial.printf("nb-mem heap_free=%u heap_min=%u psram_free=%u psram_min=%u\n",
                (unsigned)ESP.getFreeHeap(), (unsigned)ESP.getMinFreeHeap(),
                (unsigned)ESP.getFreePsram(), (unsigned)ESP.getMinFreePsram());
}

static void handleLine(char *line) {
  // `ask <text>` takes the raw rest of the line (spaces intact).
  if (strncmp(line, "ask ", 4) == 0) {
    if (claudeSubmit(line + 4)) Serial.println("claude: submitted");
    else Serial.println("claude: busy (message already pending)");
    return;
  }
  // tokenize in place
  char *argv[8] = {};
  int argc = 0;
  for (char *tok = strtok(line, " \t"); tok && argc < 8; tok = strtok(nullptr, " \t"))
    argv[argc++] = tok;
  if (argc == 0) return;

  if (argc == 1 && handleQuickCommand(argv[0])) {
    return;
  } else if (strcmp(argv[0], "help") == 0 || strcmp(argv[0], "?") == 0) {
    printHelp();
  } else if (strcmp(argv[0], "emit") == 0 && argc >= 2) {
    nbEmitEnable(strcmp(argv[1], "on") == 0);
    Serial.printf("emit %s\n", nbEmitEnabled() ? "on" : "off");
  } else if (strcmp(argv[0], "show") == 0) {
    printShow();
  } else if (strcmp(argv[0], "mem") == 0) {
    printMem();
  } else if (strcmp(argv[0], "peers") == 0) {
    uint32_t now = millis();
    static CensusView rows[192];
    size_t n = census().snapshot(rows, 192, now);
    Serial.printf("peers: %d live / %u seen (observed=%u.%u%%)\n",
                  census().liveCount(now), (unsigned)n,
                  census().observedPermille() / 10,
                  census().observedPermille() % 10);
    for (size_t i = 0; i < n; ++i) {
      Serial.printf(
          " %02X%02X%02X age=%lus rssi=%d ewma=%d pdr=%u winpdr=%s soc=%u%% "
          "bv=%d cls=%u prog=%u\n",
          rows[i].id[0], rows[i].id[1], rows[i].id[2],
          (unsigned long)(rows[i].ageMs / 1000), rows[i].rssi, rows[i].rssiEwma,
          rows[i].pdrX1000,
          rows[i].winPdrX1000 == 0xFFFF ? "-" : String(rows[i].winPdrX1000).c_str(),
          rows[i].soc, rows[i].battMv, rows[i].fixtureClass,
          rows[i].activeProgram);
    }
  } else if (strcmp(argv[0], "probe") == 0) {
    ProbeReport r;
    halProbeRun(&r);
    Serial.printf("probe: psram=%d(%lu) kbd=%d touch=%d es7210=0x%02X gps=%d@%lu%s\n",
                  r.psram, (unsigned long)r.psramBytes, r.keyboard, r.touch,
                  r.es7210Addr, r.gpsNmea, (unsigned long)r.gpsBaud,
                  r.gpsSwapped ? " (swapped)" : "");
  } else if (strcmp(argv[0], "reboot") == 0) {
    Serial.println("rebooting");
    delay(100);
    ESP.restart();
  } else if (strcmp(argv[0], "wifi") == 0 && argc >= 2) {
    if (strcmp(argv[1], "retry") == 0) netMgrRetry();
    else if (strcmp(argv[1], "off") == 0) { netMgrOff(); Serial.println("wifi off (mesh-only)"); }
    else Serial.println("wifi retry|off");
  } else if (strcmp(argv[0], "set") == 0 && argc >= 3) {
    Settings &s = settings();
    if (strcmp(argv[1], "wifi") == 0 && argc >= 4) {
      strlcpy(s.ssid, argv[2], sizeof(s.ssid));
      strlcpy(s.psk, argv[3], sizeof(s.psk));
      storeSave();
      Serial.printf("wifi credentials stored for '%s'; `wifi retry` to join\n", s.ssid);
    } else if (strcmp(argv[1], "key") == 0) {
      strlcpy(s.apiKey, argv[2], sizeof(s.apiKey));
      storeSave();
      Serial.println("api key stored");
    } else if (strcmp(argv[1], "model") == 0) {
      strlcpy(s.model, argv[2], sizeof(s.model));
      storeSave();
      Serial.printf("model=%s\n", s.model);
    } else if (strcmp(argv[1], "channel") == 0) {
      int ch = atoi(argv[2]);
      if (ch < 1 || ch > 13) { Serial.println("channel must be 1-13"); return; }
      s.channel = (uint8_t)ch;
      storeSave();
      Serial.printf("mesh channel=%d (reboot to apply cleanly)\n", ch);
    } else if (strcmp(argv[1], "display") == 0) {
      if (strcmp(argv[2], "day") == 0) s.dayMode = true;
      else if (strcmp(argv[2], "night") == 0) s.dayMode = false;
      else { Serial.println("display must be day or night"); return; }
      storeSave();
      halDisplaySetBacklight(s.dayMode ? 255 : s.backlight);
      Serial.printf("display=%s (reboot applies theme)\n",
                    s.dayMode ? "day" : "night");
    } else if (strcmp(argv[1], "bl") == 0) {
      int v = atoi(argv[2]);
      if (v < 0) v = 0;
      if (v > 255) v = 255;
      s.backlight = (uint8_t)v;
      storeSave();
      if (!s.dayMode) halDisplaySetBacklight(s.backlight);
      Serial.printf("night backlight=%d%s\n", v,
                    s.dayMode ? " (day stays at 255)" : "");
    } else {
      printHelp();
    }
  } else {
    printHelp();
  }
}

void serialCliTick() {
  static char buf[192];
  static size_t n = 0;
  while (Serial.available()) {
    char ch = (char)Serial.read();
    if (ch == '\n' || ch == '\r') {
      if (n > 0) {
        buf[n] = 0;
        n = 0;
        handleLine(buf);
      }
    } else if (n < sizeof(buf) - 1) {
      buf[n++] = ch;
    }
  }
}
