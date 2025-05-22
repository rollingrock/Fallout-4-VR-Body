#pragma once

#include <deque>
#include <f4se/NiTypes.h>

namespace frik {
	class SmoothMovementVR {
	public:
		SmoothMovementVR() {
			QueryPerformanceFrequency(&_hpcFrequency);
			QueryPerformanceCounter(&_prevTime);
		}

		void onFrameUpdate();

	private:
		NiPoint3 smoothedValue(const NiPoint3& curPos, const NiPoint3& prevPos);

		bool _notMoving = false;
		std::deque<NiPoint3> _lastPositions;
		NiPoint3 _smoothedPos;
		float _lastAppliedLocalX = 0;
		float _lastAppliedLocalY = 0;

		LARGE_INTEGER _hpcFrequency;
		LARGE_INTEGER _prevTime;
		float _frameTime = 0;
	};
}
