# v1.1 placement handoff

Open `build/capbank_placement.kicad_pcb` — same 31 footprints, **zero tracks**,
so you can drag freely with only the ratsnest to guide you. I regenerate all
routing from your positions, so breaking traces costs nothing.

## Don't move these (hard constraints)

- **4 mounting holes** — your plates are drilled for them.
- **PANEL / DRIVER / TELE / RECVR** — must stay on the bottom edge. PANEL and
  DRIVER are right-angle SMT: their wires exit off the edge, so the footprint
  has to sit at it. Sliding them left/right along that edge is fine.
- **C2/C3/C4** — the three cans, and the VBOOST spine that feeds them.

## Move freely (this is where the crowding is)

- **Boost cluster**: U2 (MT3608), L1, D2, C8, C9, R8, R9, R10, R11, R12, JP1
- **One-shot cluster**: SW1, R1, C1, C1B, R2, R3, D1
- **LDO group**: U1, C7, R7
- **Telemetry**: R4, R5, C5

## What helps me most

1. **2–3 mm of clear space around every part.** That single thing is why the
   routing kept failing — parts were 0.1 mm apart with three nets to thread
   between them.
2. **Keep L1 → U2 → D2 → C9 as a tight group** (that's the switching loop, it
   wants to stay compact) but give the *group* room on all sides.
3. **Leave a 3–4 mm clear lane** from the boost cluster down to the bottom
   edge — EN has to reach TELE, and FB/VBOOST have to reach the cap spine.
4. Don't sweat exact alignment. Rough and roomy beats tidy and tight.

Save it, tell me, and I'll read the positions and route.
