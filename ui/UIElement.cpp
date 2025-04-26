#include "UIElement.h"

namespace ui {
	std::string UIElement::toString() const {
		return std::format("UIElement: {}, Pos({:.2f}, {:.2f}, {:.2f}), Size({:.2f}, {:.2f})",
			_visible ? "V" : "H",
			_transform.pos.x,
			_transform.pos.y,
			_transform.pos.z,
			_size.width,
			_size.height
		);
	}

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
	NiTransform UIElement::calculateTransform() const {
		if (!_parent) {
			return _transform;
		}

		auto calTransform = _transform;
		const auto parentTransform = _parent->calculateTransform();
		calTransform.pos += parentTransform.pos;
		calTransform.scale *= parentTransform.scale;
		// TODO: add rotation handling
		return calTransform;
	}

	/**
	 * Call "onPressEventFired" on this element and all elements up the UI tree.
	 */
	void UIElement::onPressEventFiredPropagate(UIElement* element, UIFrameUpdateContext* context) {
		onPressEventFired(element, context);
		if (_parent) {
			_parent->onPressEventFiredPropagate(element, context);
		}
	}
}
