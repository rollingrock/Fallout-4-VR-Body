#include "UIButton.h"

#include <format>

namespace vrui {
	std::string UIButton::toString() const {
		return std::format("UIButton({}): {}{}, Pos({:.2f}, {:.2f}, {:.2f}), Size({:.2f}, {:.2f})",
			_node->name.c_str(),
			_visible ? "V" : "H",
			isPressable() ? "P" : ".",
			_transform.translate.x,
			_transform.translate.y,
			_transform.translate.z,
			_size.width,
			_size.height
		);
	}

	void UIButton::onPressEventFired(UIElement* element, UIFrameUpdateContext* context) {
		UIWidget::onPressEventFired(element, context);
		if (_onPressEventHandler) {
			_onPressEventHandler(this);
		}
	}
}
