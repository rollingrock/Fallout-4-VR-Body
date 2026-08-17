#pragma once

#include <array>
#include <cstdint>
#include <map>
#include <optional>
#include <string>

#include "HandPoseData.h"
#include "RE/NetImmerse/NiTransform.h"

namespace frik
{
    /**
     * The geometry of hand posing: turning authored poses into per-bone transforms, reading
     * animated bones back into pose values, and mirroring a pose onto the opposite hand.
     *
     * Everything here is a pure function of the authored bone data in HandPoseData, plus which
     * variant of the rest translations applies (normal or power armor). Nothing reads or writes
     * the live skeleton, which is what lets an external mod resolve a pose through the API
     * before FRIK has built one.
     *
     * HandPose owns the runtime side - the override registry and the per-frame blend into the
     * bone tree - and depends on this. The dependency never runs the other way.
     */
    struct HandPoseMath
    {
        /**
         * All 15 finger bones enabled, one bit per flat bone index.
         */
        static constexpr std::uint16_t FULL_LOCAL_TRANSFORM_MASK = 0x7FFF;

        /**
         * Return whether a hand bone belongs to the left hand based on its name prefix.
         */
        static bool isLeftHandBone(const std::string& boneName);

        /**
         * Convert a finger bone name like Finger31 into a zero-based finger index.
         */
        static int boneToFingerIndex(const std::string& boneName);

        /**
         * Convert a finger bone name into its flat index across all 15 finger bones.
         * Laid out as 3 joints (prox, mid, dist) per finger, thumb first.
         */
        static int boneToFlexIndex(const std::string& boneName);

        /**
         * Resolve the authored open pose into per-bone local transforms, keyed by bone name.
         *
         * This is the reference pose a skeleton is built against: the rest translations it
         * carries are the only authored values that differ between normal and power armor.
         */
        static std::map<std::string, RE::NiTransform> buildOpenPoseTransforms(bool inPowerArmor);

        /**
         * Blend between the authored closed/open rotations and optionally apply proximal splay.
         */
        static RE::NiMatrix3 blendBoneRotation(const std::string& boneName, float flex, float splay);

        /**
         * Resolve the target rotation for one bone from a full hand pose definition.
         */
        static RE::NiMatrix3 getPoseBoneRotation(const std::string& boneName, const HandFingersPose& pose);

        /**
         * Convert one bone's animated rotation into the equivalent rotation on the opposite hand.
         *
         * The two hands do not share a bind pose, so this cannot mirror rotations geometrically.
         * Instead the source bone is measured back into flex/splay against its own authored
         * open/closed pair, and those values are re-applied against the target bone's pair. The
         * thumb base is special-cased, because its rotation axis does not decompose cleanly into
         * flex and splay.
         *
         * @return nullopt if the bones are unknown or the result is not finite.
         */
        static std::optional<RE::NiMatrix3> mirrorBoneRotation(const std::string& sourceBoneName, const std::string& targetBoneName, const RE::NiMatrix3& animatedRotation);

        /**
         * Resolve the per-bone local transforms one hand would take under an authored pose.
         *
         * Lets an external system read FRIK's authored poses as explicit transforms, adjust them,
         * and publish the result back as a hand pose override.
         *
         * @return true only if all 15 bones resolved; on failure the outputs are zeroed so a
         * partial pose can never be published.
         */
        static bool buildFingerLocalTransformsForPose(bool isLeft, const HandFingersPose& pose, std::array<RE::NiTransform, skeleton::data::FINGER_BONE_COUNT>& outTransforms,
            std::uint16_t& outEnabledMask);

        /**
         * Convert a complete finger pose on one hand into the opposite hand's anatomical pose.
         *
         * @return true only if all 15 bones mirrored; a partial source mask is rejected outright
         * and any failure leaves the outputs zeroed.
         */
        static bool mirrorFingerLocalTransforms(bool sourceIsLeft, const std::array<RE::NiTransform, skeleton::data::FINGER_BONE_COUNT>& sourceTransforms,
            std::uint16_t sourceEnabledMask, std::array<RE::NiTransform, skeleton::data::FINGER_BONE_COUNT>& outTargetTransforms, std::uint16_t& outTargetEnabledMask);

    private:
        static float inverseBlendFlex(const std::string& boneName, const RE::NiMatrix3& animatedRotation);
        static void measureAnimatedFlexSplay(const std::string& sourceBoneName, const RE::NiMatrix3& animatedRotation, float& outFlex, float& outSplay);
        static bool tryTransferMirroredThumbBase(const std::string& sourceBoneName, const std::string& targetBoneName, const RE::NiMatrix3& animatedRotation,
            RE::NiMatrix3& outRotation);
        static std::optional<RE::NiTransform> mirrorBoneToOppositeHand(const std::string& sourceBoneName, const std::string& targetBoneName, const RE::NiMatrix3& animatedRotation,
            bool inPowerArmor);
    };
}
