#pragma once

#include <vector>

namespace frik
{
    class CullGeometryHandler
    {
    public:
        void cullPlayerGeometry();

    private:
        void restoreGeometry();
        void restoreEquipment();
        void preProcessHideGeometryIndexes(RE::BSFadeNode* rn);
        static void setEquipmentSlotByIndexVisibility(int slotId, bool toHide);

        // used to handle update to hide flags to know to restore culled geometries
        bool _isGeometryCulled = false;
        bool _isEquipmentCulled = false;

        time_t _lastPreProcessTime = 0;
        int _lastHiddenGeometryIdx = -1;
        std::string _lastHiddenGeometryName;
        std::vector<std::uint32_t> _hideFaceSkinGeometryIndexes;
    };
}
