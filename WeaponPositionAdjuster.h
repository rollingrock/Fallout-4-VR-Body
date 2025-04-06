#pragma once

#include "Skeleton.h"
#include "api/VRHookAPI.h"
#include "f4se/NiNodes.h"

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
			return _inRepositionConfigMode;
		}
		inline void toggleWeaponRepositionMode() {
			_inRepositionConfigMode = !_inRepositionConfigMode;
		}

		void onFrameUpdate();

	private:
		void checkEquippedWeaponChanged();
		void handleScopeCameraAdjustmentByWeaponOffset(NiNode* weapon);
		void handleWeaponRepositionConfigMode(NiNode* weapon);
		void checkIfOffhandIsGripping(NiNode* weapon);
		void handleWeaponGrippingRotationAdjustment(NiNode* weapon);
		bool isOffhandCloseToBarrel(NiNode* weapon);
		bool isOffhandMovedFastAway();
		NiPoint3 getOffhand2WeaponVector(NiNode* weapon);
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

		// configuration mode to adjust weapon offset, offhand offset, and ??
		bool _inRepositionConfigMode = false;

		NiTransform _weaponOriginalTransform;
		NiTransform _weaponOffsetTransform;
		NiTransform _offhandOffsetTransform;
	};
}
