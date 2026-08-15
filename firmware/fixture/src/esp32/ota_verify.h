// Deferred OTA self-verification (ADR 0010 hardening, previously unimplemented
// anywhere in the tree). CONFIG_APP_ROLLBACK_ENABLE=y ships in the stock
// esp32s3 Arduino libs; the weak hooks live in a C file, so the overrides MUST
// be extern "C" -- a mangled C++ override silently never runs and a bad image
// sticks (POWERFEATHER_NOTES).
//
// Flow: verifyRollbackLater() returns true -> initArduino leaves the image
// PENDING_VERIFY -> otaVerifyTick() runs the self-test once at uptime > 20 s
// -> mark valid, or mark invalid + reboot (bootloader reverts to last-good).
// A watchdog/panic before the mark also reverts: hangs roll back for free.
#pragma once

void otaVerifyTick();       // call from loop
bool otaVerifyPending();    // true until the image is marked valid (blocks sleep)
