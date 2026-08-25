#pragma once

#include <stdint.h>

struct RxItem;

// Serial nb-* emitters, byte-compatible with the cores3_bridge lines that
// ops/bench/net_bench_dashboard.py parses (RX_MASTER/RX_PEER regexes at
// dashboard :30-61). Cadence: nbEmitTick prints nb-master + one nb-peer line
// per tracked fixture at 1 Hz. `emit off` silences the periodic lines for
// human CLI sessions; frame-triggered emitters (scanap/rssi) stay on.
void nbEmitTick(uint32_t nowMs);
void nbEmitEnable(bool on);
bool nbEmitEnabled();
void nbEmitScanAp(const RxItem &item);
void nbEmitNeighborReport(const RxItem &item);
void nbEmitTimeQuality(const RxItem &item);
void nbEmitLocalGps(const uint8_t sourceId[3], uint32_t utcS, uint16_t subMs,
                    uint32_t ageMs, uint16_t uncertaintyMs, uint16_t bootId);
