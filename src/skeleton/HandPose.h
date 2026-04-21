#pragma once

#include <map>
#include <string>

#include "common/CommonUtils.h"
#include "f4vr/BSFlattenedBoneTree.h"

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

        constexpr FingerPose(const float proxValue, const float midValue, const float distValue, const float splayValue = 0.0f) noexcept :
            prox(proxValue), mid(midValue), dist(distValue), splay(splayValue) {}
    };

    // Full hand pose: all 5 fingers, each with flex + splay, plus optional palm motion.
    struct HandFingersPose
    {
        FingerPose thumb;
        FingerPose index;
        FingerPose middle;
        FingerPose ring;
        FingerPose pinky;
        float palmPitch = 0.0f; // wrist flexion (+) / extension (-), degrees
        float palmYaw = 0.0f; // radial (+) / ulnar (-) deviation, degrees

        constexpr HandFingersPose() noexcept = default;

        constexpr HandFingersPose(const FingerPose thumbPose, const FingerPose indexPose, const FingerPose middlePose, const FingerPose ringPose, const FingerPose pinkyPose,
            const float palmPitch = 0.0f, const float palmYaw = 0.0f) noexcept :
            thumb(thumbPose), index(indexPose), middle(middlePose), ring(ringPose), pinky(pinkyPose), palmPitch(palmPitch), palmYaw(palmYaw) {}

        FingerPose& getFingerAt(int fingerIndex) noexcept;
        const FingerPose& getFingerAt(int fingerIndex) const noexcept;
        float getFlexAt(int boneIndex) const noexcept;
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

        // API-driven pose overrides
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
        enum class HandPoseSourceKind : uint8_t
        {
            // Default controller-driven finger curl, with no authored palm pose.
            Dynamic,
            // Explicit or implicit authored pose, such as mod overrides or thumbs-up.
            OverridePose,
            // Weapon-driven hand source. This may carry an authored pose pointer, or a null pose
            // to indicate the right-handed path should copy the first-person hand transform.
            PrimaryWeaponPose
        };

        struct HandPoseSource
        {
            HandPoseSourceKind kind = HandPoseSourceKind::Dynamic;
            // Present only when the source is backed by authored pose data.
            const HandFingersPose* pose = nullptr;
        };

        struct PalmBlendState
        {
            float pitch = 0.0f; // degrees
            float yaw = 0.0f; // degrees
        };

        static HandPoseSource resolveHandPoseSource(bool isLeft);
        static void applyPalmPose(f4cf::f4vr::BSFlattenedBoneTree* boneTree, bool isLeft, const HandPoseSource& source, PalmBlendState& blendState, float frameTime);
        void applyPrimaryWeaponHandPose(const std::string& boneName, const HandPoseSource& source);
        void applyDynamicHandPose(const std::string& boneName, float frameTime);
        void applyOverrideHandPose(const std::string& boneName, const HandFingersPose* activePose, float frameTime);
        void blendBoneTowardRotation(const std::string& boneName, const RE::NiMatrix3& targetRotation, float frameTime);
        RE::NiMatrix3 getPoseBoneRotation(const std::string& boneName, const HandFingersPose& pose) const;
        RE::NiMatrix3 blendBoneRotation(const std::string& boneName, float flex, float splay) const;
        static bool shouldUseThumbsUpPose(bool isLeft);
        static void setHandPoseOverride(bool setActive, bool isLeft, const HandFingersPose& pose);
        static HandOverrideState& getHandOverrideState(bool isLeft);

        std::map<std::string, RE::NiTransform> _handClosed;
        std::map<std::string, RE::NiTransform> _handOpen;
        std::map<std::string, RE::NiTransform> _handBones;
        PalmBlendState _leftPalmBlend;
        PalmBlendState _rightPalmBlend;
        inline static HandOverrideState _leftHandOverride;
        inline static HandOverrideState _rightHandOverride;
    };
}
