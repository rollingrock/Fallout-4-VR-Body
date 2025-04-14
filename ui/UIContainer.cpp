#include "UIContainer.h"
#include "UIManager.h"

namespace ui {
	/**
	 * Propagate frame update to all child elements.
	 */
	void UIContainer::onFrameUpdate(UIModAdapter* adapter) {
		for (const auto& childElm : _childElements) {
			childElm->onFrameUpdate(adapter);
		}
	}

	/**
	 * Attach all the elements in this container to the given node.
	 */
	void UIContainer::attachToNode(NiNode* attachNode) {
		if (_attachNode)
			throw std::runtime_error("Attempt to attach already attached container");

		_attachNode = attachNode;
		for (const auto& childElm : _childElements) {
			g_uiManager->attachElement(childElm, attachNode);
		}
	}

	/**
	 * Detach all the elements in this container from the attached node.
	 */
	void UIContainer::detachFromAttachedNode(const bool releaseSafe) {
		UIElement::detachFromAttachedNode(releaseSafe);
		for (const auto& childElm : _childElements) {
			g_uiManager->detachElement(childElm, releaseSafe);
		}
	}

	/**
	 * Add a widget to this container.
	 */
	void UIContainer::addElement(const std::shared_ptr<UIElement>& element) {
		element->setParent(this);
		_childElements.emplace_back(element);
		if (_attachNode) {
			g_uiManager->attachElement(element, _attachNode);
		}
	}
}
