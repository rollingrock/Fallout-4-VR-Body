#include "HandPose.h"

#include <algorithm>
#include <optional>
#include <string>
#include <string_view>

#include "Config.h"
#include "ExternalAuthority.h"
#include "FRIK.h"
#include "HandPoseData.h"
#include "HandPoseMath.h"
#include "common/MatrixUtils.h"
#include "common/PerfMonitor.h"
#include "common/Quaternion.h"
#include "f4vr/BSFlattenedBoneTree.h"
#include "f4vr/F4VRSkelly.h"
#include "f4vr/F4VRUtils.h"
#include "utils.h"
#include "vrcf/VRControllersManager.h"

using namespace common;
using namespace f4vr;
using namespace vrcf;
using namespace frik::skeleton::data;

namespace
{
    /**
     * Map a (non-thumb) finger to the controller button that should drive its dynamic curl.
     * The index finger follows the trigger; the bottom three fingers (middle, ring, pinky) follow the grip.
     */
    VRButtonId getTrackedButton(const std::string& bone)
    {
        return frik::HandPoseMath::boneToFingerIndex(bone) == 1 ? k_EButton_SteamVR_Trigger : k_EButton_Grip;
    }

    /**
     * Map a tracked finger button to its analog curl-depth axis, when the controller exposes one.
     * Touch-only inputs (face buttons, thumbstick) report no axis.
     */
    std::optional<Axis> getTrackedButtonAxis(const VRButtonId button)
    {
        switch (button) { // NOLINT(clang-diagnostic-switch-enum)
        case k_EButton_SteamVR_Trigger:
            return Axis::Trigger;
        case k_EButton_Grip:
            return Axis::Grip;
        default:
            return std::nullopt;
        }
    }

    struct DynamicThumbCurl
    {
        float curl; // 0 = open, 1 = fully closed
        float splay; // proximal lateral splay, right-hand convention
    };

    /**
     * Resolve the thumb's dynamic curl and proximal splay from the face control it rests on.
     *
     * The thumb has no usable curl-depth axis, so touching any of its controls curls it to a fixed
     * mid pose while the touched control selects a distinct splay, shifting the thumb tip toward the
     * thumbstick, A, or B button. Returns a neutral open thumb when nothing is touched. The A and B
     * buttons are checked before the thumbstick so a deliberate button reach wins over the resting
     * thumbstick contact.
     */
    DynamicThumbCurl resolveDynamicThumbCurl(const Hand hand)
    {
        if (VRControllers.isTouching(hand, k_EButton_A)) {
            return { .curl = 0.7f, .splay = 0.2f };
        }
        if (VRControllers.isTouching(hand, k_EButton_ApplicationMenu)) {
            return { .curl = 0.2f, .splay = 0.1f };
        }
        if (VRControllers.isTouching(hand, k_EButton_SteamVR_Touchpad)) {
            return { .curl = 0.4f, .splay = 0.3f };
        }
        return { .curl = 0.0f, .splay = 0.0f };
    }

    /**
     * Refresh one flattened bone transform after its local transform changes.
     */
    void refreshFlattenedBoneTransform(const BSFlattenedBoneTree* const boneTree, const int pos)
    {
        auto& transform = boneTree->transforms[pos];
        if (transform.refNode) {
            transform.refNode->local = transform.local;
        }

        const auto parentWorld = transform.refNode && transform.refNode->parent ? transform.refNode->parent->world : boneTree->transforms[transform.parPos].world;
        RE::NiPoint3 p = transform.local.translate;
        p = parentWorld.rotate.Transpose() * (p * parentWorld.scale);
        transform.world.translate = parentWorld.translate + p;
        transform.world.rotate = transform.local.rotate * parentWorld.rotate;
        transform.world.scale = transform.local.scale * parentWorld.scale;

        if (transform.refNode) {
            transform.refNode->world = transform.world;
        }
    }

    /**
     * Blend one palm axis toward its target and snap the tail to avoid endless tiny drift.
     */
    void blendPalmAxisToward(float& currentValue, const float targetValue, const float frameTime)
    {
        if (fEqual(currentValue, targetValue)) {
            currentValue = targetValue;
            return;
        }

        currentValue += (targetValue - currentValue) * std::clamp(frameTime * 7.0f, 0.0f, 1.0f);
        if (fEqual(currentValue, targetValue)) {
            currentValue = targetValue;
        }
    }

