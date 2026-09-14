#pragma once

#include <string>

namespace frik::devbench
{
    /**
     * Dev-only exerciser of the v2.3 API from inside FRIK, driven by the devbench 'frik' tool
     * (action 'probe'), so the frame phases, hand claims, grips and weapon parent can be tested
     * without a client mod. Runs on the game thread only.
     */
    std::string runProbe(const std::string& argsJson);
}
