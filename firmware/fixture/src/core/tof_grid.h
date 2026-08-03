// VL53L5CX ULD result-grid selection (pure/native). The ULD's per-target
// arrays (distance_mm, target_status) are zone-major -- element index
// zone * targetsPerZone + target -- while nb_target_detected is per-zone;
// entries past nb_target_detected[zone] are stale from earlier frames.
#pragma once

#include <stdint.h>

// Scan zones 0..zones-1. Per zone keep the FARTHEST valid target as the
// ground candidate: the 2-targets-per-zone vendored edit exists so an
// occluded zone can report the bamboo splay (near) AND the floor behind it
// (far), and the far return is the one on the ground plane (sway_demo donor
// idiom). closestMm gets the nearest valid return of any zone/target (the
// presence proxy: a person in front of the floor IS the near target).
// Valid = target status 5 or 9 and minMm < mm < maxMm.
// zoneMm[z] = chosen distance, 0 = no valid target. Returns zones kept.
uint8_t l5cxSelectGround(const int16_t *distanceMm, const uint8_t *targetStatus,
                         const uint8_t *nbTargetDetected, uint8_t zones,
                         uint8_t targetsPerZone, int16_t minMm, int16_t maxMm,
                         uint16_t *zoneMm, uint16_t *closestMm);
