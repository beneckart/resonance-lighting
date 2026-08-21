#pragma once
// Knocker: single solenoid strike or knock-all (client-side per-ID expansion —
// strikes are never broadcast) behind the confirm rail. Synced schedules are
// an ADR 0031 stub. Fixtures refuse at night / below full power tier.
void appKnockerOpen();
