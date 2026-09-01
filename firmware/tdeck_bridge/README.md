# tdeck_bridge — Resonance Bridge OS (LilyGO T-Deck Plus)

App-launcher handheld per ADR 0047 (platform of record; constraints from ADR
0037): simultaneously an ESP-NOW mesh citizen on the fleet channel (census +
command TX) and a Wi-Fi STA (Claude API over TLS, laptop services).

Field operators and IT support should start with the illustrated
[`Bridge OS field manual`](../../docs/howto/BRIDGE_OS_FIELD_MANUAL.md). This
README remains the implementation, build, and acceptance record.

**Status 2026-08-27:** M0-M4 complete and hardware-verified. Working apps:
**Claude** (streaming chat + 6-tool agent loop with the confirm rail),
**Fleet** (stable roster/live views, class, raw-VBAT, charge-phase, program,
and exact-firmware filters, sortable
voltage, reported-color chips, detail/identify, and confirmed filtered-cohort
blink), **Health** (single-screen voltage or charge-phase grid for the
production registry plus live node detail), **LED Studio** (class-targeted solid colors and 1 Hz
cohort blink via sustained 8 Hz direct-frame streaming, client-side dim),
**Blackout / Sleep** (confirmed 10 min or 1-12 h in one-hour steps; blackout
leases or rails-off timer sleep), **Knocker** (single strike plus selectable targeted
roll, immediate fleet multicast, and shared +1.0 s deadline multicast),
**Wake / Schedule** (GPS UTC status plus Auto / Wake Fleet / Night Show),
**CA Studio** (Greenberg-Hastings wildfire with light or daytime knock output),
**Contagion** (Color Virus or Epidemic with light, knock, or both output and an
exact manual seed),
**Default** (exact-target commission fallback: ready beacon, light CA, or dark),
**Patterns** (manual deterministic RGBW streaming),
**RF Diagnostics** (read-only mesh survey), **Settings**, **SunTest**.
Remaining: Locate, detailed sensor reports, ES7210 audio-reactive Patterns,
voice (whisperd), and polish (M5 tail + M6).

## Permanent control shell

Bridge OS permanently reserves the top 26 pixels on LVGL's top layer. Every app
uses y=26..239, so control status never appears over an app title, picker, or
action button and starting or stopping activity never shifts the layout. The
left cell names the current app. The center normally shows app or network/mesh
status; while this T-Deck owns a direct stream or tracked program lease it
becomes a scrolling activity ribbon. Direct streams show `LOCAL`, mode, target
count, and `until STOP`. A separate fixed clock cell shows elapsed time for a
permanent stream or remaining time for an expiring program, so changing digits
cannot restart the ribbon animation. A fixed Stop button appears at the right only for local
activity, ending the stream and releasing the tracked program without requiring
the operator to return to the originating app.
A Contagion stop also disables its old-fleet compatibility fanout.

The same shell passively inspects the existing fleet packet contract for the
newest active non-self controller. It names known T-Deck (`8EB508`, `979604`),
PUCA (`A4EB10`), and CoreS3 (`4D5DB0`, historical `E39F1C`) publishers and
shows the exact short ID for an unknown controller. Direct/show/lifecycle
traffic stays active while fresh for three seconds; program activity follows
the command's wire lease. Foreign-only status is informational and has no Stop
button; simultaneous local and foreign activity uses the conflict color. This
adds no packet or probe traffic.

The first full-width bottom strip was rejected on hardware because it covered
each app's operational action row. A compact top-left pill proved the ribbon
concept but still covered titles. The permanent shell replaces both overlays;
all implemented screens were migrated below its fixed boundary. This revision
is flashed on the second T-Deck, physically labelled `TSwift` (`979604`):
1,565,280 bytes, SHA-256
`3267a1b237a2a4708e5c999cf1730f497ecbc30bd80e299a8b76374764931d4c`.
Its cross-app touch, expiry, and competing-publisher behavior remain in the
hardware acceptance list in `TODO.md`.

