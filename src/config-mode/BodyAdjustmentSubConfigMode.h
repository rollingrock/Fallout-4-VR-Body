#pragma once

#include "ui/UIContainer.h"
#include "ui/UIToggleGroupContainer.h"

namespace frik
{
    /**
     * What is currently being configured
     */
    enum class BodyAdjustmentConfigTarget : uint8_t
    {
        None = 0,
        BodyHeight,
        BodyForwardOffset,
        BodyArmsLength,
        VRScale,
    };

    class BodyAdjustmentSubConfigMode
    {
    public :
        explicit BodyAdjustmentSubConfigMode(const std::function<void()>& onClose);

        void onFrameUpdate() const;

    private:
        void createConfigUI();
        void togglePlayingSeated(bool seated);
        void toggleHideHeadEquipment(bool hide);
        void closeConfig();
        void handleAdjustment() const;
        static void handleHeightAdjustment();
        static void handleForwardAdjustment();
        static void handleArmsLengthAdjustment();
        static void handleVRScaleAdjustment();
        static void saveConfig();
        void resetConfig() const;

        std::function<void()> _onClose;

        BodyAdjustmentConfigTarget _configTarget = BodyAdjustmentConfigTarget::None;

        // configuration UI
        std::shared_ptr<vrui::UIContainer> _configUI;

        std::shared_ptr<vrui::UIToggleGroupContainer> _row2Container;
        std::shared_ptr<vrui::UIWidget> _noneMsg;
        std::shared_ptr<vrui::UIWidget> _heightMsg;
        std::shared_ptr<vrui::UIWidget> _forwardMsg;
        std::shared_ptr<vrui::UIWidget> _armsLengthMsg;
        std::shared_ptr<vrui::UIWidget> _vrScaleMsg;
    };
}
