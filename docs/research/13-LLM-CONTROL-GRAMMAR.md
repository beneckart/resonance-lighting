# 13 — LLM control grammar (the tool surface)

> The command console (`app/src/command.ts`) IS the LLM operator's tool surface (PRD §F).
> An LLM is prompted with this spec; it emits one command per line; the app runs them in
> order via `store.runScript(text)`. Natural language → these commands → the tree.

## Fixture model the LLM reasons over
- **78 fixtures**, ids `F000`…`F077`. Each has a `seq` (0..77 = order AROUND the tree by
  azimuth) and a `zone` ∈ {`low`, `mid`, `high`} (by height).
- `range`/`every` address by `seq` (so "range 0-23" = the first 24 lanterns around the tree).

## Grammar (one command per line; `#` = comment, blank lines ignored)
**Global show:**
- `pattern <id>` — id ∈ solid | breathe | chase | ripple | sparkle | sequence | spectrum | tricolor
- `hue <0..1>` · `bri <0..1>` · `sat <0..1>` · `speed <0..3>`
- `all pattern <id>` (same as `pattern <id>`)

**Per-fixture / group overrides** — `<target> <action>`:
- targets: `all` · `zone <low|mid|high>` · `range <a-b>` · `every <n>` · `fixture <id|seq>`
- actions: `color <#hex|cssname>` · `on` · `off`
- examples:
  - `zone high color #00aaff`
  - `range 0-23 color red`
  - `every 4 color #ffaa00`
  - `fixture F012 off`

**Whole-tree:**
- `on` — clear overrides (everything back to the pattern)
- `off` — all fixtures dark
- `clear` — remove all overrides

## Example LLM outputs
"Make the bottom of the tree deep blue and run a rainbow up top":
```
zone low color #0033ff
zone high color clear   # (use 'on' to release; per-zone clear via re-color)
pattern spectrum
```
"Strobe the whole tree red on the drop" → (strobe is a DJ control; via overrides:)
```
all color red
```
"Reset":
```
clear
pattern breathe
hue 0.08
```

## Not yet in the grammar (future)
- DJ controls (crossfade/EQ/master/strobe), auto-VJ toggle, beat-sync — currently UI-only;
  add `dj <param> <v>` / `auto on|off` verbs so the LLM can drive the whole console.
- A real LLM bridge (natural-language → this grammar) lives on the cortex (PRD §F); the app
  side is ready — anything that can POST a script to `runScript` drives the tree.

---

## LLM Operator — NL → command tool-spec (cycle 42, shipped in src/llm.ts)

The command console IS the LLM's tool surface. An external LLM (or the offline
`interpret()` stand-in) emits these grammar lines, run via `runScript`:

**Tool: `run_lighting_commands(commands: string[])`** — each line is one of:
- `pattern <id>` — id ∈ {solid,breathe,chase,ripple,sparkle,sequence,spectrum,tricolor,spiral,godray,rising,planewipe,warmcool,wind,ember,rain,beacon}
- `hue <0..1>` · `bri <0..1>` · `sat <0..1>` · `speed <0..3>` (global)
- `<target> color <cssName|#hex>` · `<target> on` · `<target> off`
  - target = `all` | `zone <low|mid|high>` | `range <a-b>` | `every <n>` | `fixture <id|seq>`
- `clear`

**NL mapping (interpret()):** target words (canopy/top→`zone high`, trunk/base→`zone low`,
middle→`zone mid`, "every other"→`every 2`, else `all`); pattern names + synonyms
(rainbow→spectrum, pulse→breathe, comet→chase, shaft/godray→godray, fire→ember…);
CSS colours only; fast/slow→speed; bright/dim→bri; vivid/pastel→sat; off/blackout→`<target> off`.
Deterministic + offline + unit-tested (llm.test.ts) — this is also the contract the
AI-VJ / smart-sound mode (#32) drives.