The Default app is present in the current combined `8EB508` image but is not yet
hardware-validated. It
never broadcasts a persistent mutation: `ALL: targeted fresh` walks the fresh
census by deterministic short ID. `until reboot` changes RAM only; `persist
after reboot` writes fixture NVS after confirmation. The setting affects only
commission profile and never changes field scheduling. Active LED/program
leases still override it until release or expiry.

LED Studio white is semantic rather than a raw fourth-channel command.
Downlights retain dedicated-W white; perimeter, uplight, and chandelier classes
receive full `R=G=B` with `W=0`, so deployed three-channel RGB modules illuminate
correctly. This source fix is also pending mixed-hardware validation.

The combined Health, UTC/Schedule, Patterns v1, and RF Diagnostics image is
USB-flashed to exact T-Deck `8EB508` as `tdeck-dev-local`: 1,542,448 bytes,
binary SHA-256
`705119167e51ae8dff399a6c46cfd442b1610d14d0acb5d8a470c63461242b46`.
The upload verified, all onboard peripheral probes passed, and the bridge
rejoined channel 11 with live fleet receive and zero observed TX failures. Ben
reports the preceding Health/Schedule physical smoke check looked good; the
explicit acceptance matrices remain in `TODO.md`.

The current source checkpoint adds permanent operator callsigns, recognizes
`Thor [F40344]` as the protected one-off `magic_wand` role, and reconciles seven
live field fixtures that were missing from the canonical registry. The resulting
141-fixture roster is USB-flashed to exact T-Deck `8EB508` in a 1,550,224-byte
binary, SHA-256
`3026593615bd58304c2a6b8893bf4f92cd8f9f92211f9222a5a28517fedf6e32`.
The upload verified, all onboard peripheral probes passed, all seven reconciled
IDs returned fresh heartbeats, and the bridge rejoined the channel 11 mesh with
zero send failures. On-screen callsign and named-command hardware acceptance
remain open in `TODO.md`.

Ben field-smoke-tested LED Studio, Sleep / Dark, Knocker, and CA Studio on the
night of 2026-08-23/24; all behaved as designed. That run exposed the old
Knocker `knock all` behavior: it selected at most 32 fresh fixtures in
heartbeat order and then dispatched a per-ID targeted request every 300 ms.
Current P0 source replaces it with an honestly labelled, deterministic 80 ms
targeted roll over the full 192-entry census. The picker also offers one
immediate fleet multicast and one shared +1.0 s multicast deadline using the
existing `NbEvent.fire_in_ms` wire field. Updated fixtures deduplicate repeated
copies, timestamp radio receipt, arm only one pending strike, and refuse a
strike more than 250 ms late. Under ADR 0065 every deliberate Knocker mode
bypasses lifecycle/solar/tier qualification while retaining fixture-local arm,
pulse, rest, maintenance, durable load-marker, and failsafe gates. The full
native suites and both embedded development builds pass; isolated hardware
timing and mixed-firmware checks remain before flashing this UI.

**Board: T-Deck Plus, LCD variant — NOT the T-Deck Pro** (e-paper; different
touch/keyboard drivers; the names are one word apart and that is the easiest
available mistake — ADR 0037 §10).

Known handheld identities are primary Bridge OS `8EB508`
(`44:1B:F6:8E:B5:08`) and camp-labelled `TSwift` `979604`
(`44:1B:F6:97:96:04`). Treat COM ports only as observations; TSwift was on
COM157 for the 2026-08-27 banner/charge-filter flash.

## Hardware verdicts (probed on real hardware, 2026-08-19)

| Peripheral | Verdict |
|---|---|
| PSRAM | 8 MB OPI, detected (`psram=1(8388608)`) |
| Keyboard aux MCU | present at I2C 0x55 |
| GT911 touch | present (0x5D) |
| ES7210 mic ADC | **present at 0x40** — Patterns audio reactivity is unblocked |
| GPS | **NMEA at 38400 baud**, pins as named in `pins_tdeck.h` (not swapped) |
| Display | ST7789 320x240 via hand-configured LovyanGFX, PSRAM canvas |
| First build | 1.04 MB flash (33% of 3 MB app), 50.8 KB static RAM, heap free ~263 KB, PSRAM free ~8.2 MB |

## Build / flash

