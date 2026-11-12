#include <catch2/catch_test_macros.hpp>

#include "Config.h"
#include "common/CommonUtils.h"

// FRIK's config as it ships: data/config/FRIK.toml, embedded in this executable as in FRIK's DLL.

TEST_CASE("Config: the shipped config loads with its values")
{
    frik::Config config;

    // the embedded default alone, with no file in the user's Documents read or written
    config.loadEmbeddedDefaultOnly();

    // values the shipped file sets away from the hardcoded fallbacks of Config::loadConfigInternal, so they come from the file
    REQUIRE(f4cf::common::fEqual(config.fVrScale, 71.0f));
    REQUIRE(f4cf::common::fEqual(config.armLength, 36.0f));
    REQUIRE(config.twoHandedGripMode == frik::TwoHandedGripMode::Mode3);
}
