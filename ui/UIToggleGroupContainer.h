#pragma once

#include "UIElement.h"
#include "UIContainer.h"
#include "UIToggleButton.h"

namespace ui {
	class UIToggleGroupContainer : public UIContainer {
	public:
		explicit UIToggleGroupContainer(const UIContainerLayout layout = UIContainerLayout::Manual, const float padding = 0)
			: UIContainer(layout, padding) {}

		void addElement(const std::shared_ptr<UIToggleButton>& button);

		[[nodiscard]] virtual std::string toString() const override;

	protected:
		virtual void onPressEventFired(UIElement* eventElement, UIFrameUpdateContext* context) override;
	};
}
