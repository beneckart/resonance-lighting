// Production fixture firmware version string.
//
// fleet_usb_bringup.py --expect-fw matches this exactly. New artifacts use
// fx-YYMMDD-<recipe7>-<class>; class b is a supervised bench/canary build.
#pragma once

#ifndef RES_FIXTURE_VERSION
  #ifdef RES_DEV_BUILD
    // Mutable bytes may share this name. Never promote a dev-cache image.
    #define RES_FIXTURE_VERSION "dev-local"
  #else
    #define RES_FIXTURE_VERSION "fx-260816-prtrel1-b"
  #endif
#endif
#define RES_BOARD_NAME "powerfeather_v2"