```bash
firmware/tdeck_bridge/build.sh --dev-cache           # fast locked local build
firmware/tdeck_bridge/build.sh --dev-cache --port COM152  # one named USB target
firmware/tdeck_bridge/build.sh                       # fresh retained build
firmware/tdeck_bridge/tests/run_tests.sh             # wrapper + native tests
```

- FQBN is pinned in `build.sh`: generic `esp32:esp32:esp32s3` with
  `FlashMode=qio,FlashSize=16M,PSRAM=opi` (ESP32-S3FN16R8 — a wrong PSRAM mode
  boot-loops), `PartitionScheme=app3M_fat9M_16MB`, USB CDC on boot.
- Fresh builds get a unique `--build-path`; an explicit retained path must be
  new or empty. Never resume a killed build directory.
  T-Deck cold builds vary sharply with host load and have taken about 51-55
  minutes on this laptop -- wait, don't restart.
- `--dev-cache` is the opt-in local iteration path ported from the accepted
  fixture cache. It is single-writer, recipe-pinned to the FQBN, flags,
  Arduino/ESP32 versions, LVGL, and LovyanGFX, and always reports
  `tdeck-dev-local`. Arduino owns `build/dev-cache`; wrapper recipe and
  interruption state live separately in `build/dev-cache.state`, so an Arduino
  source-graph cleanup cannot erase the cache identity or safety marker. The
  first schema-2 seed is still a cold build; subsequent no-op and leaf builds
  reuse completed objects. On 2026-08-27, a 3,051-second seed followed by an
  unchanged 143-second build and a final 145-second confirmation (about 21x
  faster); the warm runs changed no object timestamps and reproduced the exact
  binary SHA. A regression simulates Arduino deleting all internal build-path
  entries and proves this boundary, and a legacy schema-1 interruption marker
  still fails closed into the recovery path.
- If a cached build is interrupted, first confirm no Arduino, Xtensa, or
  esptool process remains, then run `build.sh --recover-dev-cache`. Recovery
  quarantines the Arduino cache, wrapper state, and lock; it never resumes
  partial objects. Use `build.sh --clean-dev-cache` only for a healthy unlocked
  cache; it removes both the cache and sibling state.
- The wire contract is included as `fixture/src/core/packet.h` via `-I` to the
  firmware root. One contract, one file; `tests/test_packet_include.cpp` pins
  the golden sizes on the native side.

## Provisioning (NVS only — no secret is compiled in or committed)

Serial 115200, line-based:

```
set wifi <ssid> <psk>     # ssid/psk must each be one token
set key <anthropic-key>
set model claude-sonnet-5
set channel 11            # mesh channel; 11 = commissioned fleet
set display day|night     # reboot applies the complete theme
set bl 200                # saved night backlight, 0-255; day stays at 255
wifi retry | wifi off
show                      # api key redacted
probe | mem | reboot | help
```

For daylight fixture interaction checks, `A<ID>[:secs]` grants one exact
fixture a self-expiring CA lease (180 s by default, capped at 900 s) without
waking the rest of the fleet; `A<ID>:0` releases it early. The command remains
available over USB when the handheld has no WAN connection.

## Channel guard (ADR 0036/0037)

One 2.4 GHz radio; in STA mode the AP picks the channel. On association the
firmware compares the AP channel to the stored mesh channel:

- match → ONLINE (SNTP starts, mesh stays up alongside)
- mismatch → **Wi-Fi dropped, mesh kept**, radio re-pinned, red
  `GUARD:ch-mismatch` on the status page. No auto-retry against a wrong-channel
AP - fix the AP (`docs/howto/CAMP_NETWORK_SETUP.md`), then `wifi retry`.

## Field LED Studio and rest controls (ADR 0048, ADR 0064)

Open **LEDs** from the launcher (or **Rest** from Fleet):

- LED Studio targets all fresh fixtures or one reported class: downlights,
  perimeter, uplights, or chandelier. Pick a labelled color, set dim, and use
  solid or 1 Hz blink. The class map comes from full heartbeats and can take
  about 60 seconds to fill after bridge boot. Streams cover the complete
  192-entry census, not only the first screenful.
