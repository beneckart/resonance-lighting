# ADR 0074/0075 partial fleet rollout -- 2026-09-01

## Outcome

The operator ended the tree visit after the ordinary fleet waves completed and
before the PROTECT cohort could be gathered. The bridge reports no active
maintenance campaign at handoff.

- Installed-tree census: 114 fixtures.
- Exact inspection artifact: 70 fixtures.
- Remaining prior artifacts: 44 fixtures.
- Confirmed physical canopy/downlights remaining: 5.
- The final PROTECT gather found no maintenance endpoints and uploaded nothing.

Success required a fresh mesh heartbeat with the exact new revision after the
job start and beyond the 25-second pending-verify gate. `F4042C` missed its
job's six-minute freshness deadline, but a supplemental read 18 seconds later
provided an exact-revision heartbeat only 581 ms old at 419,516 ms uptime after
a software reboot; it is included in the 70 proven fixtures.

## Artifact identity

- Revision: `fx-260831-f121868-b`
- Source commit: `7c1f71ddeb22152ecb81adc76a21fc2bd55e976f`
- Binary: `firmware/fixture/build/fx-260831-f121868-b/fixture.ino.bin`
- Size: 1,217,856 bytes
- SHA-256:
  `569fa5a584019e5b4d1dedcfbea832c2b72339246f12badb6042ee5adcd29c2e`
- Recipe: field profile, channel 11, basic listener, 300 mA precharge,
  120-second day/radio-off cadence, 12-second listen, WiFi profile label
  `party-in-the-woods-v1`, artifact variant `b`.

## Verified fixtures

```text
9D7884, 9E5954, 9E5A58, 9E5A5C, 9E5A74, 9E5A84, 9E5A88, 9E5A94
9E5AB0, 9E5AC8, 9E5AE0, 9E5AE4, 9E5B04, 9E5B10, 9E5B14, 9E5B18
9E5B34, 9E5B44, 9E5B48, 9E5B68, 9E5B8C, 9E668C, 9F0E4C, 9F0E5C
9F0E7C, 9F2638, 9F2664, 9F266C, 9F2680, 9F26AC, 9F26B0, 9F26BC
9F26C0, 9F26C4, 9F26D4, 9F26D8, 9F26E4, 9F26E8, 9F2718, 9F2720
9F2738, 9F275C, F2BDB0, F2BE08, F2BE0C, F2BE20, F2BE38, F2BE60
F2BE94, F2BEA4, F2BEE4, F2BEF4, F2BF54, F2BFE0, F3FC8C, F3FC90
F3FD50, F3FD60, F40174, F401A8, F40254, F40268, F402C4, F40310
F40314, F40350, F40364, F403DC, F403F0, F4042C
```

## Resume roster

These 44 fixtures still report prior artifacts. Battery state is a changing
snapshot, not a durable classification. At final census 39 reported PROTECT,
three reported tier 0, and two lacked a current tier field.

```text
9E5AD4, 9E5B5C, 9F0E30, 9F0E54, 9F2648, 9F268C, 9F2694, 9F26B4
9F2714, F2B7DC, F2B900, F2BCF0, F2BCF4, F2BD00, F2BDB4, F2BDC4
F2BDD4, F2BDFC, F2BE10, F2BE1C, F2BE3C, F2BE48, F2BE6C, F2BE70
F2BEB4, F2BF60, F2BF74, F2BF7C, F2BF8C, F2BF90, F3FC9C, F3FCAC
F3FD28, F3FD88, F4019C, F401CC, F401DC, F402A8, F402B8, F40308
F40348, F4035C, F40384, F4043C
```

Confirmed physical canopy/downlights in that roster are `9F0E54`, `F2B7DC`,
`F2BE48`, `F2BF8C`, and `F40384`. Eight other remaining registry rows have no
resolved physical role; do not silently count them as canopies.

`9F0E30` and `9F26B4` run older `-p` images. Both continue to report mesh
heartbeats, but dedicated ordinary gathers did not produce shared-WiFi
maintenance endpoints. Treat them as explicit rescue exceptions during the
resume pass.

## Job evidence

| Job | Scope | Result |
|---|---|---|
| `733D1066` | Initial 109-fixture gather | 16 endpoints captured; frozen; no upload |
| `C6221E09` | Wave 1 | 16 verified |
| `6D759E3E` | Wave 2 | 13 verified; `9F26B4` deferred |
| `F353EF6F` | Wave 3 | 13 job-verified plus supplemental proof for `F4042C` |
| `2F98053B` | Wave 4 | 14 verified |
| `FD87A601` | Wave 5 | 13 verified; `F2B900` deferred |
| `A228D405` | Five ordinary exceptions | No endpoints; no upload |
| `EB99FAB6` | First 14-board PROTECT gather | No endpoints; no upload; final bridge state inactive |

The final cleanup did not receive the exact phase-2 acknowledgement for
`EB99FAB6`; the subsequent live bridge status was idle job `00000000`,
`active=false`, zero targets, and zero dispatches. No campaign remained active
when the operator departed.

Canonical job ledgers are retained under
`ops/bench/data/Black Rock City/20260901-*-fleet-ota-job.jsonl`. Uploader result
files remain locally gitignored; the accepted counts and exact rosters are
preserved here.
