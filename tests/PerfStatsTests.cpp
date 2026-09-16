#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "devbench/PerfStats.h"

using frik::devbench::PerfStats;
using namespace std::chrono_literals;
using Catch::Matchers::WithinAbs;

namespace
{
    const PerfStats::Clock::time_point T0{};
}

TEST_CASE("PerfStats: empty stats summarise to zero")
{
    const PerfStats stats;
    const auto s = stats.summary(T0 + 5s);
    REQUIRE(s.count == 0);
    REQUIRE(s.windowMs == 0);
    REQUIRE_FALSE(s.percentilesTruncated);
}

TEST_CASE("PerfStats: count, total, min, max, average and busy over the reset-to-read window")
{
    PerfStats stats;
    stats.record(1ms, T0);
    stats.record(3ms, T0 + 100ms);
    stats.record(2ms, T0 + 200ms);

    const auto s = stats.summary(T0 + 1s);
    REQUIRE(s.count == 3);
    REQUIRE_THAT(s.windowMs, WithinAbs(1000.0, 1e-9));
    REQUIRE_THAT(s.totalMs, WithinAbs(6.0, 1e-9));
    REQUIRE_THAT(s.avgMs, WithinAbs(2.0, 1e-9));
    REQUIRE_THAT(s.minMs, WithinAbs(1.0, 1e-9));
    REQUIRE_THAT(s.maxMs, WithinAbs(3.0, 1e-9));
    REQUIRE_THAT(s.busyPct, WithinAbs(0.6, 1e-9));
}

TEST_CASE("PerfStats: nearest-rank percentiles")
{
    PerfStats stats;
    for (int i = 1; i <= 100; ++i) {
        stats.record(std::chrono::milliseconds(i), T0 + std::chrono::milliseconds(i));
    }
    const auto s = stats.summary(T0 + 1s);
    REQUIRE_THAT(s.p95Ms, WithinAbs(95.0, 1e-9));
    REQUIRE_THAT(s.p99Ms, WithinAbs(99.0, 1e-9));
}

TEST_CASE("PerfStats: a negative duration clamps to zero")
{
    PerfStats stats;
    stats.record(-5ms, T0);
    const auto s = stats.summary(T0 + 1s);
    REQUIRE(s.minMs == 0);
    REQUIRE(s.totalMs == 0);
}

TEST_CASE("PerfStats: reset drops everything and the next sample restarts the window")
{
    PerfStats stats;
    stats.record(4ms, T0);
    stats.reset();
    REQUIRE(stats.summary(T0 + 1s).count == 0);

    stats.record(2ms, T0 + 10s);
    const auto s = stats.summary(T0 + 11s);
    REQUIRE(s.count == 1);
    REQUIRE_THAT(s.windowMs, WithinAbs(1000.0, 1e-9));
    REQUIRE_THAT(s.minMs, WithinAbs(2.0, 1e-9));
}

TEST_CASE("PerfStats: past the sample cap count and extremes keep updating and the summary says percentiles are partial")
{
    PerfStats stats;
    for (std::size_t i = 0; i < PerfStats::kMaxSamples; ++i) {
        stats.record(1ms, T0);
    }
    REQUIRE_FALSE(stats.summary(T0 + 1s).percentilesTruncated);

    stats.record(9ms, T0 + 1s);
    const auto s = stats.summary(T0 + 2s);
    REQUIRE(s.count == PerfStats::kMaxSamples + 1);
    REQUIRE_THAT(s.maxMs, WithinAbs(9.0, 1e-9));
    REQUIRE_THAT(s.p99Ms, WithinAbs(1.0, 1e-9));
    REQUIRE(s.percentilesTruncated);
}
