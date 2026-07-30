// D7/VDC solarnoid gate driver (ADR 0030), ported verbatim from net_bench's
// proven safety pattern: bounded pulse (5-300 ms clamp), 80 ms coil rest,
// esp_timer one-shot AND a loop failsafe deadline, gate LOW on every exit path
// (boot, OTA, maintenance, sleep). Runtime NVS `sol_en` replaces the old
// -DNB_SOLENOID_D7 build flag: the code is always present, the pin is always
// parked, and strikes are refused when disarmed.
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
void solenoidStop(const char *why);  // force gate LOW (OTA/maintenance/sleep)
bool solenoidStrike(uint16_t pulseMs, const char *why);
void solenoidButtonHandleWake();     // fire the one strike an EXT0 wake earned
void solenoidButtonTick();
void solenoidButtonPrepareSleep();   // re-arm EXT0 wake only from released
void solenoidFailsafeTick();

// Telemetry accessors.
bool solenoidGateOn();
uint32_t solenoidStrikes();
uint32_t solenoidBlocked();
uint32_t solenoidFailsafes();
uint16_t solenoidLastPulseMs();
