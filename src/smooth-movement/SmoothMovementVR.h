#pragma once

#include <deque>

#include "common/CommonUtils.h"

namespace frik
{
    class SmoothMovementVR
    {
    public:
        SmoothMovementVR()
        {
            QueryPerformanceFrequency(&_hpcFrequency);
            QueryPerformanceCounter(&_prevTime);
        }

        void reset();
        void onFrameUpdate();

    private:
        void resetState();
        void seedCurrentPosition(const RE::NiPoint3& curPos);
        void resetFrameTimer();
        static void clearAppliedPlayerWorldOffset();

        RE::NiPoint3 smoothedValue(const RE::NiPoint3& curPos, const RE::NiPoint3& prevPos);

        bool _notMoving = false;
        std::deque<RE::NiPoint3> _lastPositions;
        RE::NiPoint3 _smoothedPos{ 0.0f, 0.0f, 0.0f };
        bool _hasSmoothedPosition = false;
        float _lastAppliedLocalX = 0;
        float _lastAppliedLocalY = 0;

        LARGE_INTEGER _hpcFrequency;
        LARGE_INTEGER _prevTime;
        float _frameTime = 0;
    };
}
