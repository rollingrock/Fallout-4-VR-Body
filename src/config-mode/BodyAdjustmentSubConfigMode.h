#pragma once

#include "ui/UIContainer.h"

namespace frik
{
    /**
     * What is currently being configured
     */
    enum class BodyAdjustmentConfigTarget : uint8_t
    {
        BodyHeight = 0,
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
        void closeConfig();
        void handleAdjustment() const;
        static void handleHeightAdjustment();
        static void handleForwardAdjustment();
        static void handleArmsLengthAdjustment();
        static void handleVRScaleAdjustment();
        static void saveConfig();
        void resetConfig() const;

        std::function<void()> _onClose;

        BodyAdjustmentConfigTarget _configTarget = BodyAdjustmentConfigTarget::BodyHeight;

        // configuration UI
        std::shared_ptr<vrui::UIContainer> _configUI;
    };
}
