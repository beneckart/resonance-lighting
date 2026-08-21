#include "agent_tools.h"

#include <Arduino.h>
#include <stdio.h>
#include <string.h>

#include "../core/census.h"
#include "../core/sse_parser.h"
#include "../ui/ui_confirm.h"
#include "census_svc.h"
#include "mesh_tx.h"

static const char *className(uint8_t c) {
  switch (c) {
    case 1: return "downlight";
    case 2: return "perimeter";
    case 3: return "uplight";
    case 4: return "chandelier";
    default: return "unknown";
  }
}

static const char *programName(uint8_t p) {
  switch (p) {
    case 0: return "idle";
    case 1: return "ca";
    case 2: return "bridge-show";
    case 3: return "direct";
    case 4: return "dark";
    default: return "?";
  }
}

static bool parseHexId(const char *s, uint8_t out[3]) {
  for (int i = 0; i < 6; ++i)
    if (!isxdigit((unsigned char)s[i])) return false;
  for (int i = 0; i < 3; ++i) {
    char b[3] = {s[i * 2], s[i * 2 + 1], 0};
    out[i] = (uint8_t)strtoul(b, nullptr, 16);
  }
  return true;
}

static int appendRow(char *out, size_t cap, int o, bool first,
                     const CensusView &v) {
  int n = snprintf(out + o, cap - o,
                   "%s{\"id\":\"%02X%02X%02X\",\"age_s\":%lu,\"rssi\":%d,"
                   "\"pdr_pct\":%u,\"soc\":%d,\"class\":\"%s\",\"program\":"
                   "\"%s\"}",
                   first ? "" : ",", v.id[0], v.id[1], v.id[2],
                   (unsigned long)(v.ageMs / 1000), v.rssiEwma,
                   v.pdrX1000 / 10, v.soc == 255 ? -1 : v.soc,
                   className(v.fixtureClass), programName(v.activeProgram));
  return n < 0 || (size_t)(o + n) >= cap ? -1 : o + n;
}

