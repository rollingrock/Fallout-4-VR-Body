#pragma once
#include "f4vr/PapyrusGatewayBase.h"

namespace frik {
	/**
	 * FRIK Papyrus Gateway.
	 */
	class PapyrusGateway : public f4vr::PapyrusGatewayBase {
	public:
		explicit PapyrusGateway()
			: PapyrusGatewayBase("FRIK:FRIK") {}

		/**
		 * Enable/disable the player ability to move, fire weapon, use vats, etc.
		 * @param enable if true will enable everything, if false will disable specifics
		 * @param combat only when disabling, if true will disable combat controls (fire weapon)
		 * @param drawWeapon only when enabling, if true will draw equipped weapon
		 */
		void enableDisablePlayerControls(const bool enable, const bool combat, const bool drawWeapon) const {
			auto arguments = getArgs(enable, combat, drawWeapon);
			executePapyrusScript("EnableDisablePlayerControls", arguments);
		}

		/**
		 * Enable/disable that VATS feature in the game.
		 * Disabling will free the "B" button for general use.
		 */
		void enableDisableVats(const bool enable) const {
			auto arguments = getArgs(enable);
			executePapyrusScript("EnableDisableVats", arguments);
		}

		/**
		 * Draw the currently equipped weapon if any.
		 */
		void drawWeapon() const {
			executePapyrusScript("DrawWeapon");
		}

		/**
		 * Holster currently drawn weapon if any.
		 */
		void holsterWeapon() const {
			executePapyrusScript("HolsterWeapon");
		}

		/**
		 * Un-equip the currently equipped weapon.
		 * NOT the same as holstering (not available)
		 */
		void UnEquipCurrentWeapon(const bool enable) const {
			executePapyrusScript("UnEquipCurrentWeapon");
		}
	};
}
