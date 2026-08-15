# Camp network setup: Starlink + Beryl AX on channel 11

Runbook for standing up the camp/art-site network so it can coexist with the
ESP-NOW fleet. Decision and rationale: `docs/decisions/0036-camp-network-channel-11-ap.md`.

**Do the home rehearsal (section 2) before the event.** The Starlink bypass-mode
switch is not field-improvisable: leaving bypass mode requires a factory reset.

---

## 1. Why the channel matters

The ESP32-S3 has one 2.4 GHz radio. WiFi STA and ESP-NOW share it, and in STA
mode **the access point picks the channel**. The fleet is pinned to channel 11.

An AP on automatic channel selection lands on 1 or 6 most of the time. Any
device that associates to it drags its radio off 11 and goes deaf to the entire
fleet -- silently. No error, no log line, the mesh just stops arriving.

This affects any device that is on the mesh and the network at the same time:
a Claude handheld (ADR 0037), or a laptop-plus-bridge session while the fleet is
live. It does **not** affect fixtures doing normal shows (no infrastructure
needed at all) or fixtures in OTA maintenance mode (they have already left
ESP-NOW by design).

---

## 2. Home rehearsal (do this before the playa)

### 2.1 Starlink generation -- resolved

The project has **Gen 3 and Gen 4** dishes (possibly all Gen 4), confirmed
2026-08-15. Ethernet is built in on both, so **no Starlink Ethernet Adapter is
needed**. Nothing to order.

Still do two things by hand: confirm the Ethernet port physically on the specific
unit that travels, and confirm bypass mode is present in that unit's app
settings before relying on it.

(For reference, if a Gen 2 dish ever turns up in the pile: it needs the Starlink
Ethernet Adapter, there is no substitute, and there is no field workaround.)

### 2.2 Rehearse bypass mode

Bypass mode turns off the Starlink router's own WiFi and DHCP so the Beryl is the
only router on the link.

1. Starlink app -> Settings -> **Bypass Mode** -> enable.
2. Confirm the Starlink SSID disappears.
3. Confirm the Beryl gets a WAN address and reaches the internet.
4. **Rehearse the undo**: leaving bypass mode requires a factory reset of the
   Starlink router. Do it once at home so the recovery is known, not discovered.

Record which physical cable goes where. In dust, at night, this matters more
than it sounds.

### 2.3 Configure the Beryl AX (GL-MT3000)

Web UI, typically `http://192.168.8.1`.

| Setting | Value | Why |
|---|---|---|
| 2.4 GHz channel | **11 (fixed)** | Must match the fleet. Never "auto". |
| 2.4 GHz width | **20 MHz (HT20)** | Narrow and robust; avoids 40 MHz bonding onto adjacent channels |
| 2.4 GHz security | WPA2-PSK | WPA3/mixed mode has caused ESP32 association trouble |
| 2.4 GHz SSID | dedicated, e.g. `Resonance24` | Must be separate from the 5 GHz SSID |
| 5 GHz SSID | separate name | Where laptops and phones belong |
| Power | USB-C, about 5 W | Budget against camp batteries |

**Do not** give the two bands the same SSID. Band steering will hand a client
5 GHz and the fleet-facing device will never see channel 11.

Push every laptop and phone onto the 5 GHz SSID. About 130 fixtures share
channel 11; ESP-NOW frames are short and duty cycle is low, but there is no
reason to put video calls in the same 20 MHz.

### 2.4 Verify the channel

Do not trust the config page alone -- verify over the air:

- Phone WiFi analyzer, or
- The bridge's own scan path (`NB_SCANAP` / `--scan-report`), which reports
  `bssid`, `ap_rssi`, `ch`, `enc`, and `ssid` per AP.

Expect `ch=11`. If it reports anything else, fix it before going further.

### 2.5 Coexistence test

With a fixture beaconing on the bench:

1. Associate one device to the pinned 2.4 GHz SSID.
2. Confirm ESP-NOW RX continues, both directions, for at least an hour.
3. Deliberately set the AP to channel 1. Confirm the channel guard fires: WiFi
   dropped, mesh retained, mismatch surfaced.
4. Set it back to 11.

### 2.6 OTA over the camp router

Run one parallel shared-WiFi OTA over the Beryl, not just the house network:

```bash
python ops/bench/net_bench_ota.py --help
```

Follow the normal fleet OTA path (targeted `U<id>` maintenance, then
`field_cycle_ota.py` or `net_bench_ota.py --reboot comms`). Do **not** use or
build the deprecated per-board maintenance-AP path.

Maintenance-mode fixtures may associate on any channel -- the guard does not
apply to them, because they have already left ESP-NOW. This step is about
proving the camp router serves the OTA path at all, not about channel behavior.

### 2.7 Power

Measure the Beryl's actual draw powered the way it will actually be powered
(USB-C off the camp battery, not a wall wart). Record it against the camp energy
budget. Nominal is about 5 W; confirm rather than assume.

---

## 3. Field bring-up checklist

- [ ] Starlink sited, powered, dish clear
- [ ] Ethernet adapter installed (Gen 2 only)
- [ ] Starlink in bypass mode; Starlink SSID gone
- [ ] Beryl powered from camp battery, WAN address acquired
- [ ] 2.4 GHz on channel 11, HT20, WPA2-PSK, dedicated SSID -- **verified by scan**
- [ ] 5 GHz SSID up and separate; laptops and phones on it
- [ ] One device associated + mesh RX confirmed simultaneously
- [ ] Fleet still running autonomous shows with the router off (it must)

---

## 4. Troubleshooting

| Symptom | Likely cause | Fix |
|---|---|---|
| Device joins WiFi, mesh goes silent | AP not on channel 11 | Verify by scan, not by config page. Re-pin. |
| Device shows a channel-mismatch message | The guard worked | Fix the AP; the device stays on the mesh meanwhile |
| Device on 5 GHz, no mesh | Band steering, or the device joined the wrong SSID | Separate SSIDs per band; join the 2.4 GHz one |
| Association fails on an ESP32 | WPA3 / mixed mode | Set WPA2-PSK only |
| Mesh fine, no internet | Starlink not in bypass, or double NAT | Re-check bypass; confirm the Beryl holds the WAN address |
| OTA discovery times out | Sleeping peer, not a network fault | A timeout means no OTA was attempted. Allow a full sleep cadence (360 s default); do not shorten it |
| Fleet unaffected by all of the above | Correct and expected | Fixtures need no infrastructure (ADR 0004) |

**A wrong-channel AP is the first thing to check** whenever a device that was
working on the bench goes quiet on the mesh at camp.

---

## 5. Open items

- One virtual SSID spanning the camp and art-site Starlinks (queued in
  `TODO.md`). Both APs must be on channel 11; if they are to serve OTA they must
  also share SSID and PSK.
- The router is ordered but not received; nothing here has been executed yet.
- Beryl draw is unmeasured against the camp battery budget.

## References

- `docs/decisions/0036-camp-network-channel-11-ap.md`
- `docs/decisions/0037-claude-mesh-bridge-handheld.md`
- `docs/decisions/0004-mesh-esp-now.md`
- `docs/decisions/0010-standard-ota-no-mesh-firmware-gossip.md`
- `AGENTS.md` -- OTA fleet path and sleeping-peer OTA timing gotchas
- `firmware/cores3_bridge/README.md`
