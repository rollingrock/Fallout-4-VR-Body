#pragma once

#include "f4se/NiNodes.h"

#include "IUIModAdapter.h"
#include "../matrix.h"

namespace ui {

	class UIElement
	{
	public:
		UIElement() {
			_transform.pos = NiPoint3(0, 0, 0);
			auto ident = F4VRBody::Matrix44();
			ident.makeIdentity();
			_transform.rot = ident.make43();
			_transform.scale = 1;
		}

		/// <summary>
		/// Set the position of the UI element relative to the parent.
		/// </summary>
		/// <param name="x">horizontal: positive-right, negative-left</param>
		/// <param name="y">depth: positive-forward, negative-backward</param>
		/// <param name="z">vertical: positive-up, negative-down</param>
		inline void setPosition(float x, float y, float z) { _transform.pos = NiPoint3(x, y, z); }
		inline void updatePosition(float x, float y, float z) { _transform.pos += NiPoint3(x, y, z); }
		inline NiPoint3 getPosition(float x, float y, float z) const { return _transform.pos; }

		inline bool isVisible() const { return _visible; }
		inline void setVisibility(bool visible) { _visible = visible; }

		inline UIElement* getParent() { return _parent; }
		inline void setParent(UIElement* parent) { _parent = parent; }

	protected:
		virtual NiTransform calculateTransform();
		virtual bool calculateVisibility();

		// Attach the UI element to the given game node.
		virtual void attachToNode(NiNode* node) = 0;

		// Handle UI interation code on each frame of the game.
		virtual void onFrameUpdate(IUIModAdapter* adapter) = 0;

		UIElement* _parent = nullptr;
		NiTransform _transform;
		bool _visible = true;

		// Game node the main node is attached to
		NiNode* _attachNode = nullptr;

		// Used to allow hiding attachToNode, onFrameUpdate from public API
		friend class UIManager;
	};
}
