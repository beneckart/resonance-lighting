#include <cstdio>
#include <cstring>

#include "core/maintenance_campaign.h"

static int checks = 0;
static int failures = 0;

#define CHECK(expr)                                                           \
  do {                                                                        \
    ++checks;                                                                 \
    if (!(expr)) {                                                            \
      std::printf("FAIL line %d: %s\n", __LINE__, #expr);                    \
      ++failures;                                                             \
    }                                                                         \
  } while (0)

static void targetFor(unsigned value, uint8_t out[3]) {
  out[0] = (uint8_t)(value >> 16);
  out[1] = (uint8_t)(value >> 8);
  out[2] = (uint8_t)value;
}

int main() {
  MaintenanceCampaign campaign;
  MaintenanceCampaignStatus initial = campaign.status(100);
  CHECK(initial.phase == MAINT_CAMPAIGN_IDLE);
  CHECK(!campaign.begin(0, 1000, 100));
  CHECK(!campaign.begin(1, 0, 100));
  CHECK(!campaign.begin(1, MaintenanceCampaign::kMaxDurationMs + 1, 100));
  CHECK(campaign.begin(0x1234ABCD, 150000, 100));

  uint8_t zero[3] = {};
  uint8_t id[3] = {};
  targetFor(1, id);
  CHECK(!campaign.add(0xDEADBEEF, id));
  CHECK(!campaign.add(0x1234ABCD, zero));
  CHECK(campaign.add(0x1234ABCD, id));
  CHECK(campaign.add(0x1234ABCD, id));
  CHECK(campaign.status(100).targetCount == 1);

  for (unsigned i = 2; i <= 130; ++i) {
    targetFor(i, id);
    CHECK(campaign.add(0x1234ABCD, id));
  }
  MaintenanceCampaignStatus full = campaign.status(100);
  CHECK(full.targetCount == 130);
  CHECK(full.cycleMs == 1300);
  CHECK(full.remainingMs == 150000);

  uint8_t first[3] = {};
  uint8_t second[3] = {};
  CHECK(campaign.next(100, first));
  CHECK(!campaign.next(109, second));
  CHECK(campaign.next(110, second));
  CHECK(first[2] == 1);
  CHECK(second[2] == 2);

  uint8_t roundTrip[3] = {};
  uint32_t now = 120;
  for (unsigned i = 0; i < 128; ++i, now += 10) CHECK(campaign.next(now, id));
  CHECK(campaign.next(now, roundTrip));
  CHECK(std::memcmp(first, roundTrip, 3) == 0);
  CHECK(campaign.status(now).dispatchCount == 131);

  CHECK(!campaign.freeze(0x11111111, now));
  CHECK(campaign.freeze(0x1234ABCD, now));
  CHECK(campaign.status(now).phase == MAINT_CAMPAIGN_FROZEN);
  CHECK(!campaign.next(now + 1000, id));
  CHECK(!campaign.add(0x1234ABCD, id));
  CHECK(!campaign.freeze(0x1234ABCD, now + 1));

  CHECK(campaign.begin(0x87654321, 100, 0xFFFFFFF0UL));
  targetFor(44, id);
  CHECK(campaign.add(0x87654321, id));
  CHECK(campaign.next(0xFFFFFFF0UL, first));
  CHECK(campaign.status(0x00000020UL).phase == MAINT_CAMPAIGN_GATHER);
  CHECK(campaign.status(0x00000060UL).phase == MAINT_CAMPAIGN_EXPIRED);
  CHECK(!campaign.next(0x00000060UL, second));

  MaintenanceCampaign capacity;
  CHECK(capacity.begin(7, 1000, 0));
  for (unsigned i = 1; i <= MaintenanceCampaign::kCapacity; ++i) {
    targetFor(i, id);
    CHECK(capacity.add(7, id));
  }
  targetFor(MaintenanceCampaign::kCapacity + 1, id);
  CHECK(!capacity.add(7, id));

  std::printf("maintenance campaign: %d checks, %d failures\n", checks,
              failures);
  return failures ? 1 : 0;
}
