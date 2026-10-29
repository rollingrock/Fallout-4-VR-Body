#include "ConfigUIStyle.h"

#include "Config.h"
#include "vrui/UITextPanel.h"

using namespace vrui;

namespace
{
    /**
     * A boxed container as tall as another element beside it: the box's top and bottom padding grow to
     * make up the difference, keeping the elements centred, but never shrink below the style's own.
     */
    class HeightMatchingBox : public UIContainer
    {
    public:
        HeightMatchingBox(const std::string& name, const UIPanelStyle& boxStyle, std::weak_ptr<UIElement> matchHeightOf)
            : UIContainer(name, UIContainerLayout::HorizontalCenter, 0.3f),
              _baseStyle(boxStyle),
              _matchHeightOf(std::move(matchHeightOf))
        {
            setBoxStyle(boxStyle);
        }

        /**
         * Set the padding before the base layout puts the box around the elements. The element matched
         * has to be laid out earlier in the frame (added to the shared row before this box), and the
         * elements' own height is last frame's, which is enough for content that does not change size.
         */
        void onLayoutUpdate(UIFrameUpdateContext* context) override
        {
            if (const auto other = _matchHeightOf.lock(); other && other->calcVisibility()) {
                float contentHeight = 0;
                for (const auto& child : childElements()) {
                    if (child->calcVisibility()) {
                        contentHeight = (std::max)(contentHeight, child->calcSize().height);
                    }
                }
                if (contentHeight > 0) {
                    const auto missing = (other->calcSize().height - contentHeight) / calcScale() - 2 * (_baseStyle.borderThicknessUnits + _baseStyle.padding.top);
                    const auto extra = (std::max)(0.0f, missing / 2);
                    if (!common::fEqual(extra, _extraPadding)) {
                        _extraPadding = extra;
                        auto style = _baseStyle;
                        style.padding.top += extra;
                        style.padding.bottom += extra;
                        setBoxStyle(style);
                    }
                }
            }
            UIContainer::onLayoutUpdate(context);
        }

    private:
        UIPanelStyle _baseStyle;
        std::weak_ptr<UIElement> _matchHeightOf;
        float _extraPadding = 0;
    };
}

namespace frik
{
    /**
     * The box telling how to toggle selfie mode, shown beside a config screen's footer: the label on two
     * lines, and the icon of the configured binding to its right, centred on them. Given the footer, the
     * box is as tall as it; the footer must come before the box in their row.
     */
    std::shared_ptr<UIContainer> createToggleSelfieMsg(const std::shared_ptr<UIElement>& matchHeightOf)
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

        const auto box = std::make_shared<HeightMatchingBox>("SelfieMsg", F4VR_PANEL_STYLE, matchHeightOf);
        box->addElement(label);
        box->addElement(button);
        return box;
    }
}
