#include "common/IDebugLog.h"  // IDebugLog
#include "f4se_common/f4se_version.h"  // RUNTIME_VERSION
#include "f4se/PluginAPI.h"  // SKSEInterface, PluginInfo
#include "f4sE_common/Relocation.h"
#include "F4SE_common/SafeWrite.h"


#include <ShlObj.h>  // CSIDL_MYDOCUMENTS

#include "f4se/GameData.h"

#include "version.h"
#include "hook.h"
#include "F4VRBody.h"
#include "SmoothMovementVR.h"
#include "patches.h"
#include "GunReload.h"
#include "VR.h"



void* g_moduleHandle = nullptr;


uint64_t g_mainLoopCounter = 0;


void PatchBody() {
	_MESSAGE("Patch Body In");
	_MESSAGE("addr = %016I64X", RelocAddr<uintptr_t>(0xF08D5B).GetUIntPtr());
	
	// For new game
	SafeWrite8(RelocAddr<uintptr_t>(0xF08D5B).GetUIntPtr(), 0x74);
	
	// now for existing games to update
	SafeWrite32(RelocAddr<uintptr_t>(0xf29ac8), 0x9090D231);   // This was movzx EDX,R14B.   Want to just zero out EDX with an xor instead
	_MESSAGE("Patch Body Succeeded");
}


void OnBetterScopesMessage(F4SEMessagingInterface::Message* msg) {
	if (msg) {
		if (msg->type == 15) {
			F4VRBody::c_isLookingThroughScope = (bool)msg->data;
		}
	}
}


//Listener for F4SE Messages
void OnF4SEMessage(F4SEMessagingInterface::Message* msg)
{
	if (msg)
	{
		if (msg->type == F4SEMessagingInterface::kMessage_GameLoaded)
		{
			F4VRBody::startUp();
			SmoothMovementVR::StartFunctions();

			SmoothMovementVR::MenuOpenCloseHandler::Register();

			VRHook::InitVRSystem();
			_MESSAGE("kMessage_GameLoaded Completed");

		}
		if (msg->type == F4SEMessagingInterface::kMessage_PostLoad) {
			bool gripConfig = !F4VRBody::c_staticGripping;
			g_messaging->Dispatch(g_pluginHandle, 15, (void*) gripConfig, sizeof(bool), "FO4VRBETTERSCOPES");

			g_messaging->RegisterListener(g_pluginHandle, "FO4VRBETTERSCOPES", OnBetterScopesMessage);
		}
	}
}

extern "C" {
	bool F4SEPlugin_Query(const F4SEInterface* a_f4se, PluginInfo* a_info)
	{
		Sleep(5000);
		gLog.OpenRelative(CSIDL_MYDOCUMENTS, R"(\\My Games\\Fallout4VR\\F4SE\\Fallout4VRBody.log)");
		gLog.SetPrintLevel(IDebugLog::kLevel_DebugMessage);
		gLog.SetLogLevel(IDebugLog::kLevel_DebugMessage);

		g_moduleHandle = reinterpret_cast<void*>(GetModuleHandleA("FRIK.dll"));

		_MESSAGE("F4VRBODY v%s", F4VRBODY_VERSION_VERSTRING);

		a_info->infoVersion = PluginInfo::kInfoVersion;
		a_info->name = "F4VRBody";
		a_info->version = F4VRBODY_VERSION_MAJOR;

		if (a_f4se->isEditor) {
			_FATALERROR("[FATAL ERROR] Loaded in editor, marking as incompatible!\n");
			return false;
		}

		a_f4se->runtimeVersion;
		if (a_f4se->runtimeVersion < RUNTIME_VR_VERSION_1_2_72)
		{
			_FATALERROR("Unsupported runtime version %s!\n", a_f4se->runtimeVersion);
			return false;
		}

		return true;
	}


	bool F4SEPlugin_Load(const F4SEInterface* a_f4se)
	{
		_MESSAGE("F4VRBody Init");

		g_pluginHandle = a_f4se->GetPluginHandle();

		if (g_pluginHandle == kPluginHandle_Invalid) {
			return false;
		}

		g_messaging = (F4SEMessagingInterface*)a_f4se->QueryInterface(kInterface_Messaging);
		g_messaging->RegisterListener(g_pluginHandle, "F4SE", OnF4SEMessage);

		if (!g_branchTrampoline.Create(1024 * 128))
		{
			_ERROR("couldn't create branch trampoline. this is fatal. skipping remainder of init process.");
			return false;
		}

		if (!g_localTrampoline.Create(1024 * 128, g_moduleHandle))
		{
			_ERROR("couldn't create codegen buffer. this is fatal. skipping remainder of init process.");

			return false;
		}

		if (!F4VRBody::loadConfig()) {
			_ERROR("could not open ini config file");
			return false;
		}

		g_papyrus = (F4SEPapyrusInterface*)a_f4se->QueryInterface(kInterface_Papyrus);


		_MESSAGE("register papyrus funcs");

		if (!g_papyrus->Register(F4VRBody::RegisterFuncs)) {
			_MESSAGE("FAILED TO REGISTER PAPYRUS FUNCTIONS!!");
			return false;
		}

		PatchBody();
		
		if (!patches::patchAll()) {
			_MESSAGE("error loading misc patches");
			return false;
		}

		F4VRBody::InitGunReloadSystem();
		hookMain();

		_MESSAGE("F4VRBody Loaded");

		return true;
	}
};
