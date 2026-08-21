#include <lvgl.h>

#include <Arduino.h>
#include <stdio.h>
#include <string.h>

#include "lvgl_glue.h"
#include "ui_confirm.h"

static ConfirmYesFn gOnYes = nullptr;
static void *gUser = nullptr;
static lv_obj_t *gBox = nullptr;
static bool gBoxIsCrossTask = false;

// Cross-task request slot (net task <-> ui task).
static volatile int gXPhase = 0;    // 0 idle, 1 requested, 2 shown
static volatile int gXVerdict = 0;  // 0 pending, 1 yes, 2 no
static volatile bool gXCancel = false;
static char gXSummary[120];
static char gXOrigin[24];

static void closeBox() {
  if (gBox) {
    lv_msgbox_close(gBox);
    gBox = nullptr;
  }
}

static void yesCb(lv_event_t *) {
  if (gBoxIsCrossTask) {
    gXVerdict = 1;
    gXPhase = 0;
  } else if (gOnYes) {
    ConfirmYesFn fn = gOnYes;
    void *user = gUser;
    gOnYes = nullptr;
    closeBox();
    fn(user);
    return;
  }
  gOnYes = nullptr;
  closeBox();
}

static void noCb(lv_event_t *) {
  if (gBoxIsCrossTask) {
    gXVerdict = 2;
    gXPhase = 0;
  }
  gOnYes = nullptr;
  closeBox();
}

static void showBox(const char *summary, const char *origin, bool crossTask) {
  gBoxIsCrossTask = crossTask;
  gBox = lv_msgbox_create(nullptr);
  char title[48];
  snprintf(title, sizeof(title), "%s asks:", origin);
  lv_msgbox_add_title(gBox, title);
  lv_msgbox_add_text(gBox, summary);
  lv_obj_t *yes = lv_msgbox_add_footer_button(gBox, LV_SYMBOL_OK " confirm");
  lv_obj_t *no = lv_msgbox_add_footer_button(gBox, LV_SYMBOL_CLOSE " cancel");
  lv_obj_add_event_cb(yes, yesCb, LV_EVENT_CLICKED, nullptr);
  lv_obj_add_event_cb(no, noCb, LV_EVENT_CLICKED, nullptr);
  // Focus lands on CANCEL: confirming takes a deliberate move + press.
  lv_group_add_obj(lvglGroup(), yes);
  lv_group_add_obj(lvglGroup(), no);
  lv_group_focus_obj(no);
}

void uiConfirm(const char *summary, const char *origin, ConfirmYesFn onYes,
               void *user) {
  if (gBox) return;  // one confirm in flight; a second request is dropped
  gOnYes = onYes;
  gUser = user;
  showBox(summary, origin, false);
}

void uiConfirmPollTick() {
  if (gXCancel) {  // requester gave up (timeout); tear the modal down
    gXCancel = false;
    if (gBox && gBoxIsCrossTask) closeBox();
  }
  if (gXPhase == 1 && !gBox) {
    gXPhase = 2;
    showBox(gXSummary, gXOrigin, true);
  }
}

ConfirmVerdict uiConfirmBlocking(const char *summary, const char *origin,
                                 uint32_t timeoutMs) {
  if (gXPhase != 0) return ConfirmVerdict::TIMEOUT;  // rail busy
  strlcpy(gXSummary, summary, sizeof(gXSummary));
  strlcpy(gXOrigin, origin, sizeof(gXOrigin));
  gXVerdict = 0;
  gXPhase = 1;
  uint32_t t0 = millis();
  while (millis() - t0 < timeoutMs) {
    if (gXVerdict == 1) return ConfirmVerdict::CONFIRMED;
    if (gXVerdict == 2) return ConfirmVerdict::DENIED;
    vTaskDelay(pdMS_TO_TICKS(50));
  }
  gXPhase = 0;
  gXCancel = true;  // ui task closes the stale modal
  return ConfirmVerdict::TIMEOUT;
}
