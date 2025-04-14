#include "UIToggleButton.h"

namespace ui {
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
	bool UIToggleButton::isPressable() {
		return _onToggleEventHandler != nullptr && !(_isToggleOn && !_isUnToggleAllowed);
	}

	/**
	 * Handle toggle frame visibility.
	 */
	void UIToggleButton::onFrameUpdate(UIModAdapter* adapter) {
		UIWidget::onFrameUpdate(adapter);
		if (!_attachNode) {
			return;
		}

		const bool visible = _isToggleOn && calculateVisibility();
		setNodeVisibility(_toggleFrameNode, visible);
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
