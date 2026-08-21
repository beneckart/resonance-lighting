# Completeness Audit — what stands between this software and "done"

2026-08-15, lighting-architect. Elliot: "find out what other improvements and
changes we need to make this software more complete." Grounded in: the PRD,
docs 01–19, Ben's TODO/ADRs (upstream), Justin's cambium/constellate, and —
most heavily — today's first real-fleet field day. Ordered by what actually
blocks the tree lighting up on playa, not by engineering appeal.

Dates that rule everything: build week Aug 24 · gate Aug 30 · burn Sep 5.

## TIER 1 — blocks "the real tree does what the twin says" (do first)

1. **MAC↔slot join (roll call).** Fixtures.json ids are slots (F000…F129);
   real lanterns are MAC-derived. Until joined, patterns land on the wrong
   physical lanterns. Machinery exists (Calibrate commissioning, selfmap
   assisted-solve ≤30 confirms → ≥95%, Constellate photogrammetry). Missing:
   the operator FLOW that walks someone through it in 20 minutes. Justin
   on-site now = the week to do it.
2. **OTA path to consistent comms.** 25 of 48 commissioned units run the
   07-30 day-sleep image and vanish in daylight. Fix = Ben's .2 artifact OTA'd
   via maintenance mode. Software side done (maint/rate endpoints, comms-net
   loop); needs Ben's .bin + charged batteries. Chase daily until closed.
3. **Multi-driver arbitration in cambium.** drive-real defaults ON; two phones
   = last-writer flicker on the physical tree. Daemon needs a driver lease
   (first ws client with drive on holds it; others read-only + a "someone
   else is driving" surface in the app). Small, Justin's repo, high stakes.
4. **maint_status / fw_rev surfaced to /fleet.** Cambium parses ~50 hb fields,
   forwards 8. Without maint_status the OTA gate is blind; without fw_rev we
   cannot see who runs what image. One-file change in cambium + Locate column.

## TIER 2 — operator experience at the tree (build week)

5. **Presence/ToF events on the radio (Ben).** NB_EVENT is reserved-unsent.
   Everything interactive ("what happens when someone walks under") is sim
   until this ships. Our side is ready (taps already model it). Standing ask.
6. **Sensor panel per light.** lux, panel/board temp, humidity, solar supply,
   charger state all parse in cambium and die there. Extend /fleet + the
   LightSheet. (Elliot asked for exactly this today.)
7. **Constellate phone-camera flow.** /constellate/light + mapping/ exist in
   cambium; the app needs the Calibrate button that steps lights while the
   phone films. Pairs with #1.
8. **Smooth color tweens.** The walk-under brief wanted "slowly turn to
   purple"; grammar can only step+wait. Add `fade <color> <secs>` (lerp in
   the twin, direct-frame ramp when driving real).
9. **Battery dashboard trend.** Locate shows now-values; the fleet's story
   today was the TREND (2927→3250 rising). Sparkline per light + fleet
   median-over-time; the data is already in the registry.

## TIER 3 — robustness the playa will demand

10. **Bridge watchdog + auto-restart.** The daemon died once today (serial
    wedge) and only a human noticed. launchd plist + the frozen-counter
    detector (tx_ok flat while rx climbs = wedged CoreS3, from 08-15 03:52
    incident) + auto-replug guidance on the Locate page.
11. **Session persistence for the twin.** Saved modes persist (localStorage)
    but pattern/theme/drive state don't survive refresh. Playa operators will
    refresh constantly.
12. **Multi-phone sync (one tree, many controllers).** Deferred design from
    08-14; cambium-as-hub is the natural shape once driver-lease (#3) exists.
13. **AI operator: conversation memory + fleet awareness.** interpretRemote is
    one-shot; feeding it the live census + last-3 exchanges would let "make it
    warmer" mean something. Cheap, high-feel.
14. **Show scheduler (ADR-0031 alignment).** Shows fire manually; production
    intent is site/date dusk-dawn schedules. The duskgate exists for Solar
    Ray — generalize to a nightly playlist.

## EXPLICITLY NOT SOFTWARE (tracked, not ours)

- Solarnoid fitment (are strikers physically mounted?) — Ben, hardware.
- .bin artifacts for OTA — Ben's bench.
- Trunk/root lights (24 ground uplights) — Ben+blender class-change decision.
- 33140 15Ah threshold re-derive (ADR-0023 says in terms) — Ben.

## Done today (for the record)

Real-by-default connection · Command/Locate/Settings tabs · per-light
LightSheet · honest census (listen-window semantics) · daemon bootstrap ·
OpenRouter operator trained on the full grammar (shows/themes/cues/gol/blink/
wait sequencing) · instant CA-sim buttons · knock/maint/rate/profile/channel
daemon endpoints · 10× glb scale fix (with blender) · gh-pages republished ×3.
