#pragma once

#include "Skeleton.h"
#include "WeaponPositionConfigMode.h"

namespace frik
{
    class WeaponPositionAdjuster
    {
        // To simplify changing offsets during configuration
        friend class WeaponPositionConfigMode;

        // use as weapon name when no weapon in equipped. specifically for default back of hand UI offset.
        static constexpr auto EMPTY_HAND = "EmptyHand";

    public:
        explicit WeaponPositionAdjuster(Skeleton* skelly)
        {
            _skelly = skelly;

            _scopeCameraBaseMatrix = RE::NiMatrix3();
            _scopeCameraBaseMatrix.entry[2][0] = 1.0; // new X = old Z
            _scopeCameraBaseMatrix.entry[0][1] = 1.0; // new Y = old X
            _scopeCameraBaseMatrix.entry[1][2] = 1.0; // new Z = old Y

            // the angle was calculated by looking at the weapon in hand, seems to work for all weapons
            _twoHandedPrimaryHandManualAdjustment = common::getMatrixFromEulerAngles(0, common::degreesToRads(-6), common::degreesToRads(7));
        }

        bool isWeaponDrawn() const { return _currentWeapon != EMPTY_HAND; }
        bool inWeaponRepositionMode() const { return _configMode != nullptr; }

        void toggleWeaponRepositionMode();

        void onFrameUpdate();
        void loadStoredOffsets(const std::string& weaponName);

    private:
        void handleThrowableWeapon();
        void handlePrimaryWeapon();
        void checkEquippedWeaponChanged(RE::NiNode* weapon, bool emptyHand);
        void handleScopeCameraAdjustmentByWeaponOffset(const RE::NiNode* weapon) const;
        void checkIfOffhandIsGripping(const RE::NiNode* weapon);
        void setOffhandGripping(bool isGripping);
        void handleWeaponGrippingRotationAdjustment(RE::NiNode* weapon) const;
        void handleWeaponScopeCameraGrippingRotationAdjustment(const RE::NiNode* weapon, common::Quaternion rotAdjust, RE::NiPoint3 adjustedWeaponVec) const;
        bool isOffhandCloseToBarrel(const RE::NiNode* weapon) const;
        bool isOffhandMovedFastAway() const;
        RE::NiPoint3 getPrimaryHandPosition() const;
        RE::NiPoint3 getOffhandPosition() const;
        void handleSpecialWeaponFix(RE::NiNode* weapon) const;
        void handleBetterScopes(RE::NiNode* weapon) const;
        static void fixMuzzleFlashPosition();
        static RE::NiNode* getBackOfHandUINode();
        void debugPrintWeaponPositionData(RE::NiNode* weapon) const;

        // Define a basis remapping matrix to correct coordinate system for scope camera
        RE::NiMatrix3 _scopeCameraBaseMatrix;

        // For unknown reason my primary hand calculation is off by specific angle
        RE::NiMatrix3 _twoHandedPrimaryHandManualAdjustment;

        Skeleton* _skelly;

        // used to know if weapon changed to load saved offsets
        std::string _currentWeapon;
        bool _currentlyInPA = false;

        // is offhand (secondary hand) gripping the weapon barrel
        bool _offHandGripping = false;

        // weapon original transform before changing it
        RE::NiTransform _weaponOriginalTransform = RE::NiTransform();
        RE::NiTransform _weaponOriginalWorldTransform = RE::NiTransform();

        // custom weapon transform to update
        RE::NiTransform _weaponOffsetTransform = RE::NiTransform();

        // custom offhand rotation offsets matrix
        RE::NiMatrix3 _offhandOffsetRot = RE::NiMatrix3();

        // custom throwable weapon transform to update
        RE::NiTransform _throwableWeaponOriginalTransform = RE::NiTransform();
        RE::NiTransform _throwableWeaponOffsetTransform = RE::NiTransform();
        std::string _currentThrowableWeaponName;

        // custom back of hand UI transform to update
        RE::NiTransform _backOfHandUIOffsetTransform = RE::NiTransform();

        // configuration mode to update custom transforms
        std::unique_ptr<WeaponPositionConfigMode> _configMode;
    };
}
