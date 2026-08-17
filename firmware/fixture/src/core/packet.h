// ESP-NOW wire protocol v1 (packed, little-endian). Extracted VERBATIM from
// firmware/net_bench/net_bench.ino -- this is the fleet wire contract; the 26
// commissioned bench units and all host tooling (net_bench_log.py,
// net_bench_dashboard.py, serial bridge masters) parse these exact layouts.
//
// ============================= TYPE REGISTRY =================================
// This enum is the single allocation authority for NbType values. Rules:
//   - NEVER reuse or renumber a type. NB_PROTO_VER stays 1; a header change is
//     the only thing that would ever bump it (flag day -- avoid).
//   - Struct evolution is APPEND-ONLY: receivers length-check with
//     NB_HAS_HB_FIELD-style gates and ignore unknown tails, which also makes
//     send-side truncation at any tail boundary valid (see hb-short).
//   - 1..17  net_bench era (bench masters still send/understand these).
//   - 18..24 fixture era (production behavior layer).
//   - 20/22/23 are RESERVED: struct defined, parse-stubbed, NOT sent yet.
//   - 25..26 cambium era (browser-sim serial bridge streaming).
//   - 27+    free.
// =============================================================================
//
// Native-testable: no Arduino includes. test_packet_layout.cpp pins golden
// sizeof/offsetof values so an accidental insert/reorder fails at build time.
#pragma once

#include <stdint.h>
#include <stddef.h>
#include <string.h>

#define NB_PROTO_VER 1

enum NbType : uint8_t {
  NB_HEARTBEAT = 1,   // peer -> all: state + telemetry summary (append-only tails)
  NB_SHOWFRAME = 2,   // bridge -> all: show command (append-only tail added, era 18+)
  NB_ENTER_MAINT = 3, // bridge -> all: ADR-0010 metadata, "enter maintenance"
  NB_RESUME = 4,      // bridge -> all: leave maintenance
  NB_SET_RATE = 5,    // bridge -> all: set heartbeat/frame rate (Hz)
  NB_IDENTIFY = 6,    // bridge -> target (or all): locate blink (+color tail, era 18+)
  NB_SCANAP = 7,      // field peer -> bridge: one scanned 2.4 GHz AP (bench-only; not sent by fixture)
  NB_SET_MAINTAIN = 8,   // bridge -> all: charger VINDPM/maintain V (v10)
  NB_SET_CAPACITY = 9,   // bridge -> all: persist battery capacity (mAh), reboot to apply
  NB_SET_CHARGE_MA = 10, // bridge -> all: persist/apply charger current cap (mA)
  NB_SLEEP_FOR = 11,     // bridge -> all: cut rails and timed deep-sleep (seconds)
  NB_DRAWDOWN = 12,      // bench-only drawdown load (fixture: not implemented, ignored)
  NB_TARGET_SLEEP_FOR = 13,   // bridge -> target: timed deep-sleep
  NB_TARGET_CAPACITY = 14,    // bridge -> target: persist capacity, reboot to apply
  NB_TARGET_CHARGE_MA = 15,   // bridge -> target: persist/apply charge cap
  NB_TARGET_ENTER_MAINT = 16, // bridge -> target: enter shared-WiFi maintenance/OTA
  NB_TARGET_SOLENOID = 17,    // bridge -> target: one bounded D7 gate pulse (ms)
  // ---- fixture era ----------------------------------------------------------
  NB_CHOREO_STATE = 18,  // peer -> all: compact fast show/CA state (NIGHT only)
  NB_PROGRAM_SET = 19,   // bridge -> all/target: program override lease
  NB_TIME_QUALITY = 20,  // RESERVED (ADR 0031 time anchors) -- defined, not sent
  NB_PROFILE = 21,       // bridge -> all/target: commission/field profile flip
  NB_NEIGHBOR_REPORT = 22, // RESERVED (M2 locate: censored-median RSSI) -- defined, not sent
  NB_EVENT = 23,         // RESERVED (M2 event fabric) -- defined, not sent
  NB_NEIGHBOR_SET = 24,  // bridge -> target: pinned CA adjacency (<=8 neighbors)
  // ---- cambium era ----------------------------------------------------------
  NB_DIRECT_FRAME = 25,    // bridge -> all: per-fixture RGBW (ids ride IN the entries)
  NB_FORCE_LIFECYCLE = 26, // bridge -> all/target: force day/night/auto (RAM-only)
};

struct __attribute__((packed)) NbHeader {
  uint8_t ver;
  uint8_t type;
  uint8_t src_id[3];
  uint32_t seq;
  uint32_t uptime_ms;
};

