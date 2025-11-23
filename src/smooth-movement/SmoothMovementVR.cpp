#include "SmoothMovementVR.h"

#include "Config.h"
#include "FRIK.h"

using namespace common;

// Adapted from original code by Shizof mod with permission.  Thanks Shizof!!

namespace frik
{
    void SmoothMovementVR::onFrameUpdate()
    {
        if (g_config.disableSmoothMovement) {
            return;
        }
        const auto playerNodes = f4vr::getPlayerNodes();
        if (!playerNodes || !playerNodes->playerworldnode) {
            return;
        }

        const auto player = RE::PlayerCharacter::GetSingleton();
        const RE::NiPoint3 curPos = player->GetPosition();

        if (_lastPositions.size() < 2 && fNotEqual(curPos.z, 0)) {
            _smoothedPos = curPos;
        }

        if (_lastPositions.size() >= 4) {
            const RE::NiPoint3 pos = _lastPositions.at(0);
            bool same = true;
            for (unsigned int i = 1; i < _lastPositions.size(); i++) {
                if (fNotEqual(_lastPositions.at(i).x, pos.x) || fNotEqual(_lastPositions.at(i).y, pos.y)) {
                    same = false;
                    break;
                }
            }
            _notMoving = same;
        } else {
            _notMoving = false;
        }

        _lastPositions.emplace_back(curPos);
        if (_lastPositions.size() > 5) {
            _lastPositions.pop_front();
        }

        const auto newPos = smoothedValue(curPos, _smoothedPos);
        _smoothedPos = newPos;

        auto& playerLocalTransformPos = playerNodes->playerworldnode->local.translate;
        if (_notMoving && MatrixUtils::distanceNoSqrt2d(newPos.x - curPos.x, newPos.y - curPos.y, _lastAppliedLocalX, _lastAppliedLocalY) > 100) {
            _smoothedPos = curPos;
            playerLocalTransformPos.z = 0;
            logger::sample("[SmoothMovement] Not moving values exceed normal; curPos:({:.2f}, {:.2f}), curPos:({:.2f}, {:.2f}), lastApplied:({:.2f}, {:.2f})",
                curPos.x, curPos.y, newPos.x, newPos.y, _lastAppliedLocalX, _lastAppliedLocalY);
        } else {
            playerLocalTransformPos = newPos - curPos;
            _lastAppliedLocalX = playerLocalTransformPos.x;
            _lastAppliedLocalY = playerLocalTransformPos.y;
        }

        // logger::sample("[SmoothMovement] curPos:({:.2f}, {:.2f}, {:.2f}), newPos:({:.2f}, {:.2f}, {:.2f}), appliedPos:({:.2f}, {:.2f}, {:.2f})",
        // 	curPos.x, curPos.y, curPos.z, newPos.x, newPos.y, newPos.z, playerLocalTransformPos.x, playerLocalTransformPos.y, playerLocalTransformPos.z);

        playerLocalTransformPos.z += Skeleton::getAdjustedPlayerHMDOffset();
    }

    /**
     * Calculate the new smoothed position based on the player current position and the previous smoothed position.
     */
    RE::NiPoint3 SmoothMovementVR::smoothedValue(const RE::NiPoint3& curPos, const RE::NiPoint3& prevPos)
    {
        LARGE_INTEGER newTime;
        QueryPerformanceCounter(&newTime);
        _frameTime = min(0.05f, static_cast<float>(newTime.QuadPart - _prevTime.QuadPart) / static_cast<float>(_hpcFrequency.QuadPart));
        _prevTime = newTime;

        if (g_config.disableInteriorSmoothingHorizontal && f4vr::isInInternalCell()) {
            // don't smooth if in interior cell and smoothing is disabled for it
            return curPos;
        }

        if (MatrixUtils::distanceNoSqrt(curPos, prevPos) > 4000000.0f) {
            // don't smooth if values are way off
            logger::sample("[SmoothMovement] Values exceed normal; curPos:({:.2f}, {:.2f}, {:.2f}), SmoothPos:({:.2f}, {:.2f}, {:.2f})",
                curPos.x, curPos.y, curPos.z, prevPos.x, prevPos.y, prevPos.z);
            return curPos;
        }

        auto newPos = RE::NiPoint3(curPos.x, curPos.y, curPos.z);
        if (fNotEqual(g_config.dampingMultiplierHorizontal, 0) && fNotEqual(g_config.smoothingAmountHorizontal, 0)) {
            // DO smoothing
            const float absValX = min(50, max(0.1f, abs(curPos.x - prevPos.x)));
            newPos.x = prevPos.x + _frameTime * ((curPos.x - prevPos.x) /
                (g_config.smoothingAmountHorizontal * (g_config.dampingMultiplierHorizontal / absValX) * (_notMoving ? g_config.stoppingMultiplierHorizontal : 1.0f)));

            const float absValY = min(50, max(0.1f, abs(curPos.y - prevPos.y)));
            newPos.y = prevPos.y + _frameTime * ((curPos.y - prevPos.y) /
                (g_config.smoothingAmountHorizontal * (g_config.dampingMultiplierHorizontal / absValY) * (_notMoving ? g_config.stoppingMultiplierHorizontal : 1.0f)));
        } else {
            logger::sample("shouldn't be here!");
        }

        // Don't smooth vertical movement if jumping or in air as it will break the jump
        if (!f4vr::isJumpingOrInAir() && fNotEqual(g_config.dampingMultiplier, 0) && fNotEqual(g_config.smoothingAmount, 0)) {
            const float absVal = min(50, max(0.1f, abs(curPos.z - prevPos.z)));
            newPos.z = prevPos.z + _frameTime * ((curPos.z - prevPos.z) /
                (g_config.smoothingAmount * (g_config.dampingMultiplier / absVal) * (_notMoving ? g_config.stoppingMultiplier : 1.0f)));
        }

        return newPos;
    }
}
