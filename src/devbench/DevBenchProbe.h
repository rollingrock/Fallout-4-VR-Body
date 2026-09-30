#pragma once

#include <nlohmann/json.hpp>

namespace frik::devbench
{
    /**
     * Dev-only exerciser of FRIK's API from inside FRIK, run as the devbench tool's probe action, with "op" choosing
     * what it does, so the frame phases, hand claims, grips and weapon parent can be tested without a client mod.
     * Game thread only.
     */
    nlohmann::json runProbe(const nlohmann::json& args);
}
