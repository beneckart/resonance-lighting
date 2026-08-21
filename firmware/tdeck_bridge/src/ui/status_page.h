#pragma once

// M0 raw-LovyanGFX status page (replaced by the LVGL shell in M2).
// Also doubles as the direct-sun test surface: `sun` mode renders the
// high-contrast test pattern from the ADR 0037 M0 readability check.
void statusPageTick();        // redraws at ~5 Hz; cheap no-op between frames
void statusPageNoteKey(char c);
void statusPageSunTest(bool on);
