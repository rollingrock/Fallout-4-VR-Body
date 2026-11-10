#include <catch2/catch_test_macros.hpp>

#include "api/ClaimLogDebounce.h"

using frik::api::ClaimLogDebounce;
using namespace std::chrono_literals;

namespace
{
    const ClaimLogDebounce::Clock::time_point T0{};
}

TEST_CASE("Claim log: a plain claim logs its start, and its end once the window has passed")
{
    ClaimLogDebounce log(1s);
    REQUIRE(log.onStart("A", false));
    log.onEnd("A", false, T0);
    REQUIRE(log.takeEnded(T0 + 999ms).empty());

    const auto ended = log.takeEnded(T0 + 1s);
    REQUIRE(ended.size() == 1);
    REQUIRE(ended[0].tag == "A");
    REQUIRE_FALSE(ended[0].isLeft);
    REQUIRE(ended[0].restarts == 0);
    REQUIRE(log.takeEnded(T0 + 5s).empty());
}

TEST_CASE("Claim log: a claim cleared and re-set every frame logs one start and one end")
{
    ClaimLogDebounce log(1s);
    REQUIRE(log.onStart("A", true));
    auto now = T0;
    for (int frame = 0; frame < 90; ++frame) {
        log.onEnd("A", true, now);
        REQUIRE(log.takeEnded(now).empty());
        REQUIRE_FALSE(log.onStart("A", true));
        now += 11ms;
    }
    log.onEnd("A", true, now);
    REQUIRE(log.takeEnded(now + 500ms).empty());

    const auto ended = log.takeEnded(now + 1s);
    REQUIRE(ended.size() == 1);
    REQUIRE(ended[0].restarts == 90);
}

TEST_CASE("Claim log: a re-set after the end was reported is a new claim")
{
    ClaimLogDebounce log(1s);
    REQUIRE(log.onStart("A", false));
    log.onEnd("A", false, T0);
    REQUIRE(log.takeEnded(T0 + 2s).size() == 1);
    REQUIRE(log.onStart("A", false));
}

TEST_CASE("Claim log: a claim dropped by a skeleton release without a clear logs its next start")
{
    ClaimLogDebounce log(1s);
    REQUIRE(log.onStart("A", false));
    REQUIRE(log.onStart("A", false));
}

TEST_CASE("Claim log: tags and hands are separate claims")
{
    ClaimLogDebounce log(1s);
    REQUIRE(log.onStart("A", false));
    REQUIRE(log.onStart("A", true));
    REQUIRE(log.onStart("B", false));
    log.onEnd("A", false, T0);
    REQUIRE_FALSE(log.onStart("A", false));
    REQUIRE(log.takeEnded(T0 + 1s).empty());
}
