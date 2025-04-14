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
			auto ident = F4VRBody::Matrix44();
			ident.makeIdentity();
			_transform.rot = ident.make43();
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
		[[nodiscard]] NiPoint3 getPosition() const { return _transform.pos; }

		[[nodiscard]] bool isVisible() const { return _visible; }
		void setVisibility(const bool visible) { _visible = visible; }

		[[nodiscard]] UISize getSize() const { return _size; }
		void setSize(const UISize size) { _size = size; }
		void setSize(const float width, const float height) { _size = {width, height}; }

		[[nodiscard]] UIElement* getParent() const { return _parent; }
		void setParent(UIElement* parent) { _parent = parent; }

		[[nodiscard]] virtual std::string toString() const;

		// Internal: 
		virtual void onLayoutUpdate(UIModAdapter* adapter) {}

		// Internal: Handle UI interaction code on each frame of the game.
		virtual void onFrameUpdate(UIModAdapter* adapter) = 0;

	protected:
		virtual NiTransform calculateTransform();
		virtual bool calculateVisibility();
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

		// Used to allow hiding attachToNode, onFrameUpdate from public API
		friend class UIManager;
	};
}