    constexpr std::string_view PIPBOY_HAND_POSE_TAG = "frik.pipboy";
    constexpr std::string_view FORCE_POINTING_HAND_POSE_TAG = "frik.force_pointing";
    constexpr std::string_view OFFHAND_GRIP_HAND_POSE_TAG = "frik.offhand_grip";
    constexpr std::string_view ATTABOY_HAND_POSE_TAG = "frik.attaboy";

}

namespace frik
{
    // -- HandFingersPose ----------------------------------------------------------------

    /**
     * Return a mutable finger pose by zero-based finger index.
     */
    FingerPose& HandFingersPose::getFingerAt(const int fingerIndex) noexcept
    {
        FingerPose* const fingers[5] = { &thumb, &index, &middle, &ring, &pinky };
        return *fingers[fingerIndex];
    }

    /**
     * Return a read-only finger pose by zero-based finger index.
     */
    const FingerPose& HandFingersPose::getFingerAt(const int fingerIndex) const noexcept
    {
        const FingerPose* const fingers[5] = { &thumb, &index, &middle, &ring, &pinky };
        return *fingers[fingerIndex];
    }

    /**
     * Return a flex value by flat bone index.
     *
     * Bone order is 0-2: thumb, 3-5: index, 6-8: middle, 9-11: ring, 12-14: pinky,
     * with each finger laid out as prox, mid, dist.
     */
    float HandFingersPose::getFlexAt(const int boneIndex) const noexcept
    {
        const FingerPose& f = getFingerAt(boneIndex / 3);
        switch (boneIndex % 3) {
        case 0:
            return f.prox;
        case 1:
            return f.mid;
        default:
            return f.dist;
        }
    }

    // -- Lifecycle ----------------------------------------------------------------------

    /**
     * Resolve the authored open transforms against this skeleton's rest translations, which is the
     * only part of the reference data that depends on whether the player is in power armor.
     */
    HandPose::HandPose(const bool inPowerArmor)
    {
        _handOpen = HandPoseMath::buildOpenPoseTransforms(inPowerArmor);
        _handBones = _handOpen;
    }

    /**
     * Drop every tagged pose override when the skeleton is released, FRIK's own interaction poses
     * along with the external ones, because the bones they were resolved against are going away.
     * Clients must republish after the next skeleton-ready event, and FRIK's own poses are re-set by
     * the subsystems that own them once the new skeleton starts updating.
     *
     * The overrides outlive any single HandPose instance, so this is deliberately driven by the
     * release path rather than by the next HandPose constructor: resetting shared state as a
     * side effect of building the next instance would also discard anything published in between.
     */
    void HandPose::clearHandPoseOverridesForSkeletonRelease()
    {
        const auto dropped = _leftHandOverrides.size() + _rightHandOverrides.size();
        _leftHandOverrides.clear();
        _rightHandOverrides.clear();
        _nextOverrideSequence = 0;

        if (dropped > 0) {
            logger::info("Skeleton release: dropped {} hand pose override(s), FRIK's own and external alike", dropped);
        }
    }

    /**
     * Activate an explicit pose override for one hand at an explicit priority.
     *
     * Overrides are ordered by priority, and within one priority by registration
     * order, so a caller can layer several of its own tags deterministically.
     * See PRIORITY_EXTERNAL_DEFAULT and PRIORITY_FRIK_INTERNAL for the scale.
     */
    void HandPose::setHandPoseOverride(const bool isLeft, const std::string_view tag, const HandFingersPose& pose, const int priority)
    {
        setHandPoseOverrideIntr(isLeft, tag, pose, priority);
    }

