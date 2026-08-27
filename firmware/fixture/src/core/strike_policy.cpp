#include "strike_policy.h"

bool strikePolicyMayAttempt(StrikeOrigin origin, bool energyPermitted) {
  return origin == StrikeOrigin::OPERATOR_CONTROL || energyPermitted;
}
