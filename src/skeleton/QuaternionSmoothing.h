#pragma once

#include "common/Quaternion.h"
#include "skeleton/RotationSmoothing.h"

namespace frik
{
    /**
     * slerpToward for the framework's Quaternion: `q` turned toward `target` by `t`, with no small-angle deadzone.
     */
    inline common::Quaternion slerpToward(const common::Quaternion& q, const common::Quaternion& target, const float t)
    {
        const auto r = slerpToward(QuatValues{ q.w, q.x, q.y, q.z }, QuatValues{ target.w, target.x, target.y, target.z }, t);
        return common::Quaternion(r.x, r.y, r.z, r.w);
    }
}
