#pragma once

#include <cmath>

namespace frik
{
    /**
     * A unit quaternion as plain values, so the rotation smoothing is testable without the game.
     */
    struct QuatValues
    {
        float w = 1.0f;
        float x = 0.0f;
        float y = 0.0f;
        float z = 0.0f;
    };

    /**
     * `from` turned toward `to` by `t` of the angle between them, the short way round: 0 keeps `from`, 1 reaches `to`.
     * Nearly equal rotations blend linearly and renormalise, so a small difference still moves the result. The framework's
     * Quaternion::slerp returns `from` unchanged below about 0.36 degrees, which made every smoothing hold and then step.
     */
    inline QuatValues slerpToward(const QuatValues& from, QuatValues to, const float t)
    {
        float cosine = from.w * to.w + from.x * to.x + from.y * to.y + from.z * to.z;
        if (cosine < 0.0f) {
            to = { -to.w, -to.x, -to.y, -to.z };
            cosine = -cosine;
        }
        if (cosine > 0.9995f) {
            const QuatValues mixed{ from.w + t * (to.w - from.w), from.x + t * (to.x - from.x), from.y + t * (to.y - from.y), from.z + t * (to.z - from.z) };
            const float length = std::sqrt(mixed.w * mixed.w + mixed.x * mixed.x + mixed.y * mixed.y + mixed.z * mixed.z);
            return { mixed.w / length, mixed.x / length, mixed.y / length, mixed.z / length };
        }
        const float angle = std::acos(cosine);
        const float sinAngle = std::sin(angle);
        const float a = std::sin((1.0f - t) * angle) / sinAngle;
        const float b = std::sin(t * angle) / sinAngle;
        return { a * from.w + b * to.w, a * from.x + b * to.x, a * from.y + b * to.y, a * from.z + b * to.z };
    }
}
