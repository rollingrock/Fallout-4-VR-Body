#pragma once

#include "UIElement.h"

namespace vrui
{
    static std::string getDebugSphereNifName() { return "FRIK/1x1Sphere.nif"; }
    static std::string getToggleButtonFrameNifName() { return "FRIK/ui_common_btn_border.nif"; }

    static UISize getButtonDefaultSize() { return { 2.2f, 2.2f }; }

    f4vr::PlayerNodes* getPlayerNodes();
    void setNodeVisibility(RE::NiNode* node, bool visible, float originalScale);
    RE::NiNode* getClonedNiNodeForNifFile(const std::string& path);
    RE::NiNode* loadNifFromFile(const char* path);
    RE::NiNode* findNode(const char* nodeName, RE::NiNode* node);
}
