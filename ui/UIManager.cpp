#include "UIManager.h"

namespace ui {

	UIManager* g_uiManager;

	/// <summary>
	/// Run frame update on all the containers.
	/// </summary>
	void UIManager::onFrameUpdate(IUIModAdapter* adapter) {
		for (auto element : _elements) {
			element->onFrameUpdate(adapter);
		}
	}

	void UIManager::onFrameUpdate(IUIModAdapter* adapter, UIElement* element) {
		element->onFrameUpdate(adapter);
	}

	void UIManager::attachElement(UIElement* element, NiNode* attachNode) {
		element->attachToNode(attachNode);
		if (!element->getParent()) {
			_elements.emplace_back(element);
		}
	}
}