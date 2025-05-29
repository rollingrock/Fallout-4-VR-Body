#include <chrono>
#include <ShlObj.h>  // CSIDL_MYDOCUMENTS
#include <F4SE_common/BranchTrampoline.h>

#include "FRIK.h"
#include "hook.h"
#include "patches.h"
#include "version.h"
#include "common/Logger.h"
#include "f4se/PluginAPI.h"  // SKSEInterface, PluginInfo
#include "f4se_common/f4se_version.h"  // RUNTIME_VERSION

using namespace common;

extern "C" {
bool F4SEPlugin_Query(const F4SEInterface* f4se, PluginInfo* info) {
	Log::init("FRIK.log");
	Log::info("FRIK v%s", FRIK_VERSION_VERSTRING);

	info->infoVersion = PluginInfo::kInfoVersion;
	info->name = "F4VRBody";
	info->version = FRIK_VERSION_MAJOR;

	if (f4se->isEditor) {
		Log::fatal("[FATAL ERROR] Loaded in editor, marking as incompatible!");
		return false;
	}

	if (f4se->runtimeVersion < RUNTIME_VR_VERSION_1_2_72) {
		Log::fatal("Unsupported runtime version %s!", f4se->runtimeVersion);
		return false;
	}

	return true;
}

bool F4SEPlugin_Load(const F4SEInterface* f4se) {
	try {
		Log::info("FRIK Init - %s", getCurrentTimeString().data());

		constexpr size_t LEN = 1024ULL * 128;
		if (!g_branchTrampoline.Create(LEN)) {
			throw std::exception("couldn't create branch trampoline");
		}

		const auto moduleHandle = reinterpret_cast<void*>(GetModuleHandleA("FRIK.dll"));
		if (!g_localTrampoline.Create(LEN, moduleHandle)) {
			throw std::exception("couldn't create codegen buffer");
		}

		Log::info("Run patches...");
		patches::patchAll();

		Log::info("Hook main...");
		hookMain();

		Log::info("FRIK plugin loaded...");
		frik::g_frik.initialize(f4se);

		Log::info("FRIK Loaded successfully");
		return true;
	} catch (const std::exception& e) {
		Log::fatal("Fatal error in F4SEPlugin_Load: %s", e.what());
		return false;
	}
}
};
