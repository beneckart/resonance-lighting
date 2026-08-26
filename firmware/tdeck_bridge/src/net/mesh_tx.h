#pragma once

#include <stdint.h>

// The ONLY translation unit that emits Nb packets (single-writer doctrine;
// espnow_link deliberately does not export a raw send). Burst-repeat follows
// the fleet RF convention: broadcast cmds 4x/5 ms, targeted 6x/8 ms.
// M2's TxService adds the confirm rail + stream ownership ON TOP of these.
// The one sleep exception is a local-UI-only, confirmed fleet timer sleep;
// agent tools and the serial CLI intentionally do not expose it (ADR 0048).

void meshTxBegin();                 // derive our 3-byte id from the STA MAC
void meshTxTick();                  // bounded resend campaigns for sleeping peers
const uint8_t *meshMyId();          // [3]
uint32_t meshTxSeq();
uint32_t meshTxSendOk();
uint32_t meshTxSendFail();

// One-shot commands (clamps enforced here AND fixture-side).
void meshIdentify(const uint8_t target[3], uint8_t secs, uint8_t color = 0,
                  uint8_t blink = 0, uint8_t value = 255);
bool meshStrike(const uint8_t id[3], uint16_t pulseMs);  // false: id==00:00:00
bool meshStrikeBroadcast(uint16_t pulseMs, uint32_t fireInMs);
void meshProgramLease(const uint8_t target[3], uint8_t programId,
                      uint16_t leaseS, uint8_t flags, const uint8_t params[8]);
bool meshSleepAll(uint16_t seconds);  // local confirmed UI only; 1..65535 s
void meshForceLifecycle(uint8_t mode); // 0=day 1=night 2=auto; RAM-only fleet
bool meshEnterMaintenance(const uint8_t target[3]);
// Exact-target only. Commission-default persistence is an NVS mutation, so
// Bridge OS never emits this command with the all-zero broadcast target.
bool meshCommissionDefault(const uint8_t target[3], uint8_t mode, bool persist);
bool meshTimeQuality(uint32_t utcS, uint16_t subMs, uint16_t ageS,
                     uint16_t uncertaintyMs, uint16_t bootId);

// Streaming frame (single send, no burst; streams re-send at 8 Hz).
// Used ONLY by stream_svc; apps go through the service, never here.
struct MeshDirectEntry {
  uint8_t id[3];
  uint8_t r, g, b, w;
};
void meshDirectFrame(const MeshDirectEntry *entries, uint8_t count,
                     uint8_t flags);
