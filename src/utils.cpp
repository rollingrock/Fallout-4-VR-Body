#include "utils.h"
#include <filesystem>
#include <shlobj_core.h>
#include <f4se/GameMenus.h>
#include <f4se/GameRTTI.h>

#include "Config.h"
#include "common/CommonUtils.h"
#include "f4se/PapyrusEvents.h"
#include "f4vr/F4VRUtils.h"

using namespace common;

namespace frik {
	void turnPlayerRadioOn(bool isActive) {
		CallGlobalFunctionNoWait1<bool>("Game", "TurnPlayerRadioOn", isActive);
	}

	void configureGameVars() {
		f4vr::setINIFloat("fPipboyMaxScale:VRPipboy", 3.0000);
		f4vr::setINIFloat("fPipboyMinScale:VRPipboy", 0.0100f);
		f4vr::setINIFloat("fVrPowerArmorScaleMultiplier:VR", 1.0000);
	}

	void windowFocus() {
		common::windowFocus("Fallout4VR");
	}

	void simulateExtendedButtonPress(WORD vkey) {
		if (auto hwnd = ::FindWindowEx(nullptr, nullptr, "Fallout4VR", nullptr)) {
			HWND foreground = GetForegroundWindow();
			if (foreground && hwnd == foreground) {
				INPUT input;
				input.type = INPUT_KEYBOARD;
				input.ki.wScan = MapVirtualKey(vkey, MAPVK_VK_TO_VSC);
				input.ki.time = 0;
				input.ki.dwExtraInfo = 0;
				input.ki.wVk = vkey;
				if (vkey == VK_UP || vkey == VK_DOWN) {
					input.ki.dwFlags = KEYEVENTF_EXTENDEDKEY; //0; //KEYEVENTF_KEYDOWN
				} else {
					input.ki.dwFlags = 0;
				}
				SendInput(1, &input, sizeof(INPUT));
				Sleep(30);
				if (vkey == VK_UP || vkey == VK_DOWN) {
					input.ki.dwFlags = KEYEVENTF_KEYUP | KEYEVENTF_EXTENDEDKEY;
				} else {
					input.ki.dwFlags = KEYEVENTF_KEYUP;
				}
				SendInput(1, &input, sizeof(INPUT));
			}
		}
	}

	void turnPipBoyOn() {
		/*  From IdleHands
			Utility.SetINIFloat("fHMDToPipboyScaleOuterAngle:VRPipboy", 0.0000)
			Utility.SetINIFloat("fHMDToPipboyScaleInnerAngle:VRPipboy", 0.0000)
			Utility.SetINIFloat("fPipboyScaleOuterAngle:VRPipboy", 0.0000)
			Utility.SetINIFloat("fPipboyScaleInnerAngle:VRPipboy", 0.0000)
		*/
		Setting* set = GetINISetting("fHMDToPipboyScaleOuterAngle:VRPipboy");
		set->SetDouble(0.0);

		set = GetINISetting("fHMDToPipboyScaleInnerAngle:VRPipboy");
		set->SetDouble(0.0);

		set = GetINISetting("fPipboyScaleOuterAngle:VRPipboy");
		set->SetDouble(0.0);

		set = GetINISetting("fPipboyScaleInnerAngle:VRPipboy");
		set->SetDouble(0.0);

		if (g_config.autoFocusWindow && g_config.switchUIControltoPrimary) {
			windowFocus();
		}
	}

	void turnPipBoyOff() {
		/*  From IdleHands
	Utility.SetINIFloat("fHMDToPipboyScaleOuterAngle:VRPipboy", 20.0000)
	Utility.SetINIFloat("fHMDToPipboyScaleInnerAngle:VRPipboy", 5.0000)
	Utility.SetINIFloat("fPipboyScaleOuterAngle:VRPipboy", 20.0000)
	Utility.SetINIFloat("fPipboyScaleInnerAngle:VRPipboy", 5.0000)
		*/
		Setting* set = GetINISetting("fHMDToPipboyScaleOuterAngle:VRPipboy");
		set->SetDouble(20.0);

		set = GetINISetting("fHMDToPipboyScaleInnerAngle:VRPipboy");
		set->SetDouble(5.0);

		set = GetINISetting("fPipboyScaleOuterAngle:VRPipboy");
		set->SetDouble(20.0);

		set = GetINISetting("fPipboyScaleInnerAngle:VRPipboy");
		set->SetDouble(5.0);

		f4vr::setControlsThumbstickEnableState(true);
	}

	/**
	 * Check if ANY pipboy open by checking if pipboy menu can be found in the UI.
	 * Returns true for wrist, in-front, and projected pipboy.
	 */
	bool isAnyPipboyOpen() {
		BSFixedString pipboyMenu("PipboyMenu");
		return (*g_ui)->GetMenu(pipboyMenu) != nullptr;
	}

	// Function to check if the camera is looking at the object and the object is facing the camera
	bool isCameraLookingAtObject(const NiAVObject* cameraNode, const NiAVObject* objectNode, const float detectThresh) {
		return common::isCameraLookingAtObject(cameraNode->m_worldTransform, objectNode->m_worldTransform, detectThresh);
	}

	/**
	 * detect if the player has an armor item which uses the headlamp equipped as not to overwrite it
	 */
	bool isArmorHasHeadLamp() {
		if (const auto equippedItem = (*g_player)->equipData->slots[0].item) {
			if (const auto torchEnabledArmor = DYNAMIC_CAST(equippedItem, TESForm, TESObjectARMO)) {
				return f4vr::hasKeyword(torchEnabledArmor, 0xB34A6);
			}
		}
		return false;
	}

	/**
	 * @return true if BetterScopesVR mod is loaded in the game, false otherwise.
	 */
	bool isBetterScopesVRModLoaded() {
		DataHandler* dataHandler = *g_dataHandler;
		const auto mod = dataHandler ? dataHandler->LookupModByName("3dscopes-replacer.esp") : nullptr;
		return mod != nullptr;
	}
}
