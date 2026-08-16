// Production fixture firmware version string.
//
// fleet_usb_bringup.py --expect-fw matches this exactly. New artifacts use
// fx-YYMMDD-<recipe7>-<class>; class b is a supervised bench/canary build.
#pragma once

#define RES_FIXTURE_VERSION "fx-260816-prtrel1-b"
#define RES_BOARD_NAME "powerfeather_v2"
