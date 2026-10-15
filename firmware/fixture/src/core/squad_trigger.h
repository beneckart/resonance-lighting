#pragma once
#include <stdint.h>
#include <string.h>

// Shared Atom/perimeter roster. Pulses: Brian's 2026-10-14 tone map.
struct SquadMember { const char *name; uint8_t id[3]; uint8_t pulseMs; };
static constexpr SquadMember SQUAD[] = {
  {"Ness",    {0x9F,0x27,0x38},16},
  {"Eevee",   {0x9F,0x0E,0x54},16},
  {"Rikku",   {0x9F,0x26,0xB0},16},
  {"Leeloo",  {0xF4,0x02,0x54},16},
  {"Bidoof",  {0x9F,0x26,0xD8},18},
  {"Pinsir",  {0x9F,0x26,0xBC},15},
  {"Tangela", {0x9F,0x0E,0x5C},15},
  {"Sakura",  {0xF2,0xBE,0x0C},13},
  {"Lucas",   {0x9F,0x26,0x38},17},
};
static constexpr unsigned SQUAD_COUNT = sizeof(SQUAD) / sizeof(SQUAD[0]);
static constexpr uint16_t SQUAD_ALL = (1U << SQUAD_COUNT) - 1;
inline int squadIndex(const uint8_t id[3]) {
  for (unsigned i=0; i<SQUAD_COUNT; ++i)
    if (!memcmp(id, SQUAD[i].id, 3)) return (int)i;
  return -1;
}
struct SquadCycle {
  uint8_t next = 0;
  bool fired = false;
  uint32_t lastMs = 0, count = 0;
  uint16_t targets() const { return next == SQUAD_COUNT ? SQUAD_ALL : 1U << next; }
  bool canFire(uint32_t now, uint16_t ready) const {
    return (ready & targets()) == targets() && (!fired || now-lastMs >= 2500);
  }
  void sent(uint32_t now) {
    lastMs=now; fired=true; ++count; next=(next+1) % (SQUAD_COUNT+1);
  }
};
// Clear space after boot/fault and between approaches. Two fresh near frames;
// stale/failed frames never count as clearance. <150 mm excludes close housing.
struct SquadPresence {
  bool armed=false, clearing=false;
  uint8_t nearFrames=0;
  uint32_t clearSince=0, lastMs=0;
  void invalidate() { armed=false; clearing=false; nearFrames=0; }
  bool sample(uint32_t now, bool valid, const uint16_t zones[16]) {
    if (!valid || (lastMs && now-lastMs > 1000)) invalidate();
    lastMs=now;
    if (!valid) return false;
    bool near=false, occupied=false;
    for (unsigned i=0;i<16;++i) {
      if (zones[i]>=150 && zones[i]<=700) near=true;
      if (zones[i]>=150 && zones[i]<=800) occupied=true;
    }
    if (!occupied) {
      nearFrames=0;
      if (!clearing) { clearing=true; clearSince=now; }
      if (now-clearSince>=800) armed=true;
      return false;
    }
    clearing=false;
    if (!near) { nearFrames=0; return false; }
    if (nearFrames<2) ++nearFrames;
    if (!armed || nearFrames<2) return false;
    armed=false;
    return true;
  }
};
