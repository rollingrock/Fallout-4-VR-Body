#include "UIButton.h"

namespace ui {
	void UIButton::onPressEventFired(UIElement* element, UIModAdapter* adapter) {
		UIWidget::onPressEventFired(element, adapter);
		if (_onPressEventHandler) {
			_onPressEventHandler(this);
		}
	}
}
