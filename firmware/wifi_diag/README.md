# wifi_diag — WiFi range / roaming diagnostic (PowerFeather V2, ESP32-S3)

> **Which tool to use:** for **wireless** yard coverage mapping with no laptop in the
> field, use the **ESP-NOW scan-report + serial bridge** in `../net_bench/`
> (`--scan-report` / `--serial-bridge`) — that's the primary path. THIS sketch is the
> **tethered association/roaming probe**: it actually associates to the Eero and catches
> a live *missed-roam* decision (the scan-only ESP-NOW path defers that). Use it close-in
> on USB when you want to confirm the board clings to the far BSSID instead of roaming.

A tiny standalone sketch to answer item (b) of
`docs/tests/SOLAR_TELEMETRY_RANGE_PLAN_2026-06-08.md`: confirm **why the ESP32 falls
off the house Eero mesh in the yard** while 5/6 GHz devices stay happy. Hypothesis
(from the field observation): the ESP32-S3 is **2.4 GHz only** and **clings to one
Eero BSSID** instead of roaming to a nearer node as RSSI collapses. This makes that
visible and quantifies the 2.4 GHz coverage for placing a field maintenance AP.

## What it does

- Connects WiFi-STA to the configured SSID, **TX power forced to MAX** (apples-to-
  apples; `--tx-low` drops to 8.5 dBm to compare), modem sleep **off**.
- Every `--assoc-s` seconds streams `wd-assoc` — `RSSI / BSSID / channel / SSID /
  TX-dBm` for the currently-associated AP.
- Every `--scan-s` seconds runs a full **2.4 GHz scan** (`wd-scan` + one `wd-ap` per
  AP) and emits `wd-roam`: compares the associated AP to the **strongest same-SSID
  AP**. If a stronger same-SSID node beats the association by ≥ the roam margin
  (default 8 dB) it's flagged `better=1` — **a stronger Eero node was available but
  not roamed to** (the smoking gun).
- Tracks drop/reconnect transitions (`wd-event`, with `after_ms` to reassociate),
  auto-reconnect on, so you see whether/how fast it recovers in the yard.

## It's a SERIAL/USB tool, not OTA

You carry a tethered laptop on the walk to read the stream anyway, so there's no OTA
path — flash over USB, open the serial monitor (115200), walk. Because it **streams
every interval** (not just at boot), opening the monitor late still catches data —
sidesteps the native-USB-CDC boot-banner quirk (see `../POWERFEATHER_NOTES.md`).

## Use

```
./build.sh --port /dev/ttyACM0                 # USB flash
./build.sh --assoc-s 1 --scan-s 10 --port ...  # faster cadence for a walk
./build.sh --tx-low --port ...                 # 8.5 dBm instead of MAX (compare)
```

Capture the walk to a log for the write-up, e.g.:

```
python3 -c "import serial,sys; s=serial.Serial('/dev/ttyACM0',115200); \
  [sys.stdout.write(s.readline().decode('utf8','replace')) or sys.stdout.flush() for _ in iter(int,1)]" \
  | tee ops/bench/data/ca/$(date +%F)-wifi-diag.log
```
(or just use `arduino-cli monitor -p /dev/ttyACM0 -c baudrate=115200 | tee ...`)

Then grep `wd-roam` / `wd-event` and pull the RSSI track from `wd-assoc` for the note.

## Reading the result

- **`wd-roam better=1`** appearing as RSSI degrades in the yard = confirms the
  missed-roam hypothesis: a nearer Eero node existed and the S3 stuck to the far one.
- **`wd-event drop` with no/slow `reconnect`** in the yard, recovering near the house
  = the association-collapse zone. The RSSI at which drops begin (expect ~−85…−90
  dBm) sets where a 2.4 GHz field maintenance AP must sit relative to the tree.
- The `wd-ap` list shows the full 2.4 GHz landscape (which Eero nodes, what channels)
  — useful for picking a clean channel for the field AP.

Deliverable: a short note in `LOG.md` (the 2.4 GHz RSSI map + roaming behavior + the
maintenance-AP implication), per the plan doc.
