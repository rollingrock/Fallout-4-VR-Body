#include <catch2/catch_test_macros.hpp>

#include "TagBlockSet.h"

using frik::TagBlockSet;

TEST_CASE("TagBlockSet is unblocked when empty")
{
    TagBlockSet set;
    REQUIRE_FALSE(set.isBlocked());
    REQUIRE(set.blockingCount() == 0);
}

TEST_CASE("TagBlockSet rejects an empty tag")
{
    TagBlockSet set;
    REQUIRE_FALSE(set.setBlocked("", true));
    REQUIRE_FALSE(set.isBlocked());
}

TEST_CASE("TagBlockSet stays blocked until every tag releases")
{
    TagBlockSet set;
    REQUIRE(set.setBlocked("a", true));
    REQUIRE(set.setBlocked("b", true));
    REQUIRE(set.blockingCount() == 2);

    REQUIRE(set.setBlocked("a", false));
    REQUIRE(set.isBlocked());

    REQUIRE(set.setBlocked("b", false));
    REQUIRE_FALSE(set.isBlocked());
}

TEST_CASE("TagBlockSet reports whether a call changed the set")
{
    TagBlockSet set;
    bool changed = false;

    REQUIRE(set.setBlocked("a", true, &changed));
    REQUIRE(changed);

    REQUIRE(set.setBlocked("a", true, &changed));
    REQUIRE_FALSE(changed);

    REQUIRE(set.setBlocked("missing", false, &changed));
    REQUIRE_FALSE(changed);
    REQUIRE(set.blockingCount() == 1);
}

TEST_CASE("TagBlockSet clear drops every tag")
{
    TagBlockSet set;
    set.setBlocked("a", true);
    set.setBlocked("b", true);

    set.clear();
    REQUIRE_FALSE(set.isBlocked());
    REQUIRE(set.blockingCount() == 0);
}