bool agentExecuteTool(const char *name, const char *inputJson, size_t inputLen,
                      char *result, size_t resultCap) {
  uint32_t now = millis();

  if (strcmp(name, "mesh_census") == 0) {
    long quietS = jsonFindInt(inputJson, inputLen, "quiet_s", 0);
    static CensusView rows[64];
    size_t n = quietS > 0
                   ? census().quietList((uint32_t)quietS, rows, 64, now)
                   : censusSnapshotSafe(rows, 64, now);
    int live = 0, seen = 0;
    censusCountsSafe(&live, &seen, now);
    int o = snprintf(result, resultCap,
                     "{\"live\":%d,\"seen\":%d,\"observed_pct\":%u,"
                     "\"fixtures\":[",
                     live, seen, census().observedPermille() / 10);
    for (size_t i = 0; i < n && i < 24 && o > 0; ++i)
      o = appendRow(result, resultCap, o, i == 0, rows[i]);
    if (o < 0) return false;
    snprintf(result + o, resultCap - o, "]%s}",
             n > 24 ? ",\"truncated\":true" : "");
    return true;
  }

  if (strcmp(name, "node_status") == 0) {
    char ids[16] = {0};
    if (jsonFindString(inputJson, inputLen, "id", ids, sizeof(ids)) < 6) {
      snprintf(result, resultCap, "id must be 6 hex digits");
      return false;
    }
    uint8_t id[3];
    if (!parseHexId(ids, id)) {
      snprintf(result, resultCap, "bad id '%s'", ids);
      return false;
    }
    PeerStat p;
    if (!censusPeerSafe(id, &p)) {
      snprintf(result, resultCap, "fixture %s not in census", ids);
      return false;
    }
    uint32_t total = p.recv + p.gaps;
    snprintf(result, resultCap,
             "{\"id\":\"%.6s\",\"age_s\":%lu,\"rssi\":%d,\"rssi_ewma\":%d,"
             "\"recv\":%lu,\"gaps\":%lu,\"pdr_pct\":%lu,\"batt_mv\":%d,"
             "\"soc\":%d,\"supply_mv\":%d,\"class\":\"%s\",\"program\":\"%s\","
             "\"life_state\":%u,\"power_tier\":%u,\"led\":{\"rail\":%u,"
             "\"r\":%u,\"g\":%u,\"b\":%u,\"w\":%u,\"lit\":%u},\"fw\":\"%s\"}",
             ids, (unsigned long)((now - p.lastHeardMs) / 1000), p.rssi,
             p.rssiEwma, (unsigned long)p.recv, (unsigned long)p.gaps,
             total ? (unsigned long)((uint64_t)p.recv * 100 / total) : 0,
             p.battMv, p.soc == 255 ? -1 : p.soc, p.supplyMv,
             className(p.classLatched),
             programName(p.hasFixtureState ? p.activeProgram : 0),
             p.hasFixtureState ? p.lifeState : 0,
             p.hasFixtureState ? p.powerTier : 0, p.ledRailOn, p.ledR, p.ledG,
             p.ledB, p.ledW, p.ledLitPixels, p.hasFw ? p.fwRev : "");
    return true;
  }

  if (strcmp(name, "identify") == 0) {
    char ids[16] = {0};
    if (jsonFindString(inputJson, inputLen, "id", ids, sizeof(ids)) < 6) {
      snprintf(result, resultCap, "id must be 6 hex digits (000000 = all)");
      return false;
    }
    uint8_t id[3];
    if (!parseHexId(ids, id)) {
      snprintf(result, resultCap, "bad id '%s'", ids);
      return false;
    }
    long secs = jsonFindInt(inputJson, inputLen, "secs", 8);
    if (secs < 1) secs = 1;
    if (secs > 255) secs = 255;
    char color[12] = {0};
    jsonFindString(inputJson, inputLen, "color", color, sizeof(color));
    uint8_t c = 0;
    if (strcmp(color, "red") == 0) c = 1;
    else if (strcmp(color, "green") == 0) c = 2;
    else if (strcmp(color, "blue") == 0) c = 3;
    else if (strcmp(color, "yellow") == 0) c = 4;
    else if (strcmp(color, "white") == 0) c = 5;
    meshIdentify(id, (uint8_t)secs, c, 1, 128);
    snprintf(result, resultCap, "{\"ok\":true,\"id\":\"%.6s\",\"secs\":%ld}",
             ids, secs);
    return true;
  }

  if (strcmp(name, "strike") == 0) {
    char ids[16] = {0};
    if (jsonFindString(inputJson, inputLen, "id", ids, sizeof(ids)) < 6) {
      snprintf(result, resultCap, "strike needs one real fixture id");
      return false;
    }
    uint8_t id[3];
    if (!parseHexId(ids, id)) {
      snprintf(result, resultCap, "bad id '%s'", ids);
      return false;
    }
    long ms = jsonFindInt(inputJson, inputLen, "pulse_ms", 40);
    if (!meshStrike(id, (uint16_t)ms)) {
      snprintf(result, resultCap, "strikes are never broadcast; give one id");
      return false;
    }
    snprintf(result, resultCap,
             "{\"ok\":true,\"id\":\"%.6s\",\"pulse_ms\":%ld,\"note\":"
             "\"fixture may refuse at night / low power\"}",
             ids, ms);
    return true;
  }

  if (strcmp(name, "set_program") == 0) {
    char target[16] = {0};
    if (jsonFindString(inputJson, inputLen, "target", target, sizeof(target)) < 1) {
      snprintf(result, resultCap, "target must be 'all' or a 6-hex id");
      return false;
    }
    long prog = jsonFindInt(inputJson, inputLen, "program_id", -1);
    if (prog < 0 || prog > 4) {
      snprintf(result, resultCap, "program_id must be 0-4");
      return false;
    }
    long leaseS = jsonFindInt(inputJson, inputLen, "lease_s", 600);
    if (leaseS < 0) leaseS = 0;
    if (leaseS > 65535) leaseS = 65535;
    uint8_t id[3] = {0, 0, 0};
    bool fleetWide = strcmp(target, "all") == 0 ||
                     strcmp(target, "000000") == 0;
    if (!fleetWide && !parseHexId(target, id)) {
      snprintf(result, resultCap, "bad target '%s'", target);
      return false;
    }
    if (fleetWide) {
      char summary[96];
      snprintf(summary, sizeof(summary),
               "PROGRAM_SET all -> %s, %ld s lease", programName((uint8_t)prog),
               leaseS);
      // Blocks on the on-device confirm rail (30 s TTL). Denial round-trips
      // as an is_error tool result the model has to verbalize.
      ConfirmVerdict v = uiConfirmBlocking(summary, "Claude", 30000);
      if (v == ConfirmVerdict::DENIED) {
        snprintf(result, resultCap, "operator DENIED the fleet-wide lease");
        return false;
      }
      if (v == ConfirmVerdict::TIMEOUT) {
        snprintf(result, resultCap,
                 "no operator confirmation within 30 s; not sent");
        return false;
      }
    }
    meshProgramLease(id, (uint8_t)prog, (uint16_t)leaseS, 0x01, nullptr);
    snprintf(result, resultCap,
             "{\"ok\":true,\"target\":\"%s\",\"program\":\"%s\",\"lease_s\":"
             "%ld,\"note\":\"lease self-expires\"}",
             fleetWide ? "all" : target, programName((uint8_t)prog), leaseS);
    return true;
  }

  if (strcmp(name, "sniffer_tail") == 0) {
    long want = jsonFindInt(inputJson, inputLen, "n", 10);
    if (want < 1) want = 1;
    if (want > 32) want = 32;
    static TailFrame frames[32];
    size_t n = censusTailSafe(frames, (size_t)want);
    int o = snprintf(result, resultCap, "{\"frames\":[");
    for (size_t i = 0; i < n && o > 0 && (size_t)o < resultCap - 80; ++i) {
      int w = snprintf(result + o, resultCap - o,
                       "%s{\"id\":\"%02X%02X%02X\",\"type\":%u,\"rssi\":%d,"
                       "\"age_ms\":%lu}",
                       i ? "," : "", frames[i].id[0], frames[i].id[1],
                       frames[i].id[2], frames[i].type, frames[i].rssi,
                       (unsigned long)(now - frames[i].ms));
      if (w < 0) break;
      o += w;
    }
    snprintf(result + o, resultCap - o, "]}");
    return true;
  }

  snprintf(result, resultCap, "unknown tool '%s'", name);
  return false;
}
