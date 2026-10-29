#include "GripMode.h"

#include <algorithm>
#include <cctype>
#include <string>
#include <string_view>

namespace
{
    /**
     * Read a legacy grip flag of an imported FRIK.ini, which the import keeps as text since FRIK.toml has no such key.
     * Read as SimpleIni's GetBoolValue did: text starting with f, n, 0 or "of" is false, anything else or none is true.
     */
    bool iniFlag(const toml::node_view<const toml::node> section, const std::string_view key)
    {
        auto text = section[key].value_or(std::string());
        std::ranges::transform(text, text.begin(), [](const unsigned char c) {
            return static_cast<char>(std::tolower(c));
        });
        return !text.starts_with('f') && !text.starts_with('n') && !text.starts_with('0') && !text.starts_with("of");
    }
}

namespace frik
{
    /**
     * The grip mode the old menu showed for the three grip flags of an imported FRIK.ini's main section.
     * A combination that is not one of the four modes is mode 3, as the old menu showed it.
     */
    TwoHandedGripMode gripModeFromIniFlags(const toml::node_view<const toml::node> section)
    {
        const bool grab = iniFlag(section, "EnableGripButton");
        const bool hold = iniFlag(section, "EnableGripButtonOnePress");
        const bool release = iniFlag(section, "EnableGripButtonToLetGo");
        if (!grab && !hold) {
            return release ? TwoHandedGripMode::Mode2 : TwoHandedGripMode::Mode1;
        }
        if (grab && !hold && release) {
            return TwoHandedGripMode::Mode4;
        }
        return TwoHandedGripMode::Mode3;
    }
}
