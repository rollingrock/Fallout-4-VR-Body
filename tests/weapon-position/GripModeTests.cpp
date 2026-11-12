#include <array>
#include <catch2/catch_test_macros.hpp>
#include <string>

#include "weapon-position/GripMode.h"

using frik::TwoHandedGripMode;

TEST_CASE("Grip mode: an imported INI's flags retain each of the four menu modes")
{
    const std::array<std::string, 4> flags{ "EnableGripButton = 'False'\nEnableGripButtonOnePress = '0'\nEnableGripButtonToLetGo = 'off'",
        "EnableGripButton = 'no'\nEnableGripButtonOnePress = 'false'\nEnableGripButtonToLetGo = 'True'",
        "EnableGripButton = 'yes'\nEnableGripButtonOnePress = '1'\nEnableGripButtonToLetGo = 'No'",
        "EnableGripButton = 'On'\nEnableGripButtonOnePress = 'false'\nEnableGripButtonToLetGo = 'true'" };
    for (int i = 0; i < 4; ++i) {
        CAPTURE(i);
        const auto config = toml::parse("[Main]\n" + flags[i]);
        CHECK(static_cast<int>(frik::gripModeFromIniFlags(config["Main"])) == i + 1);
    }
}

TEST_CASE("Grip mode: an imported INI's abbreviated flags are read as SimpleIni read them")
{
    const auto config = toml::parse("[Main]\nEnableGripButton = 'F'\nEnableGripButtonOnePress = 'N'\nEnableGripButtonToLetGo = 'Y'\n");
    CHECK(frik::gripModeFromIniFlags(config["Main"]) == TwoHandedGripMode::Mode2);
}

TEST_CASE("Grip mode: missing and unrecognized INI flag combinations use hold to grip")
{
    const std::array<std::string, 5> inputs{ "",
        "EnableGripButton = 'false'\nEnableGripButtonOnePress = 'true'\nEnableGripButtonToLetGo = 'false'",
        "EnableGripButton = 'false'\nEnableGripButtonOnePress = 'true'\nEnableGripButtonToLetGo = 'true'",
        "EnableGripButton = 'true'\nEnableGripButtonOnePress = 'false'\nEnableGripButtonToLetGo = 'false'",
        "EnableGripButton = 'true'\nEnableGripButtonOnePress = 'true'\nEnableGripButtonToLetGo = 'true'" };
    for (const auto& input : inputs) {
        CAPTURE(input);
        const auto config = toml::parse("[Main]\n" + input);
        CHECK(frik::gripModeFromIniFlags(config["Main"]) == TwoHandedGripMode::Mode3);
    }
}
