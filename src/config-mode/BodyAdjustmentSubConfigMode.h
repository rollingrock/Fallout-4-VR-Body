#pragma once

#include "vrui/UIButtonPanel.h"
#include "vrui/UIContainer.h"
#include "vrui/UITextPanel.h"
#include "vrui/UIToggleButtonPanel.h"
#include "vrui/UIToggleGroupContainer.h"

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
    public:
        explicit BodyAdjustmentSubConfigMode(const std::function<void()>& onClose);

        ~BodyAdjustmentSubConfigMode();

        void onFrameUpdate();

        static void updateLegSlack(const float skeletonLegSlack)
        {
            _skeletonLegSlack = skeletonLegSlack;
        };

    private:
        /**
         * A value the height adjustment changes, as last seen, and when it last changed, so the values
         * panel can highlight the ones that are moving.
         */
        struct TrackedValue
        {
            float value = 0;
            uint64_t changedTime = 0;
        };

        void createConfigUI();
        void trackHeightValues();
        void updateHeightValuesPanelVisibility() const;
        static vrui::TextRow heightValueRow(std::string_view label, const TrackedValue& value);
        void clearConfigTarget();
        void togglePlayingSeated(bool seated);
        void toggleHideHeadEquipment(bool hide);
        void closeConfig();
        void handleAdjustment() const;
        void appendFooterRows(std::vector<vrui::TextRow>& rows) const;
        static void handleHeightAdjustment();
        static void handleForwardAdjustment();
        static void handleArmsLengthAdjustment();
        static void handleVRScaleAdjustment();
        void saveConfig();
        void resetConfig();

        // roundabout way to recalculate the leg slack adjust offset by knowing the skeleton leg slack value
        inline static float _skeletonLegSlack;

        std::function<void()> _onClose;

        BodyAdjustmentConfigTarget _configTarget = BodyAdjustmentConfigTarget::None;

        // configuration UI
        std::shared_ptr<vrui::UIContainer> _configUI;

        std::shared_ptr<vrui::UIToggleGroupContainer> _row2Container;
        std::shared_ptr<vrui::UIButtonPanel> _saveBtn;
        std::shared_ptr<vrui::UIButtonPanel> _resetBtn;
        std::shared_ptr<vrui::UITextPanel> _heightValuesPanel;
        std::array<std::shared_ptr<vrui::UIToggleButtonPanel>, 3> _nonHeightTargetBtns;

        // the three values the height adjustment changes, for the values panel
        TrackedValue _hmdOffsetUp;
        TrackedValue _bodyOffsetUp;
        TrackedValue _legSlackAdjustOffset;
    };
}
