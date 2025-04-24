#pragma once

#include "f4se/NiNodes.h"

namespace ui {
	class UIModAdapter {
	public:
		/**
		 * @return true if the game is in left-handed mode
		 */
		virtual bool isLeftHandedMode() = 0;

		/**
		 * Get the world position to be used for all UI interactions.
		 * Like knowing if a button is pressed.
		 */
		virtual NiPoint3 getInteractionBonePosition() = 0;


		/**
		 * Fire heptic on the interaction controller.
		 * Used to indicate press handling.
		 */
		virtual void fireInteractionHeptic() = 0;

		virtual ~UIModAdapter() = default;
	};
}
