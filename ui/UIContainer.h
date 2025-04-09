#pragma once

#include "UIElement.h"
#include "UIWidget.h"

namespace ui {

	class UIContainer : public UIElement
	{
	public:
		void onFrameUpdate(IUIModAdapter* adapter) override;
		void UIContainer::addElement(UIElement* element);

	private:
		void attachToNode(NiNode* node) override;
		
		// all the elements under this container
		std::vector<UIElement*> _elements;
	};
}

