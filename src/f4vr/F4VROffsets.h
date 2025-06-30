#pragma once

// ReSharper disable CppClangTidyClangDiagnosticReservedIdentifier
// ReSharper disable CppClangTidyBugproneReservedIdentifier

#include <f4se/GameSettings.h>

#include "MiscStructs.h"
#include "f4se/BSGeometry.h"
#include "f4se/GameData.h"
#include "f4se/GameForms.h"
#include "f4se/GameObjects.h"
#include "f4se/GameReferences.h"
#include "f4se/NiNodes.h"
#include "f4se/NiObjects.h"
#include "f4sE_common/Relocation.h"

class BSAnimationManager;

namespace f4vr {
	template <class T>
	class StackPtr {
	public:
		StackPtr() { p = nullptr; }
		T p;
	};

	inline RelocAddr<UInt64> testin(0x5a3b888);

	using _IsInAir = bool(*)(Actor* actor);
	inline RelocAddr<_IsInAir> IsInAir(0x00DC3230);

	typedef int (*_GetSitState)(VirtualMachine* registry, UInt64 stackID, Actor* actor);
	inline RelocAddr<_GetSitState> GetSitState(0x40e410);

	typedef bool (*_IsSneaking)(Actor* a_actor);
	inline RelocAddr<_IsSneaking> IsSneaking(0x24d20);

	using _AIProcess_getAnimationManager = void(*)(uint64_t aiProcess, StackPtr<BSAnimationManager*>& manager);
	inline RelocAddr<_AIProcess_getAnimationManager> AIProcess_getAnimationManager(0xec5400);

	inline RelocPtr<bool> iniLeftHandedMode(0x37d5e48); // location of bLeftHandedMode:VR ini setting

	using _BSAnimationManager_setActiveGraph = void(*)(BSAnimationManager* manager, int graph);
	inline RelocAddr<_BSAnimationManager_setActiveGraph> BSAnimationManager_setActiveGraph(0x1690240);

	inline RelocAddr<uint64_t> EquippedWeaponData_vfunc(0x2d7fcf8);

	using _NiNode_UpdateWorldBound = void(*)(NiNode* node);
	inline RelocAddr<_NiNode_UpdateWorldBound> NiNode_UpdateWorldBound(0x1c18ab0);

	using _AIProcess_Set2DUpdateFlags = void(*)(Actor::MiddleProcess* proc, uint64_t flags);
	inline RelocAddr<_AIProcess_Set2DUpdateFlags> AIProcess_Set3DUpdateFlags(0x0);

	using _CombatUtilities_IsActorUsingMelee = bool(*)(Actor* a_actor);
	inline RelocAddr<_CombatUtilities_IsActorUsingMelee> CombatUtilities_IsActorUsingMelee(0x1133bb0);

	using _CombatUtilities_IsActorUsingMagic = bool(*)(Actor* a_actor);
	inline RelocAddr<_CombatUtilities_IsActorUsingMagic> CombatUtilities_IsActorUsingMagic(0x1133c30);

	using _AttackBlockHandler_IsPlayerThrowingWeapon = bool(*)();
	inline RelocAddr<_AttackBlockHandler_IsPlayerThrowingWeapon> AttackBlockHandler_IsPlayerThrowingWeapon(0xfcbcd0);

	using _Actor_CanThrow = bool(*)(Actor* a_actor, uint32_t equipIndex);
	inline RelocAddr<_Actor_CanThrow> Actor_CanThrow(0xe52050);
	inline RelocAddr<uint32_t> g_equipIndex(0x3706d2c);

	using _IAnimationGraphManagerHolder_NotifyAnimationGraph = void(*)(IAnimationGraphManagerHolder* a_holder, const BSFixedString& event);
	inline RelocAddr<_IAnimationGraphManagerHolder_NotifyAnimationGraph> IAnimationGraphManagerHolder_NotifyAnimationGraph(0x80e7f0);

