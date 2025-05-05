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
#include "VR.h"
#include "common/IDebugLog.h"
#include "f4se/PluginAPI.h"  // SKSEInterface, PluginInfo
#include "f4se_common/f4se_version.h"  // RUNTIME_VERSION
#include "ui/UIManager.h"

void onBetterScopesMessage(F4SEMessagingInterface::Message* msg) {
	if (!msg) {
		return;
	}

	if (msg->type == 15) {
		FRIK::c_isLookingThroughScope = static_cast<bool>(msg->data);
	}
}

//Listener for F4SE Messages
void onF4SEMessage(F4SEMessagingInterface::Message* msg) {
	if (!msg) {
		return;
	}

	if (msg->type == F4SEMessagingInterface::kMessage_GameLoaded) {
		FRIK::startUp();
		VRHook::InitVRSystem();
		SmoothMovementVR::StartFunctions();
		SmoothMovementVR::MenuOpenCloseHandler::Register();
		_MESSAGE("kMessage_GameLoaded Completed");
	}

	if (msg->type == F4SEMessagingInterface::kMessage_PostLoad) {
		constexpr bool gripConfig = false; // !FRIK::g_config->staticGripping;
		g_messaging->Dispatch(g_pluginHandle, 15, static_cast<void*>(nullptr), sizeof(bool), "FO4VRBETTERSCOPES");
		g_messaging->RegisterListener(g_pluginHandle, "FO4VRBETTERSCOPES", onBetterScopesMessage);
		_MESSAGE("kMessage_PostLoad Completed");
	}
}

extern "C" {
bool F4SEPlugin_Query(const F4SEInterface* a_f4se, PluginInfo* a_info) {
	Sleep(5000);
	IDebugLog::OpenRelative(CSIDL_MYDOCUMENTS, R"(\\My Games\\Fallout4VR\\F4SE\\FRIK.log)");
	IDebugLog::SetPrintLevel(IDebugLog::kLevel_Message);
	IDebugLog::SetLogLevel(IDebugLog::kLevel_Message);

	_MESSAGE("FRIK v%s", FRIK_VERSION_VERSTRING);

	a_info->infoVersion = PluginInfo::kInfoVersion;
	a_info->name = "FRIK";
	a_info->version = FRIK_VERSION_MAJOR;

	if (a_f4se->isEditor) {
		_FATALERROR("[FATAL ERROR] Loaded in editor, marking as incompatible!\n");
		return false;
	}

	a_f4se->runtimeVersion;
	if (a_f4se->runtimeVersion < RUNTIME_VR_VERSION_1_2_72) {
		_FATALERROR("Unsupported runtime version %s!\n", a_f4se->runtimeVersion);
		return false;
	}

	return true;
}

bool F4SEPlugin_Load(const F4SEInterface* f4se) {
	try {
		_MESSAGE("FRIK Init - %s", FRIK::getCurrentTimeString().data());

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

		_MESSAGE("Init config...");
		FRIK::initConfig();

		_MESSAGE("Init UI Manager...");
		VRUI::initUIManager();

		_MESSAGE("Register papyrus functions...");
		g_papyrus = static_cast<F4SEPapyrusInterface*>(f4se->QueryInterface(kInterface_Papyrus));
		if (!g_papyrus->Register(FRIK::registerPapyrusFunctions)) {
			throw std::exception("FAILED TO REGISTER PAPYRUS FUNCTIONS!!");
		}

		_MESSAGE("Run patches...");
		patches::patchAll();

		_MESSAGE("Init gun reload system...");
		FRIK::InitGunReloadSystem();

		_MESSAGE("Hook main...");
		hookMain();

		_MESSAGE("FRIK Loaded successfully");
		return true;
	} catch (const std::exception& e) {
		_FATALERROR("Fatal error in F4SEPlugin_Load: %s", e.what());
		return false;
	}
}
};
