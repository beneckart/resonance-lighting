#pragma once

#include <stdint.h>

// The one-keypress confirm rail (ADR 0037 §6): every fleet-wide action passes
// through this modal before any packet leaves the radio. `origin` is shown so
// a Claude-initiated action is visibly distinct from an app button.

// UI-task path (app buttons): callback on confirm, silent close on cancel.
typedef void (*ConfirmYesFn)(void *user);
void uiConfirm(const char *summary, const char *origin, ConfirmYesFn onYes,
               void *user);

// Cross-task path (agent tools on the net task): blocks the CALLING task until
// the operator answers on-device or the timeout lapses. Focus lands on cancel.
enum class ConfirmVerdict : uint8_t { CONFIRMED, DENIED, TIMEOUT };
ConfirmVerdict uiConfirmBlocking(const char *summary, const char *origin,
                                 uint32_t timeoutMs);

// UI-task only: surfaces pending cross-task requests (called by a timer).
void uiConfirmPollTick();