	using _TESObjectREFR_UpdateAnimation = void(*)(TESObjectREFR* obj, float a_delta);
	inline RelocAddr<_TESObjectREFR_UpdateAnimation> TESObjectREFR_UpdateAnimation(0x419b50);

	inline RelocPtr<NiNode*> worldRootCamera1(0x6885c80);

	// loadNif native func
	//typedef int(*_loadNif)(const char* path, uint64_t parentNode, uint64_t flags);
	using _loadNif = int(*)(uint64_t path, uint64_t mem, uint64_t flags);
	inline RelocAddr<_loadNif> loadNif(0x1d0dee0);
	//RelocAddr<_loadNif> loadNif(0x1d0dd80);

	using _cloneNode = NiNode * (*)(const NiNode* node, f4vr::NiCloneProcess* obj);
	inline RelocAddr<_cloneNode> cloneNode(0x1c13ff0);

	using _addNode = NiNode * (*)(uint64_t attachNode, const NiAVObject* node);
	inline RelocAddr<_addNode> addNode(0xada20);

	using _BSFadeNode_UpdateGeomArray = void* (*)(NiNode* node, int somevar);
	inline RelocAddr<_BSFadeNode_UpdateGeomArray> BSFadeNode_UpdateGeomArray(0x27a9690);

	using _BSFadeNode_UpdateDownwardPass = void* (*)(NiNode* node, NiAVObject::NiUpdateData* updateData, int somevar);
	inline RelocAddr<_BSFadeNode_UpdateDownwardPass> BSFadeNode_UpdateDownwardPass(0x27a8db0);

	using _BSFadeNode_MergeWorldBounds = void* (*)(NiNode* node);
	inline RelocAddr<_BSFadeNode_MergeWorldBounds> BSFadeNode_MergeWorldBounds(0x27a9930);

	using _NiNode_UpdateTransformsAndBounds = void* (*)(NiNode* node, NiAVObject::NiUpdateData* updateData);
	inline RelocAddr<_NiNode_UpdateTransformsAndBounds> NiNode_UpdateTransformsAndBounds(0x1c18ce0);

	using _NiNode_UpdateDownwardPass = void* (*)(NiNode* node, NiAVObject::NiUpdateData* updateData, uint64_t unk, int somevar);
	inline RelocAddr<_NiNode_UpdateDownwardPass> NiNode_UpdateDownwardPass(0x1c18620);

	using _BSGraphics_Utility_CalcBoneMatrices = void* (*)(BSSubIndexTriShape* node, uint64_t counter);
	inline RelocAddr<_BSGraphics_Utility_CalcBoneMatrices> BSGraphics_Utility_CalcBoneMatrices(0x1dabc60);

	using _TESObjectCELL_GetLandHeight = uint64_t(*)(TESObjectCELL* cell, const RE::NiPoint3* coord, const float* height);
	inline RelocAddr<_TESObjectCELL_GetLandHeight> TESObjectCell_GetLandHeight(0x039b230);

	using _Actor_SwitchRace = void(*)(Actor* a_actor, TESRace* a_race, bool param3, bool param4);
	inline RelocAddr<_Actor_SwitchRace> Actor_SwitchRace(0xe07850);

	using _Actor_Reset3D = void(*)(Actor* a_actor, double param2, uint64_t param3, bool param4, uint64_t param5);
	inline RelocAddr<_Actor_Reset3D> Actor_Reset3D(0xddad60);

	using _PowerArmor_ActorInPowerArmor = bool(*)(Actor* a_actor);
	inline RelocAddr<_PowerArmor_ActorInPowerArmor> PowerArmor_ActorInPowerArmor(0x9bf5d0);

	using _PowerArmor_SwitchToPowerArmor = bool(*)(Actor* a_actor, TESObjectREFR* a_refr, uint64_t a_char);
	inline RelocAddr<_PowerArmor_SwitchToPowerArmor> PowerArmor_SwitchToPowerArmor(0x9bfbc0);

