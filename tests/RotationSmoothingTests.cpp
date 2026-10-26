#include <cmath>
#include <numbers>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include "skeleton/RotationSmoothing.h"

using frik::QuatValues;
using frik::slerpToward;

namespace
{
    constexpr float kDeg = std::numbers::pi_v<float> / 180.0f;

    QuatValues aboutZ(const float degrees)
    {
        return { std::cos(degrees * kDeg / 2.0f), 0.0f, 0.0f, std::sin(degrees * kDeg / 2.0f) };
    }

    // atan2 on the relative rotation in double: acos of a float cosine cannot resolve angles below about 0.04 deg
    float angleBetween(const QuatValues& a, const QuatValues& b)
    {
        const double w = static_cast<double>(a.w) * b.w + static_cast<double>(a.x) * b.x + static_cast<double>(a.y) * b.y + static_cast<double>(a.z) * b.z;
        const double x = static_cast<double>(a.w) * b.x - static_cast<double>(a.x) * b.w - static_cast<double>(a.y) * b.z + static_cast<double>(a.z) * b.y;
        const double y = static_cast<double>(a.w) * b.y + static_cast<double>(a.x) * b.z - static_cast<double>(a.y) * b.w - static_cast<double>(a.z) * b.x;
        const double z = static_cast<double>(a.w) * b.z - static_cast<double>(a.x) * b.y + static_cast<double>(a.y) * b.x - static_cast<double>(a.z) * b.w;
        return static_cast<float>(2.0 * std::atan2(std::sqrt(x * x + y * y + z * z), std::abs(w)) * 180.0 / std::numbers::pi);
    }

    float length(const QuatValues& q)
    {
        return std::sqrt(q.w * q.w + q.x * q.x + q.y * q.y + q.z * q.z);
    }
}

TEST_CASE("Rotation smoothing: a small difference still moves (the framework slerp held below 0.36 deg)")
{
    for (const float degrees : { 0.05f, 0.2f, 0.36f, 1.0f, 3.0f }) {
        const auto r = slerpToward(aboutZ(0.0f), aboutZ(degrees), 0.5f);
        REQUIRE(angleBetween(aboutZ(0.0f), r) == Catch::Approx(degrees / 2.0f).margin(0.002));
    }
}

TEST_CASE("Rotation smoothing: t = 0 keeps the start, t = 1 reaches the target, and the result stays unit")
{
    for (const float degrees : { 0.1f, 10.0f, 90.0f, 170.0f }) {
        const auto from = aboutZ(0.0f);
        const auto to = aboutZ(degrees);
        REQUIRE(angleBetween(slerpToward(from, to, 0.0f), from) == Catch::Approx(0.0f).margin(0.01));
        REQUIRE(angleBetween(slerpToward(from, to, 1.0f), to) == Catch::Approx(0.0f).margin(0.01));
        REQUIRE(length(slerpToward(from, to, 0.3f)) == Catch::Approx(1.0f).margin(1e-5));
    }
}

TEST_CASE("Rotation smoothing: large angles interpolate the angle, not the components")
{
    const auto r = slerpToward(aboutZ(0.0f), aboutZ(90.0f), 0.5f);
    REQUIRE(angleBetween(aboutZ(0.0f), r) == Catch::Approx(45.0f).margin(0.01));
}

TEST_CASE("Rotation smoothing: the same rotation with a flipped sign takes the short way round")
{
    const auto to = aboutZ(20.0f);
    const QuatValues flipped{ -to.w, -to.x, -to.y, -to.z };
    const auto r = slerpToward(aboutZ(0.0f), flipped, 0.5f);
    REQUIRE(angleBetween(aboutZ(0.0f), r) == Catch::Approx(10.0f).margin(0.01));
}

TEST_CASE("Rotation smoothing: repeated smoothing converges on a target 0.2 deg away instead of stopping short")
{
    auto q = aboutZ(0.0f);
    const auto target = aboutZ(0.2f);
    for (int i = 0; i < 40; ++i) {
        q = slerpToward(q, target, 0.44f);
    }
    REQUIRE(angleBetween(q, target) < 0.001f);
}
