#include "UIManager.h"

namespace ui {
	// globals, globals everywhere...
	UIManager* g_uiManager;

	/**
	 * Run frame update on all the containers.
	 */
	void UIManager::onFrameUpdate(UIModAdapter* adapter) {
		_releaseSafeList.clear();
		for (const auto& element : _rootElements) {
			element->onLayoutUpdate(adapter);
		}
		for (const auto& element : _rootElements) {
			element->onFrameUpdate(adapter);
		}
	}

	/**
	 * Attach the given element and subtree to the given attach game node to be rendered and layout with relation to the node.
	 */
	void UIManager::attachElement(const std::shared_ptr<UIElement>& element, NiNode* attachNode) {
		element->attachToNode(attachNode);
		// only the root can exists in the manager collection
		if (!element->getParent()) {
			_MESSAGE("UI Manager root element added and attached to '%s'", attachNode->m_name.c_str());
			_rootElements.emplace_back(element);
		}
	}

	/**
	 * Remove the element and subtree from attached game node.
	 * Safe Release: If <true>, the element will be added to release queue to be released on the next frame update
	 * so finishing access to it on this frame update is still safe (release UI while handling UI event).
	 */
	void UIManager::detachElement(const std::shared_ptr<UIElement>& element, const bool releaseSafe) {
		element->detachFromAttachedNode(releaseSafe);

		// only the root can exists in the manager collection
		if (element->getParent()) {
			return;
		}
		for (auto it = _rootElements.begin(); it != _rootElements.end(); ++it) {
			if (it->get() == element.get()) {
				if (releaseSafe) {
					_releaseSafeList.push_back(*it);
				}
				_MESSAGE("UI Manager root element removed (ReleaseSafe: %d)", releaseSafe);
				_rootElements.erase(it);
				break;
			}
		}
	}
}
