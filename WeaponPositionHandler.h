#pragma once

#include "Skeleton.h"
#include "api/VRHookAPI.h"
#include "f4se/NiNodes.h"

namespace F4VRBody {

	enum RepositionMode {
		weapon = 0, // move weapon
		offhand, // move offhand grip position
		resetToDefault, // reset to default and exit reposition mode
		total = resetToDefault
	};

	class WeaponPositionHandler
	{
	public:
		WeaponPositionHandler(Skeleton* skelly, OpenVRHookManagerAPI* hook) {
			_skelly = skelly;
			_vrhook = hook;
		}

		inline bool inWeaponRepositionMode() const {
			return _inWeaponRepositionMode;
		}
		inline void toggleWeaponRepositionMode() {
			_inWeaponRepositionMode = !_inWeaponRepositionMode;
		}

		void onFrameUpdate();

	private:
		void handleWeapon();
		void offHandToBarrel();
		void offHandToScope();

		Skeleton* _skelly;
		OpenVRHookManagerAPI* _vrhook;

		bool _offHandGripping = false;
		bool _hasLetGoGripButton = false;

		bool _zoomModeButtonHeld = false;

		bool _useCustomWeaponOffset = false;
		bool _useCustomOffHandOffset = false;

		bool _inWeaponRepositionMode = false;

		bool _repositionButtonHolding = false;
		bool _inRepositionMode = false;
		bool _repositionModeSwitched = false;
		bool _hasLetGoRepositionButton = false;


		RepositionMode _repositionMode = RepositionMode::weapon;
		uint64_t _repositionButtonHoldStart = 0;
		
		NiMatrix43 _originalWeaponRot;

		NiPoint3 _startFingerBonePos = NiPoint3(0, 0, 0);
		NiPoint3 _endFingerBonePos = NiPoint3(0, 0, 0);
		NiPoint3 _offsetPreview = NiPoint3(0, 0, 0);

		NiTransform _customTransform;
		
		std::string _lastWeapon = "";

		NiTransform _offhandOffset; // Saving as NiTransform in case we need rotation in future

		NiPoint3 _offhandPos{ 0, 0, 0 };

		Quaternion _aimAdjust;
	};
}
