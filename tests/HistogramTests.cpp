#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <atomic>
#include <cmath>
#include <thread>
#include <vector>

#include "perf/Histogram.h"

using f4cf::perf::Histogram;
using namespace std::chrono_literals;
using Catch::Matchers::WithinAbs;

namespace
{
    constexpr std::uint64_t US = 1'000;

    /**
     * Exact nearest-rank percentile over the values 1..n (in any unit), which is what a histogram of them estimates.
     */
    std::uint64_t exactRank(const std::uint64_t n, const double percentile)
    {
        return static_cast<std::uint64_t>(std::ceil(percentile / 100.0 * static_cast<double>(n)));
    }
}

TEST_CASE("Histogram: bucket boundaries")
{
    // below 2^10 ns: flat 64 ns buckets
    STATIC_REQUIRE(Histogram::bucketIndex(0) == 0);
    STATIC_REQUIRE(Histogram::bucketIndex(63) == 0);
    STATIC_REQUIRE(Histogram::bucketIndex(64) == 1);
    STATIC_REQUIRE(Histogram::bucketIndex(1023) == 15);
    // each power of two from 2^10 on splits into 16 buckets
    STATIC_REQUIRE(Histogram::bucketIndex(1024) == 16);
    STATIC_REQUIRE(Histogram::bucketIndex(1024 + 63) == 16);
    STATIC_REQUIRE(Histogram::bucketIndex(1024 + 64) == 17);
    STATIC_REQUIRE(Histogram::bucketIndex(2047) == 31);
    STATIC_REQUIRE(Histogram::bucketIndex(2048) == 32);
    // 2^30 ns and up overflow
    STATIC_REQUIRE(Histogram::bucketIndex((1ull << 30) - 1) == Histogram::OVERFLOW_BUCKET - 1);
    STATIC_REQUIRE(Histogram::bucketIndex(1ull << 30) == Histogram::OVERFLOW_BUCKET);
    STATIC_REQUIRE(Histogram::bucketIndex(~0ull) == Histogram::OVERFLOW_BUCKET);
    STATIC_REQUIRE(Histogram::bucketLowerBound(Histogram::OVERFLOW_BUCKET) == 1ull << 30);
}

TEST_CASE("Histogram: buckets are contiguous, and no wider than 1/16 of their lower bound from 1 us on")
{
    for (std::size_t i = 0; i < Histogram::OVERFLOW_BUCKET; ++i) {
        const auto lower = Histogram::bucketLowerBound(i);
        const auto upper = Histogram::bucketUpperBound(i);
        REQUIRE(lower < upper);
        REQUIRE(Histogram::bucketIndex(lower) == i);
        REQUIRE(Histogram::bucketIndex(upper - 1) == i);
        REQUIRE(Histogram::bucketLowerBound(i + 1) == upper);
        if (i >= Histogram::SUB_BUCKETS) {
            REQUIRE((upper - lower) * Histogram::SUB_BUCKETS <= lower);
        }
    }
}

TEST_CASE("Histogram: an empty snapshot summarises to zero")
{
    Histogram histogram;
    const auto s = histogram.drain().summary();
    REQUIRE(s.count == 0);
    REQUIRE(s.totalMs == 0);
    REQUIRE(s.minMs == 0);
    REQUIRE(s.maxMs == 0);
    REQUIRE(s.p99Ms == 0);
}

TEST_CASE("Histogram: count, total, average, min and max are exact")
{
    Histogram histogram;
    histogram.record(1ms);
    histogram.record(3ms);
    histogram.record(2ms);

    const auto s = histogram.drain().summary();
    REQUIRE(s.count == 3);
    REQUIRE_THAT(s.totalMs, WithinAbs(6.0, 1e-9));
    REQUIRE_THAT(s.avgMs, WithinAbs(2.0, 1e-9));
    REQUIRE_THAT(s.minMs, WithinAbs(1.0, 1e-9));
    REQUIRE_THAT(s.maxMs, WithinAbs(3.0, 1e-9));
}

TEST_CASE("Histogram: percentiles are within 1/32 of the exact nearest rank")
{
    constexpr std::uint64_t n = 10'000;
    Histogram histogram;
    for (std::uint64_t i = 1; i <= n; ++i) {
        histogram.recordNs(i * US);
    }
    const auto snapshot = histogram.drain();

    for (const double percentile : { 1.0, 10.0, 50.0, 90.0, 95.0, 99.0, 99.9 }) {
        const auto exact = static_cast<double>(exactRank(n, percentile) * US);
        const auto estimate = static_cast<double>(snapshot.percentileNs(percentile));
        INFO("p" << percentile << ": exact " << exact << " ns, estimate " << estimate << " ns");
        REQUIRE_THAT(estimate, WithinAbs(exact, exact / 32.0));
    }
}

TEST_CASE("Histogram: the lowest and highest ranks are the exact min and max")
{
    Histogram histogram;
    histogram.recordNs(1'234'567);
    histogram.recordNs(7'654'321);
    histogram.recordNs(2'000'000);
    const auto snapshot = histogram.drain();
    REQUIRE(snapshot.percentileNs(0.0) == 1'234'567);
    REQUIRE(snapshot.percentileNs(100.0) == 7'654'321);
}

