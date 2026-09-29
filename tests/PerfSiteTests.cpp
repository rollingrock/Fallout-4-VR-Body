#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstring>
#include <iostream>
#include <new>
#include <string>
#include <thread>

#include "perf/Perf.h"

namespace perf = f4cf::perf;
using namespace std::chrono_literals;

namespace
{
    void spin(const std::chrono::microseconds duration)
    {
        const auto end = std::chrono::steady_clock::now() + duration;
        while (std::chrono::steady_clock::now() < end) {
        }
    }

    void macroUser()
    {
        F4CF_PERF_FUNCTION();
        {
            F4CF_PERF_SCOPE("block");
        }
    }

    /**
     * The site in a function whose name contains functionPart: its whole-function site for an empty label.
     */
    const perf::Site* findSite(const std::string_view functionPart, const std::string_view label)
    {
        for (const auto* site : perf::sites()) {
            if (std::string_view(site->function()).find(functionPart) == std::string_view::npos) {
                continue;
            }
            if (label.empty() ? site->isWholeFunction() : !site->isWholeFunction() && label == site->label()) {
                return site;
            }
        }
        return nullptr;
    }
}

TEST_CASE("Perf: a scope records nothing while recording is off")
{
    perf::setEnabled(false);
    static perf::Site site("Test::off", nullptr);
    {
        const perf::Scope scope(site);
    }
    REQUIRE(site.read().durations.count == 0);
    REQUIRE(site.caller() == nullptr);
}

