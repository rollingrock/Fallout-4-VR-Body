#include "common/IDebugLog.h"  // IDebugLog
#include "f4se_common/f4se_version.h"  // RUNTIME_VERSION
#include "f4se/PluginAPI.h"  // SKSEInterface, PluginInfo
#include "f4sE_common/Relocation.h"
#include "F4SE_common/SafeWrite.h"


#include <ShlObj.h>  // CSIDL_MYDOCUMENTS

#include "f4se/GameData.h"

#include "version.h"

static PluginHandle g_pluginHandle = kPluginHandle_Invalid;

void PatchBody() {
	_MESSAGE("Patch Body In");
	SafeWrite8(RelocAddr<uintptr_t>(0xF08D5B).GetUIntPtr(), 0x74);
	_MESSAGE("Patch Body Succeeded");
}


extern "C" {
	bool F4SEPlugin_Query(const F4SEInterface* a_f4se, PluginInfo* a_info)
	{
		gLog.OpenRelative(CSIDL_MYDOCUMENTS, R"(\\My Games\\Fallout4VR\\F4SE\\Fallout4VRBody.log)");
		gLog.SetPrintLevel(IDebugLog::kLevel_DebugMessage);
		gLog.SetLogLevel(IDebugLog::kLevel_DebugMessage);


		_MESSAGE("F4VRBODY v%s", F4VRBODY_VERSION_VERSTRING);

		a_info->infoVersion = PluginInfo::kInfoVersion;
		a_info->name = "F4VRBody";
		a_info->version = F4VRBODY_VERSION_MAJOR;

		if (a_f4se->isEditor) {
			_FATALERROR("[FATAL ERROR] Loaded in editor, marking as incompatible!\n");
			return false;
		}

		a_f4se->runtimeVersion;
		if (a_f4se->runtimeVersion <= RUNTIME_VR_VERSION_1_2_72)
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

		//if (config::LoadConfig(R"(.\Data\F4SE\plugins\F4VRBody.ini)"))
		//{
		//	_MESSAGE("loaded config successfully");
		//}
		//else
		//{
		//	_MESSAGE("config load failed, using default config");
		//}

		PatchBody();

		_MESSAGE("F4VRBody Loaded");

		return true;
	}
};
