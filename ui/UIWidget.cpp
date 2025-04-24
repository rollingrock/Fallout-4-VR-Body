#include "UIWidget.h"

#include "../Debug.h"

namespace ui {
	std::string UIWidget::toString() const {
		return std::format("UIWidget({}): {}{}, Pos({:.2f}, {:.2f}, {:.2f}), Size({:.2f}, {:.2f})",
			_node->m_name.c_str(),
			_visible ? "V" : "H",
			isPressable() ? "P" : ".",
			_transform.pos.x,
			_transform.pos.y,
			_transform.pos.z,
			_size.width,
			_size.height
		);
	}

	/**
	 * Attach this widget NiNode to the given node.
	 */
	void UIWidget::attachToNode(NiNode* attachNode) {
		UIElement::attachToNode(attachNode);
		_attachNode->AttachChild(_node, true);
	}

	/**
	 * Remove this widget from attached node.
	 */
	void UIWidget::detachFromAttachedNode(const bool releaseSafe) {
		if (!_attachNode)
			throw std::runtime_error("Attempt to detach NOT attached widget");
		NiPointer<NiAVObject> out;
		_attachNode->DetachChild(_node, out);
		UIElement::detachFromAttachedNode(releaseSafe);
		out = nullptr;
	}

	/**
	 * Handle widget visibility, location, and press handling.
	 */
	void UIWidget::onFrameUpdate(UIModAdapter* adapter) {
		if (!_attachNode) {
			return;
		}

		const auto visible = calculateVisibility();
		setNodeVisibility(_node, visible);
		if (!visible) {
			return;
		}

		handlePressEvent(adapter);

		_node->m_localTransform = calculateTransform();

		if (adapter->isLeftHandedMode()) {
			adjustForLeftHandedMode(_node);
		}
	}

	/**
	 * Add soft press mimic to the transform.
	 */
	NiTransform UIWidget::calculateTransform() const {
		auto trans = UIElement::calculateTransform();
		trans.pos += NiPoint3(0, _pressYOffset, 0);
		return trans;
	}

	/**
	 * Handle pressing even on the UI.
	 * Detect if interaction bone is close to the widget node and fire press event ONCE when it is.
	 * Only allow firing of the press event again when interaction bone move away enough from the widget.
	 */
	void UIWidget::handlePressEvent(UIModAdapter* adapter) {
		if (!isPressable()) {
			return;
		}

		const auto finger = adapter->getInteractionBonePosition();
		const auto widgetCenter = _node->m_worldTransform.pos;

		// Generally outside the bounds of the widget
		if (!_pressEventFired && F4VRBody::vec3_len(finger - widgetCenter) > _node->m_worldBound.m_fRadius) {
			_pressYOffset = 0;
			return;
		}

		// calculate the distance only in the y-axis
		const NiPoint3 forward = _node->m_worldTransform.rot * NiPoint3(0, 1, 0);
		const NiPoint3 vectorToCurr = widgetCenter - finger;
		const float distance = F4VRBody::vec3_dot(forward, vectorToCurr);

		// clear press state only when finger in-front of the widget
		if (_pressEventFired) {
			// far enough to clear press flag used to prevent multi-press
			_pressEventFired = distance > 0.4 ? false : _pressEventFired;
			return;
		}

		// distance in y-axis from original location before press offset
		const NiPoint3 vectorToOrg = vectorToCurr - _node->m_worldTransform.rot * NiPoint3(0, _pressYOffset, 0);
		const float pressDistance = -F4VRBody::vec3_dot(forward, vectorToOrg);

		if (std::isnan(pressDistance) || pressDistance < 0) {
			_pressYOffset = 0;
			return;
		}

		static constexpr int PRESS_TRIGGER_DISTANCE = 2;

		// mimic soft press of the UI, extra check to make sure button is not pressed when moving hand backwards
		const float prevYOff = _pressYOffset;
		if (prevYOff != 0.f || pressDistance < PRESS_TRIGGER_DISTANCE / 2.0) {
			// mimic soft press, smoothing with prev value
			_pressYOffset = pressDistance + (prevYOff - pressDistance) / 2;
		}

		// use previous position to prevent press when moving hand backwards
		if (_pressYOffset > PRESS_TRIGGER_DISTANCE) {
			// widget pushed enough, fire press event
			_MESSAGE("UI Widget '%s' pressed", _node->m_name.c_str());
			onPressEventFiredPropagate(this, adapter);
		}
	}

	void UIWidget::onPressEventFired(UIElement* element, UIModAdapter* adapter) {
		_pressYOffset = 0;
		_pressEventFired = true;
		adapter->fireInteractionHeptic();
		UIElement::onPressEventFired(element, adapter);
	}
}
