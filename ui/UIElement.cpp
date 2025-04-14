#include "UIElement.h"

namespace ui {
	void UIElement::attachToNode(NiNode* attachNode) {
		if (_attachNode)
			throw std::runtime_error(
				"Attempt to attach already attached widget: " + std::string(attachNode->m_name.c_str()));
		_attachNode = attachNode;
	}

	void UIElement::detachFromAttachedNode(bool releaseSafe) {
		if (!_attachNode)
			throw std::runtime_error("Attempt to detach NOT attached widget");
		_attachNode = nullptr;
	}

	/**
	 * calculate the transform of the element with respect to all parents.
	 */
	NiTransform UIElement::calculateTransform() {
		if (!_parent) {
			return _transform;
		}

		auto calTransform = _transform;
		const auto parentTransform = _parent->calculateTransform();
		calTransform.pos += parentTransform.pos;
		// TODO: add rotation handling
		return calTransform;
	}

	/**
	 * Calculate if the element should be visible with respect to all parents.
	 */
	bool UIElement::calculateVisibility() {
		return _visible && (_parent ? _parent->calculateVisibility() : true);
	}

	/**
	 * Call "onPressEventFired" on this element and all elements up the UI tree.
	 */
	void UIElement::onPressEventFiredPropagate(UIElement* element, UIModAdapter* adapter) {
		onPressEventFired(element, adapter);
		if (_parent) {
			_parent->onPressEventFiredPropagate(element, adapter);
		}
	}
}
