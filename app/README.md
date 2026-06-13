# app/ — iPad R3F PWA (twin + console)

React-Three-Fiber Progressive Web App: the hardware-true 3-D digital twin + control console.
Screens: Jam · Cues · Schedule · Monitor · Commission · Twin. Offline-first, red-shifted night UI.

**Status:** scaffold placeholder. First build (gated on `fixtures.json` from the Blender workflow):
the Twin viewer loading glTF + `fixtures.json`, rendering each fixture's *reported* state in
physical units (lumens/beam/CCT). See `../docs/research/PRD-lighting-environment.md` §3–8 and
`../docs/research/04-ADDENDUM-C-dj-vj-midi-blender.md`.

Stack: TypeScript · React Three Fiber · zustand · CBOR. Control plane: WS binary tweak frames
(commit path = JSON desired-state docs). Never streams pixels.
