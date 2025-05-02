#pragma once

#include "UIWidget.h"

namespace VRUI {
	class UIButton : public UIWidget {
	public:
		explicit UIButton(const std::string& nifPath)
			: UIButton(getClonedNiNodeForNifFile(nifPath)) {}

		explicit UIButton(NiNode* node)
			: UIWidget(node) {
			// TODO: replace with proper calculation of node size
			_size = getButtonDefaultSize();
		}

		void setOnPressHandler(std::function<void(UIWidget*)> handler) {
			_onPressEventHandler = std::move(handler);
		}

		[[nodiscard]] virtual std::string toString() const override;

	protected:
		[[nodiscard]] virtual bool isPressable() const override { return _visible && _onPressEventHandler != nullptr; }
		virtual void onPressEventFired(UIElement* element, UIFrameUpdateContext* context) override;

		std::function<void(UIButton*)> _onPressEventHandler;
	};
}
