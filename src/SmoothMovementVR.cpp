#include "SmoothMovementVR.h"

#include "Config.h"
#include "FRIK.h"
#include "MenuChecker.h"
#include "f4se/NiExtraData.h"

using namespace common;

// From Shizof mod with permission.  Thanks Shizof!!

namespace frik {
	void SmoothMovementVR::onFrameUpdate() {
		if (g_config.disableSmoothMovement) {
			return;
		}
		const auto playerNodes = f4vr::getPlayerNodes();
		if (!playerNodes || !playerNodes->playerworldnode) {
			return;
		}

		const NiPoint3 curPos = (*g_player)->pos;

		if (_first && fNotEqual(curPos.z, 0)) {
			_smoothedX = curPos.x;
			_smoothedY = curPos.y;
			_smoothedZ = curPos.z;
			_first = false;
		}

		if (_lastPositions.size() >= 4) {
			const NiPoint3 pos = _lastPositions.at(0);
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

		_lastPositions.emplace_back((*g_player)->pos);
		if (_lastPositions.size() > 5) {
			_lastPositions.pop_front();
		}

		NiPoint3 newPos = smoothedValue(curPos);

		if (_notMoving && distanceNoSqrt2d(newPos.x - curPos.x, newPos.y - curPos.y, _lastAppliedLocalX, _lastAppliedLocalY) > 100) {
			newPos = curPos;
			_smoothedX = newPos.x;
			_smoothedY = newPos.y;
			playerNodes->playerworldnode->m_localTransform.pos.z = newPos.z - curPos.z;
		} else {
			playerNodes->playerworldnode->m_localTransform.pos.x = newPos.x - curPos.x;
			playerNodes->playerworldnode->m_localTransform.pos.y = newPos.y - curPos.y;
			playerNodes->playerworldnode->m_localTransform.pos.z = newPos.z - curPos.z;
			_lastAppliedLocalX = playerNodes->playerworldnode->m_localTransform.pos.x;
			_lastAppliedLocalY = playerNodes->playerworldnode->m_localTransform.pos.y;
		}

		// Log::info("playerPos: %g %g %g  --newPos:  %g %g %g  --appliedLocal: %g %g %g", curPos.x, curPos.y, curPos.z, newPos.x, newPos.y, newPos.z, playerWorldNode->m_localTransform.pos.x, playerWorldNode->m_localTransform.pos.y, playerWorldNode->m_localTransform.pos.z);
		// Log::info("playerWorldNode: %g %g %g", playerWorldNode->m_localTransform.pos.x, playerWorldNode->m_localTransform.pos.y, playerWorldNode->m_localTransform.pos.z);

		const float cameraHeightOffset = f4vr::isInPowerArmor()
			? g_config.PACameraHeight + g_config.cameraHeight + g_frik.getDynamicCameraHeight()
			: g_config.cameraHeight + g_frik.getDynamicCameraHeight();
		playerNodes->playerworldnode->m_localTransform.pos.z += cameraHeightOffset;
	}

	NiPoint3 SmoothMovementVR::smoothedValue(const NiPoint3 newPosition) {
		LARGE_INTEGER newTime;
		QueryPerformanceCounter(&newTime);
		_frameTime = min(0.05f, static_cast<float>(newTime.QuadPart - _prevTime.QuadPart) / static_cast<float>(_hpcFrequency.QuadPart));
		_prevTime = newTime;

		if (f4vr::isJumpingOrInAir()) {
			// don't smooth if in the air
			_smoothedX = newPosition.x;
			_smoothedY = newPosition.y;
			_smoothedZ = newPosition.z;
		} else if (distanceNoSqrt(newPosition, NiPoint3(_smoothedX, _smoothedY, _smoothedZ)) > 4000000.0f) {
			// don't smooth if values are way off
			_smoothedX = newPosition.x;
			_smoothedY = newPosition.y;
			_smoothedZ = newPosition.z;
		} else {
			const bool skipInteriorCell = g_config.disableInteriorSmoothingHorizontal && f4vr::isInInternalCell();
			if (skipInteriorCell) {
				// don't smooth if in interior cell and smoothing is disabled for it
				_smoothedX = newPosition.x;
				_smoothedY = newPosition.y;
			} else if (fEqual(g_config.dampingMultiplierHorizontal, 0) || fEqual(g_config.smoothingAmountHorizontal, 0)) {
				// don't smooth if config will zero it
				_smoothedX = newPosition.x;
				_smoothedY = newPosition.y;
			} else {
				// DO smoothing
				const float absValX = min(50, max(0.1f, abs(newPosition.x - _smoothedX)));
				_smoothedX += _frameTime * ((newPosition.x - _smoothedX) / (g_config.smoothingAmountHorizontal * (g_config.dampingMultiplierHorizontal / absValX) *
					(_notMoving ? g_config.stoppingMultiplierHorizontal : 1.0f)));
				const float absValY = min(50, max(0.1f, abs(newPosition.y - _smoothedY)));
				_smoothedY += _frameTime * ((newPosition.y - _smoothedY) / (g_config.smoothingAmountHorizontal * (g_config.dampingMultiplierHorizontal / absValY) *
					(_notMoving ? g_config.stoppingMultiplierHorizontal : 1.0f)));
			}

			if (skipInteriorCell) {
				_smoothedZ = newPosition.z;
			} else {
				const float absVal = min(50, max(0.1f, abs(newPosition.z - _smoothedZ)));
				_smoothedZ += _frameTime * ((newPosition.z - _smoothedZ) / (g_config.smoothingAmount * (g_config.dampingMultiplier / absVal) * (_notMoving
					? g_config.stoppingMultiplier
					: 1.0f)));
			}
		}

		return NiPoint3(_smoothedX, _smoothedY, _smoothedZ);
	}
}
