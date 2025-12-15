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

    std::string_view FRIK_CALL getModVersion()
    {
        return Version::NAME;
    }

    bool FRIK_CALL isSkeletonReady()
    {
        return g_frik.isSkeletonReady();
    }

    bool FRIK_CALL isConfigOpen()
    {
        return g_frik.isMainConfigurationModeActive() || g_frik.isPipboyConfigurationModeActive() || g_frik.inWeaponRepositionMode();
    }

    bool FRIK_CALL isSelfieModeOn()
    {
        return g_frik.isSelfieModeOn();
    }

    void FRIK_CALL setSelfieModeOn(const bool setOn)
    {
        g_frik.setSelfieMode(setOn);
    }

    bool FRIK_CALL isOffHandGrippingWeapon()
    {
        return g_frik.isOffHandGrippingWeapon();
    }

    bool FRIK_CALL isWristPipboyOpen()
    {
        return g_frik.isPipboyOn();
    }

    RE::NiPoint3 FRIK_CALL getIndexFingerTipPosition(const FRIKApi::Hand hand)
    {
        return g_frik.getIndexFingerTipWorldPosition(static_cast<vrcf::Hand>(hand));
    }

    std::string FRIK_CALL getHandPoseSetTag(const FRIKApi::Hand hand)
    {
        // TODO: implement 
        return "";
    }

    void FRIK_CALL setHandPoseFingerPositionsV2(const FRIKApi::Hand hand, std::string tag, const float thumb, const float index, const float middle, const float ring,
        const float pinky)
    {
        // TODO: implement tag usage
        setFingerPositionScalar(getIsLeftForHandEnum(hand), thumb, index, middle, ring, pinky);
    }

    void FRIK_CALL clearHandPoseFingerPositionsV2(const FRIKApi::Hand hand, std::string tag)
    {
        // TODO: implement tag usage
        restoreFingerPoseControl(getIsLeftForHandEnum(hand));
    }

    void FRIK_CALL setHandPoseFingerPositions(const FRIKApi::Hand hand, const float thumb, const float index, const float middle, const float ring, const float pinky)
    {
        setFingerPositionScalar(getIsLeftForHandEnum(hand), thumb, index, middle, ring, pinky);
    }

    void FRIK_CALL clearHandPoseFingerPositions(const FRIKApi::Hand hand)
    {
        restoreFingerPoseControl(getIsLeftForHandEnum(hand));
    }

    void FRIK_CALL registerOpenModSettingButtonToMainConfig(const FRIKApi::OpenExternalModConfigData& data)
    {
        g_frik.registerOpenSettingButton(data);
    }

    constexpr FRIKApi FRIK_API_FUNCTIONS_TABLE{
        .getVersion = &getVersion,
        .getModVersion = &getModVersion,
        .isSkeletonReady = &isSkeletonReady,
        .isConfigOpen = &isConfigOpen,
        .isSelfieModeOn = &isSelfieModeOn,
        .setSelfieModeOn = &setSelfieModeOn,
        .isOffHandGrippingWeapon = &isOffHandGrippingWeapon,
        .isWristPipboyOpen = &isWristPipboyOpen,
        .getIndexFingerTipPosition = &getIndexFingerTipPosition,
        .getHandPoseSetTag = &getHandPoseSetTag,
        .setHandPoseFingerPositionsV2 = &setHandPoseFingerPositionsV2,
        .clearHandPoseFingerPositionsV2 = &clearHandPoseFingerPositionsV2,
        .setHandPoseFingerPositions = &setHandPoseFingerPositions,
        .clearHandPoseFingerPositions = &clearHandPoseFingerPositions,
        .registerOpenModSettingButtonToMainConfig = &registerOpenModSettingButtonToMainConfig
    };
}

namespace frik::api
{
    FRIK_API const FRIKApi* FRIK_CALL FRIKAPI_GetApi()
    {
        return &FRIK_API_FUNCTIONS_TABLE;
    }
}
