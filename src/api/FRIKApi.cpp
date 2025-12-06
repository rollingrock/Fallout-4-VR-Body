#define FRIK_API_EXPORTS
#include "FRIKAPI.h"

#include "FRIK.h"
#include "skeleton/HandPose.h"

namespace
{
    using namespace frik;
    using namespace frik::api;

    bool getIsLeftForHandEnum(const FRIKApi::Hand hand)
    {
        switch (hand) {
        case FRIKApi::Hand::Primary:
            return f4vr::isLeftHandedMode();
        case FRIKApi::Hand::Offhand:
            return !f4vr::isLeftHandedMode();
        case FRIKApi::Hand::Right:
            return false;
        case FRIKApi::Hand::Left:
            return true;
        }
        return false;
    }

    std::uint32_t FRIK_CALL getVersion()
    {
        return FRIK_API_VERSION;
    }

    bool FRIK_CALL isSkeletonReady()
    {
        return g_frik.isSkeletonReady();
    }

    RE::NiPoint3 FRIK_CALL getIndexFingerTipPosition(const FRIKApi::Hand hand)
    {
        return g_frik.getIndexFingerTipWorldPosition(static_cast<vrcf::Hand>(hand));
    }

    void FRIK_CALL setHandPoseFingerPositions(const FRIKApi::Hand hand, const float thumb, const float index, const float middle, const float ring, const float pinky)
    {
        setFingerPositionScalar(getIsLeftForHandEnum(hand), thumb, index, middle, ring, pinky);
    }

    void FRIK_CALL clearHandPoseFingerPositions(const FRIKApi::Hand hand)
    {
        restoreFingerPoseControl(getIsLeftForHandEnum(hand));
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
