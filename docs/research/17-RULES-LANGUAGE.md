# 17 · The Rules Language — from human words to fleet behavior, and back

*2026-07-09 · lighting-architect · answers Elliot: "how does the language of
humans turn into the computer language that the fleet can use? How do we know
what program they are running, and tweak only the rules they want? Think about
how we are going to give new rules and updates in a language they understand."*

Grounded in Ben's repo: ADR-0010 (standard OTA, no mesh firmware gossip —
therefore **behavior must be data, not code**), ADR-0009 (minimize per-fixture
ops), the ESP-NOW control plane (ADR-0004, 250-byte frames), and the existing
`app/src/rules.ts` compiler + RULES panel.

## The four layers (each one human-checkable)

```
 1 ENGLISH      "after 10pm if the battery is low, glow ember dimly"
      │  LLM translator (llm.ts pattern) — SUGGESTS, never flashes
      ▼
 2 THE DSL      when hour >= 22 and soc < 30 -> pattern=ember bri=40
      │  parseRules() — deterministic, validated, previewable in the twin
      ▼
 3 BYTECODE     ≤240 B, fits ONE ESP-NOW broadcast frame (rules.ts encode)
      │  broadcast flash + per-node ack
      ▼
 4 THE NODE     evaluates its rule table locally every tick — radio optional
```

**The DSL is the contract, not the LLM.** English is ambiguous, so the LLM
only *drafts* DSL; the operator always sees (and can edit) the exact lines
before anything is flashed, and the twin simulates the ruleset first — the sim
IS the proof of meaning. The vocabulary (sensors, ops, patterns) is a shared
append-only enum table mirrored in firmware: numbers are never renumbered, so
old nodes ignore verbs they don't know rather than misread them.

## "How do we know what program they are running?"

Every compiled ruleset gets an identity: `(epoch, crc16)` — epoch increments
per flash, crc is over the bytecode. Two firmware additions (one line each in
the heartbeat encoder):

- **Heartbeat carries `rules_epoch` + `rules_crc`** (3 bytes). The twin's
  FleetPanel ledger then shows, per node, *which program it runs* — and any
  node whose (epoch, crc) mismatches the last flash is flagged STALE with a
  one-tap targeted re-send (unicast, not broadcast).
- **Decompiler (already trivial: bytecode ↔ DSL is 1:1)**: click any node in
  the ledger → see its program *as DSL text*. The fleet's behavior is always
  readable back in the same language the human wrote. No hidden state.

## "Tweak only the rules they want"

A full ruleset is ≤240 B = **one frame** — so the atomic unit of change is the
whole set, which is *safer* than in-place patching (no torn tables, no rule
ordering drift; first-match-wins order is preserved by construction). The
human-level "tweak one rule" experience lives in the panel:

1. Panel decompiles the fleet's current program to DSL (from any healthy node,
   cross-checked by crc consensus).
2. Operator edits one line (or asks the LLM to: "make the presence response
   blue instead").
3. Diff view: exactly the changed lines highlighted — nothing else moved.
4. Re-flash = one broadcast; acks + heartbeat crc confirm adoption per node.

For **per-group programs** (canopy vs chandelier behaving differently), the
flash frame gains a 1-byte group mask; nodes only adopt rulesets addressed to
their group. Ledger shows program identity per group.

## "A language they understand" — design principles

1. **One line = one behavior**, readable aloud: `when presence > 0 ->
   pattern=ripple bri=255 speed=3`. If a camp member can't read a rule over a
   radio, it's too clever.
2. **First match wins, last line is the default** — the whole execution model
   fits in one sentence.
3. **The twin previews every ruleset before flashing** (env sliders: hour,
   presence, soc) — meaning is demonstrated, not documented.
4. **Append-only vocabulary**: new sensors/patterns extend the enums; old
   programs stay valid forever (Ben's ADR discipline applied to language).
5. **English in, English out**: LLM drafts DSL; decompiler renders any node's
   bytecode back to DSL. Humans never have to read bytes.

## Build queue (cortex side, sim-first)

- [ ] rules.ts: crc16 over bytecode + epoch surfaced in the panel (small)
- [ ] mock heartbeat: carry (epoch, crc); FleetPanel ledger column + STALE flag
- [ ] decompile view: node → DSL text (encode/decode round-trip test exists)
- [ ] LLM drafting box in the RULES panel (reuse llm.ts interpret pattern),
      always landing in the editable DSL textarea — never straight to flash
- [ ] group-mask byte in the flash frame (schema note → upstream question for
      Ben before firmware lands it)

**Question filed for Ben** (brainstem, his call): heartbeat gains 3 bytes
(rules_epoch u8, rules_crc u16)? And does the rules VM live in the RTOS tick
task (ADR-0005) or its own?
