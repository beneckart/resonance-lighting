// First-instruction fail-safe parks. Call bootParkAll() as the FIRST statement
// of setup(), before Serial, NVS, or Board.init(): VDC is always live (solenoid
// gate), pixels latch their last frame, and the SDK enables the 3V3 rail on
// cold init. See POWERFEATHER_NOTES.md ("A POR can erase low-voltage
// protection") and ADR 0013.
#pragma once

// GPIO4 = EN_3V3 (switchable header rail, active HIGH, RTC-held by the SDK).
// GPIO10 = A0, the one shared LED data pin for every class (ADR 0029).
// D7 = GPIO37, solenoid MOSFET gate (net_bench README: "not GPIO7").
#define RES_LED_DATA_PIN 10
#define RES_SOLENOID_PIN D7
#define RES_STATUS_LED_PIN 46

void bootParkAll();

// Re-assert the RTC-domain hold on EN_3V3 (used after every Board.init attempt,
// which briefly reconfigures the pad).
void bootParkRailLow();
