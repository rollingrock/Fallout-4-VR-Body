#pragma once

#include "utils.h"
#include "api/VRHookAPI.h"

namespace F4VRBody {
	class WeaponPositionAdjuster;

	/**
	 * What is currently being configured
	 */
	enum class RepositionTarget : uint8_t {
		Weapon = 0,
		Offhand,
		BetterScopes,
	};

	/**
	 * Configuration mode to adjust weapon offset, offhand offset, and BetterScopesVR
	 */
	class WeaponPositionConfigMode {
	public :
		explicit WeaponPositionConfigMode(WeaponPositionAdjuster* adjuster)
			: _adjuster(adjuster) {}

		// Default small offset to use if no custom transform exist.
		static NiPoint3 getDefaultOffhandTransform() { return {0, 0, 2}; }

		[[nodiscard]] bool isInOffhandRepositioning() const { return _repositionTarget == RepositionTarget::Offhand; }

		void onFrameUpdate(NiNode* weapon);

	private:
		void handleReposition(NiNode* weapon) const;
		void handleWeaponReposition(NiNode* weapon) const;
		void handleOffhandReposition(NiNode* weapon) const;
		static void handleBetterScopesReposition();
		void resetConfig() const;
		void saveConfig() const;
		void resetWeaponConfig() const;
		void saveWeaponConfig() const;
		void resetOffhandConfig() const;
		void saveOffhandConfig() const;
		void resetBetterScopesConfig() const;
		void saveBetterScopesConfig() const;

		// access the weapon/offhand transform to change
		WeaponPositionAdjuster* _adjuster;

		// what is currently being repositioned
		RepositionTarget _repositionTarget = RepositionTarget::Weapon;
	};
}
