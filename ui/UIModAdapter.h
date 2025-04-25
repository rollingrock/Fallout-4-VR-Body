#pragma once

namespace ui {
	class UIModAdapter {
	public:
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
