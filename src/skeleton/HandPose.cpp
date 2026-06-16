#include "HandPose.h"

#include <algorithm>
#include <optional>
#include <string>
#include <string_view>

#include "Config.h"
#include "FRIK.h"
#include "HandPoseData.h"
#include "common/MatrixUtils.h"
#include "common/PerfMonitor.h"
#include "common/Quaternion.h"
#include "f4vr/BSFlattenedBoneTree.h"
#include "f4vr/F4VRSkelly.h"
#include "f4vr/F4VRUtils.h"
#include "vrcf/VRControllersManager.h"

using namespace common;
using namespace f4vr;
using namespace vrcf;
using namespace frik::skeleton::data;

namespace
{
    /**
     * Copy the authored 3x4 rotation rows from pose data into a runtime transform.
     */
    void copyRotationIntoTransform(const RotationData& rotationData, RE::NiTransform& transform)
    {
        for (int row = 0; row < 3; row++) {
            for (int col = 0; col < 4; col++) {
                transform.rotate.entry[row][col] = rotationData[row * 4 + col];
            }
        }
    }

    /**
     * Return whether a hand bone belongs to the left hand based on its name prefix.
     */
    bool isLeftHandBone(const std::string& boneName)
    {
        return boneName[0] == 'L';
    }

    /**
     * Convert a finger bone name like Finger31 into a zero-based finger index.
     */
    int boneToFingerIndex(const std::string& bone)
    {
        return bone[bone.size() - 2] - '1';
    }

