#include "utils.h"
#include <filesystem>
#include <fstream>
#include <shlobj_core.h>
#include <sstream>
#include <f4se/GameMenus.h>
#include "Config.h"
#include "common/CommonUtils.h"
#include "common/matrix.h"
#include "f4se/PapyrusEvents.h"
#include "f4vr/F4VRUtils.h"

using namespace common;

namespace frik {
	RelocAddr<_AIProcess_ClearMuzzleFlashes> AIProcess_ClearMuzzleFlashes(0xecc710);
	RelocAddr<_AIProcess_CreateMuzzleFlash> AIProcess_CreateMuzzleFlash(0xecc570);

	using _SettingCollectionList_GetPtr = Setting* (*)(SettingCollectionList* list, const char* name);
	RelocAddr<_SettingCollectionList_GetPtr> SettingCollectionList_GetPtr(0x501500);

	void updateTransforms(NiNode* node) {
		if (!node->m_parent) {
			return;
		}

		const auto& parentTransform = node->m_parent->m_worldTransform;
		const auto& localTransform = node->m_localTransform;

		// Calculate world position
		const NiPoint3 pos = parentTransform.rot * (localTransform.pos * parentTransform.scale);
		node->m_worldTransform.pos = parentTransform.pos + pos;

		// Calculate world rotation
		common::Matrix44 loc;
		loc.makeTransformMatrix(localTransform.rot, NiPoint3(0, 0, 0));
		node->m_worldTransform.rot = loc.multiply43Left(parentTransform.rot);

		// Calculate world scale
		node->m_worldTransform.scale = parentTransform.scale * localTransform.scale;
	}

	void updateTransformsDown(NiNode* nde, const bool updateSelf) {
		if (updateSelf) {
			updateTransforms(nde);
		}

		for (UInt16 i = 0; i < nde->m_children.m_emptyRunStart; ++i) {
			if (const auto nextNode = nde->m_children.m_data[i] ? nde->m_children.m_data[i]->GetAsNiNode() : nullptr) {
				updateTransformsDown(nextNode, true);
			} else if (const auto triNode = nde->m_children.m_data[i] ? nde->m_children.m_data[i]->GetAsBSTriShape() : nullptr) {
				updateTransforms(reinterpret_cast<NiNode*>(triNode));
			}
		}
	}

	void toggleVis(NiNode* nde, const bool hide, const bool updateSelf) {
		if (updateSelf) {
			nde->flags = hide ? nde->flags | 0x1 : nde->flags & ~0x1;
		}

		for (UInt16 i = 0; i < nde->m_children.m_emptyRunStart; ++i) {
			if (const auto nextNode = nde->m_children.m_data[i] ? nde->m_children.m_data[i]->GetAsNiNode() : nullptr) {
				toggleVis(nextNode, hide, true);
			}
		}
	}

	void showMessagebox(const std::string& asText) {
		CallGlobalFunctionNoWait1<BSFixedString>("Debug", "Messagebox", BSFixedString(asText.c_str()));
	}

	void showNotification(const std::string& asText) {
		CallGlobalFunctionNoWait1<BSFixedString>("Debug", "Notification", BSFixedString(asText.c_str()));
	}

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

	/**
	 * Return true if the node is visible, false if it is hidden or null.
	 */
	bool isNodeVisible(const NiNode* node) {
		return node && !(node->flags & 0x1);
	}

	/**
	 * Update the node flags to show/hide it.
	 */
	void showHideNode(NiAVObject* node, const bool toHide) {
		if (toHide) {
			node->flags |= 0x1; // hide
		} else {
			node->flags &= 0xfffffffffffffffe; // show
		}
	}

