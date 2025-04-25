#pragma once

#include "f4se/NiNodes.h"

#include "UIModAdapter.h"
#include "../matrix.h"

namespace ui {
	struct UISize {
		float width;
		float height;

		UISize(const float width, const float height)
			: width(width), height(height) {}
	};

	class UIElement {
	public:
		UIElement() {
			_transform.pos = NiPoint3(0, 0, 0);
			_transform.rot = F4VRBody::Matrix44::getIdentity43();
			_transform.scale = 1;
		}

		virtual ~UIElement() = default;

		/**
		 * Set the position of the UI element relative to the parent.
		 * @param x horizontal: positive-right, negative-left
		 * @param y depth: positive-forward, negative-backward
		 * @param z vertical: positive-up, negative-down
		 */
		void setPosition(const float x, const float y, const float z) { _transform.pos = NiPoint3(x, y, z); }
		void updatePosition(const float x, const float y, const float z) { _transform.pos += NiPoint3(x, y, z); }
		[[nodiscard]] const NiPoint3& getPosition() const { return _transform.pos; }

		[[nodiscard]] float getScale() const { return _transform.scale; }
		void setScale(const float scale) { _transform.scale = scale; }

		[[nodiscard]] bool isVisible() const { return _visible; }
		void setVisibility(const bool visible) { _visible = visible; }

		[[nodiscard]] const UISize& getSize() const { return _size; }
		void setSize(const UISize size) { _size = size; }
		void setSize(const float width, const float height) { _size = {width, height}; }

		[[nodiscard]] UIElement* getParent() const { return _parent; }
		void setParent(UIElement* parent) { _parent = parent; }

		[[nodiscard]] virtual std::string toString() const;

		// Internal: 
		virtual void onLayoutUpdate(UIModAdapter* adapter) {}

		// Internal: Handle UI interaction code on each frame of the game.
		virtual void onFrameUpdate(UIModAdapter* adapter) = 0;

		// NOTE: those can be called a lot, shouldn't be an issue for our usage but maybe worth checking one day
		// Internal: Calculate if the element should be visible with respect to all parents.
		bool calcVisibility() const { return _visible && (_parent ? _parent->calcVisibility() : true); }
		// Internal: Calculate the scale adjustment of the element with respect to all parents.
		float calcScale() const { return _transform.scale * (_parent ? _parent->calcScale() : 1); }
		// Internal: Calculate the size with relation to scale with respect to all parents.
		UISize calcSize() const {
			const auto scale = calcScale();
			return {_size.width * scale, _size.height * scale};
		}

	protected:
		virtual NiTransform calculateTransform() const;
		virtual void onPressEventFired(UIElement* element, UIModAdapter* adapter) {}
		void onPressEventFiredPropagate(UIElement* element, UIModAdapter* adapter);

		// Attach the UI element to the given game node.
		virtual void attachToNode(NiNode* attachNode);
		virtual void detachFromAttachedNode(bool releaseSafe);

		UIElement* _parent = nullptr;
		NiTransform _transform;
		bool _visible = true;

		// the width (x) and height (y) of the widget
		UISize _size = UISize(0, 0);

		// Game node the main node is attached to
		NiPointer<NiNode> _attachNode = nullptr;

		// Used to allow hiding attachToNode, detachFromAttachedNode from public API
		friend class UIManager;
	};
}
