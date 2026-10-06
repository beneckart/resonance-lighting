# 2026 postmortem and 2027 future work

Updated: 2026-10-06. Start here for the post-event storage, power/recovery and
service-tooling work. This index gathers the records already in this repository;
it is not a claim that every 2026 installation issue has been investigated.
Open work remains in [TODO](../../TODO.md); dated history is in [LOG](../../LOG.md).

## Evidence and operating state

| Record | What it establishes | What remains unresolved |
|---|---|---|
| [September 12 container census and storage](../tests/CONTAINER_STORAGE_CENSUS_2026-09-12.md) | 106 observed fixtures received the exact storage image and acknowledged indefinite ship requests; final radio-quiet check passed. | Assembled storage current and physical wake; eight originally unobserved fixtures were not commanded. |
| [Storage and wake operator guide](../howto/FIXTURE_STORAGE_AND_WAKE.md) | Mode distinctions, T-Deck/host workflow, deployed artifact identities and evidence links. | USB/solar/QON wake and RESET-wake qualification, plus physical UI inspection. |
| [September 14 Oakland inventory](../tests/OAKLAND_LANTERN_INVENTORY_2026-09-14.md) | Five identified sightings, none in the stored 106; Kairi/Wooper account for two original census absences. | Map the remaining bin fixtures, inspect cells and locate the six remaining unknowns. Ben wants the Oakland units operational, without deliberate storage. |
| [High-voltage PROTECT investigation](../tests/PROTECT_RECOVERY_INVESTIGATION_2026-09-14.md) | 40 of 74 PROTECT reports were >=3.25 V; offline reproduction exposes an awake-only recovery-proof limitation. | This does not prove 40 healthy cells or 40 false latches. Preserve specimens and measure full sleep/wake charge cycles. |
| [August 26 fleet OTA postmortem](../tests/FLEET_OTA_120X3_ROLLOUT_POSTMORTEM_2026-08-26.md) | Earlier rollout evidence and lessons. | Use alongside later firmware/field records, not as the current deployment state. |

The curated [storage evidence](../../ops/bench/data/ca/20260912-155847-container/README.md)
includes exact rosters, OTA verification, matched storage receipts, final
reconciliation and SHA-256 identities. Large raw captures remain local, as do
private device backups. Field observations and synthetic reproductions are
explicitly distinguished from measured electrical behavior.

## Lessons to carry into planning

- Chimes were a strong part of the installation in Ben's field experience.
  Both capboard revisions already support optional divider sensing; weak/null
  underfilled-cap strikes are an acceptable outcome. Preserve the working
  strike path and prioritize command reception, lighting and presence sensing.
- A high resting LFP voltage is not a capacity measurement. Recovery evidence
  must cover sleep and listening energy, charger state and durable entry cause.
- Wake Fleet/Performance Hold do not clear PROTECT or make its radios listen
  continuously. Reduced chimes during the last Burning Man days also have a
  known firmware confounder: September 1 inspection images disabled autonomous
  chimes. The relative contributions remain unresolved.
- Service wake should get a fixture into a useful, bounded maintenance session.
  A hardware reset alone can preserve PROTECT and promptly return to sleep.
  Instant wake could remove OTA gathering delays, especially at a 15-minute
  PROTECT cadence; upload time and verified reboot remain separate costs.

## Brainstorming and proposed experiments

These are discussion drafts, not approved hardware, a BOM or a fleet rollout:

- [2027 power-platform exploration](../projects/2027-power-platform/EXPLORATION.md):
  autonomous LFP charging/recovery, private power bus, solar energy measurement,
  gauge/SOC qualification, monitoring overhead, independent cutoff, sleep/wake
  contracts and a broad feature keep/improve/consolidate/retire review.
- [RFID service access and instant maintenance wake](../projects/2027-power-platform/RFID_SERVICE_ACCESS.md):
  battery-free identification, independent targeted reset/ship wake, RTC
  coin-cell sharing, passive versus powered actuation range, retained service
  intent, and fleet OTA latency/coverage. No part or reader has been selected.
- [Future OTA speed options](../design/FLEET_OTA_FUTURE_SPEED_OPTIONS.md):
  additional transport and activation improvements, with separate proof needs.

Keep the optional chime-cap divider as a diagnostic option, rather than adding
another strike gate. Keep useful gauge/current measurements while testing the
SOC model on both cell sizes. Compare complete standby and receive energy, not
just individual component headline currents.

## Next useful work

1. Preserve Oakland specimen state; characterize actual cells, input power and
   recovery across a complete sleep/wake cycle before selecting a PROTECT fix.
2. Qualify the existing storage/wake implementation during physical service.
3. Review the complete operator workflow and prioritize a small number of
   measured improvements before selecting a custom board or retiring features.
4. Prove one addressed independent wake/reset candidate on an assembled lantern,
   including power-off operation, range, leakage and repeated-request behavior.

The T-Deck controlled un-PROTECT command remains explicitly deferred at Ben's
request. No new architecture was adopted by this planning session; accepted
contracts remain in [the ADRs](../decisions/).
