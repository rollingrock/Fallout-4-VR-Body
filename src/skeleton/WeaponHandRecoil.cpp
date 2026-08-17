#include "WeaponHandRecoil.h"

#include "Config.h"
#include "FRIK.h"
#include "api/RecoilControllerRuntime.h"
#include "common/CommonUtils.h"
#include "common/MatrixUtils.h"
#include "common/Quaternion.h"
#include "f4vr/F4VRUtils.h"
#include "utils.h"

using namespace common;
using namespace f4vr;

namespace
{
    /**
     * Compose a local transform onto a parent frame, the way the engine derives a
     * node's world transform from its parent's world and its own local.
     */
    RE::NiTransform composeWorldFromFrameLocal(const RE::NiTransform& frame, const RE::NiTransform& local)
    {
        RE::NiTransform world;
        world.rotate = local.rotate * frame.rotate;
        world.translate = frame.translate + frame.rotate.Transpose() * (local.translate * frame.scale);
        world.scale = frame.scale * local.scale;
        return world;
    }

    /**
     * Invert a transform, so that composing it back onto the original yields identity.
     * Used to move a frame out of one wand's space and to sandwich the kick into a
     * world-space delta. A degenerate scale falls back to 1 rather than dividing by it.
     */
    RE::NiTransform invertFrameTransform(const RE::NiTransform& transform)
    {
        const float safeScale = fEqual(transform.scale, 0.0f) ? 1.0f : transform.scale;
        RE::NiTransform inverse;
        inverse.rotate = transform.rotate.Transpose();
        inverse.translate = (transform.rotate * transform.translate) * (-1.0f / safeScale);
        inverse.scale = 1.0f / safeScale;
        return inverse;
    }

    /**
     * Reflect a transform across the body's sagittal plane, turning a relation authored
     * for one wand into the same relation on the other. The rotation is conjugated by the
     * x-flip so it stays a rotation, and only the lateral component of the offset flips.
     */
    RE::NiTransform mirrorRecoilLocalAcrossSagittal(const RE::NiTransform& local)
    {
        const RE::NiMatrix3 mx = MatrixUtils::getMatrix(-1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f);
        RE::NiTransform mirrored = local;
        mirrored.rotate = mx * local.rotate * mx;
        mirrored.translate = RE::NiPoint3(-local.translate.x, local.translate.y, local.translate.z);
        return mirrored;
    }
}

namespace frik
{
    /**
     * Zero the native kick node and push the change down its subtree, so the game's
     * first-person arm update inside the scope solves against an unkicked wand.
     *
     * Stays inert unless a controller owns the frame, since the native kick is the
     * only recoil the hands get otherwise.
     */
    WeaponHandRecoil::ScopedNativeKickNeutralizer::ScopedNativeKickNeutralizer(const WeaponHandRecoil& recoil)
        : _node(recoil._playerNodes ? recoil._playerNodes->primaryWeaponKickbackRecoilNode : nullptr)
    {
        if (!_node || !recoil._responseAccepted || !isFiniteTransform(_node->local)) {
            return;
        }

        _savedLocal = _node->local;
        _node->local.MakeIdentity();
        updateTransformsDown(_node, true);
        _active = true;
    }

    /**
     * Put the animated kick back, leaving the node exactly as the game left it.
     */
    WeaponHandRecoil::ScopedNativeKickNeutralizer::~ScopedNativeKickNeutralizer()
    {
        if (!_active) {
            return;
        }
        _node->local = _savedLocal;
        updateTransformsDown(_node, true);
    }

