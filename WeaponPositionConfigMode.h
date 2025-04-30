#pragma once

#include "utils.h"
#include "api/VRHookAPI.h"
#include "ui/UIButton.h"
#include "ui/UIContainer.h"
#include "ui/UIToggleButton.h"

namespace F4VRBody {
	class WeaponPositionAdjuster;

	/**
	 * What is currently being configured
	 */
	enum class RepositionTarget : uint8_t {
		Weapon = 0,
		Offhand,
		BackOfHandUI,
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

		static NiTransform getMeleeWeaponDefaultAdjustment(const NiTransform& originalTransform);
		static NiTransform getBackOfHandUIDefaultAdjustment(const NiTransform& originalTransform, bool inPA);

		[[nodiscard]] bool isInOffhandRepositioning() const { return _repositionTarget == RepositionTarget::Offhand; }

		void onFrameUpdate(NiNode* weapon) const;

	private:
		void handleReposition(NiNode* weapon) const;
		void handleTransformRepositionByControllersInput(NiTransform& transform) const;
		void handleWeaponReposition(NiNode* weapon) const;
		void handleOffhandReposition(NiNode* weapon) const;
		void handleBackOfHandUIReposition() const;
		static void handleBetterScopesReposition();
		void resetConfig() const;
		void saveConfig() const;
		void resetWeaponConfig() const;
		void saveWeaponConfig() const;
		void resetOffhandConfig() const;
		void saveOffhandConfig() const;
		void resetBackOfHandUIConfig() const;
		void doHaptic() const;
		void saveBackOfHandUIConfig() const;
		void resetBetterScopesConfig() const;
		void saveBetterScopesConfig() const;
		void createConfigUI();

		// access the weapon/offhand transform to change
		WeaponPositionAdjuster* _adjuster;

		// what is currently being repositioned
		RepositionTarget _repositionTarget = RepositionTarget::Weapon;

		// configuration UI
		std::shared_ptr<ui::UIContainer> _configUI;
		std::shared_ptr<ui::UIWidget> _complexAdjustFooter;
		std::shared_ptr<ui::UIWidget> _simpleAdjustFooter;
		std::shared_ptr<ui::UIToggleButton> _weaponModeButton;
		std::shared_ptr<ui::UIToggleButton> _offhandModeButton;
		std::shared_ptr<ui::UIWidget> _emptyHandsMessageBox;
		std::shared_ptr<ui::UIToggleButton> _betterScopesModeButton;
		std::shared_ptr<ui::UIButton> _saveButton;
		std::shared_ptr<ui::UIButton> _resetButton;
	};
}
