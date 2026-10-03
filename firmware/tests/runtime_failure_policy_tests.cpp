#include "enku/runtime/runtime_failure_policy.hpp"

#include <cassert>

using namespace enku;

int main() {
    RuntimeFailurePolicy policy;

    assert(policy.threshold() == 3U);
    assert(policy.consecutiveFailures() == 0U);
    assert(!policy.shouldRecover());

    assert(!policy.recordFailure());
    assert(policy.consecutiveFailures() == 1U);

    assert(!policy.recordFailure());
    assert(policy.consecutiveFailures() == 2U);

    // A successful cycle breaks the failure streak.
    policy.recordSuccess();
    assert(policy.consecutiveFailures() == 0U);
    assert(!policy.shouldRecover());

    assert(!policy.recordFailure());
    assert(!policy.recordFailure());
    assert(policy.recordFailure());
    assert(policy.shouldRecover());
    assert(policy.consecutiveFailures() == 3U);

    // Saturate at the threshold so long-lived failures cannot overflow.
    assert(policy.recordFailure());
    assert(policy.consecutiveFailures() == 3U);

    policy.recordSuccess();
    assert(!policy.shouldRecover());

    RuntimeFailurePolicy clamped{0U};
    assert(clamped.threshold() == 1U);
    assert(clamped.recordFailure());

    return 0;
}