    /**
     * Replace the explicit per-bone finger transforms of an existing override.
     *
     * Only bones whose bit is set in enabledMask are taken; the rest keep falling
     * back to the tag's authored pose. The tag must already hold an override, and
     * any later setHandPoseOverride* call on it drops these transforms again, so
     * callers that refresh their pose must republish the transforms with it.
     *
     * @return false if the tag holds no override or the priority is negative.
     */
    bool HandPose::setHandPoseOverrideLocalTransforms(const bool isLeft, const std::string_view tag,
        const std::array<RE::NiTransform, skeleton::data::FINGER_BONE_COUNT>& localTransforms, const std::uint16_t enabledMask, const int priority)
    {
        if (tag.empty() || priority < 0) {
            return false;
        }

        auto& overrides = getHandOverrides(isLeft);
        const auto overrideIt = std::ranges::find_if(overrides, [tag](const TaggedHandPoseOverride& overrideEntry) {
            return overrideEntry.tag == tag;
        });
        if (overrideIt == overrides.end()) {
            return false;
        }

        // Sequence is deliberately left alone: it records registration order, and
        // this call reorders through the explicit priority it was given.
        overrideIt->priority = priority;
        overrideIt->localTransformMask = static_cast<std::uint16_t>(enabledMask & HandPoseMath::FULL_LOCAL_TRANSFORM_MASK);
        overrideIt->localTransforms = localTransforms;

        sortHandOverrides(overrides);

        return true;
    }

    /**
     * Release any explicit override and return control to runtime hand logic.
     */
    void HandPose::clearHandPoseOverride(const bool isLeft, const std::string_view tag)
    {
        clearHandPoseOverrideIntr(isLeft, tag);
    }

    /**
     * Return whether a specific override tag is absent, active, or shadowed by another tag.
     */
    HandPoseOverrideTagState HandPose::getHandPoseSetTagState(const bool isLeft, const std::string_view tag)
    {
        const auto& overrides = getHandOverrides(isLeft);
        const auto overrideIt = std::ranges::find_if(overrides, [tag](const TaggedHandPoseOverride& overrideEntry) {
            return overrideEntry.tag == tag;
        });
        if (overrideIt == overrides.end()) {
            return HandPoseOverrideTagState::None;
        }

        const auto* activeOverride = getActiveHandPoseOverride(isLeft);
        if (activeOverride && activeOverride->tag == tag) {
            const auto source = resolveHandPoseSource(isLeft);
            if (source.kind == HandPoseSourceKind::OverridePose && source.pose == &activeOverride->pose) {
                return HandPoseOverrideTagState::Active;
            }
        }

        return HandPoseOverrideTagState::Overridden;
    }

    /**
     * Return the effective hand pose kind resolved for the current frame.
     */
    HandPoseKind HandPose::getCurrentHandPoseKind(const bool isLeft)
    {
        const auto source = resolveHandPoseSource(isLeft);
        if (source.kind == HandPoseSourceKind::PrimaryWeaponPose) {
            return source.pose ? source.pose->kind : HandPoseKind::HoldingWeapon;
        }

        if (source.kind == HandPoseSourceKind::OverridePose && source.pose) {
            return source.pose->kind;
        }

        return HandPoseKind::Unset;
    }

    /**
     * Return the fixed authored weapon pose used when runtime hand posing cannot copy the first-person hand.
     */
    const HandFingersPose& HandPose::getFixedPrimaryWeaponPose()
    {
        return isUnarmedWeaponDrawn() ? getFistPose() : (g_frik.isMeleeWeaponDrawn() ? getMeleeGripPose() : getGunGripPose());
    }

    /**
     * Force the Pip-Boy interaction hand into the pointing pose.
     */
    void HandPose::setPipboyHandPose()
    {
        setHandPoseOverrideIntr(g_config.leftHandedPipBoy, PIPBOY_HAND_POSE_TAG, getPointingPose(), HandPose::PRIORITY_FRIK_INTERNAL);
    }

    /**
     * Release the temporary Pip-Boy pointing pose.
     */
    void HandPose::disablePipboyHandPose()
    {
        clearHandPoseOverrideIntr(g_config.leftHandedPipBoy, PIPBOY_HAND_POSE_TAG);
    }

    /**
     * Toggle a pointing override on either the primary or offhand.
     */
    void HandPose::setForceHandPointingPose(const bool primaryHand, const bool forcePointing)
    {
        if (forcePointing) {
            setHandPoseOverrideIntr(primaryHand == isLeftHandedMode(), FORCE_POINTING_HAND_POSE_TAG, getPointingPose(), HandPose::PRIORITY_FRIK_INTERNAL);
        } else {
            clearHandPoseOverrideIntr(primaryHand == isLeftHandedMode(), FORCE_POINTING_HAND_POSE_TAG);
        }
    }

