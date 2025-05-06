#pragma once

#include "Skeleton.h"
#include "WeaponPositionConfigMode.h"

namespace frik {
	class WeaponPositionAdjuster {
		// To simplify changing offsets during configuration
		friend class WeaponPositionConfigMode;

	public:
		WeaponPositionAdjuster(Skeleton* skelly, OpenVRHookManagerAPI* hook) {
			_skelly = skelly;
			_vrHook = hook;

			auto ident = common::Matrix44();
			ident.data[2][0] = 1.0; // new X = old Z
			ident.data[0][1] = 1.0; // new Y = old X
			ident.data[1][2] = 1.0; // new Z = old Y
			ident.data[3][3] = 1.0;
			_scopeCameraBaseMatrix = ident.make43();
		}

		bool inWeaponRepositionMode() const { return _configMode != nullptr; }

		void toggleWeaponRepositionMode();

		void onFrameUpdate();
		void loadStoredOffsets(const std::string& weaponName);

	private:
		void handleThrowableWeapon();
		void handlePrimaryWeapon();
		void checkEquippedWeaponChanged(bool emptyHand);
		void handleScopeCameraAdjustmentByWeaponOffset(const NiNode* weapon) const;
		void checkIfOffhandIsGripping(const NiNode* weapon);
		void handleWeaponGrippingRotationAdjustment(NiNode* weapon) const;
		bool isOffhandCloseToBarrel(const NiNode* weapon) const;
		bool isOffhandMovedFastAway() const;
		NiPoint3 getOffhandPosition() const;
		void handleBetterScopes(NiNode* weapon) const;
		NiNode* getBackOfHandUINode() const;
		void debugPrintWeaponPositionData(NiNode* weapon) const;

		// Define a basis remapping matrix to correct coordinate system for scope camera
		NiMatrix43 _scopeCameraBaseMatrix;

		Skeleton* _skelly;
		OpenVRHookManagerAPI* _vrHook;

		// used to know if weapon changed to load saved offsets
		std::string _currentWeapon;
		bool _currentlyInPA = false;

		// is offhand (secondary hand) gripping the weapon barrel
		bool _offHandGripping = false;

		// weapon original transform before changing it
		NiTransform _weaponOriginalTransform = NiTransform();

		// custom weapon transform to update
		NiTransform _weaponOffsetTransform = NiTransform();

		// custom offhand rotation offsets matrix
		NiMatrix43 _offhandOffsetRot = NiMatrix43();

		// custom throwable weapon transform to update
		NiTransform _throwableWeaponOriginalTransform = NiTransform();
		NiTransform _throwableWeaponOffsetTransform = NiTransform();
		std::string _currentThrowableWeaponName;

		// custom back of hand UI transform to update
		NiTransform _backOfHandUIOffsetTransform = NiTransform();

		// configuration mode to update custom transforms
		std::unique_ptr<WeaponPositionConfigMode> _configMode;
	};
}
