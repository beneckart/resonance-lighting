// Production fixture firmware version string.
//
// fleet_usb_bringup.py --expect-fw matches this exactly; bump the date/counter
// on every artifact that gets flashed to a board (same convention as net_bench:
// <sketch>-<yyyy-mm-dd>.<n>).
#pragma once

#ifdef RES_FIXTURE_VERSION_TOKEN
#define RES_VERSION_STRINGIFY_INNER(value) #value
#define RES_VERSION_STRINGIFY(value) RES_VERSION_STRINGIFY_INNER(value)
#define RES_FIXTURE_VERSION RES_VERSION_STRINGIFY(RES_FIXTURE_VERSION_TOKEN)
#else
#define RES_FIXTURE_VERSION "fixture-2026-08-15.4"
#endif
#define RES_BOARD_NAME "powerfeather_v2"
