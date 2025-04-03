#pragma once

#include "Skeleton.h"
#include "f4se/NiNodes.h"
#include "WeaponPositionConfigMode.h"

namespace F4VRBody {

	class WeaponPositionAdjuster
	{
	public:
		WeaponPositionAdjuster(Skeleton* skelly, OpenVRHookManagerAPI* hook) {
			_skelly = skelly;
			_vrhook = hook;

			Matrix44 ident = Matrix44();
			ident.data[2][0] = 1.0; // new X = old Z
			ident.data[0][1] = 1.0; // new Y = old X
			ident.data[1][2] = 1.0; // new Z = old Y
			ident.data[3][3] = 1.0;
			_scopeCameraBaseMatrix = ident.make43();
		}

		inline bool inWeaponRepositionMode() const {
			return _configMode.inWeaponRepositionMode();
		}
		inline void toggleWeaponRepositionMode() {
			_configMode.toggleWeaponRepositionMode();
		}

		void onFrameUpdate();

	private:
		void checkEquippedWeaponChanged();
		void handleScopeCameraAdjustmentByWeaponOffset(NiNode* weapon);
		void checkIfOffhandIsGripping(NiNode* weapon);
		void handleWeaponGrippingRotationAdjustment(NiNode* weapon);
		bool isOffhandCloseToBarrel(NiNode* weapon);
		bool isOffhandMovedFastAway();
		NiPoint3 getOffhandPosition(NiNode* weapon);
		void handleBetterScopes(NiNode* weapon);
		void debugPrintWeaponPositionData();

		// Define a basis remapping matrix to correct coordinate system for scope camera
		NiMatrix43 _scopeCameraBaseMatrix;

		Skeleton* _skelly;
		OpenVRHookManagerAPI* _vrhook;

		// used to know if weapon changed to load saved offsets
		std::string _lastWeapon = "";
		bool _lastWeaponInPA = false;

		// is offhand (secondary hand) gripping the weapon barrel
		bool _offHandGripping = false;
		
		// weapon original transform before changing it
		NiTransform _weaponOriginalTransform;
		
		// custom weapon transform to update
		NiTransform _weaponOffsetTransform;
		
		// custom offhand position offsets
		NiPoint3 _offhandOffsetPos;

		// configuration mode to update custom transforms
		WeaponPositionConfigMode _configMode;
	};
}