struct __attribute__((packed)) NbShowFrame {
  NbHeader h;
  uint16_t phase;
  uint8_t hue;
  uint8_t flags; // bit0 (era 18+): implicit 10 s micro-lease for PROG_BRIDGE_SHOW
  // --- APPEND-ONLY tail 1 (fixture era). Old net_bench masters send 17 B
  // frames (no tail); receivers length-check and default the tail.
  uint8_t val;        // HSV value 0-255 (0 -> receiver treats as 255 for parity)
  uint8_t bright;     // global brightness request, clamped by local PowerBudget
  uint8_t effect;     // class-mapped effect selector (0 = default mapping)
  uint8_t beat_phase; // DJ/audio beat phase 0-255 wrapping
  uint8_t energy;     // audio/show energy 0-255
};

struct __attribute__((packed)) NbHeartbeat {
  NbHeader h;
  int16_t batt_mv;
  int16_t batt_ma;
  uint8_t soc_pct;
  uint8_t reset_reason; // (uint8_t)esp_reset_reason()
  uint8_t ca_state;     // mirrors the active program's compact state (GH state)
  uint8_t mode;
  uint16_t dl_pdr_x1000; // bridge-multicast PDR as seen by this peer
  int8_t dl_rssi;        // RSSI of bridge frames at this peer
  // --- APPEND-ONLY below. New fields go at the END; receivers length-check
  // (NB_HAS_HB_FIELD) and ignore unknown tails. NEVER reorder/insert. The
  // fixture sends two lengths of this same struct:
  //   hb-short: truncated after tail 1 (supply_good) -- the 0.2 Hz cadence
  //   hb-full:  the whole struct -- every 60 s + boot/state change
  // Bench-only sources (env/INA/drawdown/MPPT) are sent zero/absent-sentinel
  // so old parsers keep decoding without a schema change.
  int16_t supply_mv;   // tail 1: panel/USB input voltage (mV)
  int16_t supply_ma;   // panel/USB input current INTO the board (mA)
  uint8_t supply_good; // charger considers the supply valid (0/1)
  // tail 2: env sensors (bench TSL2591/SHT31 -- fixture sends absent sentinels)
  uint32_t lux_x10;   // 0xFFFFFFFF = no sensor
  uint16_t light_ch0;
  uint16_t light_ch1;
  int16_t ptemp_cx10; // INT16_MIN = absent
  uint8_t prh_pct;    // 255 = absent
  int16_t btemp_cx10; // INT16_MIN = absent
  // tail 3: bench INA219 meters (instrument retired -- fixture sends INT16_MIN)
  int16_t ina_pv_mv;
  int16_t ina_pa_ma;
  int16_t ina_bv_mv;
  int16_t ina_ba_ma;
  // tail 4: runtime config (fixture: real values)
  uint16_t cfg_cap_mah;
  uint16_t cfg_charge_ma;
  // tail 5: bench drawdown (fixture sends zeros)
  uint16_t drawdown_mah_x10;
  uint16_t drawdown_budget_mah;
  uint8_t drawdown_active;
  // tail 6: identity (fixture: real value)
  char fw_rev[24]; // NUL-terminated
  // tail 7: maintenance health
  uint8_t maint_status;
  // tail 8: lifecycle summary (fixture reuses these for the production
  // lifecycle: field_phase carries the lifecycle state enum)
  uint8_t field_phase;
  uint8_t field_reason;
  uint16_t field_cycle;
  uint16_t field_elapsed_s;
  uint16_t field_charge_mah;
  uint16_t field_discharge_mah;
  uint16_t field_min_mv;
  uint16_t field_max_mv;
  // tail 9: BQ25628E charger truth (fixture: real values)
  uint16_t bq_vindpm_mv; // 0xFFFF = unknown
  uint16_t bq_ichg_ma;   // 0xFFFF = unknown
  uint16_t bq_vreg_mv;   // 0xFFFF = unknown
  uint8_t bq_reg16;
  uint8_t bq_reg18;
  uint8_t bq_stat0;
  uint8_t bq_stat1;
  uint8_t bq_fault0;
  uint8_t bq_flag0;
  uint8_t bq_flag1;
  uint8_t bq_fault_flag0;
  uint8_t bq_part;
  // tail 10: energy summary
  uint16_t field_charge_wh_x10;
  uint16_t field_discharge_wh_x10;
  uint16_t field_peak_panel_w_x100;
  uint16_t field_peak_charge_w_x100;
  uint16_t field_peak_draw_w_x100;
  uint8_t field_low_s;
  uint8_t field_charge_min;
  uint8_t field_wait_min;
  uint8_t field_draw_min;
  uint8_t field_protect_min;
  // tail 11: bench MPPT perturb (fixture sends zeros/disabled)
  uint8_t mppt_status;
  uint8_t mppt_reason;
  uint8_t mppt_runs;
  uint8_t mppt_active_v10;
  uint8_t mppt_best_v10;
  uint8_t mppt_last_v10;
  uint16_t mppt_p46_w_x100;
  uint16_t mppt_p48_w_x100;
  uint16_t mppt_p50_w_x100;
  // tail 12: low-voltage latches
  uint8_t field_load_dimmed;
  uint8_t field_protect_latched;
  // tail 13 (fixture era, slow diagnostics -- appears only in hb-full)
  uint8_t profile;        // FixtureProfile
  uint8_t life_state;     // lifecycle state enum
  uint8_t power_tier;     // LedTier
  uint8_t active_program; // choreo program id
  uint16_t night_min;     // minutes into NIGHT_SHOW (bounded-night observability)
  // tail 14 (fleet dashboard output truth -- appears only in hb-full)
  uint8_t fixture_class;  // FixtureClass; 0 = unknown
  uint8_t led_rail_on;    // physical switchable 3V3 rail state (0/1)
  uint8_t led_r;          // mean post-gamma output across currently lit pixels
  uint8_t led_g;
  uint8_t led_b;
  uint8_t led_w;          // 0 on GRB HEX fixtures
  uint8_t led_lit_pixels; // number of nonzero pixels after cap/gamma
};

