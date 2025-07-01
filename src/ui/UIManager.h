#pragma once

#include "UIContainer.h"
#include "UIUtils.h"
#include "UIWidget.h"

#include <vector>

namespace vrui
{
    class UIManager
    {
    public:
        void onFrameUpdate(UIModAdapter* adapter);
        void attachElement(const std::shared_ptr<UIElement>& element, RE::NiNode* attachNode);
        void detachElement(const std::shared_ptr<UIElement>& element, bool releaseSafe);

        void attachPresetToPrimaryWandTop(const std::shared_ptr<UIElement>& element, float zOffset = 0);
        void attachPresetToPrimaryWandLeft(const std::shared_ptr<UIElement>& element, bool leftHanded, RE::NiPoint3 offset = { 0, 0, 0 });
        void attachPresetToHMDBottom(const std::shared_ptr<UIElement>& element);

    private:
        void dumpUITree() const;
        static void dumpUITreeRecursive(UIElement* element, std::string padding);

        std::vector<std::shared_ptr<UIElement>> _rootElements;

        // used to release child elements in a safe way (on the next frame update)
        std::vector<std::shared_ptr<UIElement>> _releaseSafeList;
    };

    // Not a fan of globals but it may be easiest to refactor code right now
    extern UIManager* g_uiManager;

    static void initUIManager()
    {
        if (g_uiManager) {
            throw std::exception("UI manager already initialized");
        }
        g_uiManager = new UIManager();
    }
}
