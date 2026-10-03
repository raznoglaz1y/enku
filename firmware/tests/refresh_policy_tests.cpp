#include "enku/core/refresh.hpp"

#include <cassert>

using namespace enku;

int main() {
    RefreshGhostingPolicy policy;

    assert(policy.limit() == 5U);
    assert(policy.nonCleanUpdates() == 0U);
    assert(!policy.shouldForceCleanFull());

    for (std::uint32_t i = 0; i < 4U; ++i) {
        policy.recordNonCleanUpdate();
        assert(!policy.shouldForceCleanFull());
    }

    policy.recordNonCleanUpdate();
    assert(policy.nonCleanUpdates() == 5U);
    assert(policy.shouldForceCleanFull());

    // Saturating the counter avoids overflow during a long sequence of
    // failed/ignored clean-refresh attempts.
    policy.recordNonCleanUpdate();
    assert(policy.nonCleanUpdates() == 5U);
    assert(policy.shouldForceCleanFull());

    policy.recordCleanFull();
    assert(policy.nonCleanUpdates() == 0U);
    assert(!policy.shouldForceCleanFull());

    RefreshGhostingPolicy custom{2U};
    custom.recordNonCleanUpdate();
    assert(!custom.shouldForceCleanFull());
    custom.recordNonCleanUpdate();
    assert(custom.shouldForceCleanFull());

    // A zero limit is nonsensical for callers; clamp it to one update.
    RefreshGhostingPolicy clamped{0U};
    assert(clamped.limit() == 1U);
    assert(!clamped.shouldForceCleanFull());
    clamped.recordNonCleanUpdate();
    assert(clamped.shouldForceCleanFull());

    return 0;
}
