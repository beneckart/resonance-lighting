#pragma once

// One identity for every T-Deck translation unit and every emitted surface.
// Mutable cache builds are deliberately never given a promotable field name.
#ifndef TDECK_FW_VERSION
  #ifdef TDECK_DEV_BUILD
    #define TDECK_FW_VERSION "dev-local"
  #else
    // Bump this value for every distinct retained field binary.
    #define TDECK_FW_VERSION "0.2.0-field2"
  #endif
#endif
