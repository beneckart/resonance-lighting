#include "agent_tools.h"

#include <Arduino.h>
#include <stdio.h>
#include <string.h>

#include "../core/census.h"
#include "../core/fleet_registry_generated.h"
#include "../core/health_model.h"
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
    case 5: return "contagion";
    default: return "?";
  }
}

static bool parseHexId(const char *s, uint8_t out[3]) {
  if (!s || strlen(s) != 6) return false;
  for (int i = 0; i < 6; ++i)
    if (!isxdigit((unsigned char)s[i])) return false;
  for (int i = 0; i < 3; ++i) {
    char b[3] = {s[i * 2], s[i * 2 + 1], 0};
    out[i] = (uint8_t)strtoul(b, nullptr, 16);
  }
  return true;
}

static bool resolveFixture(const char *token, bool allowAll, uint8_t out[3],
                           const HealthRegistryEntry **entry) {
  *entry = nullptr;
  if (allowAll &&
      (strcmp(token, "all") == 0 || strcmp(token, "000000") == 0)) {
    memset(out, 0, 3);
    return true;
  }
  if (parseHexId(token, out)) {
    *entry = healthRegistryFind(kHealthRegistry, kHealthRegistryCount, out);
    return true;
  }
  *entry =
      healthRegistryFindCallsign(kHealthRegistry, kHealthRegistryCount, token);
  if (!*entry) return false;
  memcpy(out, (*entry)->id, 3);
  return true;
}

static void formatId(const uint8_t id[3], char out[7]) {
  snprintf(out, 7, "%02X%02X%02X", id[0], id[1], id[2]);
}

static const char *callsignForId(const uint8_t id[3]) {
  const HealthRegistryEntry *entry =
      healthRegistryFind(kHealthRegistry, kHealthRegistryCount, id);
  return entry ? entry->callsign : "";
}

static constexpr size_t kCensusPageSize = 24;

static int appendRow(char *out, size_t cap, int o, bool first,
                     const CensusView &v) {
  const char *callsign = callsignForId(v.id);
  int n = snprintf(out + o, cap - o,
                   "%s{\"id\":\"%02X%02X%02X\",\"name\":\"%s\","
                   "\"age_s\":%lu,\"rssi\":%d,"
                   "\"pdr_pct\":%u,\"soc\":%d,\"class\":\"%s\",\"program\":"
                   "\"%s\"}",
                   first ? "" : ",", v.id[0], v.id[1], v.id[2],
                   callsign, (unsigned long)(v.ageMs / 1000), v.rssiEwma,
                   v.pdrX1000 / 10, v.soc == 255 ? -1 : v.soc,
                   className(v.fixtureClass), programName(v.activeProgram));
  return n < 0 || (size_t)(o + n) >= cap ? -1 : o + n;
}