TEST_CASE("Histogram: a single sample is every percentile")
{
    Histogram histogram;
    histogram.recordNs(42'000);
    const auto s = histogram.drain().summary();
    REQUIRE_THAT(s.p50Ms, WithinAbs(0.042, 1e-12));
    REQUIRE_THAT(s.p99Ms, WithinAbs(0.042, 1e-12));
}

TEST_CASE("Histogram: overflow samples read as the max")
{
    Histogram histogram;
    histogram.recordNs(10 * US);
    histogram.record(2s);
    histogram.record(3s);
    const auto snapshot = histogram.drain();
    REQUIRE(snapshot.buckets[Histogram::OVERFLOW_BUCKET] == 2);
    REQUIRE(snapshot.percentileNs(50.0) == 3'000'000'000);
    REQUIRE(snapshot.maxNs == 3'000'000'000);
}

TEST_CASE("Histogram: a negative duration clamps to zero")
{
    Histogram histogram;
    histogram.record(-5ms);
    const auto snapshot = histogram.drain();
    REQUIRE(snapshot.count == 1);
    REQUIRE(snapshot.minNs == 0);
    REQUIRE(snapshot.sumNs == 0);
}

TEST_CASE("Histogram: a drain starts the histogram empty")
{
    Histogram histogram;
    histogram.record(5ms);
    REQUIRE(histogram.drain().count == 1);

    const auto empty = histogram.drain();
    REQUIRE(empty.count == 0);
    REQUIRE(empty.sumNs == 0);
    REQUIRE(empty.minNs == 0);
    REQUIRE(empty.maxNs == 0);

    // min and max restart too, rather than remembering the drained window
    histogram.record(7ms);
    const auto next = histogram.drain();
    REQUIRE(next.minNs == 7'000'000);
    REQUIRE(next.maxNs == 7'000'000);
}

TEST_CASE("Histogram: merging snapshots equals recording everything into one")
{
    Histogram a;
    Histogram b;
    Histogram both;
    for (std::uint64_t i = 1; i <= 500; ++i) {
        a.recordNs(i * 3 * US);
        both.recordNs(i * 3 * US);
    }
    for (std::uint64_t i = 1; i <= 300; ++i) {
        b.recordNs(i * 7 * US + 11);
        both.recordNs(i * 7 * US + 11);
    }

    auto merged = a.drain();
    merged.merge(b.drain());
    const auto expected = both.drain();
    REQUIRE(merged.buckets == expected.buckets);
    REQUIRE(merged.count == expected.count);
    REQUIRE(merged.sumNs == expected.sumNs);
    REQUIRE(merged.minNs == expected.minNs);
    REQUIRE(merged.maxNs == expected.maxNs);
}

TEST_CASE("Histogram: merging into an empty snapshot takes the other's min")
{
    Histogram histogram;
    histogram.record(4ms);
    Histogram::Snapshot total;
    total.merge(histogram.drain());
    total.merge(Histogram::Snapshot{});
    REQUIRE(total.count == 1);
    REQUIRE(total.minNs == 4'000'000);
}

TEST_CASE("Histogram: concurrent recording loses nothing")
{
    constexpr int threads = 4;
    constexpr std::uint64_t perThread = 100'000;
    Histogram histogram;
    std::vector<std::thread> workers;
    for (int t = 0; t < threads; ++t) {
        workers.emplace_back([&histogram, t] {
            for (std::uint64_t i = 1; i <= perThread; ++i) {
                histogram.recordNs(i + static_cast<std::uint64_t>(t));
            }
        });
    }
    for (auto& worker : workers) {
        worker.join();
    }

    const auto snapshot = histogram.drain();
    REQUIRE(snapshot.count == threads * perThread);
    std::uint64_t expectedSum = 0;
    for (int t = 0; t < threads; ++t) {
        expectedSum += perThread * (perThread + 1) / 2 + perThread * static_cast<std::uint64_t>(t);
    }
    REQUIRE(snapshot.sumNs == expectedSum);
    REQUIRE(snapshot.minNs == 1);
    REQUIRE(snapshot.maxNs == perThread + threads - 1);
}

TEST_CASE("Histogram: draining while others record keeps every sample exactly once")
{
    constexpr int threads = 3;
    constexpr std::uint64_t perThread = 200'000;
    Histogram histogram;
    std::atomic<int> running{ threads };
    std::vector<std::thread> workers;
    for (int t = 0; t < threads; ++t) {
        workers.emplace_back([&] {
            for (std::uint64_t i = 1; i <= perThread; ++i) {
                histogram.recordNs(i % 5'000 * US);
            }
            running.fetch_sub(1);
        });
    }

    Histogram::Snapshot total;
    while (running.load() > 0) {
        const auto window = histogram.drain();
        // a split sample must never leave min or max outside the buckets the window counted
        if (window.count > 0) {
            REQUIRE(window.minNs <= window.maxNs);
            REQUIRE(window.percentileNs(50.0) >= window.minNs);
            REQUIRE(window.percentileNs(50.0) <= window.maxNs);
        }
        total.merge(window);
    }
    for (auto& worker : workers) {
        worker.join();
    }
    total.merge(histogram.drain());

    REQUIRE(total.count == threads * perThread);
}
