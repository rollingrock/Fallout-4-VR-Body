#include <catch2/catch_test_macros.hpp>

#include <string>
#include <vector>

#include "FramePhaseRegistry.h"

using frik::FRAME_PHASE_COUNT;
using frik::FramePhase;
using frik::FramePhaseRegistry;

namespace
{
    using Result = FramePhaseRegistry::Result;

    std::vector<std::string> g_calls;
    FramePhaseRegistry* g_reentrant = nullptr;
    Result g_reentrantResult = Result::Registered;
    bool g_reentrantRemove = true;

    void __cdecl record(const std::uint32_t phase, void* const userData) noexcept
    {
        g_calls.push_back(static_cast<const char*>(userData) + std::string(":") + std::to_string(phase));
    }

    void __cdecl tryMutate(const std::uint32_t, void*) noexcept
    {
        g_reentrantResult = g_reentrant->set("inner", 0, &record, nullptr, 0);
        g_reentrantRemove = g_reentrant->remove("inner");
        g_reentrant->invoke(0);
    }

    constexpr std::uint32_t phase(const FramePhase value)
    {
        return static_cast<std::uint32_t>(value);
    }

    char tagA[] = "a";
    char tagB[] = "b";
    char tagC[] = "c";
}

TEST_CASE("FramePhaseRegistry validates registrations")
{
    FramePhaseRegistry registry;
    REQUIRE(registry.set("", 0, &record, nullptr, 0) == Result::BadTag);
    REQUIRE(registry.set("x", 0, nullptr, nullptr, 0) == Result::NullCallback);
    REQUIRE(registry.set("x", FRAME_PHASE_COUNT, &record, nullptr, 0) == Result::BadPhase);
    REQUIRE(registry.set("x", 0, &record, nullptr, -1) == Result::NegativePriority);
    REQUIRE(registry.count() == 0);
    REQUIRE_FALSE(registry.remove(""));
    REQUIRE(registry.remove("unknown"));
}

TEST_CASE("FramePhaseRegistry runs by priority then registration order")
{
    FramePhaseRegistry registry;
    const auto p = phase(FramePhase::BeforeArmSolve);
    REQUIRE(registry.set("a", p, &record, tagA, 10) == Result::Registered);
    REQUIRE(registry.set("b", p, &record, tagB, 20) == Result::Registered);
    REQUIRE(registry.set("c", p, &record, tagC, 10) == Result::Registered);
    REQUIRE(registry.count(p) == 3);

    g_calls.clear();
    REQUIRE(registry.invoke(p) == 3);
    REQUIRE(g_calls == std::vector<std::string>{ "b:3", "a:3", "c:3" });

    // Other phases are untouched.
    REQUIRE(registry.invoke(phase(FramePhase::AfterWorldFinal)) == 0);
    REQUIRE(registry.invoke(FRAME_PHASE_COUNT) == 0);
}

TEST_CASE("FramePhaseRegistry keeps a re-set entry in place and drops every phase of a tag")
{
    FramePhaseRegistry registry;
    REQUIRE(registry.set("a", 1, &record, tagA, 0) == Result::Registered);
    REQUIRE(registry.set("b", 1, &record, tagB, 0) == Result::Registered);
    REQUIRE(registry.set("a", 2, &record, tagA, 0) == Result::Registered);
    REQUIRE(registry.set("a", 1, &record, tagA, 0) == Result::Replaced);
    REQUIRE(registry.count() == 3);

    g_calls.clear();
    registry.invoke(1);
    REQUIRE(g_calls == std::vector<std::string>{ "a:1", "b:1" });

    // Raising the priority moves it, replacing at the same priority does not.
    REQUIRE(registry.set("b", 1, &record, tagB, 5) == Result::Replaced);
    g_calls.clear();
    registry.invoke(1);
    REQUIRE(g_calls == std::vector<std::string>{ "b:1", "a:1" });

    std::size_t removed = 0;
    REQUIRE(registry.remove("a", &removed));
    REQUIRE(removed == 2);
    REQUIRE(registry.count() == 1);
    REQUIRE(registry.count(2) == 0);
}

TEST_CASE("FramePhaseRegistry refuses mutation and nesting from inside a callback")
{
    FramePhaseRegistry registry;
    g_reentrant = &registry;
    REQUIRE(registry.set("outer", 0, &tryMutate, nullptr, 0) == Result::Registered);
    REQUIRE(registry.invoke(0) == 1);
    REQUIRE(g_reentrantResult == Result::Reentrant);
    REQUIRE_FALSE(g_reentrantRemove);
    REQUIRE_FALSE(registry.isInvoking());
    REQUIRE(registry.count() == 1);
}

TEST_CASE("FramePhaseRegistry is bounded and clears")
{
    FramePhaseRegistry registry;
    for (std::size_t i = 0; i < FramePhaseRegistry::CAPACITY; ++i) {
        REQUIRE(registry.set("t" + std::to_string(i), 0, &record, nullptr, 0) == Result::Registered);
    }
    REQUIRE(registry.set("overflow", 0, &record, nullptr, 0) == Result::Full);
    REQUIRE(registry.set("t0", 0, &record, nullptr, 1) == Result::Replaced);
    registry.clear();
    REQUIRE(registry.count() == 0);
    REQUIRE(registry.set("overflow", 0, &record, nullptr, 0) == Result::Registered);
}
