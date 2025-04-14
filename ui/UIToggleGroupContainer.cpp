#include "UIToggleGroupContainer.h"
#include "UIManager.h"

namespace ui {
	/**
	 * Add a toggle button and don't allow un-toggling it
	 */
	void UIToggleGroupContainer::addElement(const std::shared_ptr<UIToggleButton>& button) {
		button->setUnToggleAllowed(false);
		UIContainer::addElement(button);
	}

	/**
	 * On toggle of one button, un-toggle all other buttons.
	 */
	void UIToggleGroupContainer::onPressEventFired(UIElement* eventElement, UIModAdapter* adapter) {
		for (const auto& otherElement : _childElements) {
			const auto otherButton = dynamic_cast<UIToggleButton*>(otherElement.get());
			if (eventElement != otherButton) {
				otherButton->setToggleState(false);
			}
		}
	}
}
