// Production fixture firmware version string.
//
// fleet_usb_bringup.py --expect-fw matches this exactly; bump the date/counter
// on every artifact that gets flashed to a board (same convention as net_bench:
// <sketch>-<yyyy-mm-dd>.<n>).
#pragma once

#define RES_FIXTURE_VERSION "fixture-2026-08-06.5"
#define RES_BOARD_NAME "powerfeather_v2"
