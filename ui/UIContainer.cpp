#include "UIContainer.h"
#include "UIManager.h"

namespace ui {
	std::string UIContainer::toString() const {
		return std::format("UIContainer: {}, Pos({:.2f}, {:.2f}, {:.2f}), Size({:.2f}, {:.2f}), Children({}), Layout({})",
			_visible ? "V" : "H",
			_transform.pos.x,
			_transform.pos.y,
			_transform.pos.z,
			_size.width,
			_size.height,
			_childElements.size(),
			static_cast<int>(_layout)
		);
	}

	/**
	 * Propagate frame update to all child elements.
	 */
	void UIContainer::onFrameUpdate(UIModAdapter* adapter) {
		for (const auto& childElm : _childElements) {
			childElm->onFrameUpdate(adapter);
		}
	}

	/**
	 * Is the layout is one of the horizontal layout types.
	 */
	bool UIContainer::isHorizontalLayout() const {
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
	void UIContainer::onLayoutUpdate(UIModAdapter* adapter) {
		// run layout on all children
		for (const auto& childElm : _childElements) {
			childElm->onLayoutUpdate(adapter);
		}

		// calculate the size of the container by all child elements
		if (_layout == UIContainerLayout::Manual) {
			calculateSizeManualLayout(adapter);
		} else {
			calculateSizeHorizontalVerticalLayout(adapter);
		}

		const float leftHandedMult = adapter->isLeftHandedMode() ? -1.f : 1.f;

		// do the actual layout of the child elements
		switch (_layout) {
		case UIContainerLayout::HorizontalCenter:
			layoutHorizontalCenter(leftHandedMult);
			break;
		case UIContainerLayout::HorizontalRight:
			layoutHorizontalRight(leftHandedMult);
			break;
		case UIContainerLayout::HorizontalLeft:
			layoutHorizontalLeft(leftHandedMult);
			break;
		case UIContainerLayout::VerticalCenter:
			layoutVerticalCenter(leftHandedMult);
			break;
		case UIContainerLayout::VerticalUp:
			layoutVerticalUp(leftHandedMult);
			break;
		case UIContainerLayout::VerticalDown:
			layoutVerticalDown(leftHandedMult);
			break;
		case UIContainerLayout::Manual:
			break; // noop
		}
	}

	/**
	 * Width is max distance of left-most to right-most points of child elements and
	 * height is the max distance of top-most to bottom-most.
	 */
	void UIContainer::calculateSizeManualLayout(UIModAdapter* adapter) {
		float leftMost = 0, rightMost = 0, topMost = 0, bottomMost = 0;
		for (const auto& childElm : _childElements) {
			if (!childElm->calculateVisibility()) {
				continue; // skip invisible elements
			}
			leftMost = min(leftMost, childElm->getPosition().x);
			bottomMost = min(bottomMost, childElm->getPosition().z);
			rightMost = max(rightMost, childElm->getPosition().x + childElm->getSize().width);
			topMost = max(topMost, childElm->getPosition().z + childElm->getSize().height);
		}
		_size = UISize(rightMost - leftMost, topMost - bottomMost);
	}

	/**
	 * For horizontal layout: width will be the total width of all child elements + padding, height
	 * is the max height between the child elements.
	 * Same idea for vertical layout but switched.
	 */
	void UIContainer::calculateSizeHorizontalVerticalLayout(UIModAdapter* adapter) {
		int visibleChildCount = 0;
		UISize size(0, 0);
		for (const auto& childElm : _childElements) {
			if (!childElm->calculateVisibility()) {
				continue; // skip invisible elements
			}
			visibleChildCount++;
			if (isHorizontalLayout()) {
				size.width = size.width + childElm->getSize().width;
				size.height = max(size.height, childElm->getSize().height);
			} else {
				size.width = max(size.width, childElm->getSize().width);
				size.height = size.height + childElm->getSize().height;
			}
		}
		if (visibleChildCount > 0) {
			if (isHorizontalLayout()) {
				size.width += _padding * static_cast<float>(visibleChildCount - 1);
			} else {
				size.height += _padding * static_cast<float>(visibleChildCount - 1);
			}
		}
		_size = size;
	}

	/**
	 * Layout horizontally with starting offset that is half of the total width of all the elements.
	 * Small adjustment because 0,0 is the center of the element.
	 */
	void UIContainer::layoutHorizontalCenter(const float leftHandedMult) const {
		float layoutOffset = -leftHandedMult * _size.width / 2;
		for (const auto& childElm : _childElements) {
			if (childElm->calculateVisibility()) {
				childElm->setPosition(layoutOffset + leftHandedMult * childElm->getSize().width / 2, 0, 0);
				layoutOffset += leftHandedMult * (childElm->getSize().width + _padding);
			}
		}
	}

	/**
	 * Layout horizontally from 0 adding positive offset with each child and padding.
	 */
	void UIContainer::layoutHorizontalRight(const float leftHandedMult) const {
		float layoutOffset = 0;
		for (const auto& childElm : _childElements) {
			if (childElm->calculateVisibility()) {
				childElm->setPosition(layoutOffset + leftHandedMult * childElm->getSize().width / 2, 0, 0);
				layoutOffset += leftHandedMult * (childElm->getSize().width + _padding);
			}
		}
	}

	/**
	 * Layout horizontally from 0 adding negative offset with each child and padding.
	 */
	void UIContainer::layoutHorizontalLeft(const float leftHandedMult) const {
		float layoutOffset = 0;
		for (const auto& childElm : _childElements) {
			if (childElm->calculateVisibility()) {
				childElm->setPosition(layoutOffset - leftHandedMult * childElm->getSize().width / 2, 0, 0);
				layoutOffset -= leftHandedMult * (childElm->getSize().width + _padding);
			}
		}
	}

	/**
	 * Layout vertically with starting offset that is half of the total height of all the elements.
	 * Small adjustment because 0,0 is the center of the element.
	 */
	void UIContainer::layoutVerticalCenter(const float leftHandedMult) const {
		float layoutOffset = leftHandedMult * _size.height / 2;
		for (const auto& childElm : _childElements) {
			if (childElm->calculateVisibility()) {
				childElm->setPosition(0, 0, layoutOffset - leftHandedMult * childElm->getSize().height / 2);
				layoutOffset -= leftHandedMult * (childElm->getSize().height + _padding);
			}
		}
	}

	/**
	 * Layout vertically from 0 adding positive offset with each child and padding.
	 */
	void UIContainer::layoutVerticalUp(const float leftHandedMult) const {
		float layoutOffset = 0;
		for (const auto& childElm : _childElements) {
			if (childElm->calculateVisibility()) {
				childElm->setPosition(0, 0, layoutOffset + leftHandedMult * childElm->getSize().height / 2);
				layoutOffset += leftHandedMult * (childElm->getSize().height + _padding);
			}
		}
	}

	/**
	 * Layout vertically from 0 adding negative offset with each child and padding.
	 */
	void UIContainer::layoutVerticalDown(const float leftHandedMult) const {
		float layoutOffset = 0;
		for (const auto& childElm : _childElements) {
			if (childElm->calculateVisibility()) {
				childElm->setPosition(0, 0, layoutOffset - leftHandedMult * childElm->getSize().height / 2);
				layoutOffset -= leftHandedMult * (childElm->getSize().height + _padding);
			}
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
