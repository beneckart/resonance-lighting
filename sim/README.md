# sim/ — firmware pattern core → WASM

The firmware pattern engine compiled C++ → WebAssembly, so the `app/` twin renders patterns
**identically to the real fixtures** ("golden-frame parity"). Lets the controller/twin be
developed and tested without hardware.

**Status:** scaffold placeholder. Depends on Ben's firmware pattern engines (`../firmware/`,
upstream). See `../docs/research/03-ADDENDUM-B-doctrine-failure-ladder-continuation.md` (B2 testing).
