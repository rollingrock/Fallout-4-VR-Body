#define FRIK_API_EXPORTS
#include "FRIKAPI.h"

#include "FRIK.h"
#include "skeleton/HandPose.h"

namespace
{
    using namespace frik;
    using namespace frik::api;

    std::uint32_t FRIK_CALL getVersion()
    {
        return FRIK_API_VERSION;
    }

    bool FRIK_CALL isSkeletonReady()
    {
        return g_frik.isSkeletonReady();
    }

    RE::NiPoint3 FRIK_CALL getIndexFingerTipPosition(const bool primaryHand)
    {
        return g_frik.getIndexFingerTipWorldPosition(primaryHand);
    }

    void FRIK_CALL setHandPoseFingerPositions(const bool isLeft, const float thumb, const float index, const float middle, const float ring, const float pinky)
    {
        setFingerPositionScalar(isLeft, thumb, index, middle, ring, pinky);
    }

    void FRIK_CALL clearHandPoseFingerPositions(const bool isLeft)
    {
        restoreFingerPoseControl(isLeft);
    }

    constexpr FRIKApi FRIK_API_FUNCTIONS_TABLE{
        .getVersion = &getVersion,
        .isSkeletonReady = &isSkeletonReady,
        .getIndexFingerTipPosition = &getIndexFingerTipPosition,
        .setHandPoseFingerPositions = &setHandPoseFingerPositions,
        .clearHandPoseFingerPositions = &clearHandPoseFingerPositions
    };
}

namespace frik::api
{
    FRIK_API const FRIKApi* FRIK_CALL FRIKAPI_GetApi()
    {
        return &FRIK_API_FUNCTIONS_TABLE;
    }
}
