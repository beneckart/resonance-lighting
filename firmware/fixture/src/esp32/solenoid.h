// D7/VDC solarnoid gate driver (ADR 0030), ported verbatim from net_bench's
// proven safety pattern: bounded pulse (5-300 ms clamp), 80 ms coil rest,
// esp_timer one-shot AND a loop failsafe deadline. Rev-2 has a 10k D7 pulldown
// and hardware-bounded SW1/RX480E one-shot sources on the same net, so an armed
// fixture releases D7 (INPUT/high-Z) on every exit path instead of clamping
// those sources with OUTPUT LOW. A disarmed fixture actively parks D7 LOW.
// Runtime NVS `sol_en` replaces the old -DNB_SOLENOID_D7 build flag.
//
// Strikes are additionally gated by the lifecycle (daytime solar-surplus only,
// wired in P5); solenoidStrike is the mechanism, not the policy.
#pragma once

#include <stdint.h>

#define RES_SOLENOID_DEFAULT_MS 40
#define RES_SOLENOID_MIN_MS 5
#define RES_SOLENOID_MAX_MS 300
#define RES_SOLENOID_REST_MS 80
#define RES_SOLENOID_BUTTON_PIN BTN // PowerFeather USER/BOOT, GPIO0, active LOW
#define RES_SOLENOID_BUTTON_DEBOUNCE_MS 30

void solenoidInit();                 // after Serial; creates the pulse timer
void solenoidStop(const char *why);  // end MCU gate drive (OTA/maintenance/sleep)
bool solenoidStrike(uint16_t pulseMs, const char *why);
void solenoidExternalTick();         // extend capboard SW1 edge to safe 40 ms
void solenoidButtonHandleWake();     // fire the one strike an EXT0 wake earned
void solenoidButtonTick();
void solenoidButtonPrepareSleep();   // re-arm EXT0 wake only from released
void solenoidFailsafeTick();

// Telemetry accessors.
bool solenoidGateOn();
bool solenoidQuietFor(uint32_t ms); // no gate drive now or within the last ms
uint32_t solenoidStrikes();
uint32_t solenoidBlocked();
uint32_t solenoidFailsafes();
uint16_t solenoidLastPulseMs();
