#pragma once

#include "UIElement.h"
#include "UIContainer.h"
#include "UIToggleButton.h"

namespace ui {
	class UIToggleGroupContainer : public UIContainer {
	public:
		void addElement(const std::shared_ptr<UIToggleButton>& button);

	protected:
		virtual void onPressEventFired(UIElement* eventElement, UIModAdapter* adapter) override;
	};
}
