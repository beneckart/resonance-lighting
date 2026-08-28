#pragma once

#include <stdint.h>

static constexpr int UI_SHELL_BAR_HEIGHT = 26;

// Permanent Bridge OS chrome. Screens own y=26..239; the shell owns the top
// row for app identity, passive status, active-control ribbon, and global Stop.
void uiShellStart();
void uiShellSetTitle(const char *title);

// Optional app-owned idle status. Control activity temporarily overrides it
// and the text returns after Stop/expiry. Passing null or empty restores the
// normal network/mesh/battery summary.
void uiShellSetAppStatus(const char *text, uint32_t rgb = 0);