	using _AIProcess_Update3DModel = void(*)(Actor::MiddleProcess* proc, Actor* a_actor, uint64_t flags, uint64_t someNum);
	inline RelocAddr<_AIProcess_Update3DModel> AIProcess_Update3DModel(0x0e3c9c0);

	using _PowerArmor_SwitchFromPowerArmorFurnitureLoaded = void(*)(Actor* a_actor, uint64_t somenum);
	inline RelocAddr<_PowerArmor_SwitchFromPowerArmorFurnitureLoaded> PowerArmor_SwitchFromPowerArmorFurnitureLoaded(0x9c1450);

	inline RelocAddr<uint64_t> g_frameCounter(0x65a2b48);
	inline RelocAddr<UInt64*> cloneAddr1(0x36ff560);
	inline RelocAddr<UInt64*> cloneAddr2(0x36ff564);

	using _TESObjectREFR_GetWorldSpace = TESWorldSpace * (*)(TESObjectREFR* a_refr);
	inline RelocAddr<_TESObjectREFR_GetWorldSpace> TESObjectREFR_GetWorldSpace(0x3f75a0);

	using _TESDataHandler_CreateReferenceAtLocation = void* (*)(DataHandler* dataHandler, void* newRefr, f4vr::NEW_REFR_DATA* refrData);
	inline RelocAddr<_TESDataHandler_CreateReferenceAtLocation> TESDataHandler_CreateReferenceAtLocation(0x11bd80);

	using _Actor_GetCurrentWeapon = TESObjectWEAP * (*)(Actor* a_actor, TESObjectWEAP* weap, f4vr::BGSEquipIndex idx);
	inline RelocAddr<_Actor_GetCurrentWeapon> Actor_GetCurrentWeapon(0xe50da0);

	using _Actor_GetCurrentAmmo = TESAmmo * (*)(Actor* a_actor, f4vr::BGSEquipIndex idx);
	inline RelocAddr<_Actor_GetCurrentAmmo> Actor_GetCurrentAmmo(0xe05ba0);

	using _Actor_GetWeaponEquipIndex = void(*)(Actor* a_actor, f4vr::BGSEquipIndex* idx, f4vr::BGSObjectInstance* instance);
	inline RelocAddr<_Actor_GetWeaponEquipIndex> Actor_GetWeaponEquipIndex(0xe50e70);

	using _TESObjectREFR_Set3D = void(*)(TESObjectREFR* a_refr, NiAVObject* a_obj, bool unk);
	inline RelocAddr<_TESObjectREFR_Set3D> TESObjectREFR_Set3D(0x3ece40);

	using _TESObjectREFR_Set3DSimple = void(*)(TESObjectREFR* a_refr, NiAVObject* a_obj, bool unk);
	inline RelocAddr<_TESObjectREFR_Set3DSimple> TESObjectREFR_Set3DSimple(0x3edb20);

	using _TESObjectREFR_DropAddon3DReplacement = void(*)(TESObjectREFR* a_refr, NiAVObject* a_obj);
	inline RelocAddr<_TESObjectREFR_DropAddon3DReplacement> TESObjectREFR_DropAddon3DReplacement(0x3e9c70);

	using _BSPointerHandleManagerInterface_GetSmartPointer = void(*)(void* a_handle, void* a_refr);
	inline RelocAddr<_BSPointerHandleManagerInterface_GetSmartPointer> BSPointerHandleManagerInterface_GetSmartPointer(0xab60);

	using _TESObjectCell_AttachReference3D = void(*)(TESObjectCELL* a_cell, TESObjectREFR* a_refr, bool somebool, bool somebool2);
	inline RelocAddr<_TESObjectCell_AttachReference3D> TESObjectCell_AttachReference3D(0x3c8310);

	using _TESObjectREFR_AttachToParentRef3D = void(*)(TESObjectREFR* a_refr);
	inline RelocAddr<_TESObjectREFR_AttachToParentRef3D> TESObjectREFR_AttachToParentRef3D(0x46b380);

