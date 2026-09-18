#pragma once

#include "GripDetection.h"
#include "WeaponPositionConfigMode.h"
#include "f4vr/EquippedWeaponHandler.h"
#include "skeleton/Skeleton.h"

namespace frik
{
    class WeaponPositionAdjuster
    {
        // To simplify changing offsets during configuration
        friend class WeaponPositionConfigMode;

    public:
        explicit WeaponPositionAdjuster(Skeleton* skelly)
        {
            _skelly = skelly;

            _scopeCameraBaseMatrix = RE::NiMatrix3();
            _scopeCameraBaseMatrix.entry[2][0] = 1.0; // new X = old Z
            _scopeCameraBaseMatrix.entry[0][1] = 1.0; // new Y = old X
            _scopeCameraBaseMatrix.entry[1][2] = 1.0; // new Z = old Y

            // the grip memory outlives the skeleton (cell load, PA): re-check the cone and re-apply the grip pose once we run
            _grip.revalidatePending = _grip.gripping;
            _gripPoseRestorePending = _grip.gripping;
        }

        // the scope rig must not stay under a weapon node that is about to go with the skeleton
        ~WeaponPositionAdjuster()
        {
            restoreScopeRig();
        }

        WeaponPositionAdjuster(const WeaponPositionAdjuster&) = delete;
        WeaponPositionAdjuster& operator=(const WeaponPositionAdjuster&) = delete;

        bool isWeaponDrawn() const
        {
            return _equippedWeapon.isDrawn();
        }

        bool isMeleeWeaponDrawn() const
        {
            return _equippedWeapon.isMelee();
        }

        bool isOffHandGrippingWeapon() const
        {
            return _grip.gripping;
        }

        bool inWeaponRepositionMode() const
        {
            return _configMode != nullptr;
        }

        bool inThrowableWeaponRepositionMode() const
        {
            return _configMode != nullptr && _configMode->isInThrowableWeaponRepositionMode();
        }

        static bool isOffHandGrippingEnabled();
        static void setOffHandGrippingEnabled(bool enabled);
        void toggleWeaponRepositionMode();
        void resetOnDisable();

        void onFrameStart();
        void onFrameUpdate();
        void loadStoredOffsets();

    private:
        void handleThrowableWeapon();
        void handlePrimaryWeapon();
        void checkEquippedWeaponChanged();
        void carryScopeRigWithWeapon();
        void restoreScopeRig();
        static bool reparent(RE::NiNode* node, RE::NiNode* newParent, const RE::NiTransform* local);
        void handleScopeCameraAdjustmentByWeaponOffset(const RE::NiNode* weapon) const;
        void alignScopeCameraToWeapon(const RE::NiNode* weapon) const;
        void checkIfOffhandIsGripping(const RE::NiNode* weapon);
        void setOffhandGripping(bool isGripping);
        void handlePrimaryHandGripOffsetAdjustment(const RE::NiNode* weapon) const;
        void handleWeaponGrippingRotationAdjustment(RE::NiNode* weapon) const;
        bool isOffhandCloseToBarrel(const RE::NiNode* weapon, bool exitCone = false) const;
        static bool isOffhandMovedFastAway();
        RE::NiPoint3 getPrimaryHandPosition() const;
        static RE::NiPoint3 getOffhandPosition();
        static void handleBetterScopes(RE::NiNode* weapon);
        static void fixMuzzleFlashPosition();
        static RE::NiNode* getBackOfHandUINode();
        void debugPrintWeaponPositionData(RE::NiNode* weapon);

        // Define a basis remapping matrix to correct coordinate system for scope camera
        RE::NiMatrix3 _scopeCameraBaseMatrix;

        // the scope rig (ScopeParent, scope camera) is parented under the weapon for an external carry; the camera base re-expressed for it
        bool _scopeRigCarried = false;
        RE::NiMatrix3 _scopeCameraCarryBaseMatrix;
        // the rig nodes' locals on the wand chain as last seen before a carry (the engine's, or a scope mod's last write), put back on release
        bool _scopeRigRestValid = false;
        RE::NiTransform _scopeParentRestLocal;
        RE::NiTransform _scopeCameraRestLocal;

        Skeleton* _skelly;

        // detects equipped-weapon / power-armor changes and resolves the weapon name; the single
        // source of truth for the current weapon name, power-armor state, and melee state
        f4vr::EquippedWeaponHandler _equippedWeapon;

        // is offhand (secondary hand) gripping the weapon barrel, with its memory across hidden weapons and button let-go (GripDetection.h);
        // static so a skeleton rebuild (cell load) keeps the grip (#142)
        inline static grip::GripLatch _grip;

        // last drawn weapon, so a hidden-and-back weapon is told apart from a real weapon change
        inline static std::string _lastDrawnWeaponName;

        // the hand pose overrides were dropped with the old skeleton; put the grip pose back on the first frame
        bool _gripPoseRestorePending = false;

        // last frame's external primary-weapon-node ownership block, to release transient state as it engages
        bool _nodeOwnershipBlockedLastFrame = false;

        // allow to disable offhand gripping feature without disabling the whole mod features, for external mods to control it (static to persist over recreation)
        inline static bool _offHandGrippingEnabled = true;

        // weapon original transform before changing it
        RE::NiTransform _weaponOriginalTransform = RE::NiTransform();
        RE::NiTransform _weaponOriginalWorldTransform = RE::NiTransform();

        // custom weapon transform to update
        RE::NiTransform _weaponOffsetTransform = RE::NiTransform();

        // custom primary hand rotation offsets matrix
        RE::NiMatrix3 _primaryHandOffsetRot = RE::NiMatrix3();
        bool _hasPrimaryHandOffset = false;

        // custom offhand rotation offsets matrix
        RE::NiMatrix3 _offhandOffsetRot = RE::NiMatrix3();

        // custom throwable weapon transform to update
        RE::NiTransform _throwableWeaponOriginalTransform = RE::NiTransform();
        RE::NiTransform _throwableWeaponOffsetTransform = RE::NiTransform();

        // custom back of hand UI transform to update
        RE::NiTransform _backOfHandUIOffsetTransform = RE::NiTransform();

        // configuration mode to update custom transforms
        std::unique_ptr<WeaponPositionConfigMode> _configMode;
    };
}
