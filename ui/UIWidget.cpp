#include "UIWidget.h"

#include "../Config.h"
#include "../Debug.h"

namespace ui {

	/// <summary>
	/// 
	/// </summary>
	void UIWidget::onFrameUpdate(IUIModAdapter* adapter) {
		if (!_attachNode) {
			return;
		}

		auto visible = calculateVisibility();
		setNodeVisibility(_node, visible);
		if (!visible) {
			return;
		}

		handlePressEvent(adapter);

		_node->m_localTransform = calculateTransform();
	}

	/// <summary>
	/// Add soft press mimic to the transform.
	/// </summary>
	NiTransform UIWidget::calculateTransform() {
		auto trans = UIElement::calculateTransform();
		trans.pos += NiPoint3(0, _pressYOffset, 0);
		return trans;
	}

	/// <summary>
	/// Attach this widget NiNode to the given node.
	/// DO NOT call this method directly, use UIManager::attachElement() instead.
	/// </summary>
	void UIWidget::attachToNode(NiNode* node) {
		if (_attachNode)
			throw std::runtime_error("Attempt to attach already attached widget: " + std::string(_node->m_name.c_str()));

		_attachNode = node;
		_attachNode->AttachChild(_node, true);
	}

	/// <summary>
	/// Handle pressing even on the UI.
	/// Detect if interaction bone is close to the widget node and fire press event ONCE when it is.
	/// Only allow firing of the press event again when interaction bone move away enough from the widget.
	/// </summary>
	void UIWidget::handlePressEvent(IUIModAdapter* adapter) {
		if (!_onPressEventHandler) {
			return;
		}

		auto finger = adapter->getInteractionBonePosition();
		auto widgetCenter = _node->m_worldTransform.pos;

		// Generally outside the bounds of the widget
		if (!_pressEventFired && F4VRBody::vec3_len(finger - widgetCenter) > _node->m_worldBound.m_fRadius) {
			_pressYOffset = 0;
			return;
		}

		// calculate the distance only in the y-axis
		NiPoint3 forward = _node->m_worldTransform.rot * NiPoint3(0, 1, 0);
		NiPoint3 vectorToCurr = widgetCenter - finger;
		float distance = F4VRBody::vec3_dot(forward, vectorToCurr);

		// clear press state only when finger in-front of the widget
		if (_pressEventFired) {
			// far enouth to clear press flag used to prevent multi-press
			_pressEventFired = distance > 0.4 ? false : _pressEventFired;
			return;
		}

		// distance in y-axis from original location before press offset
		NiPoint3 vectorToOrg = vectorToCurr - _node->m_worldTransform.rot * NiPoint3(0, _pressYOffset, 0);
		float pressDistance = -F4VRBody::vec3_dot(forward, vectorToOrg);

		if (std::isnan(pressDistance) || pressDistance < 0) {
			_pressYOffset = 0;
			return;
		}

		static constexpr const int PRESS_TRIGGER_DISTANCE = 2;

		// mimic soft press of the UI, extra check to make sure button is not pressed when moving hand backwards
		float prevYOff = _pressYOffset;
		if (prevYOff != 0 || pressDistance < PRESS_TRIGGER_DISTANCE / 2.0) {
			// mimic soft press, smoothing with prev value
			_pressYOffset = pressDistance + (prevYOff - pressDistance) / 2;
		}

		// use previous position to prevent press when moving hand backwards
		if (PRESS_TRIGGER_DISTANCE && _pressYOffset > PRESS_TRIGGER_DISTANCE) {
			// widget pushed enough, fire press event
			_MESSAGE("UI Widget '%s' pressed", _node->m_name.c_str());
			_pressYOffset = 0;
			_pressEventFired = true;
			adapter->fireInteractionHeptic();
			_onPressEventHandler(this);
		}
	}
}