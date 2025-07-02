#include <Version.h>

#include "FRIK.h"
#include "hook.h"
#include "patches.h"
#include "common/Logger.h"

using namespace common;

namespace
{
    void logPluginGameStart()
    {
        const auto game = REL::Module::IsVR() ? "Fallout4VR" : "Fallout4";
        const auto runtimeVer = REL::Module::get().version();
        logger::info("Starting '{}' v{} ; {} v{} ; {} at {} ; BaseAddress: 0x{:X}", Version::PROJECT, Version::NAME, game, runtimeVer.string(), __DATE__, __TIME__,
            REL::Module::get().base());
    }
}

extern "C" DLLEXPORT bool F4SEAPI F4SEPlugin_Query(const F4SE::QueryInterface* a_skse, F4SE::PluginInfo* a_info)
{
    a_info->infoVersion = F4SE::PluginInfo::kVersion;
    a_info->name = "F4VRBody"; // backward compatibility name
    a_info->version = Version::MAJOR;

    if (a_skse->IsEditor()) {
        logger::critical("Loaded in editor, marking as incompatible");
        return false;
    }

    const auto ver = a_skse->RuntimeVersion();
    if (ver < (REL::Module::IsF4() ? F4SE::RUNTIME_LATEST : F4SE::RUNTIME_LATEST_VR)) {
        logger::critical("Unsupported runtime version {}", ver.string());
        return false;
    }

    return true;
}

extern "C" DLLEXPORT bool F4SEAPI F4SEPlugin_Load(const F4SE::LoadInterface* a_f4se)
{
    try {
        logger::init(Version::PROJECT);
        logPluginGameStart();

        F4SE::Init(a_f4se, false);

        // TODO: commonlibf4 migration (probably not needed anymore)
        // constexpr size_t LEN = 1024ULL * 128;
        // if (!g_branchTrampoline.Create(LEN)) {
        //     throw std::exception("couldn't create branch trampoline");
        // }
        //
        // const auto moduleHandle = reinterpret_cast<void*>(GetModuleHandleA("FRIK.dll"));
        // if (!g_localTrampoline.Create(LEN, moduleHandle)) {
        //     throw std::exception("couldn't create codegen buffer");
        // }

        logger::info("Run patches...");
        patches::patchAll();

        logger::info("Hook main...");
        hookMain();

        logger::info("FRIK plugin loaded...");
        frik::g_frik.initialize(a_f4se);

        logger::info("FRIK Loaded successfully");
        return true;
    } catch (const std::exception& ex) {
        logger::error("Unhandled exception: {}", ex.what());
        return false;
    }
}
