#pragma once

namespace frik::devbench
{
    /**
     * Set up FRIK's devbench tool on the framework's f4cf::devbench: its description, its default INI section, the
     * state behind the state action (and the transition events it emits), and the probe action. Call once while the mod
     * loads, before onGameLoaded returns, so the tool is registered with all of it.
     */
    void setupDevBenchTool();
}
