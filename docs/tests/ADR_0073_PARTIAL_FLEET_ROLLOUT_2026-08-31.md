# ADR 0073 partial fleet rollout -- 2026-08-31

## Outcome

The operator ended the field session after the ordinary fleet and a small
PROTECT cohort were complete. The OTA campaign was frozen before the bridge was
left unattended.

- Fleet census: 112 fixtures.
- Exact ADR 0073 artifact: 96 fixtures.
- Rollback ADR 0072 artifact: 16 fixtures.
- The 91 fixtures that were non-PROTECT at rollout start all passed.
- Five explicitly targeted PROTECT fixtures passed.
- No OTA campaign remained active at handoff.

Success means a fresh mesh heartbeat reported the exact new revision after the
job start and after the 25-second pending-verify window. Upload ACK alone was
not accepted.

## Artifact identity

- Revision: `fx-260831-dc82da7-b`
- Source commit: `49e06b7c9d5c9c16838f49b3b1f32213383cdbb9`
- Binary: `firmware/fixture/build/fx-260831-dc82da7-b/fixture.ino.bin`
- Size: 1,220,192 bytes
- SHA-256: `7230f81ff1737fa51ebdd357b9249748e5712657469de82a4b5b75799ac83299`
- Recipe: field profile, channel 11, basic-listener, 300 mA precharge,
  120-second day sleep, 12-second wake listen, WiFi profile label
  `party-in-the-woods-v1`, artifact variant `b`.
- Rollback revision: `fx-260831-b3e2738-b`.

Because this laptop is being retired before the 16-fixture resume pass, the five
canonical immutable artifact files listed by ADR 0040 are retained at that
normally gitignored build path and explicitly included in this handoff commit.
This allows the next bench to upload the exact bytes instead of rebuilding.

## Updated fixtures

The following 96 fixtures reported the exact new revision:

```text
9D7884, 9E5954, 9E5A58, 9E5A5C, 9E5A74, 9E5A84, 9E5A88, 9E5A94
9E5AB0, 9E5AC8, 9E5AD4, 9E5AE0, 9E5AE4, 9E5B04, 9E5B10, 9E5B14
9E5B18, 9E5B34, 9E5B44, 9E5B48, 9E5B68, 9E5B8C, 9E668C, 9F0E4C
9F0E5C, 9F0E7C, 9F2638, 9F2664, 9F266C, 9F2680, 9F268C, 9F2694
9F26AC, 9F26B0, 9F26BC, 9F26C0, 9F26C4, 9F26D4, 9F26D8, 9F26E4
9F26E8, 9F2718, 9F2720, 9F2738, 9F275C, F2B7DC, F2BCF4, F2BD00
F2BDB0, F2BDC4, F2BDD4, F2BE08, F2BE0C, F2BE10, F2BE1C, F2BE20
F2BE38, F2BE3C, F2BE60, F2BE6C, F2BE70, F2BE94, F2BEA4, F2BEB4
F2BEE4, F2BEF4, F2BF54, F2BF60, F2BF7C, F2BF8C, F2BF90, F2BFE0
F3FC8C, F3FC90, F3FC9C, F3FCAC, F3FD50, F3FD60, F40174, F4019C
F401A8, F401CC, F401DC, F40254, F40268, F402B8, F402C4, F40308
F40310, F40314, F40348, F40350, F40364, F403DC, F403F0, F4042C
```

The five passes from the explicitly targeted PROTECT cohort were:

| Short MAC | Callsign |
|---|---|
| `F2B7DC` | Ponyta |
| `F2BDC4` | Batman |
| `F2BDD4` | Gengar |
| `F2BE08` | Yoshi |
| `F2BE10` | Donkey |

Eight updated fixtures reported tier 3 in the final snapshot. Three of those
(`9E5AD4`, `9F268C`, and `F40308`) had entered PROTECT after being updated as
part of the original non-PROTECT cohort. This is changing power state, not a
revision discrepancy.

## Resume roster

These 16 fixtures still reported the exact rollback revision and field profile
in the post-stop snapshot. All reported power tier 3. Battery values are stale
mesh snapshot values, not a present-time safety measurement.

| Short MAC | Callsign | Last battery | Note |
|---|---|---:|---|
| `9E5B5C` | Mipha | 3.196 V | No upload |
| `9F0E54` | Eevee | 3.201 V | No upload |
| `9F2648` | Cubone | 3.197 V | No upload |
| `9F2714` | Chunli | 3.048 V | No upload |
| `F2B900` | Cammy | 3.044 V | No upload |
| `F2BCF0` | Spyro | 3.177 V | No upload |
| `F2BDB4` | Abra | 3.286 V | HTTP connection closed; no new-revision proof |
| `F2BDFC` | Magmar | 3.065 V | No upload |
| `F2BE48` | Lando | 3.196 V | No upload |
| `F2BF74` | Qbert | 3.151 V | No upload |
| `F3FD28` | Skitty | 3.006 V | No upload |
| `F3FD88` | Torkoal | 3.142 V | No upload |
| `F402A8` | Unassigned | 3.201 V | No upload |
| `F4035C` | Dratini | 3.268 V | No upload |
| `F40384` | Leia | 3.218 V | No upload |
| `F4043C` | Joltik | 3.134 V | No upload |

The final dashboard snapshot was 2026-08-31 06:39:22 UTC. Its maintenance
campaign record for job `98A408F1` reported phase 2 and `active=false`.

## Job evidence

| Job | Scope | Result |
|---|---|---|
| `93D3A477` | 91 ordinary fixtures | 74 passed; 17 deferred without upload |
| `5EF38540` | 17 ordinary retry | 14 passed; 3 deferred without upload |
| `DAE7326C` | 3 direct ordinary stragglers | 3 passed |
| `609E75B2` | 21 PROTECT attempt | Safe preflight stop; no upload |
| `FFC67D91` | 8 PROTECT gather | Manually frozen after endpoint capture; no upload |
| `B606B487` | 6 captured PROTECT fixtures | 5 passed; `F2BDB4` unproven/failed |
| `98A408F1` | 6 next-phase PROTECT fixtures | No endpoints, no upload; manually frozen |

Tracked job ledgers are under `ops/bench/data/Black Rock City/`. Local
`*-ota-results.jsonl` files are intentionally gitignored by repository policy;
their result counts and the final exact-revision census are preserved here.
