#pragma once
// Knocker: single solenoid strike or deterministic targeted roll (client-side
// per-ID expansion; strikes are never broadcast) behind the confirm rail.
// Synchronized fire remains separate work behind the time/event seam. Fixtures
// refuse at night or below the full power tier.
void appKnockerOpen();
