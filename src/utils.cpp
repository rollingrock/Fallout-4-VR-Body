#include "utils.h"

#include <f4se/GameMenus.h>
#include <f4se/GameRTTI.h>

#include "FRIK.h"
#include "common/CommonUtils.h"
#include "f4se/PapyrusEvents.h"
#include "f4vr/F4VRUtils.h"
#include "f4vr/PlayerNodes.h"

using namespace common;

namespace frik {
	void turnPlayerRadioOn(bool isActive) {
		CallGlobalFunctionNoWait1<bool>("Game", "TurnPlayerRadioOn", isActive);
	}

	void turnPipBoyOn() {
		Setting* set = GetINISetting("fHMDToPipboyScaleOuterAngle:VRPipboy");
		set->SetDouble(0.0);

		set = GetINISetting("fHMDToPipboyScaleInnerAngle:VRPipboy");
		set->SetDouble(0.0);

		set = GetINISetting("fPipboyScaleOuterAngle:VRPipboy");
		set->SetDouble(0.0);

		set = GetINISetting("fPipboyScaleInnerAngle:VRPipboy");
		set->SetDouble(0.0);
	}

	void turnPipBoyOff() {
		Setting* set = GetINISetting("fHMDToPipboyScaleOuterAngle:VRPipboy");
		set->SetDouble(20.0);

		set = GetINISetting("fHMDToPipboyScaleInnerAngle:VRPipboy");
		set->SetDouble(5.0);

		set = GetINISetting("fPipboyScaleOuterAngle:VRPipboy");
		set->SetDouble(20.0);

		set = GetINISetting("fPipboyScaleInnerAngle:VRPipboy");
		set->SetDouble(5.0);
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

	/**
	 * @return muzzle flash class only if it's fully loaded with fire and projectile nodes.
	 */
	f4vr::MuzzleFlash* getMuzzleFlashNodes() {
		if (const auto equipWeaponData = f4vr::getEquippedWeaponData()) {
			const auto vfunc = reinterpret_cast<uint64_t*>(equipWeaponData);
			if ((*vfunc & 0xFFFF) == (f4vr::EquippedWeaponData_vfunc & 0xFFFF)) {
				const auto muzzle = reinterpret_cast<f4vr::MuzzleFlash*>(equipWeaponData->unk28);
				if (muzzle && muzzle->fireNode && muzzle->projectileNode) {
					return muzzle;
				}
			}
		}
		return nullptr;
	}
}