// Receiver-side tail gate: does a packet of length `len` include `field`?
#define NB_HAS_HB_FIELD(len, field) \
  ((len) >= (int)(offsetof(NbHeartbeat, field) + sizeof(((NbHeartbeat *)0)->field)))

// Send-side truncation boundaries (valid because receivers use NB_HAS_HB_FIELD).
#define NB_HB_SHORT_LEN (offsetof(NbHeartbeat, supply_good) + sizeof(uint8_t))
#define NB_HB_FULL_LEN sizeof(NbHeartbeat)

struct __attribute__((packed)) NbCmd { // ENTER_MAINT / RESUME / SET_RATE / SET_MAINTAIN
  NbHeader h;
  uint8_t arg;
};
struct __attribute__((packed)) NbSetU16 {
  NbHeader h;
  uint16_t value;
};
struct __attribute__((packed)) NbTargetU16 {
  NbHeader h;
  uint8_t target_id[3]; // 00:00:00 = all; otherwise node short ID
  uint16_t value;
};
struct __attribute__((packed)) NbTargetCmd {
  NbHeader h;
  uint8_t target_id[3];
  uint8_t arg;
};
struct __attribute__((packed)) NbIdentify { // locate a board (target 00:00:00 = all)
  NbHeader h;
  uint8_t target_id[3];
  uint8_t secs;
  // --- APPEND-ONLY tail 1 (fixture era): rig color-identify. Old peers get the
  // 17 B form and blink "..-" as before; fixture peers with the tail display the
  // assigned color so ~10 units/row can be physically ordered by eye (2x10 rig).
  uint8_t color; // 0=none(blink pattern) 1=R 2=G 3=B 4=Y 5=W
  uint8_t blink; // 0=solid 1=blinking (doubles the distinguishable identities)
};
struct __attribute__((packed)) NbScanAp { // bench-era; fixture parses (ignores), never sends
  NbHeader h;
  uint8_t scan_id;
  uint8_t idx;
  uint8_t count;
  uint8_t bssid[6];
  int8_t ap_rssi;
  uint8_t channel;
  uint8_t enc;
  char ssid[20];
};

// ---- fixture-era payloads ---------------------------------------------------

struct __attribute__((packed)) NbChoreoState { // 18: fast show/CA state, NIGHT only
  NbHeader h;
  uint8_t program_id;
  uint16_t generation; // CA generation / timeline frame (observability + sync A/B)
  uint8_t state;       // bits 0-3: program state (GH: 0=quiescent 1=excited 2+=refractory); 4-7 program-defined
  uint8_t intensity;   // current render energy 0-255 (neighbor/bridge viz)
  uint16_t phase_ms;   // ms into current program cycle (mod 65536)
  uint8_t flags;       // bit0=power-limited bit1=lease-active bit2=commission profile
  uint8_t reserved;
};

struct __attribute__((packed)) NbProgramSet { // 19: bridge lease
  NbHeader h;
  uint8_t target_id[3]; // 00:00:00 = all
  uint8_t program_id;   // 0 = release lease (return to autonomous)
  uint16_t lease_s;     // TTL from receipt; 0 = release
  uint32_t seed;
  uint8_t flags;     // bit0=hard-cut (no crossfade) bit1=params-are-delta
  uint8_t params[8]; // program-defined
};

