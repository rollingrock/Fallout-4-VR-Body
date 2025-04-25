#include "UIToggleButton.h"

namespace ui {
	std::string UIToggleButton::toString() const {
		return std::format("UIToggleButton({}): {}{}{}, Pos({:.2f}, {:.2f}, {:.2f}), Size({:.2f}, {:.2f})",
			_node->m_name.c_str(),
			_visible ? "V" : "H",
			isPressable() ? "P" : ".",
			_isToggleOn ? "T" : ".",
			_transform.pos.x,
			_transform.pos.y,
			_transform.pos.z,
			_size.width,
			_size.height
		);
	}

	void UIToggleButton::attachToNode(NiNode* node) {
		UIWidget::attachToNode(node);
		_attachNode->AttachChild(_toggleFrameNode, true);
	}

	void UIToggleButton::detachFromAttachedNode(const bool releaseSafe) {
		if (!_attachNode)
			throw std::runtime_error("Attempt to detach NOT attached widget");
		NiPointer<NiAVObject> out;
		_attachNode->DetachChild(_toggleFrameNode, out);
		UIWidget::detachFromAttachedNode(releaseSafe);
	}

	/**
	 * Toggle button is pressable if there is press handler, and it is not already toggled on when toggling off is not allowed.
	 */
	bool UIToggleButton::isPressable() const {
		return _visible && _onToggleEventHandler != nullptr && !(_isToggleOn && !_isUnToggleAllowed);
	}

	/**
	 * Handle toggle frame visibility.
	 */
	void UIToggleButton::onFrameUpdate(UIModAdapter* adapter) {
		UIWidget::onFrameUpdate(adapter);
		if (!_attachNode) {
			return;
		}

		const bool visible = _isToggleOn && calcVisibility();
		setNodeVisibility(_toggleFrameNode, visible, getScale());
		if (visible) {
			_toggleFrameNode->m_localTransform = _node->m_localTransform;
		}
	}

	/**
	 * Handle toggle event of press on the button.
	 */
	void UIToggleButton::onPressEventFired(UIElement* element, UIModAdapter* adapter) {
		if (_isToggleOn && !_isUnToggleAllowed) {
			// not allowed to un-toggle
			return;
		}

		UIWidget::onPressEventFired(element, adapter);
		_isToggleOn = !_isToggleOn;
		if (_onToggleEventHandler) {
			_onToggleEventHandler(this, _isToggleOn);
		}
	}
}
