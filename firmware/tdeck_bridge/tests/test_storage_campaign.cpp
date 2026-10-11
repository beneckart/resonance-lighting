#include "core/storage_campaign.h"
#include <cassert>
#include <cstdio>
int main() {
  const uint8_t ids[2][3] = {{1, 2, 3}, {4, 5, 6}};
  const uint8_t all[1][3] = {{0, 0, 0}};
  StorageCampaign c;
  assert(!c.begin(all, 1, 2, 0, 0));
  assert(!c.begin(ids, 2, 0, 0, 0));
  assert(!c.begin(ids, 2, 0, 604801, 0));
  assert(!c.begin(ids, 2, 2, 1, 0));
  assert(c.begin(ids, 2, 0, 604800, 0xFFFFFF00UL));
  size_t i = 99;
  uint8_t id[3];
  uint32_t seconds;
  assert(c.next(0xFFFFFF00UL, i, id, seconds));
  assert(seconds == 604800 && i == 0);
  assert(c.next(744, i, id, seconds)); // one second later across millis rollover
  assert(seconds == 604799 && i == 1);
  assert(!c.begin(ids, 2, 2, 0, 0)); // cannot replace a running campaign
  assert(!c.status(959744).active);
  assert(c.begin(ids, 2, 2, 0, 100));
  c.receipt(0, 1);
  assert(c.next(100, i, id, seconds) && i == 1 && seconds == 0);
  c.receipt(1, 4);
  assert(!c.next(200, i, id, seconds));
  assert(c.status(200).prepared == 1 && c.status(200).refused == 1);
  c.receipt(0, 5); // later electrical failure supersedes PREPARED
  assert(c.status(200).prepared == 0 && c.status(200).refused == 2);
  c.receipt(0, 1); // reordered duplicate must not erase the electrical failure
  assert(c.status(200).prepared == 0 && c.status(200).refused == 2);
  c.stop();
  assert(!c.status(201).active);
  puts("storage campaign ok");
}
