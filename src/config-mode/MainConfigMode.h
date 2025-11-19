#pragma once

#include "BodyAdjustmentSubConfigMode.h"
#include "pipboy/Pipboy.h"
#include "ui/UIContainer.h"

namespace frik
{
    enum class TwoHandedGripMode : uint8_t
    {
        Mode1,
        Mode2,
        Mode3,
        Mode4,
    };

    class MainConfigMode
    {
    public :
        int isOpen() const;
        void openConfigMode();
        void onFrameUpdate();
        bool isBodyAdjustOpen() const;

    private:
        void createMainConfigUI();
        void openBodyAdjustmentSubConfigUI();
        static TwoHandedGripMode getTwoHandedGripMode();
        static void updateTwoHandedGripMode(TwoHandedGripMode mode);
        void openPipboyConfigUI();
        void openWeaponAdjustConfigUI();
        void closeMainConfigMode();

        // configuration UI
        std::shared_ptr<vrui::UIContainer> _configUI;

        std::shared_ptr<BodyAdjustmentSubConfigMode> _bodyAdjustmentSubConfig;
    };
}