    /**
     * Toggle the authored offhand weapon support pose.
     */
    void HandPose::setOffhandGripHandPose(const bool toSet)
    {
        if (toSet) {
            setHandPoseOverrideIntr(!isLeftHandedMode(), OFFHAND_GRIP_HAND_POSE_TAG, getOffhandWeaponGripPose(), HandPose::PRIORITY_FRIK_INTERNAL);
        } else {
            clearHandPoseOverrideIntr(!isLeftHandedMode(), OFFHAND_GRIP_HAND_POSE_TAG);
        }
    }

    /**
     * Toggle the authored Attaboy interaction pose on the left hand.
     */
    void HandPose::setAttaboyHandPose(const bool toSet)
    {
        if (toSet) {
            setHandPoseOverrideIntr(true, ATTABOY_HAND_POSE_TAG, getAttaboyPose(), HandPose::PRIORITY_FRIK_INTERNAL);
        } else {
            clearHandPoseOverrideIntr(true, ATTABOY_HAND_POSE_TAG);
        }
    }

    /**
     * Update all tracked hand bones for the current frame.
     *
     * First, apply any pose-driven palm offset on the hand root so child finger bones inherit it.
     *
     * For each finger bone in the flattened bone tree, this selects the highest-priority
     * pose source in this order:
     * 1. primary weapon pose for the dominant hand while a weapon is drawn
     * 2. explicit per-hand override poses, including named poses and thumbs-up
     * 3. dynamic controller-driven curl based on touch state and grip axis
     *
     * After the target local rotation is chosen, the result is written back into the
     * flattened bone tree and the bone's world transform is refreshed so downstream code
     * sees a consistent hierarchy even when a ref node is not present.
     */
    void HandPose::onFrameUpdate(RE::NiNode* root, const float frameTime)
    {
        static PerfMonitor perf("HandPose::onFrameUpdate");
        const auto timer = perf.scope();

        const auto leftHandSource = resolveHandPoseSource(true);
        const auto rightHandSource = resolveHandPoseSource(false);

        const auto rt = reinterpret_cast<BSFlattenedBoneTree*>(root);
        applyPalmPose(rt, true, leftHandSource, _leftPalmBlend, frameTime);
        applyPalmPose(rt, false, rightHandSource, _rightPalmBlend, frameTime);

        for (auto pos = 0; pos < rt->numTransforms; pos++) {
            const auto& boneName = Skelly::getBoneName(pos);
            auto handBoneIt = _handBones.find(boneName);
            if (handBoneIt == _handBones.end()) {
                continue;
            }

            const bool leftHandBone = HandPoseMath::isLeftHandBone(boneName);
            const auto& source = leftHandBone ? leftHandSource : rightHandSource;
            if (source.kind == HandPoseSourceKind::PrimaryWeaponPose) {
                applyPrimaryWeaponHandPose(boneName, source);
            } else if (source.kind == HandPoseSourceKind::OverridePose) {
                applyOverrideHandPose(boneName, source.pose, source.overrideEntry, frameTime);
            } else {
                applyDynamicHandPose(boneName, frameTime);
            }

            rt->transforms[pos].local.rotate = handBoneIt->second.rotate;
            rt->transforms[pos].local.scale = handBoneIt->second.scale;
            if (source.kind == HandPoseSourceKind::OverridePose) {
                if (const auto* localTransform = getLocalTransformOverride(source.overrideEntry, boneName)) {
                    rt->transforms[pos].local.translate = localTransform->translate;
                } else {
                    rt->transforms[pos].local.translate = _handOpen.at(boneName).translate;
                }
            } else {
                rt->transforms[pos].local.translate = _handOpen.at(boneName).translate;
            }
            refreshFlattenedBoneTransform(rt, pos);
        }
    }

