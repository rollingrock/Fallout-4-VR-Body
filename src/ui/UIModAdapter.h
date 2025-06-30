#pragma once

#include <optional>

#include "../Debug.h"

namespace vrui {
	class UIModAdapter {
	public:
		/**
		 * Get the world position to be used for all UI interactions.
		 * Like knowing if a button is pressed.
		 */
		virtual RE::NiPoint3 getInteractionBoneWorldPosition() = 0;

		/**
		 * Fire heptic on the interaction controller.
		 * Used to indicate press handling.
		 */
		virtual void fireInteractionHeptic() = 0;

		/**
		 * Set the interaction hand to a pointing position for UI interaction where index finger is the interaction bone.
		 * @param primaryHand - true - use primary hand, false - use offhand
		 * @param toPoint true - force hand to point position, false - release
		 */
		virtual void setInteractionHandPointing(bool primaryHand, bool toPoint) = 0;

		virtual ~UIModAdapter() = default;
	};

	class UIFrameUpdateContext : public UIModAdapter {
	public:
		explicit UIFrameUpdateContext(UIModAdapter* adapter)
			: _adapter(adapter) {}

		const std::optional<bool>& isAnyPressableCloseToInteraction() const { return _isAnyPressableCloseToInteraction; }

		void markAnyPressableCloseToInteraction(const bool isPressableClose) {
			_isAnyPressableCloseToInteraction = _isAnyPressableCloseToInteraction.value_or(false) || isPressableClose;
		}

		virtual RE::NiPoint3 getInteractionBoneWorldPosition() override { return _adapter->getInteractionBoneWorldPosition(); }
		virtual void fireInteractionHeptic() override { _adapter->fireInteractionHeptic(); }
		virtual void setInteractionHandPointing(const bool primaryHand, const bool toPoint) override { _adapter->setInteractionHandPointing(primaryHand, toPoint); }

	private:
		UIModAdapter* _adapter;

		// Are any of the elements in current frame update close to the interaction bone and can be pressed?
		std::optional<bool> _isAnyPressableCloseToInteraction = std::nullopt;
	};
}
