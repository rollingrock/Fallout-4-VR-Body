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
		NiPoint3 smoothedValue(NiPoint3 newPosition);

		bool _first = true;
		bool _notMoving = false;

		float _smoothedX = 0;
		float _smoothedY = 0;
		float _smoothedZ = 0;

		LARGE_INTEGER _hpcFrequency;
		LARGE_INTEGER _prevTime;
		float _frameTime = 0;

		std::deque<NiPoint3> _lastPositions;

		float _lastAppliedLocalX = 0;
		float _lastAppliedLocalY = 0;
	};
}
