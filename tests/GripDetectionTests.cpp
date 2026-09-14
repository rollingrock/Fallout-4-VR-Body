#include <catch2/catch_test_macros.hpp>

#include "weapon-position/GripDetection.h"

using frik::grip::GripLatch;
using frik::grip::isInCone;

TEST_CASE("Grip cone: enter is tighter than exit and the range is capped")
{
    REQUIRE(isInCone(0.96f, 40.0f, false));
    REQUIRE_FALSE(isInCone(0.93f, 40.0f, false));
    REQUIRE(isInCone(0.93f, 40.0f, true));
    REQUIRE_FALSE(isInCone(0.89f, 40.0f, true));
    REQUIRE_FALSE(isInCone(0.99f, 15.0f, false));
    REQUIRE_FALSE(isInCone(0.99f, 90.0f, false));
    REQUIRE(isInCone(0.99f, 89.0f, false));
}

TEST_CASE("Grip latch survives a hidden weapon and re-checks the cone on return (#142)")
{
    GripLatch latch;
    latch.grip();
    latch.onWeaponHidden();
    REQUIRE(latch.gripping);
    REQUIRE(latch.revalidatePending);
    REQUIRE_FALSE(latch.onWeaponDrawn(true));
    REQUIRE(latch.gripping);

    SECTION("hand still on the barrel keeps the grip")
    {
        REQUIRE(latch.revalidate(true));
        REQUIRE(latch.gripping);
        REQUIRE_FALSE(latch.revalidatePending);
    }
    SECTION("hand off the barrel releases")
    {
        REQUIRE_FALSE(latch.revalidate(false));
        REQUIRE_FALSE(latch.gripping);
    }
    SECTION("a different weapon releases without waiting")
    {
        REQUIRE(latch.onWeaponDrawn(false));
        REQUIRE_FALSE(latch.gripping);
        REQUIRE_FALSE(latch.revalidatePending);
    }
}

TEST_CASE("Grip latch: hidden while not gripping asks for nothing")
{
    GripLatch latch;
    latch.onWeaponHidden();
    REQUIRE_FALSE(latch.revalidatePending);
    REQUIRE(latch.revalidate(false));
    REQUIRE_FALSE(latch.onWeaponDrawn(false));
}

TEST_CASE("Grip latch: mode 2 let-go inside the cone waits for the hand to leave it")
{
    GripLatch latch;
    latch.grip();
    latch.releaseByButton(true);
    REQUIRE_FALSE(latch.gripping);
    REQUIRE_FALSE(latch.mayAutoGrip(true, true));
    REQUIRE_FALSE(latch.mayAutoGrip(true, true));
    REQUIRE_FALSE(latch.mayAutoGrip(false, false));
    REQUIRE(latch.mayAutoGrip(true, true));
}

TEST_CASE("Grip latch: button modes do not arm the wait")
{
    GripLatch latch;
    latch.grip();
    latch.releaseByButton(false);
    REQUIRE(latch.mayAutoGrip(true, true));
    REQUIRE_FALSE(latch.mayAutoGrip(false, true));
}
