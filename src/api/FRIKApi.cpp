#define FRIK_API_EXPORTS
#include "FRIKAPI.h"

#include "FRIK.h"
#include "skeleton/HandPose.h"

namespace frik::api
{
    std::uint32_t FRIKAPI_getVersion()
    {
        return FRIK_API_VERSION;
    }

    bool FRIKAPI_isSkeletonReady()
    {
        return g_frik.isSkeletonReady();
    }

    RE::NiPoint3 FRIKAPI_getIndexFingerTipPosition(const bool primaryHand)
    {
        return g_frik.getIndexFingerTipWorldPosition(primaryHand);
    }

    void FRIKAPI_setHandPoseFingerPositions(const bool isLeft, const float thumb, const float index, const float middle, const float ring, const float pinky)
    {
        setFingerPositionScalar(isLeft, thumb, index, middle, ring, pinky);
    }

    void FRIKAPI_clearHandPoseFingerPositions(const bool isLeft)
    {
        restoreFingerPoseControl(isLeft);
    }
}
