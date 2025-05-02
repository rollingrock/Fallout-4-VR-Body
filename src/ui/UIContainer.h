#pragma once

#include "UIElement.h"
#include "UIWidget.h"

namespace VRUI {
	/**
	 * How the container will lay out the child elements.
	 */
	enum class UIContainerLayout : uint8_t {
		// no layout, all elements are positioned manually
		Manual = 0,
		// all elements will be positioned centered in a row 
		HorizontalCenter,
		// all elements will be positioned in a row left to right
		HorizontalRight,
		// all elements will be positioned in a row right to left
		HorizontalLeft,
		// all elements will be positioned centered in a column
		VerticalCenter,
		// all elements will be positioned in a column bottom to top
		VerticalUp,
		// all elements will be positioned in a column top to bottom
		VerticalDown,
	};

	class UIContainer : public UIElement {
	public:
		explicit UIContainer(const UIContainerLayout layout = UIContainerLayout::Manual, const float padding = 0, const float scale = 1)
			: _layout(layout),
			  _padding(padding) {
			setScale(scale);
		}

		virtual void onFrameUpdate(UIFrameUpdateContext* adapter) override;
		virtual void onLayoutUpdate(UIFrameUpdateContext* adapter) override;
		void addElement(const std::shared_ptr<UIElement>& element);

		[[nodiscard]] UIContainerLayout layout() const { return _layout; }
		void setLayout(const UIContainerLayout layout) { _layout = layout; }
		[[nodiscard]] bool isHorizontalLayout() const;

		[[nodiscard]] float getPadding() const { return _padding; }
		void setPadding(const float padding) { _padding = padding; }

		[[nodiscard]] std::vector<std::shared_ptr<UIElement>> childElements() const { return _childElements; }

		[[nodiscard]] virtual std::string toString() const override;

	protected:
		virtual void attachToNode(NiNode* attachNode) override;
		virtual void detachFromAttachedNode(bool releaseSafe) override;
		void calculateSizeManualLayout();
		void calculateSizeHorizontalVerticalLayout();
		void layoutHorizontalCenter() const;
		void layoutHorizontalRight() const;
		void layoutHorizontalLeft() const;
		void layoutVerticalCenter() const;
		void layoutVerticalUp() const;
		void layoutVerticalDown() const;
		float calcPadding() const { return _padding * calcScale(); }

		// how to lay out the child elements
		UIContainerLayout _layout = UIContainerLayout::Manual;

		// for non-manual layout the padding to add between child elements
		float _padding = 0;

		// all the elements under this container
		std::vector<std::shared_ptr<UIElement>> _childElements;
	};
}
