#pragma once

#include "vrui/BindingPrompt.h"
#include "vrui/UIContainer.h"

namespace frik
{
    std::shared_ptr<vrui::UIContainer> createToggleSelfieMsg(const std::shared_ptr<vrui::UIElement>& matchHeightOf = nullptr);

    /**
     * Text height of the config screens' footers, the panel under the buttons that says what the
     * selected mode does and which controls drive it.
     */
    inline constexpr float CONFIG_FOOTER_TEXT_HEIGHT = 0.22f;

    /**
     * The footers' controller icons, larger than a span's default so the marks on them read.
     */
    inline constexpr vrui::BindingPromptStyle CONFIG_FOOTER_PROMPT_STYLE{ .imageHeight = 3.0f };
}
