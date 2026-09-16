#pragma once

#include <vector>

namespace frik
{
    class CullGeometryHandler
    {
    public:
        // hideAll culls every player geometry (scoped without a provider keeping the body visible), bones stay valid
        void cullPlayerGeometry(bool hideAll = false);

    private:
        void restoreGeometry();
        void restoreEquipment();
        void preProcessHideGeometryIndexes(RE::BSFadeNode* rn);
        static void setEquipmentSlotByIndexVisibility(int slotId, bool toHide);

        // used to handle update to hide flags to know to restore culled geometries
        bool _isGeometryCulled = false;
        bool _isEquipmentCulled = false;
        bool _isAllCulled = false;

        time_t _lastPreProcessTime = 0;
        int _lastHiddenGeometryIdx = -1;
        std::string _lastHiddenGeometryName;
        std::vector<std::uint32_t> _hideFaceSkinGeometryIndexes;
    };
}
