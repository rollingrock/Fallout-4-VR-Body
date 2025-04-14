#pragma once

#include "UIWidget.h"
#include "UIContainer.h"
#include "UIUtils.h"

#include <vector>

namespace ui {
	class UIManager {
	public:
		void onFrameUpdate(UIModAdapter* adapter);
		void attachElement(const std::shared_ptr<UIElement>& element, NiNode* attachNode);

		void attachElementToPrimaryWand(const std::shared_ptr<UIElement>& element) {
			attachElement(element, getPrimaryWandAttachNode());
		}

		void detachElement(const std::shared_ptr<UIElement>& element, bool releaseSafe);

		// Get attachment node for the primary wand (primary/right hand).
		static NiNode* getPrimaryWandAttachNode() {
			return findNode(getPrimaryWandNodeName().c_str(), getPlayerNodes()->primaryUIAttachNode);
		}

	private:
		void dumpUITree() const;
		static void dumpUITreeRecursive(UIElement* element, std::string padding);

		std::vector<std::shared_ptr<UIElement>> _rootElements;

		// used to release child elements in a safe way (on the next frame update)
		std::vector<std::shared_ptr<UIElement>> _releaseSafeList;
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
