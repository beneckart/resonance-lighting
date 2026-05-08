# LOG

Append-only session journal for the Resonance Lighting workstream. Most recent first.

Format per entry:

```
## YYYY-MM-DD — author — short subject

Body. What changed, what was decided, what's next.
```

---

## 2026-05-06 — Ben + Claude (Cowork) — Pre-share cleanup pass

Final cleanup before pushing the repo to GitHub and sharing with Steve and the wider team:

- **Bamboo "cone" → "lantern" / "cylinder".** The bamboo piece is geometrically a cylinder with a steam-bent flared skirt at the bottom, not a cone. The only cone-shaped object in the project is the experimental projective-geometry filter / gobo. Scrubbed every "bamboo cone" reference across BACKGROUND, ROADMAP, README, AGENTS, glossary, ADR 0007, hardware/references, ops/bom, enclosure README. Gobo "cone" references preserved.
- **Agent-neutral voice.** Rewrote BACKGROUND.md from a Ben-addressed narrative into a third-person project-context document. Replaced "Ben (you)" with "Ben Eckart" throughout. Replaced "Dad" with "Steve Eckart" outside this LOG file.
- **Scrubbed historical / distracting context** from active docs. Removed "Critical dates" stale-deadline table from BACKGROUND. Removed crossed-out resolved items from TODO and ROADMAP. The narrative of "we initially thought X, then learned Y" now lives only in this LOG; active docs present the current state cleanly.
- **New ADR 0009 — Minimize per-fixture operations at scale (O(1), not O(N)).** Captured Ben's strong constraint that anything done per-fixture is multiplied by 100. Specifies: no soldering on receipt; same firmware for every fixture; per-unit identity from MAC; investigate JLCPCB pre-flash service; design pogo-pin flashing jig as fallback. Reinforced in `README.md`, `hardware/README.md`, `TODO.md`. This is now the ninth and (so far) final ADR.

After this pass, the active docs (`README`, `AGENTS`, `BACKGROUND`, `TODO`, `ROADMAP`, `SYSTEM`, ADRs, glossary) read as a clean shared documentation set for Ben + Steve + future AI agents + the wider Resonance team. The journey from "what is this project" through "let's design solar lights" to "modular hat with LiFePO4 carrier board with O(1) ops" lives in this LOG.

---

## 2026-05-06 — Ben + Claude (Cowork) — Logistics flow confirmed: air-ship to TN, integrate at Grass Valley

Big risk-register item resolved: **Bamboo Pure is air-shipping a small batch of prototype bamboo lanterns to Steve in Tennessee.** Electronics workstream is fully decoupled from the May 10 Bali sea container. The end-to-end logistics flow:

1. Bali → TN: prototype lanterns by air for early mechanical prototyping (Phase 2).
2. Bali → Grass Valley, CA: tree structure + remaining bamboo by sea container.
3. Ben (CA): designs PCB, ships to Steve.
4. Steve (TN): finalizes hat enclosure with both bamboo and PCB in hand.
5. Steve → Ben (TN → CA): ships 100 hats.
6. Ben → Grass Valley: drives hats + electronics to meet the bamboo container at the staging area.
7. Grass Valley: final integration. Truck to BRC.

**Updated docs:**

- `docs/ROADMAP.md` — Phase 2 dependencies, Phase 6 rewritten as cross-country logistics + Grass Valley integration, risk register marked resolved, open dependencies list updated.
- `TODO.md` — removed urgency on "catch Elliot before Bali," removed ship-path decision (resolved), added air-ship-timing confirmation.

**What this changes practically:**

- Phase 2 (mechanical prototyping) can start as soon as bamboo arrives in TN, not when Elliot returns from Bali.
- Phase 5 production fab no longer races a container deadline.
- Phase 6 is a cross-country logistics piece with TN → CA → Grass Valley flow rather than US → BRC direct.
- Grass Valley pre-build staging area is now the canonical "integration site" terminology.

---

## 2026-05-06 — Ben + Claude (Cowork) — Roadmap, power-budget correction, prototyping strategy

Three additions:

**`docs/ROADMAP.md`** — phases 0–10, working backward from BM 2026 (late August). Phase 1 (TTGO bench prototype) starts 2026-05-07 and runs ~3 weeks. Phase 3 (custom carrier board v1) lands ~2026-07-01. Phase 5 (production fab) ~2026-08-01. Risk register and open dependencies on team included.

**Prototyping strategy clarification.** The "validate the architecture before committing to LiFePO4 silicon" risk is fully mitigated by Phase 1 — using the **TTGO T-Beam (with its built-in TP4056 LiPo charger)** as the LiPo prototype platform. No intermediate "LiPo carrier board" needed — that would add a board spin without de-risking anything Phase 1 doesn't already cover. The CN3058 LiFePO4 charger circuit is the only chemistry-specific portion; we lift its reference circuit from datasheet, AI-review, and validate on Phase 3 v1 board with MCP73123 as designed-in fallback. (Captured in `docs/ROADMAP.md`, not yet a separate ADR — promote to ADR if revisited.)

**Power budget correction.** Earlier estimate assumed "4 WS2812B all on at once" yielding ~10 mA LED average. Actual usage model is **1–9 LEDs per fixture, typically 1–3 lit at a time** (default ambient = 1 LED at 10%, showy = 3 LEDs at 30%, wand-burst = 9 LEDs full but rare and brief). Per-LED current scales linearly per WS2812B datasheet — confirmed against 2018 Talisman v2 measurements on the 16-LED ring (500 mA / 16 = 31 mA per LED at full white, matching). Updated `docs/block-diagram/SYSTEM.md`:

