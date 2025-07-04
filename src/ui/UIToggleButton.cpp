#include "UIToggleButton.h"

#include <format>
#include <stdexcept>

namespace vrui
{
    std::string UIToggleButton::toString() const
    {
        return std::format("UIToggleButton({}): {}{}{}, Pos({:.2f}, {:.2f}, {:.2f}), Size({:.2f}, {:.2f})",
            _node->name.c_str(),
            _visible ? "V" : "H",
            isPressable() ? "P" : ".",
            _isToggleOn ? "T" : ".",
            _transform.translate.x,
            _transform.translate.y,
            _transform.translate.z,
            _size.width,
            _size.height
            );
    }

    void UIToggleButton::attachToNode(RE::NiNode* node)
    {
        UIWidget::attachToNode(node);
        f4vr::attachChildToNode(_attachNode.get(), _toggleFrameNode.get());
    }

    void UIToggleButton::detachFromAttachedNode(const bool releaseSafe)
    {
        if (!_attachNode) {
            throw std::runtime_error("Attempt to detach NOT attached widget");
        }
        RE::NiPointer<RE::NiAVObject> out;
        _attachNode->DetachChild(_toggleFrameNode.get(), out);
        UIWidget::detachFromAttachedNode(releaseSafe);
    }

    /**
     * Toggle button is pressable if there is press handler, and it is not already toggled on when toggling off is not allowed.
     */
    bool UIToggleButton::isPressable() const
    {
        return _visible && _onToggleEventHandler != nullptr && !(_isToggleOn && !_isUnToggleAllowed);
    }

    /**
     * Handle toggle frame visibility.
     */
    void UIToggleButton::onFrameUpdate(UIFrameUpdateContext* adapter)
    {
        UIWidget::onFrameUpdate(adapter);
        if (!_attachNode) {
            return;
        }

        const bool visible = _isToggleOn && calcVisibility();
        setNodeVisibility(_toggleFrameNode.get(), visible, getScale());
        if (visible) {
            _toggleFrameNode->local = _node->local;
        }
    }

    /**
     * Handle toggle event of press on the button.
     */
    void UIToggleButton::onPressEventFired(UIElement* element, UIFrameUpdateContext* context)
    {
        if (_isToggleOn && !_isUnToggleAllowed) {
            // not allowed to un-toggle
            return;
        }

        UIWidget::onPressEventFired(element, context);
        setToggleState(!_isToggleOn);
        if (_onToggleEventHandler) {
            _onToggleEventHandler(this, _isToggleOn);
        }
    }
}
