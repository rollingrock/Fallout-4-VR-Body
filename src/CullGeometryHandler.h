#pragma once

#include <vector>

namespace frik
{
    class CullGeometryHandler
    {
    public:
        void cullPlayerGeometry();

    private:
        static void restoreGeometry();
        void preProcessHideGeometryIndexes(RE::BSFadeNode* rn);
        static void setEquipmentSlotByIndexVisibility(int slotId, bool toHide);

        time_t _lastPreProcessTime = 0;
        int _lastHiddenGeometryIdx = -1;
        std::string _lastHiddenGeometryName;
        std::vector<std::uint32_t> _hideFaceSkinGeometryIndexes;
    };
}
