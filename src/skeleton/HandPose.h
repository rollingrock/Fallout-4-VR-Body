#pragma once

#include <array>
#include <map>
#include <cstdint>
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
        static constexpr std::size_t FINGER_BONE_COUNT = 15;

        explicit HandPose(bool inPowerArmor);

        static void setHandPoseOverride(bool isLeft, std::string_view tag, const HandFingersPose& pose, bool forceTop);
        static void setHandPoseOverrideWithPriority(bool isLeft, std::string_view tag, const HandFingersPose& pose, int priority);
        static bool setHandPoseLocalTransformsWithPriority(
            bool isLeft,
            std::string_view tag,
            const std::array<RE::NiTransform, FINGER_BONE_COUNT>& localTransforms,
            std::uint16_t enabledMask,
            int priority);
        static void clearHandPoseOverride(bool isLeft, std::string_view tag);
        static skeleton::data::HandPoseOverrideTagState getHandPoseSetTagState(bool isLeft, std::string_view tag);
        static skeleton::data::HandPoseKind getCurrentHandPoseKind(bool isLeft);
        static bool blockPrimaryWeaponPose(std::string_view tag, bool block);
        static bool isPrimaryWeaponPoseBlocked();
        static void clearPrimaryWeaponPoseBlocks();
        static const HandFingersPose& getFixedPrimaryWeaponPose();
        static bool buildFingerLocalTransformsForPose(
            bool isLeft,
            const HandFingersPose& pose,
            std::array<RE::NiTransform, FINGER_BONE_COUNT>& outTransforms,
            std::uint16_t& outEnabledMask);

        /*
         * Convert a complete physical-hand finger pose into the opposite
         * hand's anatomical pose. This is instance-owned because its
         * bind/reference transforms differ between normal and power armor.
         */
        bool mirrorFingerLocalTransforms(bool sourceIsLeft, const std::array<RE::NiTransform, FINGER_BONE_COUNT>& sourceTransforms, std::uint16_t sourceEnabledMask,
            std::array<RE::NiTransform, FINGER_BONE_COUNT>& outTargetTransforms, std::uint16_t& outTargetEnabledMask) const;

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

        struct TaggedHandPoseOverride
        {
            std::string tag;
            HandFingersPose pose;
            int priority = 50;
            std::uint64_t sequence = 0;
            std::uint16_t localTransformMask = 0;
            std::array<RE::NiTransform, FINGER_BONE_COUNT> localTransforms{};
        };

        struct HandPoseSource
        {
            HandPoseSourceKind kind = HandPoseSourceKind::Dynamic;
            // Present only when the source is backed by authored pose data.
            const HandFingersPose* pose = nullptr;
            const TaggedHandPoseOverride* overrideEntry = nullptr;
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
        void applyOverrideHandPose(const std::string& boneName, const HandFingersPose* activePose, const TaggedHandPoseOverride* activeOverride, float frameTime);
        void blendBoneTowardRotation(const std::string& boneName, const RE::NiMatrix3& targetRotation, float frameTime);
        void blendBoneTowardTransform(const std::string& boneName, const RE::NiTransform& targetTransform, float frameTime);
        RE::NiMatrix3 getPoseBoneRotation(const std::string& boneName, const HandFingersPose& pose) const;
        RE::NiMatrix3 blendBoneRotation(const std::string& boneName, float flex, float splay) const;
        float inverseBlendFlex(const std::string& boneName, const RE::NiMatrix3& animatedRotation) const;
        void measureAnimatedFlexSplay(const std::string& sourceBoneName, const RE::NiMatrix3& animatedRotation, float& outFlex, float& outSplay) const;
        bool tryTransferMirroredThumbBase(const std::string& sourceBoneName, const std::string& targetBoneName, const RE::NiMatrix3& animatedRotation, RE::NiMatrix3& outRotation) const;
        static const RE::NiTransform* getLocalTransformOverride(const TaggedHandPoseOverride* activeOverride, const std::string& boneName);
        static bool shouldUseThumbsUpPose(bool isLeft);
        static void setHandPoseOverrideIntr(bool isLeft, std::string_view tag, const HandFingersPose& pose, int priority);
        static void clearHandPoseOverrideIntr(bool isLeft, std::string_view tag);
        static std::vector<TaggedHandPoseOverride>& getHandOverrides(bool isLeft);
        static const TaggedHandPoseOverride* getActiveHandPoseOverride(bool isLeft);

        std::map<std::string, RE::NiTransform> _handClosed;
        std::map<std::string, RE::NiTransform> _handOpen;
        std::map<std::string, RE::NiTransform> _handBones;
        PalmBlendState _leftPalmBlend;
        PalmBlendState _rightPalmBlend;
        inline static std::vector<TaggedHandPoseOverride> _leftHandOverrides;
        inline static std::vector<TaggedHandPoseOverride> _rightHandOverrides;
        inline static std::uint64_t _nextOverrideSequence = 0;
    };
}
