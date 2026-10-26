#include "ConfigUIStyle.h"

#include "Config.h"
#include "vrui/UITextPanel.h"

using namespace vrui;

namespace frik
{
    /**
     * The box telling how to toggle selfie mode, shown beside a config screen's footer: the label on two
     * lines, and the icon of the configured binding to its right, centred on them.
     */
    std::shared_ptr<UIContainer> createToggleSelfieMsg()
    {
        const auto label = std::make_shared<UITextPanel>("Label");
        label->setStyle(UIPanelStyle{ .color = F4VR_PANEL_STYLE.color, .padding = UIPadding::all(0.0f) });
        label->setTextHeight(CONFIG_FOOTER_TEXT_HEIGHT);
        label->setContent([](std::vector<TextRow>& rows) {
            rows.emplace_back("TOGGLE");
            rows.emplace_back("SELFIE");
        });

        const auto button = std::make_shared<UITextPanel>("Button");
        button->setStyle(UIPanelStyle{ .color = F4VR_PANEL_STYLE.color, .padding = UIPadding::all(0.0f) });
        button->setTextHeight(CONFIG_FOOTER_TEXT_HEIGHT);
        button->setContent([](std::vector<TextRow>& rows) {
            rows.push_back({});
            appendBindingPrompt(rows.back().spans, g_config.toggleSelfieBinding, CONFIG_FOOTER_PROMPT_STYLE);
        });

        const auto box = std::make_shared<UIContainer>("SelfieMsg", UIContainerLayout::HorizontalCenter, 0.3f);
        box->setBoxStyle(F4VR_PANEL_STYLE);
        box->addElement(label);
        box->addElement(button);
        return box;
    }
}
