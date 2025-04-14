#pragma once

#include "UIElement.h"
#include "UIWidget.h"

namespace ui {
	class UIContainer : public UIElement {
	public:
		virtual void onFrameUpdate(UIModAdapter* adapter) override;
		void addElement(const std::shared_ptr<UIElement>& element);

	protected:
		virtual void attachToNode(NiNode* attachNode) override;
		virtual void detachFromAttachedNode(bool releaseSafe) override;

		// all the elements under this container
		std::vector<std::shared_ptr<UIElement>> _childElements;
	};
}
