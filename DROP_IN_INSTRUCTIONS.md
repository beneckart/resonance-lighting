# Drop-in instructions

Add these files to the repo:

- `LOG_APPEND_2026-05-11.md` — copy contents into root `LOG.md` near the top.
- `docs/research/POWERFEATHER_SDK_2_0_0_NOTES_2026-05-11.md`
- `docs/tests/POWERFEATHER_V2_SDK_2_VALIDATION_PLAN_2026-05-11.md`
- `docs/decisions/0019-adopt-powerfeather-sdk-2-for-v2-prototypes.md`

Suggested TODO additions:

```md
## PowerFeather V2 / SDK 2.x validation

- [ ] Confirm Elecrow boards are V2 by macro photo and I2C scan.
- [ ] Build minimal ESP-IDF >=5.2 project with PowerFeather-SDK 2.x and `POWERFEATHER_BOARD_V2`.
- [ ] Initialize V2 with `BatteryType::Generic_LFP` and a real LiFePO4 cell.
- [ ] Log MAX17260 voltage/current/SOC/temp/health/cycles/time estimates.
- [ ] Verify `VSQT` can be disabled while PowerFeather power-management I2C remains usable.
- [ ] Test IS31FL3741 matrix power-cycle over `VSQT`.
- [ ] Test charger policy retention/reapplication across warm boots and watchdog resets.
- [ ] Validate no-battery `Board.init()` operation from USB/VDC.
- [ ] Run the full `docs/tests/POWERFEATHER_V2_SDK_2_VALIDATION_PLAN_2026-05-11.md` test plan.
```
