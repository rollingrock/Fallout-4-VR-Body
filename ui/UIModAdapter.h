#pragma once

#include "f4se/NiNodes.h"

namespace ui {
	class UIModAdapter {
	public:
		/// <summary>
		/// Get the world position to be used for all UI interactions.
		/// Like knowing if a button is pressed.
		/// </summary>
		virtual NiPoint3 getInteractionBonePosition() = 0;

		/// <summary>
		/// Fire heptic on the interaction controller.
		/// Used to indicate press handling.
		/// </summary>
		virtual void fireInteractionHeptic() = 0;

		virtual ~UIModAdapter() = default;
	};
}
