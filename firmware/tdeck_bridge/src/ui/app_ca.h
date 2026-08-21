#pragma once
// CA Studio: program leases + live GH-CA knob tuning (NbProgramSet.params).
// Until the fixture-side params-re-apply fix lands (TODO.md → Firmware track),
// "apply" uses the release-then-re-lease workaround: one visible blip.
void appCaOpen();
