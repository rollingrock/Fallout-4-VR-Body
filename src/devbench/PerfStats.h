#pragma once

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <vector>

namespace frik::devbench
{
    /**
     * Per-site duration accumulator behind the devbench "perf" action.
     *
     * Accumulates from the last reset until it is read, so the window is whatever the caller
     * chooses between a reset and a read instead of a fixed log interval. Samples are kept for
     * exact percentiles up to a cap; past it only count/sum/min/max keep updating and the
     * summary says so. Game thread only, NOT thread-safe.
     */
    class PerfStats
    {
    public:
        using Clock = std::chrono::steady_clock;

        struct Summary
        {
            std::uint64_t count = 0;
            double windowMs = 0; // first sample to the read
            double totalMs = 0;
            double avgMs = 0;
            double minMs = 0;
            double maxMs = 0;
            double p95Ms = 0;
            double p99Ms = 0;
            double busyPct = 0; // totalMs / windowMs
            bool percentilesTruncated = false; // more samples than kept; p95/p99 cover the first ones only
        };

        static constexpr std::size_t kMaxSamples = 60'000; // ~11 min at 90 Hz, 480 KB

        void record(const std::chrono::nanoseconds duration, const Clock::time_point now)
        {
            const auto ns = (std::max)(duration.count(), std::int64_t{ 0 });
            if (_count == 0) {
                _first = now;
                _min = ns;
                _max = ns;
            } else {
                _min = (std::min)(_min, ns);
                _max = (std::max)(_max, ns);
            }
            ++_count;
            _sum += ns;
            if (_samples.size() < kMaxSamples) {
                _samples.push_back(ns);
            }
        }

        [[nodiscard]] Summary summary(const Clock::time_point now) const
        {
            Summary s;
            if (_count == 0) {
                return s;
            }
            s.count = _count;
            s.windowMs = std::chrono::duration<double, std::milli>(now - _first).count();
            s.totalMs = static_cast<double>(_sum) / NS_PER_MS;
            s.avgMs = s.totalMs / static_cast<double>(_count);
            s.minMs = static_cast<double>(_min) / NS_PER_MS;
            s.maxMs = static_cast<double>(_max) / NS_PER_MS;
            s.busyPct = s.windowMs > 0 ? s.totalMs / s.windowMs * 100.0 : 0;
            s.percentilesTruncated = _samples.size() < _count;

            auto sorted = _samples;
            std::sort(sorted.begin(), sorted.end());
            s.p95Ms = percentileMs(sorted, 95.0);
            s.p99Ms = percentileMs(sorted, 99.0);
            return s;
        }

        void reset()
        {
            _count = 0;
            _sum = 0;
            _samples.clear(); // keeps capacity
        }

    private:
        static constexpr double NS_PER_MS = 1'000'000.0;

        // nearest-rank percentile over an ascending vector
        static double percentileMs(const std::vector<std::int64_t>& sorted, const double percentile)
        {
            if (sorted.empty()) {
                return 0;
            }
            const auto rank = static_cast<std::size_t>(std::ceil(percentile / 100.0 * static_cast<double>(sorted.size())));
            const auto index = (std::min)(rank == 0 ? 0 : rank - 1, sorted.size() - 1);
            return static_cast<double>(sorted[index]) / NS_PER_MS;
        }

        std::uint64_t _count = 0;
        std::int64_t _sum = 0;
        std::int64_t _min = 0;
        std::int64_t _max = 0;
        Clock::time_point _first;
        std::vector<std::int64_t> _samples;
    };
}