    /**
     * Resolve the active hand pose source for one hand for this frame.
     *
     * This is the single hand-level source selection used by both pose consumers:
     * 1. the palm prepass, which needs an authored pose before child finger bones are updated
     * 2. the per-finger loop, which needs to know whether to use weapon, override, or dynamic logic
     *
     * Source priority matches the existing runtime behavior:
     * 1. primary weapon pose for the dominant hand while a weapon is drawn
     * 2. explicit hand override, then implicit pointing, then implicit thumbs-up
     * 3. dynamic controller-driven curl
     *
     * Both weapon-pose paths yield entirely while an external system blocks them via
     * blockPrimaryWeaponPose. Separately, when an external system owns the weapon node
     * (isPrimaryWeaponNodeOwnershipBlocked) the off-side hand in right-handed mode also
     * follows the first-person weapon hand, so a two-handed grip stays consistent.
     *
     * The returned source may intentionally have `pose == nullptr` when the active source is the
     * right-handed primary weapon path. In that case, finger bones should copy the first-person hand
     * transform instead of using authored pose data, and the palm prepass should do nothing.
     */
    HandPose::HandPoseSource HandPose::resolveHandPoseSource(const bool isLeft)
    {
        const bool isPrimaryHand = isLeft == isLeftHandedMode();
        const bool shouldUseWeaponPoseForPrimaryHand =
            isWeaponDrawn() && !g_externalAuthority.isPrimaryWeaponPoseBlocked() && (isLeftHandedMode() || !g_frik.isPipboyOperatingWithFinger());

        if (shouldUseWeaponPoseForPrimaryHand && isLeftHandedMode() && isUnarmedWeaponDrawn()) {
            // Left-handed unarmed is a special authored fist case that applies to both hands.
            return HandPoseSource{ .kind = HandPoseSourceKind::PrimaryWeaponPose, .pose = &getFistPose() };
        }

        if (isLeft && !isLeftHandedMode() && isWeaponDrawn() && !g_frik.isPipboyOperatingWithFinger() && g_externalAuthority.isPrimaryWeaponNodeOwnershipBlocked() &&
            !g_externalAuthority.isPrimaryWeaponPoseBlocked()) {
            return HandPoseSource{ .kind = HandPoseSourceKind::PrimaryWeaponPose, .pose = nullptr };
        }

        if (isPrimaryHand && shouldUseWeaponPoseForPrimaryHand) {
            // Right-handed mode can copy the first-person hand transform directly. Left-handed mode
            // cannot, so it uses the fixed authored weapon pose instead.
            return HandPoseSource{ .kind = HandPoseSourceKind::PrimaryWeaponPose, .pose = isLeftHandedMode() ? &HandPose::getFixedPrimaryWeaponPose() : nullptr };
        }

        if (const auto* activeOverride = getActiveHandPoseOverride(isLeft)) {
            // Explicit mod/API override wins over gesture-driven posing.
            return HandPoseSource{ .kind = HandPoseSourceKind::OverridePose, .pose = &activeOverride->pose, .overrideEntry = activeOverride };
        }

        if (shouldUsePointingPose(isLeft)) {
            // Pointing is treated as an implicit authored override.
            return HandPoseSource{ .kind = HandPoseSourceKind::OverridePose, .pose = &getPointingPose() };
        }

        if (shouldUseThumbsUpPose(isLeft)) {
            // Thumbs-up is treated as an implicit authored override.
            return HandPoseSource{ .kind = HandPoseSourceKind::OverridePose, .pose = &getThumbsUpPose() };
        }

        // Otherwise, let controller input drive the fingers and leave the palm neutral.
        return HandPoseSource{};
    }

    /**
     * Blend and apply the authored palm offset from the active hand source to one hand root transform.
     */
    void HandPose::applyPalmPose(BSFlattenedBoneTree* const boneTree, const bool isLeft, const HandPoseSource& source, PalmBlendState& blendState, const float frameTime)
    {
        const int pos = boneTree->GetBoneIndex(isLeft ? "LArm_Hand" : "RArm_Hand");
        if (pos < 0) {
            return;
        }

        auto& transform = boneTree->transforms[pos];

        // The flattened tree locals/worlds are not rebuilt by Skeleton arm IK. Sync the hand root
        // from the live refNode every frame so child finger refreshes use the correct wrist basis
        // even when there is no additional palm offset applied.
        if (transform.refNode) {
            transform.local = transform.refNode->local;
            transform.world = transform.refNode->world;
        }

        const float targetPalmPitch = source.pose ? source.pose->palmPitch : 0.0f;
        const float targetPalmYaw = source.pose ? source.pose->palmYaw : 0.0f;
        blendPalmAxisToward(blendState.pitch, targetPalmPitch, frameTime);
        blendPalmAxisToward(blendState.yaw, targetPalmYaw, frameTime);

        if (fEqual(blendState.pitch, 0.0f) && fEqual(blendState.yaw, 0.0f)) {
            return;
        }

        // Apply the offset in the hand's local space so flexion/deviation follow the hand basis
        // instead of the parent forearm basis.
        const float deviationSign = isLeft ? -1.0f : 1.0f;
        transform.local.rotate = transform.local.rotate * MatrixUtils::getMatrixFromEulerAnglesDegrees(0.0f, deviationSign * blendState.yaw, blendState.pitch);
        refreshFlattenedBoneTransform(boneTree, pos);
    }

