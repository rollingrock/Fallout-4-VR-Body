#pragma once

#include "UIWidget.h"
#include "UIContainer.h"
#include "UIUtils.h"

#include <vector>

namespace ui {

	class UIManager
	{
	public:
		void onFrameUpdate(IUIModAdapter* adapter);
		void onFrameUpdate(IUIModAdapter* adapter, UIElement* element);
		void attachElement(UIElement* element, NiNode* attachNode);
		void attachElementToPrimaryWand(UIElement* element) { attachElement(element, getPrimaryWandAttachNode()); }

		/// <summary>
		/// Get attachment node for the primary wand (primary/right hand).
		/// </summary>
		NiNode* getPrimaryWandAttachNode() { return findNode(getPrimaryWandNodeName().c_str(), getPlayerNodes()->primaryUIAttachNode); }

	private:
		std::vector<UIElement*> _elements;
	};

	// Not a fan of globals but it may be easiest to refactor code right now
	extern UIManager* g_uiManager;

	static void initUIManager() {
		if (g_uiManager) {
			throw std::exception("UI manager already initialized");
		}
		g_uiManager = new UIManager();
	}
}

