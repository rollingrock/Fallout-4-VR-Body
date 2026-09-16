#include "devbench/PerfProbe.h"

#include "devbench/DevBenchBridge.h"

namespace frik::devbench
{
    namespace
    {
        std::vector<PerfProbe*>& registry()
        {
            static std::vector<PerfProbe*> probes;
            return probes;
        }
    }

    PerfProbe::PerfProbe(std::string name)
        : _name(std::move(name)),
          _log(_name)
    {
        registry().push_back(this);
    }

    void PerfProbe::record(const std::chrono::nanoseconds duration)
    {
        if (logger::isDebugEnabled()) {
            _log.record(duration);
        }
        if (isCollecting()) {
            _stats.record(duration, PerfStats::Clock::now());
        }
    }

    const std::vector<PerfProbe*>& PerfProbe::all()
    {
        return registry();
    }

    bool PerfProbe::isCollecting()
    {
        return g_devBenchBridge.isArmed();
    }
}
