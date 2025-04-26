#include "UIToggleGroupContainer.h"
#include "UIManager.h"

namespace ui {
	std::string UIToggleGroupContainer::toString() const {
		return std::format("UIToggleGroupContainer: {}, Pos({:.2f}, {:.2f}, {:.2f}), Size({:.2f}, {:.2f}), Children({}), Layout({})",
			_visible ? "V" : "H",
			_transform.pos.x,
			_transform.pos.y,
			_transform.pos.z,
			_size.width,
			_size.height,
			_childElements.size(),
			static_cast<int>(_layout)
		);
	}

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
	void UIToggleGroupContainer::onPressEventFired(UIElement* eventElement, UIFrameUpdateContext* context) {
		for (const auto& otherElement : _childElements) {
			const auto otherButton = dynamic_cast<UIToggleButton*>(otherElement.get());
			if (eventElement != otherButton) {
				otherButton->setToggleState(false);
			}
		}
	}
}
