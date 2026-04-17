#pragma once

#include <map>
#include <string>

#include "common/CommonUtils.h"

namespace frik
{
    // Pose values for a single finger.
    // Flex: 0.0 = bent/closed, 1.0 = straight/open.
    // Splay: positive/negative Y-axis rotation on the proximal joint (side-to-side).
    struct FingerPose
    {
        float prox = 0.0f; // proximal (MCP / knuckle) joint flex
        float mid = 0.0f; // middle (PIP) joint flex
        float dist = 0.0f; // distal (DIP / fingertip) joint flex
        float splay = 0.0f; // lateral abduction/adduction of the proximal joint

        constexpr FingerPose() noexcept = default;
        constexpr explicit FingerPose(float flex) noexcept :
            prox(flex), mid(flex), dist(flex)
        {}
        constexpr FingerPose(float proxValue, float midValue, float distValue, float splayValue = 0.0f) noexcept :
            prox(proxValue), mid(midValue), dist(distValue), splay(splayValue)
        {}
    };

    // Full hand pose: all 5 fingers, each with flex + splay.
    struct HandFingersPose
    {
        FingerPose thumb;
        FingerPose index;
        FingerPose middle;
        FingerPose ring;
        FingerPose pinky;

        constexpr HandFingersPose() noexcept = default;
        constexpr HandFingersPose(FingerPose thumbPose, FingerPose indexPose, FingerPose middlePose, FingerPose ringPose, FingerPose pinkyPose) noexcept :
            thumb(thumbPose), index(indexPose), middle(middlePose), ring(ringPose), pinky(pinkyPose)
        {}
        constexpr HandFingersPose(float thumbFlex, float indexFlex, float middleFlex, float ringFlex, float pinkyFlex) noexcept :
            thumb(thumbFlex), index(indexFlex), middle(middleFlex), ring(ringFlex), pinky(pinkyFlex)
        {}

        FingerPose& getFingerAt(int fingerIndex) noexcept;
        const FingerPose& getFingerAt(int fingerIndex) const noexcept;
        float getFlexAt(int boneIndex) const noexcept;
        float getSplayAt(int fingerIndex) const noexcept;
    };

    struct HandOverrideState
    {
        HandFingersPose pose;
        bool active = false;
    };

    class HandPose
    {
    public:
        explicit HandPose(bool inPowerArmor);

        // Papyrus / API-driven pose overrides
        static void setFingerPose(bool isLeft, const HandFingersPose& pose);
        static void restoreFingerPoseControl(bool isLeft);

        // Named pose setters
        static void setPipboyHandPose();
        static void disablePipboyHandPose();
        static void setConfigModeHandPose();
        static void disableConfigModePose();
        static void setForceHandPointingPose(bool primaryHand, bool forcePointing);
        static void setOffhandGripHandPose(bool toSet);
        static void setAttaboyHandPose(bool toSet);

        void onFrameUpdate(RE::NiNode* root, float frameTime);

    private:
        void calculateHandPose(const std::string& bone, bool isLeft, float frameTime);
        void copy1StPerson(const std::string& bone);

        bool tryGetPapyrusRotation(const std::string& bone, bool isLeft, RE::NiMatrix3& outRotation) const;
        RE::NiMatrix3 blendBoneRotation(const std::string& bone, float flex) const;
        RE::NiMatrix3 getGripBoneRotation(const std::string& bone, bool melee) const;
        RE::NiMatrix3 getThumbsUpBoneRotation(const std::string& bone, bool isLeft) const;
        RE::NiMatrix3 blendBoneRotation(const std::string& bone, float flex, float splay, bool isLeft) const;

        static void setHandPoseOverride(bool setActive, bool isLeft, const HandFingersPose& pose);
        static HandOverrideState& getHandOverrideState(bool isLeft);

        std::map<std::string, RE::NiTransform> _handClosed;
        std::map<std::string, RE::NiTransform> _handOpen;
        std::map<std::string, RE::NiTransform> _handBones;
        inline static HandOverrideState _leftHandOverride;
        inline static HandOverrideState _rightHandOverride;
    };
}