- Blackout is an expiring electrical-dark lease. The LED rail is off, but the
  ESP-NOW receiver stays awake and measured fleet draw remains roughly
  126-144 mA per fixture.
- Deep sleep cuts both switchable rails and the radio for the selected
  duration, then auto-wakes into normal behavior. It cannot be cancelled while
  the fixture is asleep. It uses the existing `NB_SLEEP_FOR` contract, not the
  transport-dark latch.
- The screen opens on the safer reversible choice: Blackout for 10 minutes.
  Deep sleep must be selected deliberately.
- Both actions show live/seen counts and require the on-device confirmation
  modal, with focus on cancel. Only currently listening fixtures can receive a
  broadcast. Starting either action stops any suspended LED Studio stream.
- Sleep remains absent from the Claude tool schema and serial quick commands;
  it is reachable only from the local physical UI.
- Bridge OS retains the newest four Sleep/Blackout/Release/Schedule actions in a
  checksummed NVS ring before sending RF. `show` prints the ring; `nb-master`
  exposes its newest entry. Availability-reducing actions are not sent if the
  audit write fails. Release remains restorative and may still transmit.

## Fleet list, filtering, and identify

Open **Fleet** for the detailed scrollable list. Its default view is the full
production registry plus any unexpected live peer, sorted alphabetically by
callsign. Registry fixtures keep their row while off air, so a two-second
telemetry refresh updates values without moving the operator's place. Grey
rows are retained/off-air; `inf` means the current bridge has not observed
that registry identity. The selected identity and scroll context also survive
refresh and a round trip through node detail.

The compact row keeps the operational values that matter most at a glance:
raw VBAT, signed battery current (`+` charging, `-` discharging), age, and active
program. Signed current appears only when tail 17 proves the MAX17260 sample is
post-guard and fresh enough; `-` means unverified, including all old firmware.
RSSI/PDR and advisory gauge SOC remain on detail. A literal `idle` requires an
observed full-heartbeat state; `?` means the program is unknown.
Full-heartbeat state is retained across intervening short heartbeats, but is
cleared on sender reboot until the new boot reports it.

Press **View** to choose independently:

- rows: registry plus live, every peer seen since bridge boot, or live now;
- class: all, downlight, perimeter, uplight, chandelier, or unknown;
- raw-VBAT band: all, good (>3.20 V), near low (>3.10 V and <=3.20 V), low
  (<=3.10 V), off air, or live with no plausible battery voltage;
- charger phase: all, `CHARGING_CC`, `CHARGING_CV`, `TOP-OFF`, `DONE/OFF`,
  `FAULT`, unknown, or off air;
- program: all, IDLE, CA, BRIDGE, DIRECT, DARK, VIRUS, or unknown;
- firmware: all, known, unknown, exact selected reference, or everything that
  does not match that reference (including unknown revision evidence); and
- sort: stable callsign, stable short ID, voltage low/high first, most recent,
  or strongest signal.

Detail spells out profile, lifecycle, program, network mode, power tier, and
charger phase as names such as `FIELD`, `DAY_CHARGE`, `DIRECT`, `COMMS`,
`PROTECT`, `CHARGING_CC`, `CHARGING_CV`, `TOP-OFF`, `DONE/OFF`, and `FAULT`
instead of showing only numeric status codes.

Voltage, age, and signal sorts are explicit operator choices; the default does
not reorder on heartbeat arrival. A live class report is authoritative. While
a fixture is absent or awaiting a full heartbeat, a known registry role can
supply its class for filtering.

Bridge OS defaults to a full-brightness day mode with light, high-contrast Fleet
cells and dark text. Press the top-right `DAY`/`NITE` button for a quick toggle,
or use Settings. Night mode restores the saved night-backlight level and uses
explicit dark-table colors; table label contrast is deterministic in both modes
rather than inherited from the LVGL theme.

Press **Blink** to snapshot only the fresh rows currently passing every filter.
The confirmation modal names the exact count and focuses cancel. On confirm,
Bridge OS sends a paced exact-target green 30-second identify to that cohort;
it does not broadcast to hidden or off-air fixtures and does not claim that an
unreachable row changed. Single-row detail retains its existing ten-second
green identify. **Power** opens Blackout / Sleep, which also contains Release.

