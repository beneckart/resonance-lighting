// USB serial commands (peer-only; the desk bridge remains a net_bench master).
//
// Kept from the net_bench contract (fleet_usb_bringup depends on t and u):
//   t            one-line telemetry JSON
//   u            local ENTER_MAINT (the USB commissioning path)
//   c            resume comms from maintenance
//   C<mah>       persist gauge capacity, reboot to apply (C alone: report)
//   G<ma>        persist/apply charge-current cap
//   X            guarded clear of persisted PROTECT for USB bare-board service
//   K<id>:<ms>   local solenoid strike when targeted at this unit
//   S[<secs>]    timed deep sleep (bare = 6 h remote-park default)
//   r            role/mode/config one-liner
// New:
//   O<0..4>      class override (0 = auto probe)   [persists]
//   F<0|1>       profile commission/field          [persists]
#pragma once

void handleSerial();
