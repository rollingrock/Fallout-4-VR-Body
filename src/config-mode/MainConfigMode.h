#pragma once

#include "BodyAdjustmentSubConfigMode.h"
#include "vrui/UIContainer.h"

namespace frik
{
    enum class TwoHandedGripMode : uint8_t
    {
        Mode1,
        Mode2,
        Mode3,
        Mode4,
    };

    struct OpenExternalModConfigData
    {
        std::string buttonIconNifPath;
        std::string callbackReceiverName;
        std::uint32_t callbackMessageType;
    };

    class MainConfigMode
    {
    public :
        int isOpen() const;
        void openConfigMode();
        void onFrameUpdate();
        bool isBodyAdjustOpen() const;
        void registerOpenExternalModSettingButton(const OpenExternalModConfigData& data);

    private:
        void createMainConfigUI();
        void openBodyAdjustmentSubConfigUI();
        static void toggleSelfieMode();
        static TwoHandedGripMode getTwoHandedGripMode();
        static void updateTwoHandedGripMode(TwoHandedGripMode mode);
        void openPipboyConfigUI();
        void openWeaponAdjustConfigUI();
        void openExternalModConfig(const OpenExternalModConfigData& data);
        void closeMainConfigMode();

        // configuration UI
        std::shared_ptr<vrui::UIContainer> _configUI;

        std::shared_ptr<BodyAdjustmentSubConfigMode> _bodyAdjustmentSubConfig;

        std::vector<OpenExternalModConfigData> _externalModConfigButtonDataList;
    };
}
