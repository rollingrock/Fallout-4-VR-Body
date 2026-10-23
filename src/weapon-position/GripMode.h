#pragma once

#include <cstdint>
#include <toml++/toml.hpp>

namespace frik
{
    // Persisted values match the mode numbers shown by both configuration menus.
    enum class TwoHandedGripMode : std::uint8_t
    {
        Mode1 = 1,
        Mode2,
        Mode3,
        Mode4,
    };

    TwoHandedGripMode gripModeFromIniFlags(toml::node_view<const toml::node> section);
}
