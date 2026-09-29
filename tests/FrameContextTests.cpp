#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <sstream>
#include <string>
#include <thread>
#include <vector>

#include "perf/FrameContext.h"
#include "perf/Report.h"

namespace perf = f4cf::perf;
using namespace std::chrono_literals;
using Catch::Matchers::WithinAbs;

namespace
{
    std::vector<std::string> lines(const std::string& text)
    {
        std::vector<std::string> out;
        std::istringstream in(text);
        std::string line;
        while (std::getline(in, line)) {
            out.push_back(line);
        }
        return out;
    }

    perf::CompositorFrame compositorFrame(const float gpuMs)
    {
        perf::CompositorFrame frame;
        frame.gpuMs = gpuMs;
        frame.gameGpuMs = gpuMs - 1.0f;
        frame.compositorGpuMs = 1.0f;
        frame.lateStartMs = 0.25f;
        frame.headroomMs = 2.0f;
        return frame;
    }
}

TEST_CASE("Frame context: frame intervals and compositor frames add up until a reset")
{
    perf::reset();
    perf::recordFrameInterval(11ms);
    perf::recordFrameInterval(12ms);

    auto late = compositorFrame(9.0f);
    late.reprojectedCpu = true;
    late.dropped = 2;
    late.misPresented = 1;
    perf::recordCompositorFrame(compositorFrame(8.0f));
    perf::recordCompositorFrame(late);

    const auto frame = perf::readFrameContext();
    REQUIRE(frame.interval.count == 2);
    REQUIRE_THAT(frame.interval.summary().avgMs, WithinAbs(11.5, 1e-9));
    REQUIRE(frame.compositorFrames == 2);
    REQUIRE_THAT(frame.gpu.summary().maxMs, WithinAbs(9.0, 1e-6));
    REQUIRE_THAT(frame.gameGpu.summary().minMs, WithinAbs(7.0, 1e-6));
    REQUIRE_THAT(frame.compositorGpu.summary().avgMs, WithinAbs(1.0, 1e-6));
    REQUIRE_THAT(frame.lateStart.summary().avgMs, WithinAbs(0.25, 1e-6));
    REQUIRE_THAT(frame.headroom.summary().avgMs, WithinAbs(2.0, 1e-6));
    REQUIRE(frame.reprojectedCpu == 1);
    REQUIRE(frame.reprojectedGpu == 0);
    REQUIRE(frame.dropped == 2);
    REQUIRE(frame.misPresented == 1);

    perf::reset();
    const auto empty = perf::readFrameContext();
    REQUIRE(empty.interval.count == 0);
    REQUIRE(empty.compositorFrames == 0);
    REQUIRE(empty.gpu.count == 0);
    REQUIRE(empty.dropped == 0);
}

TEST_CASE("Frame context: a pose request that came early is recorded as no late start")
{
    perf::reset();
    auto early = compositorFrame(8.0f);
    early.lateStartMs = -3.0f;
    perf::recordCompositorFrame(early);
    REQUIRE(perf::readFrameContext().lateStart.maxNs == 0);
}

TEST_CASE("Frame context: the refresh rate survives a reset and sets the budget")
{
    perf::setDisplayHz(90.0f);
    perf::reset();
    const auto frame = perf::readFrameContext();
    REQUIRE(frame.displayHz == 90.0f);
    REQUIRE_THAT(frame.budgetMs(), WithinAbs(11.111, 1e-3));
    perf::setDisplayHz(0.0f);
    REQUIRE(perf::readFrameContext().budgetMs() == 0.0);
}

TEST_CASE("Frame context: a site's share of the budget is its time per frame over the frame's time")
{
    perf::Histogram histogram;
    for (int i = 0; i < 4; ++i) {
        histogram.record(500us);
    }
    // 4 calls of 0.5 ms over 2 frames: 1 ms per frame, 10% of a 10 ms budget
    perf::Report report;
    report.frames = 2;
    report.frame.displayHz = 100.0f;
    const perf::Site::Stats stats{ histogram.drain(), 0 };
    REQUIRE_THAT(report.budgetPct(stats), WithinAbs(10.0, 1e-9));

    report.frame.displayHz = 0.0f;
    REQUIRE(report.budgetPct(stats) == 0.0);
}

TEST_CASE("Frame context: the text table opens with the frame, then the compositor, and shows each site's share of the budget")
{
    const perf::Site site("TextFrameTest::onFrame", nullptr);
    perf::Histogram durations;
    durations.record(1ms);
    durations.record(1ms);

    perf::Histogram interval;
    interval.record(10ms);
    interval.record(10ms);
    perf::Histogram gpu;
    gpu.record(8ms);

    perf::Report report;
    report.window = 1s;
    report.frames = 2;
    report.frame.displayHz = 100.0f;
    report.frame.interval = interval.drain();
    report.frame.compositorFrames = 1;
    report.frame.gpu = gpu.drain();
    report.frame.reprojectedGpu = 1;
    report.threads.push_back({ std::this_thread::get_id(), true, { { &site, { durations.drain(), 0 }, {} } } });

    const auto rows = lines(perf::formatReport(report));
    REQUIRE(rows.size() == 7);
    REQUIRE(rows[1] == "frame: 100 Hz, budget 10.00 | interval p50 10.00 p95 10.00 p99 10.00 max 10.00");
    REQUIRE(rows[2].starts_with("gpu:   p50 8.00 "));
    REQUIRE(rows[3].starts_with("vr:    1 frames | reprojected cpu 0, gpu 1 | dropped 0 "));
    REQUIRE(rows[4].ends_with("%budget"));
    // 1 ms per frame of a 10 ms budget
    REQUIRE(rows[6].ends_with("     1.00     10.0"));
}
