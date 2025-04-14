#pragma once

#include "UIWidget.h"

namespace ui {
	class UIButton : public UIWidget {
	public:
		explicit UIButton(const std::string& nifPath)
			: UIButton(getClonedNiNodeForNifFile(nifPath)) {}

		explicit UIButton(NiNode* node)
			: UIWidget(node) {}

		void setOnPressHandler(std::function<void(UIWidget*)> handler) {
			_onPressEventHandler = std::move(handler);
		}

	protected:
		virtual bool isPressable() override { return _onPressEventHandler != nullptr; }
		virtual void onPressEventFired(UIElement* element, UIModAdapter* adapter) override;

		std::function<void(UIButton*)> _onPressEventHandler;
	};
}
