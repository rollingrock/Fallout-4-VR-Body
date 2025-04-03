#pragma once

#include "utils.h"
#include "api/VRHookAPI.h"
#include "f4se/NiNodes.h"

namespace F4VRBody {

	enum RepostionTarget {
		Weapon = 0,
		Offhand,
		BetterScopes,
	};

	// configuration mode to adjust weapon offset, offhand offset, and BetterScopesVR
	class WeaponPositionConfigMode
	{
	public:
		inline bool inWeaponRepositionMode() const {
			return _isInRepositionConfigMode;
		}
		inline void toggleWeaponRepositionMode() {
			_isInRepositionConfigMode = !_isInRepositionConfigMode;
			rotationStickEnabledToggle(!_isInRepositionConfigMode);
			if (_isInRepositionConfigMode) {
				_repositionTarget = RepostionTarget::Weapon;
			}
		}
		inline bool isInOffhandRepositioning() const {
			return _isInRepositionConfigMode && _repositionTarget == RepostionTarget::Offhand;
		}

		NiPoint3 getDefaultOffhandTransform();
		void onFrameUpdate();
		void handleWeaponReposition(OpenVRHookManagerAPI* vrhook, NiNode* weapon, NiTransform& offsetTransform, const NiTransform& originalTransform, std::string weaponName, bool isInPA);
		void handleOffhandReposition(OpenVRHookManagerAPI* vrhook, NiNode* weapon, NiPoint3& offsetPos, std::string weaponName, bool isInPA);
		void handleBetterScopesConfig(OpenVRHookManagerAPI* vrhook) const;


	private:
		// is in configuration mode
		bool _isInRepositionConfigMode = false;

		// what is corrently being repositioned
		RepostionTarget _repositionTarget = RepostionTarget::Weapon;
	};
}
