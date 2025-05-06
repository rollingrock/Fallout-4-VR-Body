#include <chrono>
#include <ShlObj.h>  // CSIDL_MYDOCUMENTS
#include <F4SE_common/BranchTrampoline.h>
#include "Config.h"
#include "F4VRBody.h"
#include "GunReload.h"
#include "hook.h"
#include "MenuChecker.h"
#include "patches.h"
#include "SmoothMovementVR.h"
#include "version.h"
#include "common/CommonUtils.h"
#include "common/Logger.h"
#include "f4se/PluginAPI.h"  // SKSEInterface, PluginInfo
#include "f4se_common/f4se_version.h"  // RUNTIME_VERSION
#include "f4vr/VR.h"
#include "ui/UIManager.h"

using namespace common;

void onBetterScopesMessage(F4SEMessagingInterface::Message* msg) {
	if (!msg) {
		return;
	}

	if (msg->type == 15) {
		frik::c_isLookingThroughScope = static_cast<bool>(msg->data);
	}
}

//Listener for F4SE Messages
void onF4SEMessage(F4SEMessagingInterface::Message* msg) {
	if (!msg) {
		return;
	}

	if (msg->type == F4SEMessagingInterface::kMessage_GameLoaded) {
		frik::startUp();
		f4vr::InitVRSystem();
		SmoothMovementVR::startFunctions();
		SmoothMovementVR::MenuOpenCloseHandler::Register();
		Log::info("kMessage_GameLoaded Completed");
	}

	if (msg->type == F4SEMessagingInterface::kMessage_PostLoad) {
		constexpr bool gripConfig = false; // !frik::g_config->staticGripping;
		g_messaging->Dispatch(g_pluginHandle, 15, static_cast<void*>(nullptr), sizeof(bool), "FO4VRBETTERSCOPES");
		g_messaging->RegisterListener(g_pluginHandle, "FO4VRBETTERSCOPES", onBetterScopesMessage);
		Log::info("kMessage_PostLoad Completed");
	}
}

extern "C" {
bool F4SEPlugin_Query(const F4SEInterface* f4se, PluginInfo* info) {
	Sleep(5000);
	Log::init("FRIK.log");

	Log::info("FRIK v%s", FRIK_VERSION_VERSTRING);

	info->infoVersion = PluginInfo::kInfoVersion;
	info->name = "FRIK";
	info->version = FRIK_VERSION_MAJOR;

	if (f4se->isEditor) {
		Log::fatal("[FATAL ERROR] Loaded in editor, marking as incompatible!\n");
		return false;
	}

	f4se->runtimeVersion;
	if (f4se->runtimeVersion < RUNTIME_VR_VERSION_1_2_72) {
		Log::fatal("Unsupported runtime version %s!\n", f4se->runtimeVersion);
		return false;
	}

	return true;
}

bool F4SEPlugin_Load(const F4SEInterface* f4se) {
	try {
		Log::info("FRIK Init - %s", common::getCurrentTimeString().data());

		g_pluginHandle = f4se->GetPluginHandle();

		if (g_pluginHandle == kPluginHandle_Invalid) {
			throw std::exception("Invalid plugin handle");
		}

		g_messaging = static_cast<F4SEMessagingInterface*>(f4se->QueryInterface(kInterface_Messaging));
		g_messaging->RegisterListener(g_pluginHandle, "F4SE", onF4SEMessage);

		if (!g_branchTrampoline.Create(1024 * 128)) {
			throw std::exception("couldn't create branch trampoline");
		}

		const auto moduleHandle = reinterpret_cast<void*>(GetModuleHandleA("FRIK.dll"));
		if (!g_localTrampoline.Create(1024 * 128, moduleHandle)) {
			throw std::exception("couldn't create codegen buffer");
		}

		Log::info("Init config...");
		frik::initConfig();

		Log::info("Init UI Manager...");
		vrui::initUIManager();

		Log::info("Register papyrus functions...");
		g_papyrus = static_cast<F4SEPapyrusInterface*>(f4se->QueryInterface(kInterface_Papyrus));
		if (!g_papyrus->Register(frik::registerPapyrusFunctions)) {
			throw std::exception("FAILED TO REGISTER PAPYRUS FUNCTIONS!!");
		}

		Log::info("Run patches...");
		patches::patchAll();

		Log::info("Init gun reload system...");
		frik::InitGunReloadSystem();

		Log::info("Hook main...");
		hookMain();

		Log::info("FRIK Loaded successfully");
		return true;
	} catch (const std::exception& e) {
		Log::fatal("Fatal error in F4SEPlugin_Load: %s", e.what());
		return false;
	}
}
};
