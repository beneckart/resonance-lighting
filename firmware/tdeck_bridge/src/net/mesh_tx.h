#pragma once

#include <stdint.h>

// The ONLY translation unit that emits Nb packets (single-writer doctrine —
// espnow_link deliberately does not export a raw send). Burst-repeat follows
// the fleet RF convention: broadcast cmds 4x/5 ms, targeted 6x/8 ms.
// M2's TxService adds the confirm rail + stream ownership ON TOP of these;
// nothing here exposes OTA/reboot/profile/lifecycle/sleep/capacity opcodes.

void meshTxBegin();                 // derive our 3-byte id from the STA MAC
const uint8_t *meshMyId();          // [3]
uint32_t meshTxSeq();
uint32_t meshTxSendOk();
uint32_t meshTxSendFail();

// One-shot commands (clamps enforced here AND fixture-side).
void meshIdentify(const uint8_t target[3], uint8_t secs, uint8_t color = 0,
                  uint8_t blink = 0, uint8_t value = 255);
bool meshStrike(const uint8_t id[3], uint16_t pulseMs);  // false: id==00:00:00
void meshProgramLease(const uint8_t target[3], uint8_t programId,
                      uint16_t leaseS, uint8_t flags, const uint8_t params[8]);

// Streaming frame (single send, no burst — streams re-send at 8 Hz).
// Used ONLY by stream_svc; apps go through the service, never here.
struct MeshDirectEntry {
  uint8_t id[3];
  uint8_t r, g, b, w;
};
void meshDirectFrame(const MeshDirectEntry *entries, uint8_t count,
                     uint8_t flags);
