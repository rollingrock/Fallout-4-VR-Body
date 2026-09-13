#include <catch2/catch_test_macros.hpp>

#include "ScopeAuthority.h"

using frik::ScopeAuthority;
using frik::ScopeCapability;

namespace
{
    constexpr auto cap(const ScopeCapability capability)
    {
        return static_cast<std::uint32_t>(capability);
    }
}

TEST_CASE("ScopeAuthority falls back to the vanilla scope menu without a publishing provider")
{
    ScopeAuthority scope;
    REQUIRE_FALSE(scope.isLookingThroughScope(false));
    REQUIRE(scope.isLookingThroughScope(true));
    REQUIRE_FALSE(scope.hasCapability(ScopeCapability::KeepsBodyVisible));
}

TEST_CASE("ScopeAuthority rejects empty tags and unknown capability bits")
{
    ScopeAuthority scope;
    REQUIRE_FALSE(scope.setProvider("", cap(ScopeCapability::KeepsBodyVisible)));
    REQUIRE_FALSE(scope.setProvider("ts", 1u << 8));
    REQUIRE_FALSE(scope.clearProvider(""));
    REQUIRE(scope.providerCount() == 0);
}

TEST_CASE("ScopeAuthority publishing provider replaces the vanilla state")
{
    ScopeAuthority scope;
    bool changed = false;
    REQUIRE(scope.setProvider("ts", cap(ScopeCapability::PublishesLookingThrough) | cap(ScopeCapability::KeepsBodyVisible), &changed));
    REQUIRE(changed);

    REQUIRE_FALSE(scope.isLookingThroughScope(true));
    REQUIRE(scope.setLookingThroughScope("ts", true));
    REQUIRE(scope.isLookingThroughScope(false));
    REQUIRE(scope.hasCapability(ScopeCapability::KeepsBodyVisible));
    REQUIRE_FALSE(scope.hasCapability(ScopeCapability::OwnsScopeCamera));
}

TEST_CASE("ScopeAuthority only a publishing provider may set the flag")
{
    ScopeAuthority scope;
    REQUIRE(scope.setProvider("camera", cap(ScopeCapability::OwnsScopeCamera)));
    REQUIRE_FALSE(scope.setLookingThroughScope("camera", true));
    REQUIRE_FALSE(scope.setLookingThroughScope("unknown", true));
    REQUIRE(scope.isLookingThroughScope(true));
}

TEST_CASE("ScopeAuthority unions capabilities and drops the flag when the publisher leaves")
{
    ScopeAuthority scope;
    REQUIRE(scope.setProvider("a", cap(ScopeCapability::PublishesLookingThrough)));
    REQUIRE(scope.setProvider("b", cap(ScopeCapability::OwnsDamping)));
    REQUIRE(scope.setLookingThroughScope("a", true));
    REQUIRE(scope.hasCapability(ScopeCapability::OwnsDamping));
    REQUIRE(scope.isLookingThroughScope(false));

    bool changed = false;
    REQUIRE(scope.clearProvider("a", &changed));
    REQUIRE(changed);
    REQUIRE_FALSE(scope.hasCapability(ScopeCapability::PublishesLookingThrough));
    REQUIRE(scope.hasCapability(ScopeCapability::OwnsDamping));
    REQUIRE_FALSE(scope.isLookingThroughScope(false));
    REQUIRE(scope.isLookingThroughScope(true));

    REQUIRE(scope.clearProvider("missing", &changed));
    REQUIRE_FALSE(changed);
    REQUIRE(scope.providerCount() == 1);
}

TEST_CASE("ScopeAuthority re-registering a tag replaces its capabilities")
{
    ScopeAuthority scope;
    bool changed = false;
    REQUIRE(scope.setProvider("ts", cap(ScopeCapability::KeepsBodyVisible), &changed));
    REQUIRE(scope.setProvider("ts", cap(ScopeCapability::KeepsBodyVisible), &changed));
    REQUIRE_FALSE(changed);
    REQUIRE(scope.setProvider("ts", cap(ScopeCapability::OwnsScopeCamera), &changed));
    REQUIRE(changed);
    REQUIRE_FALSE(scope.hasCapability(ScopeCapability::KeepsBodyVisible));
    REQUIRE(scope.hasCapability(ScopeCapability::OwnsScopeCamera));
    REQUIRE(scope.providerCount() == 1);
}
