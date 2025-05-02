#include "UIButton.h"

namespace ui {
	std::string UIButton::toString() const {
		return std::format("UIButton({}): {}{}, Pos({:.2f}, {:.2f}, {:.2f}), Size({:.2f}, {:.2f})",
			_node->m_name.c_str(),
			_visible ? "V" : "H",
			isPressable() ? "P" : ".",
			_transform.pos.x,
			_transform.pos.y,
			_transform.pos.z,
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
