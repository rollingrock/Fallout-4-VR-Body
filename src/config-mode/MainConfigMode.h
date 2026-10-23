#pragma once

#include "BodyAdjustmentSubConfigMode.h"
#include "vrui/UIContainer.h"
#include "weapon-position/GripMode.h"

namespace frik
{
    struct OpenExternalModConfigData
    {
        std::string buttonIconNifPath;
        std::string callbackReceiverName;
        std::uint32_t callbackMessageType;
    };

    class MainConfigMode
    {
    public:
        int isOpen() const;
        void openConfigMode();
        void closeConfigMode();
        void onFrameUpdate();
        bool isBodyAdjustOpen() const;
        void registerOpenExternalModSettingButton(const OpenExternalModConfigData& data);

    private:
        void tryCreateMainConfigUI();
        void createMainConfigUI();
        static std::shared_ptr<vrui::UIContainer> createBalancedButtonRows(const std::string& name, const std::vector<std::shared_ptr<vrui::UIElement>>& buttons);
        void openBodyAdjustmentSubConfigUI();
        static void toggleSelfieMode();
        static void updateTwoHandedGripMode(TwoHandedGripMode mode);
        void openPipboyConfigUI();
        void openWeaponAdjustConfigUI();
        void openExternalModConfig(const OpenExternalModConfigData& data);
        static void openAdvancedConfig();
        static void openHelpWiki();

        // configuration UI
        std::shared_ptr<vrui::UIContainer> _configUI;

        std::shared_ptr<BodyAdjustmentSubConfigMode> _bodyAdjustmentSubConfig;

        std::vector<OpenExternalModConfigData> _externalModConfigButtonDataList;
    };
}
