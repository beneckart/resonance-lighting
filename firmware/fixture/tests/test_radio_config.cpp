#include "test_util.h"

#include "radio_config.h"

int main() {
  // Production default: an omitted build flag must still join the fleet.
  CHECK_EQ(RES_CHANNEL, 11);

  // Legacy absent/channel-6 state migrates once to the compiled channel.
  CHECK_EQ(resolveRadioChannel(0, 0), 11);
  CHECK_EQ(resolveRadioChannel(6, 0), 11);
  CHECK(radioChannelNeedsPersist(0, 0));
  CHECK(radioChannelNeedsPersist(6, 0));

  // Current explicit lab channels remain supported.
  CHECK_EQ(resolveRadioChannel(1, RES_CHANNEL_POLICY_VERSION), 1);
  CHECK_EQ(resolveRadioChannel(6, RES_CHANNEL_POLICY_VERSION), 6);
  CHECK_EQ(resolveRadioChannel(11, RES_CHANNEL_POLICY_VERSION), 11);
  CHECK_EQ(resolveRadioChannel(13, RES_CHANNEL_POLICY_VERSION), 13);
  CHECK(!radioChannelNeedsPersist(11, RES_CHANNEL_POLICY_VERSION));

  // Corrupt/out-of-range NVS falls back safely.
  CHECK_EQ(resolveRadioChannel(14, RES_CHANNEL_POLICY_VERSION), 11);
  CHECK(radioChannelNeedsPersist(14, RES_CHANNEL_POLICY_VERSION));

  return testReport("radio_config");
}
