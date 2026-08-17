#pragma once

#include <array>
#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "HandPoseData.h"
#include "common/CommonUtils.h"
#include "f4vr/BSFlattenedBoneTree.h"

namespace frik
{
    /**
     * The runtime side of hand posing: the tagged override registry that decides which pose wins,
     * and the per-frame blend of the winning pose into the skeleton's bone tree.
     *
     * The geometry itself lives in HandPoseMath, which this depends on.
     */
    class HandPose
    {
    public:
        /**
         * Priority given to an external override that does not name one.
         * This is the canonical definition of the scale; the published API
         * surfaces mirror it and static_assert against it.
         */
        static constexpr int PRIORITY_EXTERNAL_DEFAULT = 50;

        /**
         * Priority FRIK's own interaction poses use (Pip-Boy pointing, forced
         * pointing, offhand grip, Attaboy). An external override must exceed
         * this to outrank FRIK itself.
         */
        static constexpr int PRIORITY_FRIK_INTERNAL = 90;

        explicit HandPose(bool inPowerArmor);

        static void clearHandPoseOverridesForSkeletonRelease();

        static void setHandPoseOverride(bool isLeft, std::string_view tag, const HandFingersPose& pose, int priority);
        static bool setHandPoseOverrideLocalTransforms(bool isLeft, std::string_view tag, const std::array<RE::NiTransform, skeleton::data::FINGER_BONE_COUNT>& localTransforms,
            std::uint16_t enabledMask, int priority);
        static void clearHandPoseOverride(bool isLeft, std::string_view tag);
        static skeleton::data::HandPoseOverrideTagState getHandPoseSetTagState(bool isLeft, std::string_view tag);
        static skeleton::data::HandPoseKind getCurrentHandPoseKind(bool isLeft);
        static const HandFingersPose& getFixedPrimaryWeaponPose();

        static void setPipboyHandPose();
        static void disablePipboyHandPose();
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
            std::array<RE::NiTransform, skeleton::data::FINGER_BONE_COUNT> localTransforms{};
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
        static const RE::NiTransform* getLocalTransformOverride(const TaggedHandPoseOverride* activeOverride, const std::string& boneName);
        static bool shouldUsePointingPose(bool isLeft);
        static bool shouldUseThumbsUpPose(bool isLeft);
        static void setHandPoseOverrideIntr(bool isLeft, std::string_view tag, const HandFingersPose& pose, int priority);
        static void clearHandPoseOverrideIntr(bool isLeft, std::string_view tag);
        static std::vector<TaggedHandPoseOverride>& getHandOverrides(bool isLeft);
        static void sortHandOverrides(std::vector<TaggedHandPoseOverride>& overrides);
        static const TaggedHandPoseOverride* getActiveHandPoseOverride(bool isLeft);

        // The authored open pose resolved against this skeleton's rest translations.
        std::map<std::string, RE::NiTransform> _handOpen;
        // The live per-bone transforms this instance blends toward the resolved pose each frame.
        std::map<std::string, RE::NiTransform> _handBones;
        PalmBlendState _leftPalmBlend;
        PalmBlendState _rightPalmBlend;
        inline static std::vector<TaggedHandPoseOverride> _leftHandOverrides;
        inline static std::vector<TaggedHandPoseOverride> _rightHandOverrides;
        inline static std::uint64_t _nextOverrideSequence = 0;
    };
}
