#include "UIContainer.h"

#include <format>

#include "UIManager.h"

namespace vrui
{
    std::string UIContainer::toString() const
    {
        return std::format("UIContainer: {}, Pos({:.2f}, {:.2f}, {:.2f}), Size({:.2f}, {:.2f}), Children({}), Layout({})",
            _visible ? "V" : "H",
            _transform.translate.x,
            _transform.translate.y,
            _transform.translate.z,
            _size.width,
            _size.height,
            _childElements.size(),
            static_cast<int>(_layout)
            );
    }

    /**
     * Propagate frame update to all child elements.
     */
    void UIContainer::onFrameUpdate(UIFrameUpdateContext* adapter)
    {
        for (const auto& childElm : _childElements) {
            childElm->onFrameUpdate(adapter);
        }
    }

    /**
     * Is the layout is one of the horizontal layout types.
     */
    bool UIContainer::isHorizontalLayout() const
    {
        return _layout == UIContainerLayout::HorizontalCenter ||
            _layout == UIContainerLayout::HorizontalRight ||
            _layout == UIContainerLayout::HorizontalLeft;
    }

    /**
     * Layout the child elements in the container.
     * 1. Call onLayoutUpdate all the child elements first to have their size.
     * 2. Calculate the size of the container by all the children and layout type.
     * 3. Arrange the child elements in the container by the layout type.
     */
    void UIContainer::onLayoutUpdate(UIFrameUpdateContext* adapter)
    {
        // run layout on all children
        for (const auto& childElm : _childElements) {
            childElm->onLayoutUpdate(adapter);
        }

        // calculate the size of the container by all child elements
        if (_layout == UIContainerLayout::Manual) {
            calculateSizeManualLayout();
        } else {
            calculateSizeHorizontalVerticalLayout();
        }

        // do the actual layout of the child elements
        switch (_layout) {
        case UIContainerLayout::HorizontalCenter:
            layoutHorizontalCenter();
            break;
        case UIContainerLayout::HorizontalRight:
            layoutHorizontalRight();
            break;
        case UIContainerLayout::HorizontalLeft:
            layoutHorizontalLeft();
            break;
        case UIContainerLayout::VerticalCenter:
            layoutVerticalCenter();
            break;
        case UIContainerLayout::VerticalUp:
            layoutVerticalUp();
            break;
        case UIContainerLayout::VerticalDown:
            layoutVerticalDown();
            break;
        case UIContainerLayout::Manual:
            break; // noop
        }
    }

    /**
     * Width is max distance of left-most to right-most points of child elements and
     * height is the max distance of top-most to bottom-most.
     */
    void UIContainer::calculateSizeManualLayout()
    {
        float leftMost = 0, rightMost = 0, topMost = 0, bottomMost = 0;
        for (const auto& childElm : _childElements) {
            if (!childElm->calcVisibility()) {
                continue; // skip invisible elements
            }
            leftMost = min(leftMost, childElm->getPosition().x);
            bottomMost = min(bottomMost, childElm->getPosition().z);
            rightMost = max(rightMost, childElm->getPosition().x + childElm->calcSize().width);
            topMost = max(topMost, childElm->getPosition().z + childElm->calcSize().height);
        }
        _size = UISize((rightMost - leftMost) / calcScale(), (topMost - bottomMost) / calcScale());
    }

    /**
     * For horizontal layout: width will be the total width of all child elements + padding, height
     * is the max height between the child elements.
     * Same idea for vertical layout but switched.
     */
    void UIContainer::calculateSizeHorizontalVerticalLayout()
    {
        int visibleChildCount = 0;
        UISize size(0, 0);
        for (const auto& childElm : _childElements) {
            if (!childElm->calcVisibility()) {
                continue; // skip invisible elements
            }
            visibleChildCount++;
            if (isHorizontalLayout()) {
                size.width = size.width + childElm->calcSize().width;
                size.height = max(size.height, childElm->calcSize().height);
            } else {
                size.width = max(size.width, childElm->calcSize().width);
                size.height = size.height + childElm->calcSize().height;
            }
        }
        if (visibleChildCount > 0) {
            if (isHorizontalLayout()) {
                size.width += calcPadding() * static_cast<float>(visibleChildCount - 1);
            } else {
                size.height += calcPadding() * static_cast<float>(visibleChildCount - 1);
            }
        }
        _size = UISize(size.width / calcScale(), size.height / calcScale());
    }