    /**
     * Sample this frame's native kick and resolve whatever a controller wants in its place.
     *
     * Must run before the arms are solved, since everything downstream reads the result.
     * physicalPrimaryIsLeft names the hand actually holding the weapon, which the response's
     * hand mask is keyed on; it diverges from the game's left-handed setting whenever an
     * external mod owns the primary weapon node.
     *
     * Every path that ends without damping drops the smoothing history, so a later damped
     * burst always eases out from rest instead of resuming a stale value.
     */
    void WeaponHandRecoil::onFrameUpdate(f4vr::PlayerNodes* playerNodes, const bool physicalPrimaryIsLeft)
    {
        _playerNodes = playerNodes;
        _physicalPrimaryIsLeft = physicalPrimaryIsLeft;

        _responseAccepted = false;
        _handMask = 0;
        _controlledKickLocal.MakeIdentity();
        _worldDeltaValid.fill(false);

        auto* const kickbackNode = _playerNodes ? _playerNodes->primaryWeaponKickbackRecoilNode : nullptr;
        if (!kickbackNode || !isFiniteTransform(kickbackNode->local) || !kickbackNode->parent || !isFiniteTransform(kickbackNode->parent->world)) {
            _smoothedValid = false;
            return;
        }

        api::FRIKApiV2::RecoilSample sample{};
        sample.structSize = sizeof(api::FRIKApiV2::RecoilSample);
        sample.nativeKickLocal = kickbackNode->local;

        const auto resolution = api::resolveWeaponHandRecoil(sample);
        if (!resolution.accepted) {
            _smoothedValid = false;
            return;
        }

        const auto& response = resolution.response;
        _responseAccepted = true;
        _handMask = response.handMask;
        if (response.delivery == api::FRIKApiV2::RecoilDelivery::Damped) {
            _controlledKickLocal = dampen(response.controlledKickLocal);
        } else {
            _controlledKickLocal = response.controlledKickLocal;
            _smoothedValid = false;
        }
    }

    /**
     * Offset an IK hand world target by this frame's controlled recoil.
     *
     * Silently leaves the target alone when recoil is uncontrolled or the controller did not
     * select this hand, so callers can apply it unconditionally to any hand target they are
     * about to solve. The world delta is built once per hand and reused for the rest of the frame.
     *
     * @return false only when the recoil could not be applied safely, in which case the target
     * is left unmodified and the hand solves without recoil rather than to a garbage pose.
     */
    bool WeaponHandRecoil::applyToHandWorldTarget(const bool isLeft, RE::NiTransform& target)
    {
        if (!_responseAccepted) {
            return true;
        }

        const auto selectedRole = static_cast<std::uint32_t>(isLeft == _physicalPrimaryIsLeft ? api::FRIKApiV2::RecoilHandMask::Primary : api::FRIKApiV2::RecoilHandMask::Offhand);
        if ((_handMask & selectedRole) == 0) {
            return true;
        }

        const std::size_t handIndex = isLeft ? 1u : 0u;
        if (!_worldDeltaValid[handIndex]) {
            if (!buildWorldDelta(isLeft, _worldDeltas[handIndex])) {
                return false;
            }
            _worldDeltaValid[handIndex] = true;
        }

        const RE::NiTransform controlledTarget = composeWorldFromFrameLocal(_worldDeltas[handIndex], target);
        if (!isFiniteTransform(controlledTarget)) {
            return false;
        }

        target = controlledTarget;
        return true;
    }

