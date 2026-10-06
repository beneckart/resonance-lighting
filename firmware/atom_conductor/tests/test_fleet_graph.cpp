#include <stdio.h>
#include <string.h>

#include "core/fleet_graph.h"

#define CHECK(x)                                                               \
  do {                                                                         \
    if (!(x)) {                                                                \
      fprintf(stderr, "FAIL line %d: %s\n", __LINE__, #x);                    \
      return 1;                                                                \
    }                                                                          \
  } while (0)

static void id(uint8_t out[3], uint8_t value) {
  out[0] = 0xF4;
  out[1] = 0;
  out[2] = value;
}

int main() {
  AtomFleet fleet;
  atomFleetInit(fleet);
  uint8_t a[3], b[3], c[3], d[3], other[3];
  id(a, 1);
  id(b, 2);
  id(c, 3);
  id(d, 4);
  id(other, 5);

  CHECK(atomFleetObserveHeartbeat(fleet, a, -30, 900, true, 1));
  CHECK(atomFleetObserveHeartbeat(fleet, b, -40, 900, true, 1));
  CHECK(atomFleetObserveHeartbeat(fleet, c, -50, 900, true, 1));
  CHECK(atomFleetObserveHeartbeat(fleet, d, -60, 900, true, 1));
  CHECK(atomFleetObserveHeartbeat(fleet, other, -20, 900, true, 2));
  CHECK(atomFleetObserveEdge(fleet, a, b, -40));
  CHECK(atomFleetObserveEdge(fleet, b, c, -45));
  CHECK(atomFleetObserveEdge(fleet, b, d, -55));
  CHECK(atomFleetDirectedEdgeCount(fleet) == 3);

  AtomWaveTarget wave[8] = {};
  size_t n = atomFleetPlanWave(fleet, 1000, 5000, 1, wave, 8);
  CHECK(n == 4);
  CHECK(memcmp(wave[0].id, a, 3) == 0 && wave[0].layer == 0);
  CHECK(memcmp(wave[1].id, b, 3) == 0 && wave[1].layer == 1);
  CHECK(wave[2].layer == 2 && wave[3].layer == 2);
  CHECK(atomFleetFreshCount(fleet, 1000, 5000, 1) == 4);

  // A disconnected peer remains in the wave. Its Atom-RSSI distance supplies
  // a deterministic radial fallback layer.
  uint8_t isolated[3];
  id(isolated, 6);
  CHECK(atomFleetObserveHeartbeat(fleet, isolated, -72, 900, true, 1));
  n = atomFleetPlanWave(fleet, 1000, 5000, 1, wave, 8);
  CHECK(n == 5);
  bool foundIsolated = false;
  for (size_t i = 0; i < n; ++i) {
    if (memcmp(wave[i].id, isolated, 3) == 0) {
      foundIsolated = true;
      CHECK(wave[i].layer == 7); // (-30 - -72) / 6
    }
  }
  CHECK(foundIsolated);

  // Freshness and class are the only action filters; there is intentionally no
  // battery-tier input to this model.
  CHECK(atomFleetPlanWave(fleet, 6000, 5000, 1, wave, 8) == 0);

  printf("fleet_graph ok\n");
  return 0;
}
