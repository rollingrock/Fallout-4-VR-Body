#pragma once

#include <map>
#include <string>
#include <string_view>
#include <vector>

#include "HandPoseData.h"
#include "common/CommonUtils.h"
#include "f4vr/BSFlattenedBoneTree.h"

namespace frik
{
    class HandPose
    {
    public:
        explicit HandPose(bool inPowerArmor);

        static void setFingerPose(bool isLeft, std::string_view tag, const HandFingersPose& pose);
        static void restoreFingerPoseControl(bool isLeft, std::string_view tag);
        static skeleton::data::HandPoseOverrideTagState getHandPoseSetTagState(bool isLeft, std::string_view tag);
        static skeleton::data::HandPoseKind getCurrentHandPoseKind(bool isLeft);

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

        struct TaggedHandPoseOverride
        {
            std::string tag;
            HandFingersPose pose;
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
        static void setHandPoseOverride(bool isLeft, std::string_view tag, const HandFingersPose& pose);
        static void clearHandPoseOverride(bool isLeft, std::string_view tag);
        static std::vector<TaggedHandPoseOverride>& getHandOverrides(bool isLeft);
        static const TaggedHandPoseOverride* getActiveHandPoseOverride(bool isLeft);

        std::map<std::string, RE::NiTransform> _handClosed;
        std::map<std::string, RE::NiTransform> _handOpen;
        std::map<std::string, RE::NiTransform> _handBones;
        PalmBlendState _leftPalmBlend;
        PalmBlendState _rightPalmBlend;
        inline static std::vector<TaggedHandPoseOverride> _leftHandOverrides;
        inline static std::vector<TaggedHandPoseOverride> _rightHandOverrides;
    };
}