    /**
     * Apply the special primary-hand weapon pose for the current handedness mode.
     *
     * Right-handed: Copy the 1st-person bone position for the given hand bone.
     * Useful for different weapon holding hand poses.
     *
     * Left-handed: the 1st-person skeleton is not using the correct hand,
     * so use a fixed grip pose instead of copying the 1st-person weapon hand.
     */
    void HandPose::applyPrimaryWeaponHandPose(const std::string& boneName, const HandPoseSource& source)
    {
        if (source.pose) {
            auto& bone = _handBones[boneName];
            bone.rotate = HandPoseMath::getPoseBoneRotation(boneName, *source.pose);
            bone.scale = 1.0f;
        } else if (HandPoseMath::isLeftHandBone(boneName)) {
            const auto fpTree = getFirstPersonBoneTree();
            const std::string rightBoneName = "RArm_" + boneName.substr(5);
            const int pos = fpTree ? fpTree->GetBoneIndex(rightBoneName) : -1;
            if (pos < 0) {
                return;
            }
            const RE::NiTransform& animated = fpTree->transforms[pos].refNode ? fpTree->transforms[pos].refNode->local : fpTree->transforms[pos].local;
            const auto mirroredRotation = HandPoseMath::mirrorBoneRotation(rightBoneName, boneName, animated.rotate);
            if (!mirroredRotation) {
                return;
            }
            auto& bone = _handBones[boneName];
            bone.rotate = *mirroredRotation;
            bone.scale = 1.0f;
        } else {
            const auto fpTree = getFirstPersonBoneTree();
            const int pos = fpTree ? fpTree->GetBoneIndex(boneName) : -1;
            if (pos >= 0) {
                _handBones[boneName] = fpTree->transforms[pos].refNode ? fpTree->transforms[pos].refNode->local : fpTree->transforms[pos].local;
            }
        }
    }

    /**
     * Apply hand pose by the current active hand pose override from external source.
     */
    void HandPose::applyOverrideHandPose(const std::string& boneName, const HandFingersPose* const activePose, const TaggedHandPoseOverride* const activeOverride,
        const float frameTime)
    {
        if (const auto* localTransform = getLocalTransformOverride(activeOverride, boneName)) {
            blendBoneTowardTransform(boneName, *localTransform, frameTime);
            return;
        }

        auto& bone = _handBones.at(boneName);
        bone.translate = _handOpen.at(boneName).translate;
        bone.scale = 1.0f;
        const auto targetRotation = HandPoseMath::getPoseBoneRotation(boneName, *activePose);
        blendBoneTowardRotation(boneName, targetRotation, frameTime);
    }

    /**
     * Apply hand pose by what controller buttons are touched/pressed.
     *
     * Touching a finger's tracked control curls it to a baseline; if that control exposes an analog
     * axis (trigger, grip) the axis drives the remaining curl up to a full fist, otherwise the curl
     * rests at that fixed touch baseline. The thumb has no curl-depth axis, so it instead reads which face
     * control it rests on (thumbstick, A, or B) and splays its proximal joint toward that control.
     */
    void HandPose::applyDynamicHandPose(const std::string& boneName, const float frameTime)
    {
        const auto boneHand = HandPoseMath::isLeftHandBone(boneName) ? Hand::Left : Hand::Right;

        float curl = 0.0f; // 0 = open, 1 = fully closed
        float splay = 0.0f;
        if (HandPoseMath::boneToFingerIndex(boneName) == 0) {
            const auto thumb = resolveDynamicThumbCurl(boneHand);
            curl = thumb.curl;
            splay = boneName.back() == '1' ? thumb.splay : 0.0f;
        } else {
            constexpr float DYNAMIC_CURL_ON_TOUCH = 0.35f;
            const auto button = getTrackedButton(boneName);
            const bool rightTriggerIdentityRemapped =
                boneHand == Hand::Right && button == k_EButton_SteamVR_Trigger && !isLeftHandedMode() && g_externalAuthority.isPrimaryWeaponNodeOwnershipBlocked();
            if (!rightTriggerIdentityRemapped) {
                const auto axis = getTrackedButtonAxis(button);
                const float axisVal = axis ? VRControllers.getAxisValue(boneHand, *axis).x : 0.0f;
                if (axisVal > 0.1f) {
                    // Past the deadzone the analog axis drives curl from the touch baseline up to a full fist.
                    curl = DYNAMIC_CURL_ON_TOUCH + (1.0f - DYNAMIC_CURL_ON_TOUCH) * std::clamp(axisVal, 0.0f, 1.0f);
                } else if (axisVal > 0.001f || VRControllers.isTouching(boneHand, button)) {
                    // Resting on or lightly holding the control curls to the baseline only.
                    curl = DYNAMIC_CURL_ON_TOUCH;
                }
            }
        }

        blendBoneTowardRotation(boneName, HandPoseMath::blendBoneRotation(boneName, 1.0f - curl, splay), frameTime);
    }

