#include "GunReload.h"
#include "F4VRBody.h"
#include "VR.h"
#include "Offsets.h"
#include "MiscStructs.h"

namespace F4VRBody {
	GunReload* g_gunReloadSystem = nullptr;

	float g_animDeltaTime = -1.0f;


	void printNodes(NiNode* nde, long long curTime) {
		_MESSAGE("%d %s : children = %d %d: local %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f", curTime, nde->m_name.c_str(), nde->m_children.m_emptyRunStart, (nde->flags & 0x1),
			nde->m_localTransform.rot.arr[0],
			nde->m_localTransform.rot.arr[1],
			nde->m_localTransform.rot.arr[2],
			nde->m_localTransform.rot.arr[3],
			nde->m_localTransform.rot.arr[4],
			nde->m_localTransform.rot.arr[5],
			nde->m_localTransform.rot.arr[6],
			nde->m_localTransform.rot.arr[7],
			nde->m_localTransform.rot.arr[8],
			nde->m_localTransform.rot.arr[9],
			nde->m_localTransform.rot.arr[10],
			nde->m_localTransform.rot.arr[11],
			nde->m_localTransform.pos.x, nde->m_localTransform.pos.y, nde->m_localTransform.pos.z);


		if (nde->GetAsNiNode()) {
			for (auto i = 0; i < nde->m_children.m_emptyRunStart; ++i) {
				auto nextNode = nde->m_children.m_data[i] ? nde->m_children.m_data[i]->GetAsNiNode() : nullptr;
				if (nextNode) {
					printNodes((NiNode*)nextNode, curTime);
				}
			}
		}
	}

	void GunReload::DoAnimationCapture() {
		if (!startAnimCap) {
			g_animDeltaTime = -1.0f;
			return;
		}

		auto elapsed = since(startCapTime).count();
		if (elapsed > 300) {
			if (elapsed > 2000) {
				g_animDeltaTime = -1.0f;
				Offsets::TESObjectREFR_UpdateAnimation(*g_player, 0.08f);
			}
			g_animDeltaTime = 0.0f;
		}

		NiNode* weap = getChildNode("Weapon", (*g_player)->firstPersonSkeleton);
	//	printNodes(weap, elapsed);
	}

	bool GunReload::StartReloading() {
		//NiNode* offhand = c_leftHandedMode ? getChildNode("LArm_Finger21", (*g_player)->unkF0->rootNode) : getChildNode("RArm_Finger21", (*g_player)->unkF0->rootNode);
		//NiNode* bolt = getChildNode("WeaponBolt", (*g_player)->firstPersonSkeleton);
		NiNode* magNode = getChildNode("WeaponMagazine", (*g_player)->firstPersonSkeleton);
		if (!magNode) {
			return false;
		}

		//if ((bolt == nullptr) || (offhand == nullptr)) {
		//	return false;
		//}

		//float dist = abs(vec3_len(offhand->m_worldTransform.pos - bolt->m_worldTransform.pos));

		uint64_t handInput = c_leftHandedMode ? VRHook::g_vrHook->getControllerState(VRHook::VRSystem::TrackerType::Left).ulButtonPressed : VRHook::g_vrHook->getControllerState(VRHook::VRSystem::TrackerType::Right).ulButtonPressed;

		if ((!reloadButtonPressed) && (handInput & vr::ButtonMaskFromId(vr::EVRButtonId::k_EButton_Grip))) {

			NEW_REFR_DATA* refrData = new NEW_REFR_DATA();
			refrData->location = magNode->m_worldTransform.pos;
			refrData->direction = (*g_player)->rot;
			refrData->interior = (*g_player)->parentCell;
			refrData->world = Offsets::TESObjectREFR_GetWorldSpace(*g_player);

			ExtraDataList* extraData = (ExtraDataList*) Offsets::MemoryManager_Allocate(g_mainHeap, 0x28, 0, false);
			Offsets::ExtraDataList_ExtraDataList(extraData);
			extraData->m_refCount += 1;
			Offsets::ExtraDataList_setCount(extraData, 10);
			refrData->extra = extraData;
			BGSObjectInstance* instance = new BGSObjectInstance(nullptr, nullptr);
			BGSEquipIndex idx;
			Offsets::Actor_GetWeaponEquipIndex(*g_player, &idx, instance);
			currentAmmo = Offsets::Actor_GetCurrentAmmo(*g_player, idx);
			float clipAmountPct = Offsets::Actor_GetAmmoClipPercentage(*g_player, idx);

			if (clipAmountPct == 1.0f) {
				return false;
			}

			int clipAmount = Offsets::Actor_GetCurrentAmmoCount(*g_player, idx);
			Offsets::ExtraDataList_setAmmoCount(extraData, clipAmount);


			refrData->object = currentAmmo;
			void* ammoDrop = new std::size_t;

			void* newHandle = Offsets::TESDataHandler_CreateReferenceAtLocation(*g_dataHandler, ammoDrop, refrData);

			std::uintptr_t newRefr = 0x0;
			Offsets::BSPointerHandleManagerInterface_GetSmartPointer(newHandle, &newRefr);

			currentRefr = (TESObjectREFR*)newRefr;
			
			if (!currentRefr) {
				return false;
			}
			Offsets::ExtraDataList_setAmmoCount(currentRefr->extraDataList, clipAmount);
			magNode->flags |= 0x1;
			reloadButtonPressed = true;
			return true;
		}
		else {
			reloadButtonPressed = handInput && vr::ButtonMaskFromId(vr::EVRButtonId::k_EButton_Grip);
			magNode->flags &= 0xfffffffffffffffe;
		}
		return false;
	}

	bool GunReload::SetAmmoMesh() {
		if (currentRefr->unkF0 && currentRefr->unkF0->rootNode) {
			for (auto i = 0; i < currentRefr->unkF0->rootNode->m_children.m_emptyRunStart; ++i) {
				currentRefr->unkF0->rootNode->RemoveChildAt(i);
			}

			if (!magMesh) {
				magMesh = loadNifFromFile("Data/Meshes/Weapons/10mmPistol/10mmMagLarge.nif");
			}
			NiCloneProcess proc;
			proc.unk18 = Offsets::cloneAddr1;
			proc.unk48 = Offsets::cloneAddr2;

			NiNode* newMesh = Offsets::cloneNode(magMesh, &proc);
			bhkWorld* world = Offsets::TESObjectCell_GetbhkWorld(currentRefr->parentCell);

			currentRefr->unkF0->rootNode->AttachChild(newMesh, true);
			Offsets::bhkWorld_RemoveObject(currentRefr->unkF0->rootNode, true, false);
			currentRefr->unkF0->rootNode->m_spCollisionObject.m_pObject = nullptr;
			Offsets::bhkUtilFunctions_MoveFirstCollisionObjectToRoot(currentRefr->unkF0->rootNode, newMesh);
			Offsets::bhkNPCollisionObject_AddToWorld((bhkNPCollisionObject*)currentRefr->unkF0->rootNode->m_spCollisionObject.m_pObject, world);
			Offsets::bhkWorld_SetMotion(currentRefr->unkF0->rootNode, hknpMotionPropertiesId::Preset::DYNAMIC, true, true, true);
			Offsets::TESObjectREFR_InitHavokForCollisionObject(currentRefr);
			Offsets::bhkUtilFunctions_SetLayer(currentRefr->unkF0->rootNode, 5);

			//_MESSAGE("%016I64X", currentRefr);
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