	using _TESObjectREFR_AttachAllChildRef3D = void(*)(TESObjectREFR* a_refr);
	inline RelocAddr<_TESObjectREFR_AttachAllChildRef3D> TESObjectREFR_AttachAllChildRef3D(0x46b8d0);

	using _TESObjectREFR_InitHavokForCollisionObject = void(*)(TESObjectREFR* a_refr);
	inline RelocAddr<_TESObjectREFR_InitHavokForCollisionObject> TESObjectREFR_InitHavokForCollisionObject(0x3eee60);

	using _bhkUtilFunctions_MoveFirstCollisionObjectToRoot = void(*)(NiAVObject* root, NiAVObject* child);
	inline RelocAddr<_bhkUtilFunctions_MoveFirstCollisionObjectToRoot> bhkUtilFunctions_MoveFirstCollisionObjectToRoot(0x1e17050);

	using _bhkUtilFunctions_SetLayer = void(*)(NiAVObject* root, std::uint32_t collisionLayer);
	inline RelocAddr<_bhkUtilFunctions_SetLayer> bhkUtilFunctions_SetLayer(0x1e16180);

	using _bhkWorld_RemoveObject = void(*)(NiAVObject* root, bool a_bool, bool a_bool2);
	inline RelocAddr<_bhkWorld_RemoveObject> bhkWorld_RemoveObject(0x1df9480);

	using _bhkWorld_SetMotion = void(*)(NiAVObject* root, f4vr::hknpMotionPropertiesId::Preset preset, bool bool1, bool bool2, bool bool3);
	inline RelocAddr<_bhkWorld_SetMotion> bhkWorld_SetMotion(0x1df95b0);

	using _bhkNPCollisionObject_AddToWorld = void(*)(bhkNPCollisionObject* a_obj, bhkWorld* a_world);
	inline RelocAddr<_bhkNPCollisionObject_AddToWorld> bhkNPCollisionObject_AddToWorld(0x1e07be0);

	using _TESObjectCell_GetbhkWorld = bhkWorld * (*)(TESObjectCELL* a_cell);
	inline RelocAddr<_TESObjectCell_GetbhkWorld> TESObjectCell_GetbhkWorld(0x39b070);

	using _Actor_GetAmmoClipPercentage = float(*)(Actor* a_actor, f4vr::BGSEquipIndex a_idx);
	inline RelocAddr<_Actor_GetAmmoClipPercentage> Actor_GetAmmoClipPercentage(0xddf6c0);

	using _Actor_GetCurrentAmmoCount = float(*)(Actor* a_actor, f4vr::BGSEquipIndex a_idx);
	inline RelocAddr<_Actor_GetCurrentAmmoCount> Actor_GetCurrentAmmoCount(0xddf690);

	using _Actor_SetCurrentAmmoCount = float(*)(Actor* a_actor, f4vr::BGSEquipIndex a_idx, int a_count);
	inline RelocAddr<_Actor_SetCurrentAmmoCount> Actor_SetCurrentAmmoCount(0xddf790);

	using _ExtraDataList_setAmmoCount = void(*)(ExtraDataList* a_list, int a_count);
	inline RelocAddr<_ExtraDataList_setAmmoCount> ExtraDataList_setAmmoCount(0x980d0);

	using _ExtraDataList_setCount = void(*)(ExtraDataList* a_list, int a_count);
	inline RelocAddr<_ExtraDataList_setCount> ExtraDataList_setCount(0x88fe0);

	using _ExtraDataList_ExtraDataList = void(*)(ExtraDataList* a_list);
	inline RelocAddr<_ExtraDataList_ExtraDataList> ExtraDataList_ExtraDataList(0x81360);

	using _ExtraDataListSetPersistentCell = void(*)(ExtraDataList* a_list, int a_int);
	inline RelocAddr<_ExtraDataListSetPersistentCell> ExtraDataListSetPersistentCell(0x87bc0);

