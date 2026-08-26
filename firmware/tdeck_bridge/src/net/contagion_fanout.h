#pragma once

#include <stdint.h>

struct RxItem;

// Explicit compatibility adapter for deployed fixtures that understand the
// proven addressed strike command but not program 5. One selected Contagion
// source infection starts one 40 ms targeted roll over fresh downlights.
void contagionFanoutConfigure(const uint8_t source[3], uint32_t leaseMs,
                              uint16_t pulseMs);
void contagionFanoutDisable();
void contagionFanoutObserve(const RxItem &item);
void contagionFanoutTick(uint32_t nowMs);
