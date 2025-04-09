#include "UIContainer.h"
#include "UIManager.h"

namespace ui {
	
	void UIContainer::onFrameUpdate(IUIModAdapter* adapter) {
		for (auto element : _elements) {
			g_uiManager->onFrameUpdate(adapter, element);
		}
	}

	/// <summary>
	/// Attach all the elements in this container to the given node.
	/// DO NOT call this method directly, use UIManager::attachElement() instead.
	/// </summary>
	void UIContainer::attachToNode(NiNode* node) {
		if (_attachNode)
			throw std::runtime_error("Attempt to attach already attached container");

		_attachNode = node;
		for (auto element : _elements) {
			g_uiManager->attachElement(element, node);
		}
	}

	/// <summary>
	/// Add a widget to this container.
	/// </summary>
	void UIContainer::addElement(UIElement* element) {
		element->setParent(this);
		_elements.emplace_back(element);
		if (_attachNode) {
			g_uiManager->attachElement(element, _attachNode);
		}
	}
}