TEST_CASE("Perf: nested scopes link their callers and split self from children time")
{
    perf::setEnabled(true);
    static perf::Site outer("Test::nested", nullptr);
    static perf::Site inner("Test::nested", "inner");
    for (int i = 0; i < 3; ++i) {
        const perf::Scope outerScope(outer);
        spin(20us);
        {
            const perf::Scope innerScope(inner);
            spin(50us);
        }
    }

    REQUIRE(inner.caller() == &outer);
    REQUIRE(outer.caller() == nullptr);
    REQUIRE_FALSE(inner.hasMultipleCallers());

    const auto outerStats = outer.read();
    const auto innerStats = inner.read();
    REQUIRE(outerStats.durations.count == 3);
    REQUIRE(innerStats.durations.count == 3);
    // every inner duration is charged to its caller as children time, to the nanosecond
    REQUIRE(outerStats.childrenNs == innerStats.durations.sumNs);
    REQUIRE(outerStats.selfNs() >= 3 * 20'000);
    REQUIRE(innerStats.childrenNs == 0);
    REQUIRE(innerStats.selfNs() == innerStats.durations.sumNs);
}

TEST_CASE("Perf: a site called from two places keeps its first caller and says it has more")
{
    perf::setEnabled(true);
    static perf::Site first("Test::first", nullptr);
    static perf::Site second("Test::second", nullptr);
    static perf::Site shared("Test::shared", nullptr);
    // the clock ticks every 100 ns on Windows, so an empty scope can measure 0
    {
        const perf::Scope a(first);
        const perf::Scope b(shared);
        spin(5us);
    }
    REQUIRE_FALSE(shared.hasMultipleCallers());
    {
        const perf::Scope a(second);
        const perf::Scope b(shared);
        spin(5us);
    }
    REQUIRE(shared.caller() == &first);
    REQUIRE(shared.hasMultipleCallers());
    // each call still charged its own caller
    REQUIRE(first.read().childrenNs > 0);
    REQUIRE(second.read().childrenNs > 0);
}

TEST_CASE("Perf: a site first seen with no caller takes its real one when it shows up")
{
    // recording switched on while the caller was already running: the inner site looks outermost at first
    perf::setEnabled(false);
    static perf::Site outer("Test::lateOuter", nullptr);
    static perf::Site inner("Test::lateInner", nullptr);
    {
        const perf::Scope outerScope(outer);
        perf::setEnabled(true);
        const perf::Scope innerScope(inner);
    }
    REQUIRE(inner.caller() == nullptr);

    {
        const perf::Scope outerScope(outer);
        const perf::Scope innerScope(inner);
    }
    REQUIRE(inner.caller() == &outer);
    REQUIRE_FALSE(inner.hasMultipleCallers());

    // and once it has a real caller, a call that looks outermost doesn't count as another
    perf::setEnabled(false);
    {
        const perf::Scope outerScope(outer);
        perf::setEnabled(true);
        const perf::Scope innerScope(inner);
    }
    REQUIRE(inner.caller() == &outer);
    REQUIRE_FALSE(inner.hasMultipleCallers());
}

TEST_CASE("Perf: a skipped destructor is put right when the next outer scope closes")
{
    perf::setEnabled(true);
    static perf::Site outer("Test::sehOuter", nullptr);
    static perf::Site abandoned("Test::sehAbandoned", nullptr);
    static perf::Site child("Test::sehChild", nullptr);
    static perf::Site after("Test::sehAfter", nullptr);
    {
        const perf::Scope outerScope(outer);
        // never destroyed, as when an SEH recovery skips C++ unwinding
        alignas(perf::Scope) std::byte storage[sizeof(perf::Scope)];
        new (storage) perf::Scope(abandoned);
        {
            // until outer closes, what runs next is charged to the abandoned site
            const perf::Scope childScope(child);
        }
        REQUIRE(child.caller() == &abandoned);
    }
    {
        const perf::Scope afterScope(after);
    }
    REQUIRE(after.caller() == nullptr);
}

TEST_CASE("Perf: each thread has its own roots")
{
    perf::setEnabled(true);
    static perf::Site gameSite("Test::gameThread", nullptr);
    static perf::Site workerSite("Test::workerThread", nullptr);
    {
        const perf::Scope gameScope(gameSite);
        std::thread worker([] {
            const perf::Scope workerScope(workerSite);
        });
        worker.join();
    }
    REQUIRE(workerSite.read().durations.count == 1);
    REQUIRE(workerSite.caller() == nullptr);
    REQUIRE(gameSite.read().childrenNs == 0);
}

TEST_CASE("Perf: the macros name a whole-function site after the function and nest a block under it")
{
    perf::setEnabled(true);
    macroUser();
    const auto* function = findSite("macroUser", "");
    const auto* block = findSite("macroUser", "block");
    REQUIRE(function != nullptr);
    REQUIRE(block != nullptr);
    REQUIRE(function->isWholeFunction());
    REQUIRE_FALSE(block->isWholeFunction());
    REQUIRE(block->caller() == function);
}

TEST_CASE("Perf: a whole-function site is labelled with the function's last two name parts")
{
    const perf::Site member("f4cf::ModBase::onFrameUpdateSafe", nullptr);
    REQUIRE(std::string(member.label()) == "ModBase::onFrameUpdateSafe");
    REQUIRE(std::string(member.function()) == "f4cf::ModBase::onFrameUpdateSafe");

    const perf::Site block("frik::Skeleton::onFrameUpdate", "arms");
    REQUIRE(std::string(block.label()) == "arms");
    REQUIRE(std::string(block.shortFunction()) == "Skeleton::onFrameUpdate");

    const perf::Site freeFunction("hookMain", nullptr);
    REQUIRE(std::string(freeFunction.label()) == "hookMain");
}

TEST_CASE("Perf: sites() lists a site for as long as it exists")
{
    const perf::Site* address = nullptr;
    {
        const perf::Site site("Test::listed", nullptr);
        address = &site;
        const auto all = perf::sites();
        REQUIRE(std::ranges::find(all, address) != all.end());
    }
    const auto all = perf::sites();
    REQUIRE(std::ranges::find(all, address) == all.end());
}

TEST_CASE("Perf: a dynamic site is created once per function and label, and owns its label")
{
    std::string tag = "ROCK";
    auto& site = perf::dynamicSite("Test::dispatch", tag);
    tag = "overwritten";
    REQUIRE(std::string(site.label()) == "ROCK");
    REQUIRE(&perf::dynamicSite("Test::dispatch", "ROCK") == &site);
    REQUIRE(&perf::dynamicSite("Test::dispatch", "ROCK_ArmTrace") != &site);
    REQUIRE(&perf::dynamicSite("Test::otherDispatch", "ROCK") != &site);
}

TEST_CASE("Perf: reset drops what every site recorded and restarts the window")
{
    perf::setEnabled(true);
    static perf::Site site("Test::reset", nullptr);
    {
        const perf::Scope scope(site);
    }
    REQUIRE(site.read().durations.count == 1);

    const auto before = perf::windowStart();
    spin(10us);
    perf::reset();
    REQUIRE(site.read().durations.count == 0);
    REQUIRE(perf::windowStart() > before);
}

TEST_CASE("Perf: switching recording on starts from an empty window, and off keeps what was recorded")
{
    perf::setEnabled(true);
    static perf::Site site("Test::switch", nullptr);
    {
        const perf::Scope scope(site);
    }
    perf::setEnabled(false);
    REQUIRE(site.read().durations.count == 1);

    perf::setEnabled(true);
    REQUIRE(site.read().durations.count == 0);
}

TEST_CASE("Perf: cost of a scope", "[.benchmark]")
{
    static perf::Site site("Test::benchmark", nullptr);
    constexpr int iterations = 1'000'000;
    const auto measure = [] {
        const auto start = std::chrono::steady_clock::now();
        for (int i = 0; i < iterations; ++i) {
            const perf::Scope scope(site);
        }
        return std::chrono::duration<double, std::nano>(std::chrono::steady_clock::now() - start).count() / iterations;
    };
    // what a real site pays: the macro's function-local static is checked for construction on every pass
    const auto measureMacro = [] {
        const auto start = std::chrono::steady_clock::now();
        for (int i = 0; i < iterations; ++i) {
            F4CF_PERF_SCOPE("benchmark");
        }
        return std::chrono::duration<double, std::nano>(std::chrono::steady_clock::now() - start).count() / iterations;
    };
    perf::setEnabled(false);
    const auto off = measure();
    const auto macroOff = measureMacro();
    perf::setEnabled(true);
    const auto on = measure();
    const auto macroOn = measureMacro();
    std::cout << "perf scope: " << off << " ns off, " << on << " ns on; through the macro: " << macroOff << " ns off, " << macroOn << " ns on\n";
}
