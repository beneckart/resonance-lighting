#pragma once

#include <stddef.h>
#include <stdint.h>

#include "../core/show_model.h"

struct RxItem;

// Tone Lab on site (no laptop): plays the patterns and runs the Atom button
// rules from a show file exported by ops/bench/decompression_tone_lab.py.
// Strikes and keep-awake leases go out through mesh_tx; Atom presses are heard
// directly (their NB_DECOMPRESSION_CONTROL PLAY frames). Sensor rules are not
// in the show: lanterns send no sensor data over radio.

void tonelabBegin();               // load the stored show, start the player task
void tonelabTick(uint32_t nowMs);  // loop(): settle Atom clicks, renew leases
void tonelabObserve(const RxItem &item);  // census drain: every received frame

// Replace the stored show (validated first; nothing changes on failure).
bool tonelabLoadShow(const char *text, size_t len, char *err, size_t errLen);

// Controls (safe from any task).
bool tonelabPlay(int pattern, uint8_t loops = 0);  // loops 0 = until stopped
void tonelabStop();
void tonelabSetAwake(bool on);
void tonelabSetActive(int pattern, bool active);  // the Active rotation
void tonelabSetRuleEnabled(int rule, bool enabled);
void tonelabRunRule(int rule);  // as if its click arrived (for testing)

struct TonelabStatus {
  bool loaded;
  char created[28];
  char site[28];
  size_t patterns, rules, lanterns;
  int playing;       // pattern index or -1
  int loopsLeft;     // 0 = until stopped
  uint32_t loopsDone;
  int resume;        // pattern a performance returns to, or -1
  bool awake;
  uint32_t presses;  // Atom presses heard
  char last[64];     // last click / action, for the screen
};
TonelabStatus tonelabStatus();
// Copy of a pattern / rule for display (false if out of range).
bool tonelabPattern(int i, ShowPattern &out);
bool tonelabRule(int i, ShowRule &out);
void tonelabPrintStatus();
