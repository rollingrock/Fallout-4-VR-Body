#pragma once

#include <string>
#include <vector>

#include "common/PerfMonitor.h"
#include "devbench/PerfStats.h"

namespace frik::devbench
{
    /**
     * A measured site: the framework's PerfMonitor for its periodic log line (debug level only) plus
     * a PerfStats accumulator the devbench "perf" action reads and resets, active whenever devbench
     * has armed the bridge. Costs one level check and one relaxed load per call while both are off.
     *
     * One static probe per site; construction registers it, so the first call must be on the game
     * thread (every site is inside onFrameUpdate). Never destroyed: the plugin is not unloaded.
     *
     *     static devbench::PerfProbe perf("Pipboy::onFrameUpdate");
     *     const auto timer = perf.scope();
     */
    class PerfProbe
    {
    public:
        class ScopedTimer
        {
        public:
            explicit ScopedTimer(PerfProbe* const probe)
                : _probe(probe),
                  _start(probe ? PerfStats::Clock::now() : PerfStats::Clock::time_point{})
            {}

            ScopedTimer(const ScopedTimer&) = delete;
            ScopedTimer& operator=(const ScopedTimer&) = delete;
            ScopedTimer& operator=(ScopedTimer&&) = delete;

            ScopedTimer(ScopedTimer&& other) noexcept
                : _probe(other._probe),
                  _start(other._start)
            {
                other._probe = nullptr;
            }

            ~ScopedTimer()
            {
                if (_probe) {
                    _probe->record(PerfStats::Clock::now() - _start);
                }
            }

        private:
            PerfProbe* _probe;
            PerfStats::Clock::time_point _start;
        };

        explicit PerfProbe(std::string name);

        [[nodiscard]] ScopedTimer scope()
        {
            return ScopedTimer(logger::isDebugEnabled() || isCollecting() ? this : nullptr);
        }

        void record(const std::chrono::nanoseconds duration);

        [[nodiscard]] const std::string& name() const
        {
            return _name;
        }

        [[nodiscard]] const PerfStats& stats() const
        {
            return _stats;
        }

        void resetStats()
        {
            _stats.reset();
        }

        /// Every probe constructed so far, in construction order. Game thread.
        [[nodiscard]] static const std::vector<PerfProbe*>& all();

        /// True while the devbench bridge is armed, so the accumulator collects without debug logging.
        [[nodiscard]] static bool isCollecting();

    private:
        std::string _name;
        common::PerfMonitor _log;
        PerfStats _stats;
    };
}
