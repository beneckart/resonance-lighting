# The AI Operator — Training Manual

**How an AI (or any operator) creates custom lighting modes on the Resonance Tree
without changing a line of code.**

Written 2026-08-15 (lighting-architect), the day the AI operator shipped.
Audience: the AI itself (this is the long-form version of its served prompt),
Elliot, and anyone wiring a new model in.

> **Source of truth note:** the prompt actually served to the model is built at
> runtime by `grammarPrompt()` in `app/src/openrouter.ts` and injects the LIVE
> pattern/show/theme/cue registries — so it can never drift from the app. This
> document explains the same surface for humans, with the reasoning attached.
> If they disagree, the code wins.

---

## 1. The one idea that makes this work

The AI never touches code, state, or the radio. It emits **command-grammar
lines** — the same text an operator could type into the command console — and
the app validates every line before running it (`isCommandLike` shape gate,
then the fail-safe parser). The command console **is** the AI's tool surface.

This is why "custom modes without changing the code" is possible: the grammar
is compositional. A "mode" is just a stack of grammar lines, and a saved mode
(`cue save <name>`) is that stack's *result* captured as state.

The tree the AI drives is the **digital twin**. When the operator arms
📡 drive-real, the physical 130-lantern fleet mirrors the twin exactly. Same
commands, zero difference.

## 2. The vocabulary

### Look & feel (globals)
```
pattern <id>        one of ~40: solid, breathe, chase, ripple, sparkle, spectrum,
                    spiral, godray, rising, aurora, life, ripples, organism,
                    living, chains, wind, ember, rain, beacon, ...
speed <0..3>        1 = authored pace
bri <0..1>          brightness      sat <0..1>     hue <0..1>
theme <id>          colour mood for the living patterns:
                    ember energize intimate love forest ocean sunset wild random
```

### Targeting (specific lights)
```
<target> off | on | color <name|#hex>
target = all | zone low|mid|high | range a-b | every n
       | fixture <id> | light n | light n,n,n | light a-b
```
Addressing is azimuth-ordered, so `range 0-23` is the first ring around the
tree and `every 2` alternates lights.

### Shows (authored 5-minute arcs)
```
show solarray | schumann | performance | bioluminescence | aurora
     | awakening | ignition | cosmos
show stop
```

### The Game of Light (presence experience)
```
gol arm      tree goes dark in standby; the first visitor's tap ignites it,
             then every tap seeds life that spreads light-to-light
gol end
```

### Custom modes — the no-code loop
```
cue save <name>     capture the CURRENT look as a named mode
cue <name>          recall it any time
```

### Fleet ops (real lanterns)
```
blink               every reachable real lantern blinks — roll call
blink <MAC>         one lantern blinks — "which one is F2BE20?"
```

### Sequencing
```
wait <seconds>      pause the script; later lines run on a timer
```
A **new request instantly cancels any pending sequence** — the operator's
latest word always wins. This is deliberate: iteration speed beats sequence
completion every time.

## 3. Composing a custom mode — worked example

Elliot's brief, verbatim: *"when I walk under the tree I want all the lights to
turn on red and slowly turn to purple. Then I want them to all turn off and
only the ones above me turn on. Then I want the game of life and cellular
automata simulation to start so we get ripples of lights moving through the
tree at random colors."*

As grammar:

```
all color red
wait 6
all color purple
wait 6
off
wait 2
clear
zone high on
wait 3
clear
theme random
pattern ripples
cue save walk-under welcome
```

Notes on the translation:
- "slowly turn to purple" — the grammar has no tween verb yet, so the honest
  encoding is stepped colour + wait. If smooth cross-fades matter, that is a
  real feature request, not a prompt fix.
- "only the ones above me" — without presence sensing live (see §5), `zone
  high` is the stand-in. When ToF presence lands on the radio, this binds to
  a real trigger.
- The final `cue save` makes it a named mode — recallable forever with
  `cue walk-under welcome`, shown as a ★ chip in My Modes, no code anywhere.

## 4. How the AI should behave (operator doctrine)

1. **Output only grammar lines.** No prose, no fences. Anything else is
   stripped by the shape gate — a wasted line.
2. **Compose, don't minimalize.** A described *feeling* usually wants
   pattern + theme + speed + brightness together.
3. **Iterate fearlessly.** Scripts preempt cleanly; a half-run sequence
   cancelled by a new idea is the system working as designed.
4. **Save generously.** When the operator likes something, `cue save` it —
   a look that isn't saved is a look that will be rebuilt from memory later.
5. **The offline fallback is not you.** When the network drops, a
   deterministic interpreter answers with the ⚙️ badge. Its answers are
   cruder. That's fine — the tree must never go silent because the sky did.

## 5. What the AI can and cannot know (today, honestly)

| Question | Answer path | Status |
|---|---|---|
| what pattern/show is running | twin state | ✅ live |
| which real lanterns are awake, battery mV, charging | fleet telemetry (Locate) | ✅ live |
| which physical lantern is which | `blink <MAC>` + Constellate photogrammetry | 🔶 blink live; Constellate sweep pending |
| what a light does when someone walks under it | Ben's ToF presence — `NB_EVENT` is **reserved-unsent** in firmware | ⛔ not on the radio yet |
| per-light behavior/power config | heartbeat carries life_state / program / power_tier; full config needs maintenance-mode HTTP | 🔶 partial |
| sleep interval / dev mode | daemon `/debug/rate`, `/debug/maint`, `/debug/profile` | ✅ live (daemon-side) |

The AI should never claim knowledge from the ⛔ rows. "The firmware doesn't
report that yet" is the correct answer, and pretending otherwise poisons trust
in every other answer.

## 6. Wiring a model in

Interactive tab → AI operator → paste an OpenRouter key (stored in that
device's localStorage only, never committed, never sent anywhere but
OpenRouter). Model defaults to `anthropic/claude-sonnet-4.5`. The 🎤 corner
button feeds speech or typed text through `interpretRemote()`; the badge shows
which brain answered (🤖 remote / ⚙️ offline). Policy (Elliot, 08-15): the key
is used **only through the app** — never via a terminal or agent session.
