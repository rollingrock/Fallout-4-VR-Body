#pragma once

#include "utils.h"
#include "api/VRHookAPI.h"
#include "ui/UIContainer.h"

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
			: _adjuster(adjuster) {
			createConfigUI();
		}

		~WeaponPositionConfigMode();

		[[nodiscard]] bool isInOffhandRepositioning() const { return _repositionTarget == RepositionTarget::Offhand; }

		void onFrameUpdate(NiNode* weapon) const;

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
		void createConfigUI();

		// access the weapon/offhand transform to change
		WeaponPositionAdjuster* _adjuster;

		// what is currently being repositioned
		RepositionTarget _repositionTarget = RepositionTarget::Weapon;

		// configuration UI
		std::shared_ptr<ui::UIContainer> _configUI;
		std::shared_ptr<ui::UIContainer> _mainContainer;
		std::shared_ptr<ui::UIContainer> _noEquippedWeaponContainer;
		std::shared_ptr<ui::UIWidget> _footerForWeaponAdjust;
		std::shared_ptr<ui::UIWidget> _footerForOtherAdjust;
	};
}
