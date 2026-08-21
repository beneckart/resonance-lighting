#pragma once

// The UI task owns every lv_* call (LVGL is not thread-safe): launcher grid,
// status bar, screen management, ~30 Hz lv_timer_handler. Other modules are
// read through the census_svc safe accessors — never written from here.
bool uiStart();     // lvgl init + launcher; false -> caller falls back to the
                    // raw status page (plan-B path)
bool uiActive();

// For apps: return to the (resident, never-deleted) launcher screen; the
// caller's screen is deleted. Apps compare against uiLauncherScreen() before
// deleting any "old" screen themselves.
#include <lvgl.h>
void uiGoHome();
lv_obj_t *uiLauncherScreen();
