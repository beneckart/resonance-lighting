# Deliberate USB service without consuming collapse retries

Use on a running fixture with ADR 0087 service-capable firmware. Name one
target and coordinate its writer before opening USB. Never select a device
by COM-port order. Keep the production battery connected.

1. Capture fresh identity/power/history and retain the exact artifact manifest.
2. Prepare and enter ROM with a new evidence path:

   ```text
   python ops/bench/fixture_usb_service.py --target F40308 --enter-rom --out <new.jsonl>
   ```

3. The helper requires a matching READY plus fresh all-loads-off telemetry.
   On success it immediately enters ROM and verifies the same full MAC.
   Use `--before no-reset` for subsequent inspected ROM backup/write/verify
   operations. Preserve NVS, partition selection and the other app slot.
4. Reset out of ROM only after verification, then require the expected revision,
   stable valid application, battery/sensor checks and unchanged history.

Preparation expires after 30 s if no ROM entry occurs. The fixture then
software-reboots with the tier preserved. `!B<id>:0` cancels the same way.
Do not keep resending prepare: it cannot renew the deadline. A refusal or
missing acknowledgement does not authorize a raw reset fallback.

Old firmware cannot implement this handshake retroactively. Bootstrap it
through the established exact-target OTA path first. The bare-board factory
commissioner remains available for new battery-absent boards; its explicit
battery-present path now requires this service handshake and keeps the chip
in ROM between identification and upload. Ad hoc esptool commands and the
development build wrapper's raw upload still bypass preparation; they are not
the supported service path for installed fixtures.

PROTECT is never cleared by preparing service. A reset during actual armed
operation retains the original FULL -> DIM -> PROTECT collapse protection.
See ADR 0087 and the Daisy October 16 qualification record.