This Fleet work changes only the handheld view and the existing bounded
identify path. It adds no packet type and performs no OTA, reboot, profile,
lifecycle, or fixture-NVS mutation.

The complete native Bridge suite and a local ESP32-S3 build pass. The
`tdeck-dev-local` binary is 1,579,040 bytes with SHA-256
`473510ba76ec5ee9ce47e76575556ac0a7783c78445d913548473e0b3d4b819a`;
the linker reports 50% flash and 44% global RAM use. This exact image is flashed
on primary T-Deck `8EB508`; esptool verified each written region and a complete
application-region readback matched the SHA-256 above. Post-reset channel 11,
mesh traffic, peripheral probes, and memory telemetry passed. Physical dropdown
layout, day/night sunlight readability, stable scrolling, input, 192-row
memory-watermark, and filtered named-canary identify checks remain open in
`TODO.md`.

The current combined source is now flashed on exact T-Deck `8EB508` as a
1,581,168-byte `tdeck-dev-local` binary, SHA-256
`c87b2805feb8bd95c0d6c9ae3022baaa40079483bca652de6c33f738c0e69e7e`.
It includes the DAY/NIGHT display work, Fleet VBAT/signed-IBAT and rollout
filters, Default app, and current semantic-white planner. Upload plus an
independent whole-application `verify-flash` digest comparison passed. After
reset the bridge reported channel 11, DAY mode, healthy peripherals, live mesh
receive, and zero send failures. Physical feature acceptance remains in
`TODO.md`.

The ADR 0064 power-truth follow-up is USB-flashed on exact T-Deck `8EB508` as a
1,583,344-byte `tdeck-dev-local` binary, SHA-256
`3bc13ca8a8bfeb60aaa31d349721b3760522dc1600f3913dd145400fe1ef905c`.
It retains the preceding display/Fleet/filter work and adds validated-IBAT
rendering, human BQ charge phase, the Health VBAT/CHG toggle, and unambiguous
Wake/Blackout/Deep-sleep wording. Esptool verified every written region;
post-reset serial reported exact identity `8EB508`, live channel-11 time
traffic, and a running bridge. The complete native suite and embedded build
pass. Physical display checks and a fixture canary remain open.

## Fleet Health

Open **Health** for the read-only, no-scroll fleet triage view. The normal
144-device production-health roster fits as fixed squares on one 320x240 screen;
the grid automatically compacts if unexpected live IDs expand it toward the
192-entry census limit. Registry positions stay stable in short-MAC order.

Press the top-right toggle to color the same stable tiles by `VBAT` or `CHG`.
VBAT mode uses:

- green: raw reported VBAT >3.20 V;
- yellow: raw reported VBAT >3.10 V and <=3.20 V;
- red: raw reported VBAT <=3.10 V;
- grey: a rostered fixture is not currently fresh/on-air; and
- blue: a fresh heartbeat has no plausible battery voltage.

CHG mode uses green for `CHARGING_CC`, cyan for `CHARGING_CV`, purple for
`TOP_OFF`, amber for `NOT_CHARGING/DONE`, brown for `CHARGE_DISABLED`, red for
charger fault, blue for unknown, and grey for off air. The status comes from
the BQ25628E phase/fault registers, not an IBAT threshold guess. Tap a tile for
the human phase plus validated signed IBAT and input voltage/current.

These are operator triage bands, not ADR 0023 lifecycle thresholds. The app
deliberately does not use gauge SOC as its primary color. Tap a square, or use
the trackball and click, for voltage/current, age, RSSI/PDR, supply, advisory
SOC, class/program/lifecycle, sensor signature, firmware, and registry details.
The app is read-only and adds no packet type or fixture command.

The embedded roster is generated from `ops/fleet/registry.csv`; it includes
PowerFeathers whose status is `commissioned` or `commission_failed`, and omits
quarantined, bench-only, merely enumerated/demo, and bridge hardware. Any live
ID outside that roster is appended with a cyan border. The native test wrapper
regenerates the header and fails if the checked-in snapshot is stale.

Source plus native tests and the merged embedded build pass. Health is flashed
on `8EB508` in the current 1,550,224-byte image documented above. Physical
layout, input, detail, and memory-watermark checks remain open in `TODO.md`.

