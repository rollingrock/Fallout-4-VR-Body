#define FRIK_API_EXPORTS
#include "FRIKAPI.h"

#include "FRIK.h"
#include "f4vr/F4VRSkelly.h"
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

    const char* FRIK_CALL getModVersion()
    {
        // Safe to return pointer to static data
        static_assert(Version::NAME.back() != '\0' || true, "Version must be backed by a string literal");
        return Version::NAME.data();
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
        return f4vr::Skelly::getIndexFingerTipWorldPosition(static_cast<vrcf::Hand>(hand));
    }

    FRIKApi::HandPoseTagState FRIK_CALL getHandPoseSetTagState(const char* tag, const FRIKApi::Hand hand)
    {
        // TODO: implement 
        return FRIKApi::HandPoseTagState::None;
    }

    FRIKApi::HandPoses FRIK_CALL getCurrentHandPose(const FRIKApi::Hand hand)
    {
        // TODO: implement 
        return FRIKApi::HandPoses::Unset;
    }

    bool FRIK_CALL setHandPose(const char* tag, const FRIKApi::Hand hand, FRIKApi::HandPoses handPose)
    {
        if (!tag) {
            return false;
        }
        // TODO: implement tag usage
        return true;
    }

    bool FRIK_CALL setHandPoseCustomFingerPositions(const char* tag, const FRIKApi::Hand hand, const float thumb, const float index, const float middle, const float ring,
        const float pinky)
    {
        if (!tag) {
            return false;
        }

        std::string tagStr = tag;
        // do something...
        // TODO: implement tag usage
        setFingerPositionScalar(getIsLeftForHandEnum(hand), thumb, index, middle, ring, pinky);
        return true;
    }

    bool FRIK_CALL clearHandPose(const char* tag, const FRIKApi::Hand hand)
    {
        if (!tag) {
            return false;
        }
        // TODO: implement tag usage
        restoreFingerPoseControl(getIsLeftForHandEnum(hand));
        return true;
    }

    void FRIK_CALL setHandPoseFingerPositions(const FRIKApi::Hand hand, const float thumb, const float index, const float middle, const float ring, const float pinky)
    {
        setFingerPositionScalar(getIsLeftForHandEnum(hand), thumb, index, middle, ring, pinky);
    }

    void FRIK_CALL clearHandPoseFingerPositions(const FRIKApi::Hand hand)
    {
        restoreFingerPoseControl(getIsLeftForHandEnum(hand));
    }

    bool FRIK_CALL setHandPoseCustomJointPositions(const char* tag, const FRIKApi::Hand hand, const float* values)
    {
        if (!tag || !values) {
            return false;
        }
        // TODO: implement tag usage
        setFingerJointPositions(getIsLeftForHandEnum(hand), values);
        return true;
    }

    bool FRIK_CALL registerOpenModSettingButtonToMainConfig(const FRIKApi::OpenExternalModConfigData& data)
    {
        if (!data.buttonIconNifPath || !data.callbackReceiverName) {
            return false;
        }
        g_frik.registerOpenSettingButton({
            .buttonIconNifPath = data.buttonIconNifPath,
            .callbackReceiverName = data.callbackReceiverName,
            .callbackMessageType = data.callbackMessageType
        });
        return true;
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
        .getHandPoseSetTagState = &getHandPoseSetTagState,
        .getCurrentHandPose = &getCurrentHandPose,
        .setHandPose = &setHandPose,
        .setHandPoseCustomFingerPositions = &setHandPoseCustomFingerPositions,
        .clearHandPose = &clearHandPose,
        .setHandPoseFingerPositions = &setHandPoseFingerPositions,
        .clearHandPoseFingerPositions = &clearHandPoseFingerPositions,
        .registerOpenModSettingButtonToMainConfig = &registerOpenModSettingButtonToMainConfig,
        .setHandPoseCustomJointPositions = &setHandPoseCustomJointPositions
    };
}

namespace frik::api
{
    FRIK_API const FRIKApi* FRIK_CALL FRIKAPI_GetApi()
    {
        return &FRIK_API_FUNCTIONS_TABLE;
    }
}
