// Maintenance-mode WiFi OTA lifecycle (ADR 0010: standard WiFi OTA only;
// ESP-NOW carries metadata, never image bytes). Ported from net_bench with the
// master/bench branches removed -- the production image is peer-only.
//
// Contract strings fleet_usb_bringup.py and net_bench_ota.py regex against:
//   "maintenance WiFi up, ip=<a.b.c.d>"    (startWifiOta)
//   "Update complete. Rebooting."          (POST /update reply body)
// plus HTTP GET /telemetry, GET /resume, POST /update (multipart "firmware").
#pragma once

#include <stdint.h>

enum NetMode : uint8_t { MODE_COMMS = 0, MODE_MAINT = 1 };
enum MaintStatus : uint8_t {
  MAINT_STATUS_IDLE = 0,
  MAINT_STATUS_ACTIVE = 1,
  MAINT_STATUS_POWER_WARN = 2,
  MAINT_STATUS_START_FAILED = 3,
  MAINT_STATUS_TIMEOUT = 4,
  MAINT_STATUS_RESUMED = 5,
};

#define RES_MAINT_TIMEOUT_S 600
#define RES_MAINT_MIN_LFP_MV 3200
#define RES_MAINT_MIN_3V7_MV 3600
#define RES_MAINT_SUPPLY_OVERRIDE_MA 250
#define RES_MAINT_POWER_ENFORCE 0 // advisory: report low-power risk, don't strand a unit
#define RES_MAINT_REFUSE_REPORT_MS 5000

NetMode maintMode();
uint8_t maintStatus();

bool enterMaintenance(); // loads off -> ESP-NOW down -> WiFi join -> serve OTA
void enterComms();       // WiFi down -> STA unassociated on channel -> ESP-NOW up
bool maintenancePowerOk();

// Loop tick while in MODE_MAINT: serves HTTP, honors /resume, auto-resumes
// after RES_MAINT_TIMEOUT_S so a missed OTA can't strand a fixture on WiFi.
void maintenanceTick();
