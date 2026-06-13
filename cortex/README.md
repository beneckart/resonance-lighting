# cortex/ — Jetson hub services

Python services on the Jetson Orin Nano (trunk base): twin-server, occupancy, env, voice,
camp bridge, db (SQLite + sqlite-vec). The **PowerFeather master is the ESP-NOW radio-modem**
over framed USB serial — the cortex has no ESP-NOW radio of its own.

**Principle:** the mesh is a self-sufficient brainstem; the cortex wakes at dusk and dies
invisibly. Day / cortex-dead = master SoftAP + lite page.

**Status:** scaffold placeholder. See `../docs/research/PRD-lighting-environment.md` §4–6 and
`../docs/research/01-MASTER-DESIGN-REPORT-v1.0.md` PART 5 (Central Brain).