struct __attribute__((packed)) NbTimeQuality { // 20: RESERVED (ADR 0031 seam)
  NbHeader h;
  uint32_t utc_s;
  uint16_t sub_ms;
  uint8_t source; // 0=none 1=gps 2=rtc 3=peer 4=bridge
  uint8_t hops;
  uint16_t age_s;
  uint16_t uncert_ms;
  uint16_t boot_id;
  uint8_t flags;
  uint8_t reserved;
};

struct __attribute__((packed)) NbProfile { // 21: commission/field flip
  NbHeader h;
  uint8_t target_id[3]; // 00:00:00 = all
  uint8_t profile;      // FixtureProfile
  uint8_t flags;        // bit0=persist to NVS (else RAM-only until reboot)
};

#define NB_NEIGHBOR_REPORT_MAX 16
struct __attribute__((packed)) NbNeighborEntry {
  uint8_t id[3];
  int8_t med_dbm;
  uint8_t n;     // samples received in the window
  uint8_t flags; // bit0=censored (median rank lost; treat as "at least this far")
};
struct __attribute__((packed)) NbNeighborReport { // 22: RESERVED (M2 locate)
  NbHeader h;
  uint8_t count;
  uint16_t n_expected; // beacons sent per window at the known fixed rate
  NbNeighborEntry entries[NB_NEIGHBOR_REPORT_MAX];
};

struct __attribute__((packed)) NbEvent { // 23: RESERVED (M2 event fabric)
  NbHeader h;
  uint32_t event_id; // dedupe key
  uint32_t fire_in_ms;
  uint8_t kind;
  uint8_t hop_limit;
  uint8_t params[16];
};

#define NB_NEIGHBOR_SET_MAX 8
struct __attribute__((packed)) NbNeighborSet { // 24: pinned CA adjacency
  NbHeader h;
  uint8_t target_id[3]; // 00:00:00 = all (rarely useful; normally targeted)
  uint8_t flags;        // bit0=persist to NVS bit1=clear (revert to RSSI mode)
  uint8_t count;        // 0 with bit1 clear also means revert to RSSI mode
  uint8_t neighbor_ids[NB_NEIGHBOR_SET_MAX][3];
};

// ---- cambium-era payloads (browser sim -> serial bridge -> fleet) -----------

#define NB_DIRECT_MAX_ENTRIES 18
struct __attribute__((packed)) NbDirectEntry { // 7 B: one fixture's color
  uint8_t id[3];      // node short ID (never 00:00:00 -- there is no "all" entry)
  uint8_t r, g, b, w; // W ignored by GRB hex pixels, carried for wire truth
};
struct __attribute__((packed)) NbDirectFrame { // 25: per-fixture direct color
  NbHeader h;
  uint8_t flags; // bit0=10 s micro-lease grant (parallels NbShowFrame bit0)
                 // bit1=hard-cut (skip slew)
  uint8_t count; // entries actually present; wire length is 15 + 7*count, and
                 // receivers trust min(count, (len-15)/7) -- never sizeof
  NbDirectEntry entries[NB_DIRECT_MAX_ENTRIES]; // 18 caps the frame under the
                                                // 250 B ESP-NOW payload budget
};

struct __attribute__((packed)) NbForceLifecycle { // 26: bridge day/night override
  NbHeader h;
  uint8_t target_id[3]; // 00:00:00 = all
  uint8_t mode;  // 0=force day 1=force night 2=auto (mirrors serial 'N')
  uint8_t flags; // reserved 0. RAM-only BY DESIGN (no NVS mirror): a rebooting
                 // field unit must never stay forced.
};

// ---- helpers (pure; ESP-NOW send lives in esp32/espnow_link) ----------------

inline bool nbTargetMatches(const uint8_t target[3], const uint8_t myId[3]) {
  bool all = (target[0] == 0 && target[1] == 0 && target[2] == 0);
  return all || memcmp(target, myId, 3) == 0;
}

// Scan a DIRECT_FRAME's entries for `myId`. `len` is the raw rx length; the
// sender's `count` is clamped to what the wire actually carried, so a truncated
// or lying frame can never read past the packet. nullptr = frame not for us.
inline const NbDirectEntry *nbDirectFindEntry(const NbDirectFrame *f, int len,
                                              const uint8_t myId[3]) {
  int avail = (len - (int)offsetof(NbDirectFrame, entries)) / (int)sizeof(NbDirectEntry);
  int count = (int)f->count < avail ? (int)f->count : avail;
  for (int i = 0; i < count; i++)
    if (memcmp(f->entries[i].id, myId, 3) == 0) return &f->entries[i];
  return nullptr;
}
