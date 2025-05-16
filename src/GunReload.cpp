#include "GunReload.h"

#include "Config.h"
#include "F4VRBody.h"
#include "common/CommonUtils.h"
#include "f4se/GameExtraData.h"
#include "f4vr/F4VROffsets.h"
#include "f4vr/MiscStructs.h"
#include "f4vr/VRControllersManager.h"

using namespace common;

namespace frik {
	GunReload* g_gunReloadSystem = nullptr;

	float g_animDeltaTime = -1.0f;

	void GunReload::DoAnimationCapture() const {
		if (!startAnimCap) {
			g_animDeltaTime = -1.0f;
			return;
		}

		const auto elapsed = nowMillis() - startCapTime;
		if (elapsed > 300) {
			if (elapsed > 2000) {
				g_animDeltaTime = -1.0f;
				f4vr::TESObjectREFR_UpdateAnimation(*g_player, 0.08f);
			}
			g_animDeltaTime = 0.0f;
		}

		NiNode* weap = f4vr::getChildNode("Weapon", (*g_player)->firstPersonSkeleton);
		//printNodes(weap, elapsed);
	}

	bool GunReload::StartReloading() {
		//NiNode* offhand = c_leftHandedMode ? getChildNode("LArm_Finger21", (*g_player)->unkF0->rootNode) : getChildNode("RArm_Finger21", (*g_player)->unkF0->rootNode);
		//NiNode* bolt = getChildNode("WeaponBolt", (*g_player)->firstPersonSkeleton);
		NiNode* magNode = f4vr::getChildNode("WeaponMagazine", (*g_player)->firstPersonSkeleton);
		if (!magNode) {
			return false;
		}

		//if ((bolt == nullptr) || (offhand == nullptr)) {
		//	return false;
		//}

		//float dist = abs(vec3_len(offhand->m_worldTransform.pos - bolt->m_worldTransform.pos));

		const uint64_t handInput = g_config.leftHandedMode
			? f4vr::VRControllers.getControllerState_DEPRECATED(f4vr::TrackerType::Left).ulButtonPressed
			: f4vr::VRControllers.getControllerState_DEPRECATED(f4vr::TrackerType::Right).ulButtonPressed;

		if (!reloadButtonPressed && handInput & vr::ButtonMaskFromId(vr::EVRButtonId::k_EButton_Grip)) {
			const auto refrData = new f4vr::NEW_REFR_DATA();
			refrData->location = magNode->m_worldTransform.pos;
			refrData->direction = (*g_player)->rot;
			refrData->interior = (*g_player)->parentCell;
			refrData->world = f4vr::TESObjectREFR_GetWorldSpace(*g_player);

			const auto extraData = static_cast<ExtraDataList*>(f4vr::MemoryManager_Allocate(g_mainHeap, 0x28, 0, false));
			f4vr::ExtraDataList_ExtraDataList(extraData);
			extraData->m_refCount += 1;
			f4vr::ExtraDataList_setCount(extraData, 10);
			refrData->extra = extraData;
			const auto instance = new f4vr::BGSObjectInstance(nullptr, nullptr);
			f4vr::BGSEquipIndex idx;
			f4vr::Actor_GetWeaponEquipIndex(*g_player, &idx, instance);
			currentAmmo = f4vr::Actor_GetCurrentAmmo(*g_player, idx);
			const float clipAmountPct = f4vr::Actor_GetAmmoClipPercentage(*g_player, idx);

			if (clipAmountPct == 1.0f) {
				return false;
			}

			const int clipAmount = f4vr::Actor_GetCurrentAmmoCount(*g_player, idx);
			f4vr::ExtraDataList_setAmmoCount(extraData, clipAmount);

			refrData->object = currentAmmo;
			void* ammoDrop = new std::size_t;

			void* newHandle = f4vr::TESDataHandler_CreateReferenceAtLocation(*g_dataHandler, ammoDrop, refrData);

			std::uintptr_t newRefr = 0x0;
			f4vr::BSPointerHandleManagerInterface_GetSmartPointer(newHandle, &newRefr);

			currentRefr = (TESObjectREFR*)newRefr;

			if (!currentRefr) {
				return false;
			}
			f4vr::ExtraDataList_setAmmoCount(currentRefr->extraDataList, clipAmount);
			magNode->flags |= 0x1;
			reloadButtonPressed = true;
			return true;
		}
		reloadButtonPressed = handInput && vr::ButtonMaskFromId(vr::EVRButtonId::k_EButton_Grip);
		magNode->flags &= 0xfffffffffffffffe;
		return false;
	}

	bool GunReload::SetAmmoMesh() {
		if (currentRefr->unkF0 && currentRefr->unkF0->rootNode) {
			for (auto i = 0; i < currentRefr->unkF0->rootNode->m_children.m_emptyRunStart; ++i) {
				currentRefr->unkF0->rootNode->RemoveChildAt(i);
			}

			if (!magMesh) {
				magMesh = vrui::loadNifFromFile("Data/Meshes/Weapons/10mmPistol/10mmMagLarge.nif");
			}
			f4vr::NiCloneProcess proc;
			proc.unk18 = f4vr::cloneAddr1;
			proc.unk48 = f4vr::cloneAddr2;

			NiNode* newMesh = f4vr::cloneNode(magMesh, &proc);
			bhkWorld* world = f4vr::TESObjectCell_GetbhkWorld(currentRefr->parentCell);

			currentRefr->unkF0->rootNode->AttachChild(newMesh, true);
			f4vr::bhkWorld_RemoveObject(currentRefr->unkF0->rootNode, true, false);
			currentRefr->unkF0->rootNode->m_spCollisionObject.m_pObject = nullptr;
			f4vr::bhkUtilFunctions_MoveFirstCollisionObjectToRoot(currentRefr->unkF0->rootNode, newMesh);
			f4vr::bhkNPCollisionObject_AddToWorld((bhkNPCollisionObject*)currentRefr->unkF0->rootNode->m_spCollisionObject.m_pObject, world);
			f4vr::bhkWorld_SetMotion(currentRefr->unkF0->rootNode, f4vr::hknpMotionPropertiesId::Preset::DYNAMIC, true, true, true);
			f4vr::TESObjectREFR_InitHavokForCollisionObject(currentRefr);
			f4vr::bhkUtilFunctions_SetLayer(currentRefr->unkF0->rootNode, 5);

			//Log::info("%016I64X", currentRefr);
			return true;
		}
		return false;
	}

	void GunReload::Update() {
		switch (state) {
		case idle:
			if (StartReloading()) {
				state = reloadingStart;
			}
			break;

		case reloadingStart:

			if (SetAmmoMesh()) {
				state = idle;
			}

			break;

		case newMagReady:
			break;

		case magInserted:
			break;

		default:
			state = idle;
			break;
		}
	}
}