    /**
     * Smoothly blend a runtime bone toward a target rotation for this frame.
     */
    void HandPose::blendBoneTowardRotation(const std::string& boneName, const RE::NiMatrix3& targetRotation, const float frameTime)
    {
        Quaternion qc, qt;
        auto& currentBone = _handBones.at(boneName);
        qc.fromMatrix(currentBone.rotate);
        qt.fromMatrix(targetRotation);
        const float blend = std::clamp(frameTime * 7, -1.0f, 2.0f);
        qc.slerp(blend, qt);
        currentBone.rotate = qc.getMatrix();
    }

    /**
     * Smoothly blend a runtime bone toward a full target transform for this frame.
     *
     * Only the rotation is eased; translation and scale are taken directly, since an
     * explicit per-bone override supplies exact rest offsets that must not be
     * interpolated away.
     */
    void HandPose::blendBoneTowardTransform(const std::string& boneName, const RE::NiTransform& targetTransform, const float frameTime)
    {
        blendBoneTowardRotation(boneName, targetTransform.rotate, frameTime);

        auto& currentBone = _handBones.at(boneName);
        currentBone.translate = targetTransform.translate;
        currentBone.scale = targetTransform.scale;
    }

    /**
     * Return the explicit local transform an override supplies for one bone, if any.
     *
     * Yields nullptr when there is no override, the bone is not in its enabled mask,
     * or the stored transform is not finite - in every case the caller falls back to
     * the tag's authored pose, so a bad transform degrades instead of corrupting the
     * scene graph.
     */
    const RE::NiTransform* HandPose::getLocalTransformOverride(const TaggedHandPoseOverride* const activeOverride, const std::string& boneName)
    {
        if (!activeOverride) {
            return nullptr;
        }

        const int flatBoneIndex = HandPoseMath::boneToFlexIndex(boneName);
        if (flatBoneIndex < 0 || flatBoneIndex >= static_cast<int>(skeleton::data::FINGER_BONE_COUNT)) {
            return nullptr;
        }

        const std::uint16_t bit = static_cast<std::uint16_t>(1U << flatBoneIndex);
        if ((activeOverride->localTransformMask & bit) == 0) {
            return nullptr;
        }

        const auto& localTransform = activeOverride->localTransforms[flatBoneIndex];
        return isFiniteTransform(localTransform) ? &localTransform : nullptr;
    }

    /**
     * Return the current override list for one hand, kept sorted by sortHandOverrides
     * so the winning override is always at the front.
     */
    std::vector<HandPose::TaggedHandPoseOverride>& HandPose::getHandOverrides(const bool isLeft)
    {
        return isLeft ? _leftHandOverrides : _rightHandOverrides;
    }

    /**
     * Return the winning explicit override for one hand, if any: the highest
     * priority, and among equals the one that registered most recently.
     */
    const HandPose::TaggedHandPoseOverride* HandPose::getActiveHandPoseOverride(const bool isLeft)
    {
        const auto& overrides = getHandOverrides(isLeft);
        return overrides.empty() ? nullptr : &overrides.front();
    }

    /**
     * Detect the controller gesture that should temporarily map to pointing:
     * the index lifted off the trigger, the grip held, and the thumb resting on a face button (A or B).
     */
    bool HandPose::shouldUsePointingPose(const bool isLeft)
    {
        const auto hand = isLeft ? Hand::Left : Hand::Right;
        return !VRControllers.isTouching(hand, vr::k_EButton_SteamVR_Trigger) &&
               (VRControllers.getAxisValue(hand, Axis::Grip).x > 0.01f || VRControllers.isTouching(hand, k_EButton_Grip)) &&
               (VRControllers.isTouching(hand, vr::k_EButton_A) || VRControllers.isTouching(hand, vr::k_EButton_ApplicationMenu));
    }