    /**
     * Turn the controlled kick into a world-space delta that can be applied to a hand target.
     *
     * The kick is authored as a local transform under the kick node's parent, so sandwiching it
     * between that parent's world and the parent's inverse re-expresses the same rigid motion in
     * world space, where it composes onto any hand target regardless of how that hand was solved.
     *
     * That parent sits under the game-primary wand, so for the hand the game treats as the offhand
     * the whole relation is first carried across to the other wand and mirrored. Note this tests
     * the game's notion of primary, unlike the response's hand mask which is keyed on the hand
     * physically holding the weapon - the two disagree while an external mod carries the weapon.
     *
     * @return false if any frame involved is missing or non-finite, which leaves the hand
     * without recoil for the frame.
     */
    bool WeaponHandRecoil::buildWorldDelta(const bool isLeft, RE::NiTransform& outWorldDelta) const
    {
        auto* const kickbackNode = _playerNodes ? _playerNodes->primaryWeaponKickbackRecoilNode : nullptr;
        const auto* const kickParent = kickbackNode ? kickbackNode->parent : nullptr;
        if (!kickParent || !isFiniteTransform(kickParent->world) || !isFiniteTransform(_controlledKickLocal)) {
            return false;
        }

        RE::NiTransform kickParentWorld = kickParent->world;
        RE::NiTransform controlledKickLocal = _controlledKickLocal;

        // The native kick is authored in the current game-primary wand frame.
        // Mirror that rigid relation only when the selected physical hand is
        // the current game-offhand (including ROCK's external-left carry).
        const bool nativePrimaryIsLeft = isLeftHandedMode();
        if (isLeft != nativePrimaryIsLeft) {
            auto* const nativePrimaryWand = _playerNodes->primaryWandNode;
            auto* const nativeOffhandWand = _playerNodes->SecondaryWandNode;
            if (!nativePrimaryWand || !nativeOffhandWand || !isFiniteTransform(nativePrimaryWand->world) || !isFiniteTransform(nativeOffhandWand->world)) {
                return false;
            }

            const RE::NiTransform kickParentInPrimaryWand = composeWorldFromFrameLocal(invertFrameTransform(nativePrimaryWand->world), kickParentWorld);
            kickParentWorld = composeWorldFromFrameLocal(nativeOffhandWand->world, mirrorRecoilLocalAcrossSagittal(kickParentInPrimaryWand));
            controlledKickLocal = mirrorRecoilLocalAcrossSagittal(controlledKickLocal);
        }

        outWorldDelta = composeWorldFromFrameLocal(kickParentWorld, composeWorldFromFrameLocal(controlledKickLocal, invertFrameTransform(kickParentWorld)));
        return isFiniteTransform(outWorldDelta);
    }

    /**
     * Ease the controlled kick in over several frames instead of snapping to it.
     *
     * Chases the requested kick from the previous frame's smoothed value, slerping the rotation
     * and lerping the translation, so a controller asking for damped delivery gets a softened
     * recoil rather than a single-frame jolt. Deliberately reuses the same dampenHands settings
     * as normal hand dampening, including the separate vanilla-scope pair, so recoil damping
     * tracks whatever the player already tuned for their hands.
     *
     * Returns the kick untouched when dampening is off, resetting the history so re-enabling it
     * does not blend out of a value from before.
     */
    RE::NiTransform WeaponHandRecoil::dampen(const RE::NiTransform& kick)
    {
        if (!g_config.dampenHands) {
            _smoothedValid = false;
            return kick;
        }
        const bool isInScopeMenu = g_frik.isInScopeMenu();
        if (isInScopeMenu && !g_config.dampenHandsInVanillaScope) {
            _smoothedValid = false;
            return kick;
        }
        if (!_smoothedValid) {
            _smoothedKickLocal.MakeIdentity();
            _smoothedValid = true;
        }
        const float rotationFactor = isInScopeMenu ? g_config.dampenHandsRotationInVanillaScope : g_config.dampenHandsRotation;
        const float translationFactor = isInScopeMenu ? g_config.dampenHandsTranslationInVanillaScope : g_config.dampenHandsTranslation;

        Quaternion smoothedRotation, targetRotation;
        smoothedRotation.fromMatrix(_smoothedKickLocal.rotate);
        targetRotation.fromMatrix(kick.rotate);
        smoothedRotation.slerp(1 - rotationFactor, targetRotation);

        RE::NiTransform smoothed = kick;
        smoothed.rotate = smoothedRotation.getMatrix();
        smoothed.translate = kick.translate - (kick.translate - _smoothedKickLocal.translate) * translationFactor;
        _smoothedKickLocal = smoothed;
        return smoothed;
    }
}