## UTC and schedule controls (ADR 0049)

The T-Deck parses checksum-valid active GPS RMC date/time and broadcasts one
`NB_TIME_QUALITY` UTC anchor every two seconds. This cadence gives a field
fixture several chances to hear time during ADR 0064's approximately 12-second
minimum trustworthy-power window.

Open **Wake** for GPS/UTC status and four fleet controls:

- **Auto** returns to UTC civil twilight, with fixture solar/power fallback if
  trustworthy time expires;
- **Wake Fleet** catches timer wakes for six minutes and leaves each captured
  radio continuously reachable. On the emergency inspection fixture image it
  preserves Auto/static fallback and arms LED/audio direct frames. Campaign
  copies refresh the ten-minute arm while gathering, so the final copies leave
  about ten minutes of full-fleet control (about sixteen minutes maximum from
  the button press); older images retain the dark-day behavior;
- **Performance Hold** repeats that same inspection-safe Wake command for one
  hour, gathering sleepers and continually refreshing the fixture-owned
  ten-minute arm. The final copy leaves up to roughly ten minutes of bounded
  tail. The T-Deck shows the remaining campaign time;
- **Night Show** temporarily forces the nighttime baseline.

All four are RAM-only. Auto or Night Show immediately replaces Wake Fleet or
Performance Hold and closes inspection direct control. Campaign repetition
spans a full fixture sleep cadence. In the inspection image, direct LED/audio
frames are admitted only
during the bounded Wake/Performance window and do not extend it; program/show
modes stay disabled, static inspection white returns after direct-frame
staleness, and battery safety remains higher authority. Knock stays one-shot
and hard-mechanism-gated, so it is not promised as a wake command for a sleeping
fixture or as guaranteed physical motion.

Knocker's three fleet choices are deliberately distinct:

- **Targeted roll** sends one addressed command per fresh fixture at 80 ms
  intervals. It is deterministic but intentionally staggered.
- **Broadcast now** sends one logical multicast event to all updated, awake
  fixtures. Each receiver fires on its first copy, so arrival is asynchronous.
- **Sync +1.0 s** repeats that same logical event while decrementing
  `fire_in_ms` toward one bridge deadline. Each fixture schedules from its radio
  callback timestamp and duplicate event IDs cannot retrigger it.

The single-fixture picker leads with the permanent registry callsign and keeps
the authoritative short MAC beside it, for example `Luigi [F98CEF]`. A fresh
fixture absent from the production registry falls back to its short MAC.

The multicast modes require the corresponding fixture firmware; older images
ignore the new event kind. Operator knocks bypass energy qualification, not the
hard mechanism gates, and cannot wake a sleeping fixture.

## Wildfire CA

Open **CA** for the distributed Greenberg-Hastings wildfire. The operator now
chooses an actual CA output, **lights** or **knocks**, rather than internal
fixture program slots. Both modes use the same neighbor threshold, spontaneous
spark probability, refractory length, tick-period controls, and optional
**ToF seed**. Set `spark /256` to zero for a presence/neighbor-only run.

Knock mode makes each fixture's CA frame electrically dark and requests one
40 ms mallet pulse only when that fixture changes from quiescent to excited.
The request is not actuator authority: fixture-local daytime, solar-surplus,
battery tier, solenoid arm, rest, maintenance, load-marker, timer, and failsafe
gates still decide whether a physical strike occurs. Fixtures without a
solarnoid still participate in the CA graph and relay state but cannot knock.

With **ToF seed** off (the default), ignition comes only from spontaneous CA
sparks and fresh excited neighbors. With it on, a sensor-verified downlight can
also inject one local excitation from the hardened TMF rising-edge gate; every
other fixture can still relay the resulting CA state. The gate learns 90
reports of per-zone background, requires one confident zone to move at least
300 mm closer for three reports, and requires four clear reports before another
edge. The separate ToF color-wipe gossip stays suppressed while a CA lease is
active, so one physical approach starts the CA rather than two competing
propagation systems. ToF never bypasses the local knock or power gates.

