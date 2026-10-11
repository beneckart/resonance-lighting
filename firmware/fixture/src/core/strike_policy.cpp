#include "strike_policy.h"

bool strikePolicyMayAttempt(StrikeOrigin origin, bool energyPermitted) {
  return origin == StrikeOrigin::OPERATOR_CONTROL ||
         origin == StrikeOrigin::FIELD_RITUAL_BEST_EFFORT || energyPermitted;
}