    /**
     * Detect the controller gesture that should temporarily map to thumbs-up.
     */
    bool HandPose::shouldUseThumbsUpPose(const bool isLeft)
    {
        const auto hand = isLeft ? Hand::Left : Hand::Right;
        return VRControllers.isTouching(hand, k_EButton_Grip) && VRControllers.isTouching(hand, vr::k_EButton_SteamVR_Trigger) &&
               !VRControllers.isTouching(hand, vr::k_EButton_SteamVR_Touchpad) && !VRControllers.isTouching(hand, vr::k_EButton_A) &&
               !VRControllers.isTouching(hand, vr::k_EButton_ApplicationMenu);
    }

    /**
     * Order overrides so the active one is at the front: highest priority first,
     * and within one priority the tag that registered most recently stays ahead.
     */
    void HandPose::sortHandOverrides(std::vector<TaggedHandPoseOverride>& overrides)
    {
        std::ranges::sort(overrides, [](const TaggedHandPoseOverride& lhs, const TaggedHandPoseOverride& rhs) {
            if (lhs.priority != rhs.priority) {
                return lhs.priority > rhs.priority;
            }
            return lhs.sequence > rhs.sequence;
        });
    }

    /**
     * Set, update, or promote one tagged explicit hand pose override.
     */
    void HandPose::setHandPoseOverrideIntr(const bool isLeft, const std::string_view tag, const HandFingersPose& pose, const int priority)
    {
        if (tag.empty()) {
            return;
        }

        auto& overrides = getHandOverrides(isLeft);
        const std::string previousTopTag = overrides.empty() ? "---" : overrides.front().tag;
        const auto overrideIt = std::ranges::find_if(overrides, [tag](const TaggedHandPoseOverride& overrideEntry) {
            return overrideEntry.tag == tag;
        });
        const bool wasInserted = overrideIt == overrides.end();

        TaggedHandPoseOverride updatedOverride = wasInserted ? TaggedHandPoseOverride{} : *overrideIt;
        updatedOverride.tag = std::string(tag);
        updatedOverride.pose = pose;
        updatedOverride.priority = priority;
        updatedOverride.localTransformMask = 0;
        updatedOverride.localTransforms = {};
        if (wasInserted) {
            // Sequence is the tiebreak between equal priorities and is assigned once,
            // on registration, so the newest registration takes a tie. Refreshing an
            // existing pose every frame must not re-stamp it and walk the tag back over
            // an equal-priority peer that registered after it.
            updatedOverride.sequence = ++_nextOverrideSequence;
            overrides.push_back(updatedOverride);
        } else {
            *overrideIt = updatedOverride;
        }

        sortHandOverrides(overrides);

        if (wasInserted || overrides.front().tag != previousTopTag) {
            logger::debug("Hand pose: {} override tag:'{}' priority:{} for '{}' hand (top tag:'{}' -> '{}', depth {})",
                wasInserted ? "Insert" : "Update",
                tag,
                priority,
                isLeft ? "Left" : "Right",
                previousTopTag,
                overrides.front().tag,
                overrides.size());
        }
    }

    /**
     * Clear one tagged explicit hand pose override.
     */
    void HandPose::clearHandPoseOverrideIntr(const bool isLeft, const std::string_view tag)
    {
        if (tag.empty()) {
            return;
        }

        auto& overrides = getHandOverrides(isLeft);
        const std::string previousTopTag = overrides.empty() ? "---" : overrides.front().tag;
        const auto overrideIt = std::ranges::find_if(overrides, [tag](const TaggedHandPoseOverride& overrideEntry) {
            return overrideEntry.tag == tag;
        });
        if (overrideIt == overrides.end()) {
            return;
        }

        overrides.erase(overrideIt);

        logger::debug("Hand pose: Clear override tag:'{}' for '{}' hand (top tag:'{}' -> '{}', depth {})",
            tag,
            isLeft ? "Left" : "Right",
            previousTopTag,
            overrides.empty() ? "---" : overrides.front().tag,
            overrides.size());
    }
}