    /**
     * Map a (non-thumb) finger to the controller button that should drive its dynamic curl.
     * The index finger follows the trigger; the bottom three fingers (middle, ring, pinky) follow the grip.
     */
    VRButtonId getTrackedButton(const std::string& bone)
    {
        return boneToFingerIndex(bone) == 1 ? k_EButton_SteamVR_Trigger : k_EButton_Grip;
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
     * Build the authored open/closed reference transforms used to drive hand posing.
     */
    HandPose::HandPose(const bool inPowerArmor)
    {
        _handClosed.clear();
        _handOpen.clear();
        _handBones.clear();
        _leftHandOverrides.clear();
        _rightHandOverrides.clear();

        for (const auto& boneData : getHandBoneData()) {
            copyRotationIntoTransform(boneData.closedRotation, _handClosed[boneData.boneName]);
            copyRotationIntoTransform(boneData.openRotation, _handOpen[boneData.boneName]);
            _handOpen[boneData.boneName].translate = inPowerArmor ? boneData.openTranslationInPowerArmor : boneData.openTranslation;
        }

        _handBones = _handOpen;
    }

    /**
     * Activate an explicit pose override for one hand.
     */
    void HandPose::setHandPoseOverride(const bool isLeft, const std::string_view tag, const HandFingersPose& pose, const bool forceTop = false)
    {
        setHandPoseOverrideIntr(isLeft, tag, pose, forceTop);
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
        const auto overrideIt = std::ranges::find_if(overrides, [tag](const TaggedHandPoseOverride& overrideEntry) { return overrideEntry.tag == tag; });
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
            return HandPoseKind::HoldingWeapon;
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
        return isUnarmedWeaponEquipped() ? getFistPose() : (g_frik.isMeleeWeaponDrawn() ? getMeleeGripPose() : getGunGripPose());
    }

    /**
     * Force the Pip-Boy interaction hand into the pointing pose.
     */
    void HandPose::setPipboyHandPose()
    {
        setHandPoseOverrideIntr(g_config.leftHandedPipBoy, PIPBOY_HAND_POSE_TAG, getPointingPose(), true);
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
            setHandPoseOverrideIntr(primaryHand == isLeftHandedMode(), FORCE_POINTING_HAND_POSE_TAG, getPointingPose(), true);
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
            setHandPoseOverrideIntr(!isLeftHandedMode(), OFFHAND_GRIP_HAND_POSE_TAG, getOffhandWeaponGripPose(), true);
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
            setHandPoseOverrideIntr(true, ATTABOY_HAND_POSE_TAG, getAttaboyPose(), true);
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

            const bool leftHandBone = isLeftHandBone(boneName);
            const auto& source = leftHandBone ? leftHandSource : rightHandSource;
            if (source.kind == HandPoseSourceKind::PrimaryWeaponPose) {
                applyPrimaryWeaponHandPose(boneName, source);
            } else if (source.kind == HandPoseSourceKind::OverridePose) {
                applyOverrideHandPose(boneName, source.pose, frameTime);
            } else {
                applyDynamicHandPose(boneName, frameTime);
            }

            rt->transforms[pos].local.rotate = handBoneIt->second.rotate;
            rt->transforms[pos].local.translate = _handOpen.at(boneName).translate;
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
     * 2. explicit hand override, then implicit thumbs-up
     * 3. dynamic controller-driven curl
     *
     * The returned source may intentionally have `pose == nullptr` when the active source is the
     * right-handed primary weapon path. In that case, finger bones should copy the first-person hand
     * transform instead of using authored pose data, and the palm prepass should do nothing.
     */
    HandPose::HandPoseSource HandPose::resolveHandPoseSource(const bool isLeft)
    {
        const bool shouldUseWeaponPoseForPrimaryHand = IsWeaponDrawn() && (isLeftHandedMode() || !g_frik.isPipboyOperatingWithFinger());

        if (shouldUseWeaponPoseForPrimaryHand && isLeftHandedMode() && isUnarmedWeaponEquipped()) {
            // Left-handed unarmed is a special authored fist case that applies to both hands.
            return HandPoseSource{ .kind = HandPoseSourceKind::PrimaryWeaponPose, .pose = &getFistPose() };
        }

        const bool isPrimaryHand = isLeft == isLeftHandedMode();
        if (isPrimaryHand && shouldUseWeaponPoseForPrimaryHand) {
            // Right-handed mode can copy the first-person hand transform directly. Left-handed mode
            // cannot, so it uses the fixed authored weapon pose instead.
            return HandPoseSource{ .kind = HandPoseSourceKind::PrimaryWeaponPose, .pose = isLeftHandedMode() ? &HandPose::getFixedPrimaryWeaponPose() : nullptr };
        }

        if (const auto* activeOverride = getActiveHandPoseOverride(isLeft)) {
            // Explicit mod/API override wins over gesture-driven posing.
            return HandPoseSource{ .kind = HandPoseSourceKind::OverridePose, .pose = &activeOverride->pose };
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

        if (blendState.pitch == 0.0f && blendState.yaw == 0.0f) {
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
            _handBones[boneName].rotate = getPoseBoneRotation(boneName, *source.pose);
        } else {
            const auto fpTree = getFirstPersonBoneTree();
            const int pos = fpTree->GetBoneIndex(boneName);
            if (pos >= 0) {
                _handBones[boneName] = fpTree->transforms[pos].refNode ? fpTree->transforms[pos].refNode->local : fpTree->transforms[pos].local;
            }
        }
    }

    /**
     * Apply hand pose by the current active hand pose override from external source.
     */
    void HandPose::applyOverrideHandPose(const std::string& boneName, const HandFingersPose* const activePose, const float frameTime)
    {
        const auto targetRotation = getPoseBoneRotation(boneName, *activePose);
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
        const auto boneHand = isLeftHandBone(boneName) ? Hand::Left : Hand::Right;

        float curl = 0.0f; // 0 = open, 1 = fully closed
        float splay = 0.0f;
        if (boneToFingerIndex(boneName) == 0) {
            const auto thumb = resolveDynamicThumbCurl(boneHand);
            curl = thumb.curl;
            splay = boneName.back() == '1' ? thumb.splay : 0.0f;
        } else {
            constexpr float DYNAMIC_CURL_ON_TOUCH = 0.35f;
            const auto button = getTrackedButton(boneName);
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

        blendBoneTowardRotation(boneName, blendBoneRotation(boneName, 1.0f - curl, splay), frameTime);
    }

    /**
     * Resolve the target rotation for one bone from a full hand pose definition.
     */
    RE::NiMatrix3 HandPose::getPoseBoneRotation(const std::string& boneName, const HandFingersPose& pose) const
    {
        const int fingerIndex = boneToFingerIndex(boneName);
        const int boneToFlexIndex = fingerIndex * 3 + (boneName.back() - '1');
        const float flex = std::clamp(pose.getFlexAt(boneToFlexIndex), -1.0f, 2.0f);
        const float splay = boneName.back() == '1' ? pose.getFingerAt(fingerIndex).splay : 0.0f;
        return blendBoneRotation(boneName, flex, splay);
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
     * Blend between the authored closed/open rotations and optionally apply proximal splay.
     */
    RE::NiMatrix3 HandPose::blendBoneRotation(const std::string& boneName, const float flex, const float splay) const
    {
        Quaternion qOpen, qClosed;
        qOpen.fromMatrix(_handOpen.at(boneName).rotate);
        qClosed.fromMatrix(_handClosed.at(boneName).rotate);
        qClosed.slerp(flex, qOpen);
        if (splay == 0.0f) {
            return qClosed.getMatrix();
        }
        const float sign = isLeftHandBone(boneName) ? -1.0f : 1.0f;
        return MatrixUtils::getMatrixFromEulerAngles(0, sign * splay, 0) * qClosed.getMatrix();
    }

    /**
     * Return the current override list for one hand, ordered from oldest to newest.
     */
    std::vector<HandPose::TaggedHandPoseOverride>& HandPose::getHandOverrides(const bool isLeft)
    {
        return isLeft ? _leftHandOverrides : _rightHandOverrides;
    }

    /**
     * Return the newest explicit override for one hand, if any.
     */
    const HandPose::TaggedHandPoseOverride* HandPose::getActiveHandPoseOverride(const bool isLeft)
    {
        const auto& overrides = getHandOverrides(isLeft);
        return overrides.empty() ? nullptr : &overrides.back();
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
     * Set, update, or promote one tagged explicit hand pose override.
     */
    void HandPose::setHandPoseOverrideIntr(const bool isLeft, const std::string_view tag, const HandFingersPose& pose, const bool forceTop)
    {
        if (tag.empty()) {
            return;
        }

        auto& overrides = getHandOverrides(isLeft);
        const auto previousTopTag = overrides.empty() ? "---" : overrides.back().tag;
        const auto overrideIt = std::ranges::find_if(overrides, [tag](const TaggedHandPoseOverride& overrideEntry) { return overrideEntry.tag == tag; });

        if (overrideIt == overrides.end()) {
            overrides.push_back(TaggedHandPoseOverride{ .tag = std::string(tag), .pose = pose });

            logger::info("Hand pose: Insert top override tag:'{}' for '{}' hand (previous top tag:'{}', depth {})",
                tag,
                isLeft ? "Left" : "Right",
                previousTopTag,
                overrides.size());
        } else if (forceTop && std::next(overrideIt) != overrides.end()) {
            auto updatedOverride = *overrideIt;
            updatedOverride.pose = pose;
            overrides.erase(overrideIt);
            overrides.push_back(std::move(updatedOverride));

            logger::info("Hand pose: Forced to top override tag:'{}' for '{}' hand (previous top tag:'{}', depth {})",
                tag,
                isLeft ? "Left" : "Right",
                previousTopTag,
                overrides.size());
        } else {
            overrideIt->pose = pose;
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
        const auto overrideIt = std::ranges::find_if(overrides, [tag](const TaggedHandPoseOverride& overrideEntry) { return overrideEntry.tag == tag; });
        if (overrideIt == overrides.end()) {
            return;
        }

        const bool wasTop = std::next(overrideIt) == overrides.end();

        overrides.erase(overrideIt);

        logger::info("Hand pose: Cleared override tag:'{}' for '{}' hand (was top '{}', new top tag:'{}', remaining {})",
            tag,
            isLeft ? "Left" : "Right",
            wasTop ? "yes" : "no",
            overrides.empty() ? "---" : overrides.back().tag,
            overrides.size());
    }
}
