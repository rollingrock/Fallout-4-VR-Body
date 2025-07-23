#pragma once

namespace frik
{
    enum class PipboyOperation : std::uint8_t
    {
        GOTO_PREV_PAGE = 0,
        GOTO_NEXT_PAGE,
        GOTO_PREV_TAB,
        GOTO_NEXT_TAB,
        MOVE_LIST_SELECTION_UP,
        MOVE_LIST_SELECTION_DOWN,
        PRIMARY_PRESS
    };

    enum class PipboyPage : std::uint8_t
    {
        STATUS = 0,
        INVENTORY,
        UESTS,
        MAP,
        RADIO,
    };
}
