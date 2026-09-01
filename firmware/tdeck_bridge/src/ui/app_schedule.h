#pragma once

// UTC/schedule status plus RAM-only fleet baseline override. In the emergency
// inspection fixture image, Wake Fleet opens a bounded direct-frame control
// window, while Performance Hold refreshes that same arm for one hour. Both
// preserve the autonomous static-light fallback and fixture battery safety.
void appScheduleOpen();