	using _MemoryManager_Allocate = void* (*)(Heap* manager, uint64_t size, uint32_t someint, bool somebool);
	inline RelocAddr<_MemoryManager_Allocate> MemoryManager_Allocate(0x1b91950);

	using _togglePipboyLight = void* (*)(Actor* a_actor);
	inline RelocAddr<_togglePipboyLight> togglePipboyLight(0xf27720);

	using _isPipboyLightOn = bool* (*)(Actor* a_actor);
	inline RelocAddr<_isPipboyLightOn> isPipboyLightOn(0xf27790);

	using _isPlayerRadioEnabled = uint64_t(*)();
	inline RelocAddr<_isPlayerRadioEnabled> isPlayerRadioEnabled(0xd0a9d0);

	using _getPlayerRadioFreq = float(*)();
	inline RelocAddr<_getPlayerRadioFreq> getPlayerRadioFreq(0xd0a740);

	using _ClearAllKeywords = void(*)(TESForm* form);
	inline RelocAddr<_ClearAllKeywords> ClearAllKeywords(0x148270);

	using _AddKeyword = void(*)(TESForm* form, BGSKeyword* keyword);
	inline RelocAddr<_AddKeyword> AddKeyword(0x148310);

	using _ControlMap_SaveRemappings = void(*)(InputManager* a_mgr);
	inline RelocAddr<_ControlMap_SaveRemappings> ControlMap_SaveRemappings(0x1bb3f00);

	using _ForceGamePause = void(*)(MenuControls* a_mgr);
	inline RelocAddr<_ForceGamePause> ForceGamePause(0x1323370);

	using _AIProcess_ClearMuzzleFlashes = void* (*)(Actor::MiddleProcess* middleProcess);
	inline RelocAddr<_AIProcess_ClearMuzzleFlashes> AIProcess_ClearMuzzleFlashes(0xecc710);

	using _AIProcess_CreateMuzzleFlash = void* (*)(Actor::MiddleProcess* middleProcess, uint64_t projectile, Actor* actor);
	inline RelocAddr<_AIProcess_CreateMuzzleFlash> AIProcess_CreateMuzzleFlash(0xecc570);

	using _SettingCollectionList_GetPtr = Setting * (*)(SettingCollectionList* list, const char* name);
	inline RelocAddr<_SettingCollectionList_GetPtr> SettingCollectionList_GetPtr(0x501500);

	using _BSFlattenedBoneTree_GetBoneIndex = int(*)(NiAVObject* a_tree, BSFixedString* a_name);
	inline RelocAddr<_BSFlattenedBoneTree_GetBoneIndex> BSFlattenedBoneTree_GetBoneIndex(0x1c20c80);

	using _BSFlattenedBoneTree_GetBoneNode = NiNode * (*)(NiAVObject* a_tree, BSFixedString* a_name);
	inline RelocAddr<_BSFlattenedBoneTree_GetBoneNode> BSFlattenedBoneTree_GetBoneNode(0x1c21070);

	using _BSFlattenedBoneTree_GetBoneNodeFromPos = NiNode * (*)(NiAVObject* a_tree, int a_pos);
	inline RelocAddr<_BSFlattenedBoneTree_GetBoneNodeFromPos> BSFlattenedBoneTree_GetBoneNodeFromPos(0x1c21030);

	using _BSFlattenedBoneTree_UpdateBoneArray = void* (*)(NiAVObject* node);
	inline RelocAddr<_BSFlattenedBoneTree_UpdateBoneArray> BSFlattenedBoneTree_UpdateBoneArray(0x1c214b0);

	// Native function that takes the 1st person skeleton weapon node and calculates the skeleton from upper-arm down based off the offsetNode
	using _Update1StPersonArm = void* (*)(const PlayerCharacter* pc, NiNode** weapon, NiNode** offsetNode);
	inline RelocAddr<_Update1StPersonArm> Update1StPersonArm(0xef6280);
}
