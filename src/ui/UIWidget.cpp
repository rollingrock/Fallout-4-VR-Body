#include "UIWidget.h"

#include "../common/Logger.h"
#include "common/MatrixUtils.h"

using namespace common;

namespace vrui
{
    std::string UIWidget::toString() const
    {
        return std::format("UIWidget({}): {}{}, Pos({:.2f}, {:.2f}, {:.2f}), Size({:.2f}, {:.2f})",
            _node->name.c_str(),
            _visible ? "V" : "H",
            isPressable() ? "P" : ".",
            _transform.translate.x,
            _transform.translate.y,
            _transform.translate.z,
            _size.width,
            _size.height
            );
    }

    /**
     * Attach this widget RE::NiNode to the given node.
     */
    void UIWidget::attachToNode(RE::NiNode* attachNode)
    {
        UIElement::attachToNode(attachNode);
        _attachNode->AttachChild(_node.get(), true);
    }

    /**
     * Remove this widget from attached node.
     */
    void UIWidget::detachFromAttachedNode(const bool releaseSafe)
    {
        if (!_attachNode) {
            throw std::runtime_error("Attempt to detach NOT attached widget");
        }
        RE::NiPointer<RE::NiAVObject> out;
        _attachNode->DetachChild(_node.get(), out);
        UIElement::detachFromAttachedNode(releaseSafe);
        out = nullptr;
    }

    /**
     * Handle widget visibility, location, and press handling.
     */
    void UIWidget::onFrameUpdate(UIFrameUpdateContext* adapter)
    {
        if (!_attachNode) {
            return;
        }

        const auto visible = calcVisibility();
        setNodeVisibility(_node.get(), visible, getScale());
        if (!visible) {
            return;
        }

        handlePressEvent(adapter);

        _node->local = calculateTransform();
    }

    /**
     * Add soft press mimic to the transform.
     */
    RE::NiTransform UIWidget::calculateTransform() const
    {
        auto trans = UIElement::calculateTransform();
        trans.translate += RE::NiPoint3(0, _pressYOffset, 0);
        return trans;
    }

    /**
     * Handle pressing even on the UI.
     * Detect if interaction bone is close to the widget node and fire press event ONCE when it is.
     * Only allow firing of the press event again when interaction bone move away enough from the widget.
     */
    void UIWidget::handlePressEvent(UIFrameUpdateContext* context)
    {
        if (!isPressable()) {
            return;
        }

        const auto finger = context->getInteractionBoneWorldPosition();
        const auto widgetCenter = _node->world.translate;

        const float distance = vec3Len(finger - widgetCenter);

        // calculate the distance only in the y-axis
        const RE::NiPoint3 forward = _node->world.rotate.Transpose() * (RE::NiPoint3(0, 1, 0));
        const RE::NiPoint3 vectorToCurr = widgetCenter - finger;
        const float yOnlyDistance = vec3Dot(forward, vectorToCurr);

        updatePressableCloseToInteraction(context, distance, yOnlyDistance);

        // Generally outside the bounds of the widget
        if (!_pressEventFired && distance > _node->worldBound.fRadius) {
            _pressYOffset = 0;
            return;
        }

        // clear press state only when finger in-front of the widget
        if (_pressEventFired) {
            // far enough to clear press flag used to prevent multi-press
            _pressEventFired = yOnlyDistance > 0.4 ? false : _pressEventFired;
            return;
        }

        // distance in y-axis from original location before press offset
        const RE::NiPoint3 vectorToOrg = vectorToCurr - _node->world.rotate.Transpose() * (RE::NiPoint3(0, _pressYOffset, 0));
        const float pressDistance = -vec3Dot(forward, vectorToOrg);

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
            logger::info("UI Widget '{}' pressed", _node->name.c_str());
            onPressEventFiredPropagate(this, context);
        }
    }

    /**
     * is interaction bone is relatively close to widget for hand pose to change.
     * Use previously set value to create a safe buffer where the pressable won't rapidly change from true to false and back.
     */
    void UIWidget::updatePressableCloseToInteraction(UIFrameUpdateContext* context, const float distance, const float yOnlyDistance)
    {
        _wasPressableCloseToInteraction = _wasPressableCloseToInteraction
            ? yOnlyDistance > -12 && distance < 20
            : yOnlyDistance > -3 && distance < 15;
        context->markAnyPressableCloseToInteraction(_wasPressableCloseToInteraction);
    }

    void UIWidget::onPressEventFired(UIElement* element, UIFrameUpdateContext* context)
    {
        _pressYOffset = 0;
        _pressEventFired = true;
        context->fireInteractionHeptic();
        UIElement::onPressEventFired(element, context);
    }
}