The removed `idle`, `bridge`, `direct`, and `dark` choices were internal fixture
program roles, not CA algorithms: local fallback breathe, shared show-frame
consumer, per-fixture RGBW stream consumer, and electrical-dark program. Their
operator controls remain in the apps that own those jobs. Same-program CA knob
updates now reapply directly, with no release/re-lease light blip. Releasing or
expiring a knock lease restores the normal autonomous light CA defaults.

## Contagion

Open **Contagion** for the infection family kept separate from CA Studio. Start
leases program 5 to all awake updated fixtures for 10 minutes in a susceptible
state. The source list sorts alphabetically by callsign; the one-line keyboard
field filters its synced dropdown by callsign substring or short ID. Then choose
one fresh named fixture and press **Seed**.

- **Color Virus** adopts the seed hue across fresh Contagion neighbors and
  remains infected until stop, restart, or lease expiry.
- **Epidemic** moves infected -> immune -> susceptible and can be reinfected.
- Output can be lights, sound-only knocks, or lights + knocks. A perimeter
  infection is a silent relay; each infected downlight can request one 40 ms
  pulse, but all fixture-local actuator gates retain final authority.
- **Legacy fleet roll** is an explicit compatibility output for deployed
  fixtures that do not understand program 5. Select one fresh source such as
  Magmar; its infection edge starts one deterministic 40 ms addressed roll over
  fresh downlights only. Duplicate state frames stay quiet, and Stop/10-minute
  expiry disables the adapter. Do not use this mode once the participating
  mallet fleet runs native Contagion, or the two paths would duplicate intent.
- Color can be a fixed palette hue or random per local/manual seed. The hue is
  part of the transmitted state and is adopted across the graph.

A Color Virus fixture remains infected, but another clear/re-armed local ToF
gesture introduces a newer strain across the infected graph. Random mode
guarantees a different transmitted hue at the source; a fixed palette choice
deliberately stays fixed. Serial strain ordering plus a deterministic hue
tie-break makes simultaneous strains converge rather than overwrite each other
forever. If the source is the only updated fixture among old-image neighbors,
its local color proves the seed but there is no compatible graph across which
that color can spread.

The optional **ToF** control uses the learned downlight approach detector plus
a deliberate perimeter palm gesture. On the first named perimeter canary, clear
space held at 0/16 near zones while a palm hovered 5-10 cm above the sensor held
15-16/16; touching it can be too close to range. The gate requires two broad
near reports and four clear reports to re-arm. Direct sun and final geometry
still need qualification. Old fixture firmware rejects program 5 and will not
join the infection graph.

## Patterns v1

Open **Patterns** for microphone-independent, deterministic artistic control.
The first version offers Wash, Chase, Wave, and Twinkle with Ember, Forest,
Ocean, Aurora, and Moon palettes. Speed, intensity, fixture class, and stable
short-ID cohort A-D are adjustable from the handheld.

Patterns computes final per-fixture RGBW values and sends them through the same
bounded 8 Hz direct-frame stream used by LED Studio. Only one stream owner is
active: starting Patterns replaces LED Studio, and starting LED Studio replaces
Patterns. Leaving the app does not stop the selected look; use Stop explicitly.
If frames stop arriving, fixtures retain the normal three-second stale fallback.
Audio reactivity remains a separate v2 feature behind ES7210/I2S qualification.

## RF Diagnostics

Open **RF** for a read-only installation and mesh-health view. The summary page
shows live, seen, stale, production-roster unobserved, and foreign-live counts;
observation coverage; strongest and weakest fresh peers; receive drops; TX
success/failure counters; and mesh/WiFi/AP/channel-guard state. PDR is labelled
unknown until enough observations exist rather than being invented from RSSI.

The second page shows the existing newest-first valid-frame tail. Ranking is
deterministic by RSSI, then known PDR, age, and short ID. The app sends no mesh
packet and changes no fixture state or wire format.

Join timeouts / lost links fall back to mesh-only and retry with backoff. The
mesh is the primary function; Claude is the enhancement.

## Status page (M0)