bool agentExecuteTool(const char *name, const char *inputJson, size_t inputLen,
                      char *result, size_t resultCap) {
  uint32_t now = millis();

  if (strcmp(name, "mesh_census") == 0) {
    long quietS = jsonFindInt(inputJson, inputLen, "quiet_s", 0);
    long requestedOffset = jsonFindInt(inputJson, inputLen, "offset", 0);
    if (requestedOffset < 0) requestedOffset = 0;
    static CensusView rows[CENSUS_MAX_TRACKED];
    size_t n = quietS > 0
                   ? censusQuietListSafe((uint32_t)quietS, rows,
                                         CENSUS_MAX_TRACKED, now)
                   : censusSnapshotSafe(rows, CENSUS_MAX_TRACKED, now);
    size_t offset = (size_t)requestedOffset;
    if (offset > n) offset = n;
    size_t end = offset + kCensusPageSize;
    if (end > n) end = n;
    size_t returned = end - offset;
    int live = 0, seen = 0;
    censusCountsSafe(&live, &seen, now);
    int o = snprintf(result, resultCap,
                     "{\"live\":%d,\"seen\":%d,\"observed_pct\":%u,"
                     "\"matched\":%u,\"offset\":%u,\"fixtures\":[",
                     live, seen, censusObservedPermilleSafe() / 10,
                     (unsigned)n, (unsigned)offset);
    for (size_t i = offset; i < end && o > 0; ++i)
      o = appendRow(result, resultCap, o, i == offset, rows[i]);
    if (o < 0) return false;
    int tail = snprintf(result + o, resultCap - o,
                        "],\"returned\":%u,\"truncated\":%s",
                        (unsigned)returned, end < n ? "true" : "false");
    if (tail < 0 || (size_t)(o + tail) >= resultCap) return false;
    o += tail;
    if (end < n) {
      tail = snprintf(result + o, resultCap - o,
                      ",\"next_offset\":%u", (unsigned)end);
      if (tail < 0 || (size_t)(o + tail) >= resultCap) return false;
      o += tail;
    }
    if ((size_t)o + 2 >= resultCap) return false;
    result[o++] = '}';
    result[o] = '\0';
    return true;
  }

  if (strcmp(name, "node_status") == 0) {
    char ids[16] = {0};
    if (jsonFindString(inputJson, inputLen, "id", ids, sizeof(ids)) < 1) {
      snprintf(result, resultCap, "id must be a callsign or 6 hex digits");
      return false;
    }
    uint8_t id[3];
    const HealthRegistryEntry *entry = nullptr;
    if (!resolveFixture(ids, false, id, &entry)) {
      snprintf(result, resultCap, "unknown fixture '%s'", ids);
      return false;
    }
    char idHex[7];
    formatId(id, idHex);
    PeerStat p;
    if (!censusPeerSafe(id, &p)) {
      snprintf(result, resultCap, "fixture %s [%s] not in census",
               entry ? entry->callsign : idHex, idHex);
      return false;
    }
    uint32_t total = p.recv + p.gaps;
    snprintf(result, resultCap,
             "{\"id\":\"%s\",\"name\":\"%s\",\"age_s\":%lu,"
             "\"rssi\":%d,\"rssi_ewma\":%d,"
             "\"recv\":%lu,\"gaps\":%lu,\"pdr_pct\":%lu,\"batt_mv\":%d,"
             "\"soc\":%d,\"supply_mv\":%d,\"class\":\"%s\",\"program\":\"%s\","
             "\"life_state\":%u,\"power_tier\":%u,\"led\":{\"rail\":%u,"
             "\"r\":%u,\"g\":%u,\"b\":%u,\"w\":%u,\"lit\":%u},\"fw\":\"%s\"}",
             idHex, entry ? entry->callsign : "",
             (unsigned long)((now - p.lastHeardMs) / 1000), p.rssi,
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
    if (jsonFindString(inputJson, inputLen, "id", ids, sizeof(ids)) < 1) {
      snprintf(result, resultCap,
               "id must be a callsign, 6 hex digits, or all");
      return false;
    }
    uint8_t id[3];
    const HealthRegistryEntry *entry = nullptr;
    if (!resolveFixture(ids, true, id, &entry)) {
      snprintf(result, resultCap, "unknown fixture '%s'", ids);
      return false;
    }
    char idHex[7];
    formatId(id, idHex);
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
    snprintf(result, resultCap,
             "{\"ok\":true,\"id\":\"%s\",\"name\":\"%s\",\"secs\":%ld}",
             idHex, entry ? entry->callsign : "", secs);
    return true;
  }

  if (strcmp(name, "strike") == 0) {
    char ids[16] = {0};
    if (jsonFindString(inputJson, inputLen, "id", ids, sizeof(ids)) < 1) {
      snprintf(result, resultCap, "strike needs one callsign or fixture id");
      return false;
    }
    uint8_t id[3];
    const HealthRegistryEntry *entry = nullptr;
    if (!resolveFixture(ids, false, id, &entry)) {
      snprintf(result, resultCap, "unknown fixture '%s'", ids);
      return false;
    }
    char idHex[7];
    formatId(id, idHex);
    long ms = jsonFindInt(inputJson, inputLen, "pulse_ms", 40);
    if (!meshStrike(id, (uint16_t)ms)) {
      snprintf(result, resultCap, "strikes are never broadcast; give one id");
      return false;
    }
    snprintf(result, resultCap,
             "{\"ok\":true,\"id\":\"%s\",\"name\":\"%s\","
             "\"pulse_ms\":%ld,\"note\":"
             "\"fixture may refuse at night / low power\"}",
             idHex, entry ? entry->callsign : "", ms);
    return true;
  }

  if (strcmp(name, "set_program") == 0) {
    char target[16] = {0};
    if (jsonFindString(inputJson, inputLen, "target", target, sizeof(target)) < 1) {
      snprintf(result, resultCap,
               "target must be all, a callsign, or a 6-hex id");
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
    const HealthRegistryEntry *entry = nullptr;
    if (!fleetWide && !resolveFixture(target, false, id, &entry)) {
      snprintf(result, resultCap, "unknown fixture '%s'", target);
      return false;
    }
    char idHex[7];
    formatId(id, idHex);
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
             "{\"ok\":true,\"target\":\"%s\",\"name\":\"%s\","
             "\"program\":\"%s\",\"lease_s\":%ld,"
             "\"note\":\"lease self-expires\"}",
             fleetWide ? "all" : idHex,
             fleetWide || !entry ? "" : entry->callsign,
             programName((uint8_t)prog), leaseS);
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
      const char *callsign = callsignForId(frames[i].id);
      int w = snprintf(result + o, resultCap - o,
                       "%s{\"id\":\"%02X%02X%02X\",\"name\":\"%s\","
                       "\"type\":%u,\"rssi\":%d,"
                       "\"age_ms\":%lu}",
                       i ? "," : "", frames[i].id[0], frames[i].id[1],
                       frames[i].id[2], callsign, frames[i].type, frames[i].rssi,
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