- Per-LED reference table replaces "4-LED ring" table.
- Time-weighted nightly LED current ~5 mA (vs. 10 mA estimated earlier).
- Total daily drain ~120 mAh (vs. 170 mAh).
- Panel sizing recommendation now 1–2 W (vs. 2 W); 1 W is sufficient.
- Battery: 18650 still preferred for 12-night autonomy and 2-year life; 14430 (~3 nights) now reasonable if cell sourcing forces it.
- BOM updated for 1–9 LED count per fixture.

---

## 2026-05-06 — Ben + Claude (Cowork) — Handoff documents

Before switching to Claude Code for daily iteration, dumped context to handoff-friendly artifacts so future agents (Ben's Claude Code, Steve's Claude Code, Elliot's Co-Work, future Cowork sessions) can pick up cold:

- `AGENTS.md` at root — explicit preamble for any agent picking up this repo. Read order, who's working, what's known vs assumed, what the repo does NOT cover, when to ask Ben.
- `docs/block-diagram/SYSTEM.md` — the canonical system architecture. ASCII block diagram, voltage rails, current draw table grounded in 2018 Talisman v2 measurements + ESP32-C3 datasheet, single-fixture daily power budget (~170 mAh/night, well covered by 2 W panel + 1500 mAh 18650), back-of-envelope max-stress check for wand-interaction events. Cost-comparison sketch vs `INV_2026_00401`.
- `docs/decisions/` — eight ADRs: ESP32-C3-MINI-1 (0001), LiFePO4 chemistry (0002), CN3058 charger (0003), ESP-NOW mesh (0004), FreeRTOS task architecture (0005), custom PCB not dev-board-on-carrier (0006), modular hat enclosure (0007), WS2812B from Vbat with no level shifter (0008).
- `firmware/ARCHITECTURE.md` — RTOS task decomposition (`led_render_task`, `ca_tick_task`, `mesh_tx_task`, `mesh_rx callback`, `housekeeping_task`), inter-task communication via FreeRTOS queues + atomic shared state, sleep behavior, boot sequence, OTA strategy.
- `hardware/atopile/EXAMPLE.md` — sample atopile module (`voltage_regulator.ato` for the AP2112K-3.3 LDO) so the schematic-as-code pattern is concrete. List of modules to build.
- `ops/bom.md` — first-pass BOM grouped by carrier-board electronics, non-PCB electronics, and mechanical. Per-fixture target ~$23. 100-fixture total ~$2,310.
- `docs/glossary.md` — proper nouns and acronyms for new agents dropping in cold.

These files are now the canonical project context outside this conversation. The earlier `BACKGROUND.md` remains the long-form narrative.

Switching to Claude Code from here. Cowork retains read access to this repo via GitHub (when pushed) for review and project management.

---

## 2026-05-06 — Ben + Claude (Cowork) — Repo bootstrap

Stood up this repo. Ported `BACKGROUND.md` from earlier Cowork session — captures full project context, team, decisions to date, prior-art lessons from 2018 Talisman v2 build, code reusable from `beneckart/future-robotics`, and the design space for this year (electronics architecture, mandala filter program, mesh creative possibilities).

Decisions baked in so far (subject to team review):

- **MCU:** ESP32-C3-MINI-1 for production. Prototype on TTGO T-Beam and T-Ice modules already in Steve's workshop.
- **Battery chemistry:** LiFePO4. Chosen for thermal tolerance in desert deployment.
- **Charger IC:** CN3058 (LiFePO4-tuned, JLCPCB basic part, ~$0.30). Rejected TP4056, bq24074, CN3791 — all LiPo-tuned, wrong charge profile.
- **3.3 V LDO:** AP2112K-3.3 (450 mV dropout, JLCPCB basic part, fits LiFePO4's 2.5–3.6 V range).
- **LEDs:** 1–4 WS2812B per fixture, powered direct from battery rail (3.3 V GPIO satisfies WS2812B's 0.7 × Vcc threshold per Talisman v2 verification).
- **Mesh:** ESP-NOW. No infrastructure required at BRC.
- **OTA:** required from day one. One USB-C flash per device, then over-the-air forever.
- **Enclosure:** sealed 3D-printed solar "hat" that sits partially inside / partially over the bamboo cone top. Set screws absorb bamboo dimensional variability.

Open team-side questions (see `BACKGROUND.md` and `TODO.md` for full list):

- Rope attachment point: hat, bamboo, or hybrid. Pending Vishnu / Ed / Elliot.
- Container vs separate ship for electronics. Bamboo ships from Bali 2026-05-10.
- Hat dimensions confirmation to Vishnu so he can finalize renders.
- INV_2026_00401 cost decomposition.

Next concrete steps for Ben + Steve:

1. System block diagram + power budget (highest-leverage upstream artifact).
2. atopile module library: `solar_input`, `lifepo4_charger`, `power_path`, `voltage_regulator`, `esp32_module`, `led_output`. Build each from reference schematics.
3. Bench validation on existing TTGO modules — solar charging path first.

Switching to Claude Code for daily firmware/hardware iteration. Cowork retains read access to this repo via GitHub for project management and review.