Wi-Fi state / guard, SNTP UTC, mesh up + frame counter + last-frame
src/type/RSSI/age (+ MESH SILENT flag >15 s), battery mV/%, last key, trackball
pulse counts + click, touch point, probe verdicts, GPS fix summary, heap/PSRAM
watermarks. Redraw ~5 Hz into a PSRAM sprite, single pushSprite (no flicker).
`nb-mem heap_free=... psram_min=...` prints on serial every 10 s.

Press **`s`** on the keyboard to toggle the **direct-sun test pattern**
(max-contrast bars, chat-density text, RGB patches, backlight 255).

## M0 field checklist

- [x] **Direct-sun readability** (2026-08-19, Ben; re-tested same day):
      first pass looked near-illegible in direct sun, but the shipped
      **protective film was a glare confound** — with it removed and the panel
      tilted so the sun is off-perpendicular, full-sun reading is *fairly
      clear*; size 2 is comfortable, size 1 workable if a dense view needs it.
      **Field-UI consequence: size 2 is the default UI text size (M2);
      size 1 allowed for dense diagnostic tables. Operator habit: tilt the
      panel away from normal incidence in direct sun.**
- [ ] **Battery runtime**: `bat_mv` rides the 10 s `nb-mem` serial line — run
      unplugged (screen on, census running) and record hours; repeat with
      backlight 0. (Expectation: radio-RX-dominated ~168 mA + backlight →
      order 6-9 h; measure, don't assume.)
- [x] **Trackball direction map** (2026-08-19, Ben): a=UP b=RIGHT c=DOWN
      d=LEFT — recorded in `pins_tdeck.h`.
- [x] **Channel-guard live demo** (2026-08-19): mesh channel temporarily set
      to 6 vs BubbyNet on 11 → `guard: AP on ch 11 != mesh ch 6 -> wifi
      DROPPED, mesh kept`, status `GUARD:ch-mismatch`, mesh stayed up;
      restored to 11 → ONLINE again, SNTP synced, frames ticking.
- [x] **Coexistence**: ONLINE on BubbyNet (ch 11, -57 dBm) with mesh frames
      advancing 34→45 over ~7 s while associated.

## M1 acceptance (2026-08-19)

- Census port native-tested (donor-faithful tails + EWMA/windowed-PDR/eviction/
  observation ledger); live bench feathers tracked with plausible EWMA.
- `nb-master`/`nb-peer` emitters: 30/30 + 60/60 captured lines parse against
  `net_bench_dashboard.py`'s own regexes (full optional tails).
- TX proven end-to-end: identify-all was received by bench fixtures (their
  heartbeats report our downlink at `dlpdr=1.000`); strike path clamps 5-300 ms
  and the legacy addressed packet refuses an all-zero target. The Knocker UI's
  newer multicast modes use a deduplicated `NB_EVENT`, not that legacy packet.
  Quick commands `i/I/K/U/F/B/b/t` are WAN-down safe. `U<6-hex-ID>` remains the
  one-target 35-second maintenance command and has no broadcast form. Fleet OTA
  uses the job-scoped `uB/uA/uF/uS` roster contract: up to 160 exact targets,
  10 ms round-robin dispatch, structured `nb-maint` status, and a positively
  acknowledged freeze before upload (ADR 0062).
  `F<6-hex-ID>:<0|1>:<0|1>` selects commission/field profile and an explicit
  persist bit for one exact nonzero target; there is no broadcast form.
  Fleet-sized `nb-master`/`nb-peer` snapshots emit every 10 seconds because a
  complete 100+ peer snapshot cannot physically drain at 1 Hz over 115200 baud.
- **1-hour soak: heap_min bit-identical (257,608 B) across 3,643 s** with STA
  associated + census running; PSRAM low-water drift ~5 KB (bounded ring
  high-water). Log: session scratchpad `m1_soak.log`.

## Milestones

M0 bring-up (this) → M1 mesh core (census port + nb-* emitters, dashboard
compatible) → M2 LVGL shell + Fleet/Settings → M3 Claude client → M4 agent tool
loop → M5 Patterns/Zones/Knocker/CA Studio → M6 voice + diagnostics + polish.
Plan of record: `APP_ROADMAP.md` + ADR 0047/0048, with ADR 0037 retained for
constraints not superseded by the app-platform decision.
