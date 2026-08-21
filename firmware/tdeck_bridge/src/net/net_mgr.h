#pragma once

#include <stdint.h>

// Wi-Fi STA lifecycle + SNTP + the ADR 0036/0037 channel guard.
//
// The one physics constraint: one 2.4 GHz radio, and in STA mode the AP picks
// the channel. The fleet is pinned to the stored mesh channel (11). If the AP
// lands anywhere else, the guard DROPS WI-FI AND KEEPS THE MESH — never the
// reverse — then re-pins the radio and surfaces the mismatch.

enum class NetState : uint8_t {
  OFF,            // no credentials, or wifi off — mesh-only on the pinned channel
  CONNECTING,     // association in progress
  GUARD_BLOCKED,  // AP is off the mesh channel; Wi-Fi dropped, mesh retained
  ONLINE,         // associated on the mesh channel; SNTP running
};

void netMgrBegin();          // radio up (STA, PS off), pin channel, mesh up
void netMgrTick();           // drive the state machine from loop()
void netMgrRetry();          // manual re-attempt (CLI `wifi retry`)
void netMgrOff();            // drop Wi-Fi, mesh-only
NetState netState();
const char *netStateName();
int netRssi();               // valid when ONLINE
const char *netIp();         // "0.0.0.0" unless ONLINE
uint8_t netApChannel();      // last observed AP channel (guard evidence)
bool netSntpSynced();
