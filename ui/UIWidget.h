#pragma once

#include "UIElement.h"
#include "UIUtils.h"
#include "../Config.h"

namespace ui {

	class UIWidget : public UIElement
	{
	public:
		UIWidget(const std::string& nifPath)
			: _node(getClonedNiNodeForNifFile(nifPath)) {
		}

		UIWidget(NiNode* node)
			: _node(node) {
		}

		void onFrameUpdate(IUIModAdapter* adapter) override;

		void setOnPressHandler(std::function<void(UIWidget*)> handler) {
			_onPressEventHandler = std::move(handler);
		}

	protected:
		void attachToNode(NiNode* node) override;
		virtual NiTransform calculateTransform() override;
		void handlePressEvent(IUIModAdapter* adapter);
		
		// UI node to render
		NiNode* _node;

		// Press handling
		bool _pressEventFired = false;
		std::function<void(UIWidget*)> _onPressEventHandler;
		float _pressYOffset = 0;
	};
}
