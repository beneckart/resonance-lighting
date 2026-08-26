#pragma once

// Exact-target selector for the commission profile's no-command fallback.
// Ready beacon, light-only wildfire CA, and strict rails-off dark may be
// applied until reboot or explicitly persisted. Field behavior is untouched.
void appCommissionOpen();