	// Function to check if the camera is looking at the object and the object is facing the camera
	bool isCameraLookingAtObject(const NiAVObject* cameraNode, const NiAVObject* objectNode, const float detectThresh) {
		// Get the position of the camera and the object
		const NiPoint3 cameraPos = cameraNode->m_worldTransform.pos;
		const NiPoint3 objectPos = objectNode->m_worldTransform.pos;

		// Calculate the direction vector from the camera to the object
		const NiPoint3 direction = vec3Norm(NiPoint3(objectPos.x - cameraPos.x, objectPos.y - cameraPos.y, objectPos.z - cameraPos.z));

		// Get the forward vector of the camera (assuming it's the y-axis)
		const NiPoint3 cameraForward = vec3Norm(cameraNode->m_worldTransform.rot * NiPoint3(0, 1, 0));

		// Get the forward vector of the object (assuming it's the y-axis)
		const NiPoint3 objectForward = vec3Norm(objectNode->m_worldTransform.rot * NiPoint3(0, 1, 0));

		// Check if the camera is looking at the object
		const float cameraDot = vec3Dot(cameraForward, direction);
		const bool isCameraLooking = cameraDot > detectThresh; // Adjust the threshold as needed

		// Check if the object is facing the camera
		const float objectDot = vec3Dot(objectForward, direction);
		const bool isObjectFacing = objectDot > detectThresh; // Adjust the threshold as needed

		return isCameraLooking && isObjectFacing;
	}

	std::string getEquippedWeaponName() {
		const auto* equipData = (*g_player)->middleProcess->unk08->equipData;
		return equipData ? equipData->item->GetFullName() : "";
	}

	/**
	 * @return true if the equipped weapon is a melee weapon type.
	 */
	bool isMeleeWeaponEquipped() {
		if (!Offsets::CombatUtilities_IsActorUsingMelee(*g_player)) {
			return false;
		}
		const auto* inventory = (*g_player)->inventoryList;
		if (!inventory) {
			return false;
		}
		for (UInt32 i = 0; i < inventory->items.count; i++) {
			BGSInventoryItem item;
			inventory->items.GetNthItem(i, item);
			if (item.form && item.form->formType == kFormType_WEAP && item.stack->flags & 0x3) {
				return true;
			}
		}
		return false;
	}

	bool getLeftHandedMode() {
		const Setting* set = GetINISetting("bLeftHandedMode:VR");

		return set->data.u8;
	}

	NiNode* getChildNode(const char* nodeName, NiNode* nde) {
		if (!nde->m_name) {
			return nullptr;
		}

		if (!_stricmp(nodeName, nde->m_name.c_str())) {
			return nde;
		}

		for (UInt16 i = 0; i < nde->m_children.m_emptyRunStart; ++i) {
			if (const auto nextNode = nde->m_children.m_data[i] ? nde->m_children.m_data[i]->GetAsNiNode() : nullptr) {
				if (const auto ret = getChildNode(nodeName, nextNode)) {
					return ret;
				}
			}
		}

		return nullptr;
	}

	NiNode* get1StChildNode(const char* nodeName, const NiNode* nde) {
		for (UInt16 i = 0; i < nde->m_children.m_emptyRunStart; ++i) {
			if (const auto nextNode = nde->m_children.m_data[i] ? nde->m_children.m_data[i]->GetAsNiNode() : nullptr) {
				if (!_stricmp(nodeName, nextNode->m_name.c_str())) {
					return nextNode;
				}
			}
		}
		return nullptr;
	}

	Setting* getINISettingNative(const char* name) {
		Setting* setting = SettingCollectionList_GetPtr(*g_iniSettings, name);
		if (!setting) {
			setting = SettingCollectionList_GetPtr(*g_iniPrefSettings, name);
		}

		return setting;
	}

	/**
	 * @return true if BetterScopesVR mod is loaded in the game, false otherwise.
	 */
	bool isBetterScopesVRModLoaded() {
		return isModLoaded("FO4VRBETTERSCOPES");
	}

	/**
	 * @return true if a mod by specific name is loaded in the game, false otherwise.
	 */
	bool isModLoaded(const char* modName) {
		const DataHandler* dataHandler = *g_dataHandler;
		if (!dataHandler) {
			return false;
		}
		for (auto i = 0; i < dataHandler->modList.loadedModCount; ++i) {
			const auto modInfo = dataHandler->modList.loadedMods[i];
			if (_stricmp(modInfo->name, modName) == 0) {
				return true;
			}
		}
		return false;
	}
}
