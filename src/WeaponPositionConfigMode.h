#pragma once

#include "utils.h"
#include "api/VRHookAPI.h"
#include "ui/UIButton.h"
#include "ui/UIContainer.h"
#include "ui/UIToggleButton.h"

namespace frik {
	class WeaponPositionAdjuster;

	/**
	 * What is currently being configured
	 */
	enum class RepositionTarget : uint8_t {
		Weapon = 0,
		Offhand,
		Throwable,
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
		static NiTransform getThrowableWeaponDefaultAdjustment(const NiTransform& originalTransform, bool inPA);
		static NiTransform getBackOfHandUIDefaultAdjustment(const NiTransform& originalTransform, bool inPA);
		void showHideUIElements(bool weaponEquipped, bool throwableEquipped) const;

		bool isInOffhandRepositioning() const { return _repositionTarget == RepositionTarget::Offhand; }

		void onFrameUpdate(NiNode* weapon);

	private:
		void handleReposition(NiNode* weapon, NiNode* throwable) const;
		void handleWeaponReposition(NiNode* weapon) const;
		void handleOffhandReposition() const;
		void handleThrowableReposition(NiNode* throwable) const;
		void handleBackOfHandUIReposition() const;
		static void handleBetterScopesReposition();
		void resetConfig() const;
		void saveConfig() const;
		void resetWeaponConfig() const;
		void saveWeaponConfig() const;
		void resetOffhandConfig() const;
		void saveOffhandConfig() const;
		void resetThrowableConfig() const;
		void saveThrowableConfig() const;
		void resetBackOfHandUIConfig() const;
		void saveBackOfHandUIConfig() const;
		void resetBetterScopesConfig() const;
		void saveBetterScopesConfig() const;
		void createConfigUI();
		void doHaptic() const;

		// access the weapon/offhand transform to change
		WeaponPositionAdjuster* _adjuster;

		// what is currently being repositioned
		RepositionTarget _repositionTarget = RepositionTarget::Weapon;

		// configuration UI
		std::shared_ptr<vrui::UIContainer> _configUI;
		std::shared_ptr<vrui::UIWidget> _complexAdjustFooter;
		std::shared_ptr<vrui::UIWidget> _throwableAdjustFooter;
		std::shared_ptr<vrui::UIWidget> _simpleAdjustFooter;
		std::shared_ptr<vrui::UIToggleButton> _weaponModeButton;
		std::shared_ptr<vrui::UIToggleButton> _offhandModeButton;
		std::shared_ptr<vrui::UIToggleButton> _throwableUIButton;
		std::shared_ptr<vrui::UIWidget> _emptyHandsMessageBox;
		std::shared_ptr<vrui::UIToggleButton> _betterScopesModeButton;
		std::shared_ptr<vrui::UIButton> _saveButton;
		std::shared_ptr<vrui::UIButton> _resetButton;
		std::shared_ptr<vrui::UIWidget> _throwableNotEquippedMessageBox;
	};
}