    /**
     * Layout horizontally with starting offset that is half of the total width of all the elements.
     * Small adjustment because 0,0 is the center of the element.
     */
    void UIContainer::layoutHorizontalCenter() const
    {
        float layoutOffset = -calcSize().width / 2;
        for (const auto& childElm : _childElements) {
            if (childElm->calcVisibility()) {
                childElm->setPosition(layoutOffset + childElm->calcSize().width / 2, 0, 0);
                layoutOffset += childElm->calcSize().width + calcPadding();
            }
        }
    }

    /**
     * Layout horizontally from 0 adding positive offset with each child and padding.
     */
    void UIContainer::layoutHorizontalRight() const
    {
        float layoutOffset = 0;
        for (const auto& childElm : _childElements) {
            if (childElm->calcVisibility()) {
                childElm->setPosition(layoutOffset + childElm->calcSize().width / 2, 0, 0);
                layoutOffset += childElm->calcSize().width + calcPadding();
            }
        }
    }

    /**
     * Layout horizontally from 0 adding negative offset with each child and padding.
     */
    void UIContainer::layoutHorizontalLeft() const
    {
        float layoutOffset = 0;
        for (const auto& childElm : _childElements) {
            if (childElm->calcVisibility()) {
                childElm->setPosition(layoutOffset - childElm->calcSize().width / 2, 0, 0);
                layoutOffset -= childElm->calcSize().width + calcPadding();
            }
        }
    }

    /**
     * Layout vertically with starting offset that is half of the total height of all the elements.
     * Small adjustment because 0,0 is the center of the element.
     */
    void UIContainer::layoutVerticalCenter() const
    {
        float layoutOffset = calcSize().height / 2;
        for (const auto& childElm : _childElements) {
            if (childElm->calcVisibility()) {
                childElm->setPosition(0, 0, layoutOffset - childElm->calcSize().height / 2);
                layoutOffset -= childElm->calcSize().height + calcPadding();
            }
        }
    }

    /**
     * Layout vertically from 0 adding positive offset with each child and padding.
     */
    void UIContainer::layoutVerticalUp() const
    {
        float layoutOffset = 0;
        for (const auto& childElm : _childElements) {
            if (childElm->calcVisibility()) {
                childElm->setPosition(0, 0, layoutOffset + childElm->calcSize().height / 2);
                layoutOffset += childElm->calcSize().height + calcPadding();
            }
        }
    }

    /**
     * Layout vertically from 0 adding negative offset with each child and padding.
     */
    void UIContainer::layoutVerticalDown() const
    {
        float layoutOffset = 0;
        for (const auto& childElm : _childElements) {
            if (childElm->calcVisibility()) {
                childElm->setPosition(0, 0, layoutOffset - childElm->calcSize().height / 2);
                layoutOffset -= childElm->calcSize().height + calcPadding();
            }
        }
    }

    /**
     * Attach all the elements in this container to the given node.
     */
    void UIContainer::attachToNode(RE::NiNode* attachNode)
    {
        if (_attachNode) {
            throw std::runtime_error("Attempt to attach already attached container");
        }

        _attachNode = attachNode;
        for (const auto& childElm : _childElements) {
            g_uiManager->attachElement(childElm, attachNode);
        }
    }

    /**
     * Detach all the elements in this container from the attached node.
     */
    void UIContainer::detachFromAttachedNode(const bool releaseSafe)
    {
        UIElement::detachFromAttachedNode(releaseSafe);
        for (const auto& childElm : _childElements) {
            g_uiManager->detachElement(childElm, releaseSafe);
        }
    }

    /**
     * Add a widget to this container.
     */
    void UIContainer::addElement(const std::shared_ptr<UIElement>& element)
    {
        element->setParent(this);
        _childElements.emplace_back(element);
        if (_attachNode) {
            g_uiManager->attachElement(element, _attachNode);
        }
    }
}
