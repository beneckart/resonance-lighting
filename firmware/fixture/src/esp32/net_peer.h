// Peer-side protocol engine: heartbeat scheduler (hb-short/hb-full split) and
// the rx dispatcher for bridge commands. The wire format is core/packet.h; the
// unmodified net_bench serial-bridge master + dashboard must keep parsing us
// (the P1 gate).
#pragma once

#include <stdint.h>
#include "espnow_link.h"

// Heartbeat cadence. The full heartbeat (~150 B) is telemetry-only and slow;
// anything fast rides NB_CHOREO_STATE instead (airtime: 150 nodes of 1 Hz full
// heartbeats would eat ~25% of the channel).
#define RES_HB_FULL_PERIOD_MS 60000
#define RES_HB_SHORT_PERIOD_PROD_MS 5000 // 0.2 Hz
#define RES_HB_SHORT_PERIOD_DEV_MS 1000  // bench-parity dashboards
#define RES_JITTER_PCT 30

void netPeerInit();
void netPeerTick();              // drain rx + dispatch + scheduled sends
void netPeerSendHeartbeat(bool full); // immediate send (boot announce, state change)

// Rate override (NB_SET_RATE, bench sweeps): 0 = profile default cadence.
void netPeerSetRateHz(uint8_t hz);

// Downlink (bridge SHOWFRAME) delivery stats, reported inside the heartbeat.
uint16_t netPeerDlPdrX1000();
int8_t netPeerDlRssi();

// Set by later phases; carried in the heartbeat/choreo packets.
extern uint8_t gNetCaState;     // mirrors GH state (P5)
extern uint8_t gNetLifeState;   // lifecycle enum (P5)
extern uint8_t gNetPowerTier;   // LedTier (P3)
extern uint8_t gNetProgram;     // active program id (P5)
extern uint16_t gNetNightMin;   // minutes into NIGHT_SHOW (P5)

// Latest bridge show frame (consumed by PROG_BRIDGE_SHOW in P5).
struct ShowFrameIn {
  uint32_t rx_ms;    // 0 = never seen
  uint16_t phase;
  uint8_t hue, flags, val, bright, effect, beat_phase, energy;
};
const ShowFrameIn &netPeerLastShowFrame();

// Rig color-identify (NbIdentify tail): 0 = none/blink-only.
uint8_t netPeerIdentifyColor();
uint8_t netPeerIdentifyBlink();
bool netPeerIdentifyActive();
