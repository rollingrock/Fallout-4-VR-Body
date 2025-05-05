#pragma once

#include "MiscStructs.h"
#include "NiCloneProcess.h"
#include "f4se/BSGeometry.h"
#include "f4se/GameData.h"
#include "f4se/GameForms.h"
#include "f4se/GameObjects.h"
#include "f4se/GameReferences.h"
#include "f4se/GameSettings.h"
#include "f4se/NiNodes.h"
#include "f4se/NiObjects.h"
#include "f4sE_common/Relocation.h"

class BSAnimationManager;

template <class T>
class StackPtr {
public:
	StackPtr() { p = nullptr; }

	T p;
};

namespace Offsets {
	extern RelocAddr<UInt64> testin;

	using _AIProcess_getAnimationManager = void(*)(uint64_t aiProcess, StackPtr<BSAnimationManager*>& manager);
	extern RelocAddr<_AIProcess_getAnimationManager> AIProcess_getAnimationManager;

	extern RelocPtr<bool> iniLeftHandedMode; // location of bLeftHandedMode:VR ini setting

	using _AIProcess_getAnimationManager = void(*)(uint64_t aiProcess, StackPtr<BSAnimationManager*>& manager);
	extern RelocAddr<_AIProcess_getAnimationManager> AIProcess_getAnimationManager;

	using _BSAnimationManager_setActiveGraph = void(*)(BSAnimationManager* manager, int graph);
	extern RelocAddr<_BSAnimationManager_setActiveGraph> BSAnimationManager_setActiveGraph;

	extern RelocAddr<uint64_t> EquippedWeaponData_vfunc;

	using _NiNode_UpdateWorldBound = void(*)(NiNode* node);
	extern RelocAddr<_NiNode_UpdateWorldBound> NiNode_UpdateWorldBound;

	using _AIProcess_Set2DUpdateFlags = void(*)(Actor::MiddleProcess* proc, uint64_t flags);
	extern RelocAddr<_AIProcess_Set2DUpdateFlags> AIProcess_Set3DUpdateFlags;

	using _CombatUtilities_IsActorUsingMelee = bool(*)(Actor* a_actor);
	extern RelocAddr<_CombatUtilities_IsActorUsingMelee> CombatUtilities_IsActorUsingMelee;

	using _CombatUtilities_IsActorUsingMagic = bool(*)(Actor* a_actor);
	extern RelocAddr<_CombatUtilities_IsActorUsingMagic> CombatUtilities_IsActorUsingMagic;

	using _AttackBlockHandler_IsPlayerThrowingWeapon = bool(*)();
	extern RelocAddr<_AttackBlockHandler_IsPlayerThrowingWeapon> AttackBlockHandler_IsPlayerThrowingWeapon;

	using _Actor_CanThrow = bool(*)(Actor* a_actor, uint32_t equipIndex);
	extern RelocAddr<_Actor_CanThrow> Actor_CanThrow;
	extern RelocAddr<uint32_t> g_equipIndex;

	using _IAnimationGraphManagerHolder_NotifyAnimationGraph = void(*)(IAnimationGraphManagerHolder* a_holder, const BSFixedString& event);
	extern RelocAddr<_IAnimationGraphManagerHolder_NotifyAnimationGraph> IAnimationGraphManagerHolder_NotifyAnimationGraph;

	using _TESObjectREFR_UpdateAnimation = void(*)(TESObjectREFR* obj, float a_delta);
	extern RelocAddr<_TESObjectREFR_UpdateAnimation> TESObjectREFR_UpdateAnimation;

	extern RelocPtr<NiNode*> worldRootCamera1;

	// loadNif native func
	//typedef int(*_loadNif)(const char* path, uint64_t parentNode, uint64_t flags);
	using _loadNif = int(*)(uint64_t path, uint64_t mem, uint64_t flags);
	extern RelocAddr<_loadNif> loadNif;
	//RelocAddr<_loadNif> loadNif(0x1d0dd80);

	using _cloneNode = NiNode*(*)(const NiNode* node, NiCloneProcess* obj);
	extern RelocAddr<_cloneNode> cloneNode;

	using _addNode = NiNode* (*)(uint64_t attachNode, const NiAVObject* node);
	extern RelocAddr<_addNode> addNode;

	using _BSFadeNode_UpdateGeomArray = void* (*)(NiNode* node, int somevar);
	extern RelocAddr<_BSFadeNode_UpdateGeomArray> BSFadeNode_UpdateGeomArray;

	using _BSFadeNode_UpdateDownwardPass = void* (*)(NiNode* node, NiAVObject::NiUpdateData* updateData, int somevar);
	extern RelocAddr<_BSFadeNode_UpdateDownwardPass> BSFadeNode_UpdateDownwardPass;

	using _BSFadeNode_MergeWorldBounds = void* (*)(NiNode* node);
	extern RelocAddr<_BSFadeNode_MergeWorldBounds> BSFadeNode_MergeWorldBounds;

	using _NiNode_UpdateTransformsAndBounds = void* (*)(NiNode* node, NiAVObject::NiUpdateData* updateData);
	extern RelocAddr<_NiNode_UpdateTransformsAndBounds> NiNode_UpdateTransformsAndBounds;

	using _NiNode_UpdateDownwardPass = void* (*)(NiNode* node, NiAVObject::NiUpdateData* updateData, uint64_t unk, int somevar);
	extern RelocAddr<_NiNode_UpdateDownwardPass> NiNode_UpdateDownwardPass;

	using _BSGraphics_Utility_CalcBoneMatrices = void* (*)(BSSubIndexTriShape* node, uint64_t counter);
	extern RelocAddr<_BSGraphics_Utility_CalcBoneMatrices> BSGraphics_Utility_CalcBoneMatrices;

	using _TESObjectCELL_GetLandHeight = uint64_t(*)(TESObjectCELL* cell, const NiPoint3* coord, const float* height);
	extern RelocAddr<_TESObjectCELL_GetLandHeight> TESObjectCell_GetLandHeight;

	using _Actor_SwitchRace = void(*)(Actor* a_actor, TESRace* a_race, bool param3, bool param4);
	extern RelocAddr<_Actor_SwitchRace> Actor_SwitchRace;

	using _Actor_Reset3D = void(*)(Actor* a_actor, double param2, uint64_t param3, bool param4, uint64_t param5);
	extern RelocAddr<_Actor_Reset3D> Actor_Reset3D;

	using _PowerArmor_ActorInPowerArmor = bool(*)(Actor* a_actor);
	extern RelocAddr<_PowerArmor_ActorInPowerArmor> PowerArmor_ActorInPowerArmor;

	using _PowerArmor_SwitchToPowerArmor = bool(*)(Actor* a_actor, TESObjectREFR* a_refr, uint64_t a_char);
	extern RelocAddr<_PowerArmor_SwitchToPowerArmor> PowerArmor_SwitchToPowerArmor;

	using _AIProcess_Update3DModel = void(*)(Actor::MiddleProcess* proc, Actor* a_actor, uint64_t flags, uint64_t someNum);
	extern RelocAddr<_AIProcess_Update3DModel> AIProcess_Update3DModel;

	using _PowerArmor_SwitchFromPowerArmorFurnitureLoaded = void(*)(Actor* a_actor, uint64_t somenum);
	extern RelocAddr<_PowerArmor_SwitchFromPowerArmorFurnitureLoaded> PowerArmor_SwitchFromPowerArmorFurnitureLoaded;

	extern RelocAddr<uint64_t> g_frameCounter;
	extern RelocAddr<UInt64*> cloneAddr1;
	extern RelocAddr<UInt64*> cloneAddr2;

	using _TESObjectREFR_GetWorldSpace = TESWorldSpace* (*)(TESObjectREFR* a_refr);
	extern RelocAddr<_TESObjectREFR_GetWorldSpace> TESObjectREFR_GetWorldSpace;

	using _TESDataHandler_CreateReferenceAtLocation = void*(*)(DataHandler* dataHandler, void* newRefr, FRIK::NEW_REFR_DATA* refrData);
	extern RelocAddr<_TESDataHandler_CreateReferenceAtLocation> TESDataHandler_CreateReferenceAtLocation;

	using _Actor_GetCurrentWeapon = TESObjectWEAP* (*)(Actor* a_actor, TESObjectWEAP* weap, FRIK::BGSEquipIndex idx);
	extern RelocAddr<_Actor_GetCurrentWeapon> Actor_GetCurrentWeapon;

	using _Actor_GetCurrentAmmo = TESAmmo* (*)(Actor* a_actor, FRIK::BGSEquipIndex idx);
	extern RelocAddr<_Actor_GetCurrentAmmo> Actor_GetCurrentAmmo;

	using _Actor_GetWeaponEquipIndex = void(*)(Actor* a_actor, FRIK::BGSEquipIndex* idx, FRIK::BGSObjectInstance* instance);
	extern RelocAddr<_Actor_GetWeaponEquipIndex> Actor_GetWeaponEquipIndex;

	using _TESObjectREFR_Set3D = void(*)(TESObjectREFR* a_refr, NiAVObject* a_obj, bool unk);
	extern RelocAddr<_TESObjectREFR_Set3D> TESObjectREFR_Set3D;

	using _TESObjectREFR_Set3DSimple = void(*)(TESObjectREFR* a_refr, NiAVObject* a_obj, bool unk);
	extern RelocAddr<_TESObjectREFR_Set3DSimple> TESObjectREFR_Set3DSimple;

	using _TESObjectREFR_DropAddon3DReplacement = void(*)(TESObjectREFR* a_refr, NiAVObject* a_obj);
	extern RelocAddr<_TESObjectREFR_DropAddon3DReplacement> TESObjectREFR_DropAddon3DReplacement;

	using _BSPointerHandleManagerInterface_GetSmartPointer = void(*)(void* a_handle, void* a_refr);
	extern RelocAddr<_BSPointerHandleManagerInterface_GetSmartPointer> BSPointerHandleManagerInterface_GetSmartPointer;

	using _TESObjectCell_AttachReference3D = void(*)(TESObjectCELL* a_cell, TESObjectREFR* a_refr, bool somebool, bool somebool2);
	extern RelocAddr<_TESObjectCell_AttachReference3D> TESObjectCell_AttachReference3D;

	using _TESObjectREFR_AttachToParentRef3D = void(*)(TESObjectREFR* a_refr);
	extern RelocAddr<_TESObjectREFR_AttachToParentRef3D> TESObjectREFR_AttachToParentRef3D;

	using _TESObjectREFR_AttachAllChildRef3D = void(*)(TESObjectREFR* a_refr);
	extern RelocAddr<_TESObjectREFR_AttachAllChildRef3D> TESObjectREFR_AttachAllChildRef3D;

	using _TESObjectREFR_InitHavokForCollisionObject = void(*)(TESObjectREFR* a_refr);
	extern RelocAddr<_TESObjectREFR_InitHavokForCollisionObject> TESObjectREFR_InitHavokForCollisionObject;

	using _bhkUtilFunctions_MoveFirstCollisionObjectToRoot = void(*)(NiAVObject* root, NiAVObject* child);
	extern RelocAddr<_bhkUtilFunctions_MoveFirstCollisionObjectToRoot> bhkUtilFunctions_MoveFirstCollisionObjectToRoot;

	using _bhkUtilFunctions_SetLayer = void(*)(NiAVObject* root, std::uint32_t collisionLayer);
	extern RelocAddr<_bhkUtilFunctions_SetLayer> bhkUtilFunctions_SetLayer;

	using _bhkWorld_RemoveObject = void(*)(NiAVObject* root, bool a_bool, bool a_bool2);
	extern RelocAddr<_bhkWorld_RemoveObject> bhkWorld_RemoveObject;

	using _bhkWorld_SetMotion = void(*)(NiAVObject* root, FRIK::hknpMotionPropertiesId::Preset preset, bool bool1, bool bool2, bool bool3);
	extern RelocAddr<_bhkWorld_SetMotion> bhkWorld_SetMotion;

	using _bhkNPCollisionObject_AddToWorld = void(*)(bhkNPCollisionObject* a_obj, bhkWorld* a_world);
	extern RelocAddr<_bhkNPCollisionObject_AddToWorld> bhkNPCollisionObject_AddToWorld;

	using _TESObjectCell_GetbhkWorld = bhkWorld*(*)(TESObjectCELL* a_cell);
	extern RelocAddr<_TESObjectCell_GetbhkWorld> TESObjectCell_GetbhkWorld;

	using _Actor_GetAmmoClipPercentage = float(*)(Actor* a_actor, FRIK::BGSEquipIndex a_idx);
	extern RelocAddr<_Actor_GetAmmoClipPercentage> Actor_GetAmmoClipPercentage;

	using _Actor_GetCurrentAmmoCount = float(*)(Actor* a_actor, FRIK::BGSEquipIndex a_idx);
	extern RelocAddr<_Actor_GetCurrentAmmoCount> Actor_GetCurrentAmmoCount;

	using _Actor_SetCurrentAmmoCount = float(*)(Actor* a_actor, FRIK::BGSEquipIndex a_idx, int a_count);
	extern RelocAddr<_Actor_SetCurrentAmmoCount> Actor_SetCurrentAmmoCount;

	using _ExtraDataList_setAmmoCount = void(*)(ExtraDataList* a_list, int a_count);
	extern RelocAddr<_ExtraDataList_setAmmoCount> ExtraDataList_setAmmoCount;

	using _ExtraDataList_setCount = void(*)(ExtraDataList* a_list, int a_count);
	extern RelocAddr<_ExtraDataList_setCount> ExtraDataList_setCount;

	using _ExtraDataList_ExtraDataList = void(*)(ExtraDataList* a_list);
	extern RelocAddr<_ExtraDataList_ExtraDataList> ExtraDataList_ExtraDataList;

	using _ExtraDataListSetPersistentCell = void(*)(ExtraDataList* a_list, int a_int);
	extern RelocAddr<_ExtraDataListSetPersistentCell> ExtraDataListSetPersistentCell;

	using _MemoryManager_Allocate = void* (*)(Heap* manager, uint64_t size, uint32_t someint, bool somebool);
	extern RelocAddr<_MemoryManager_Allocate> MemoryManager_Allocate;
	using _togglePipboyLight = void* (*)(Actor* a_actor);
	extern RelocAddr<_togglePipboyLight> togglePipboyLight;

	using _isPipboyLightOn = bool* (*)(Actor* a_actor);
	extern RelocAddr<_isPipboyLightOn> isPipboyLightOn;

	using _isPlayerRadioEnabled = uint64_t(*)();
	extern RelocAddr<_isPlayerRadioEnabled> isPlayerRadioEnabled;

	using _getPlayerRadioFreq = float(*)();
	extern RelocAddr<_getPlayerRadioFreq> getPlayerRadioFreq;

	using _ClearAllKeywords = void(*)(TESForm* form);
	extern RelocAddr<_ClearAllKeywords> ClearAllKeywords;

	using _AddKeyword = void(*)(TESForm* form, BGSKeyword* keyword);
	extern RelocAddr<_AddKeyword> AddKeyword;
	using _ControlMap_SaveRemappings = void(*)(InputManager* a_mgr);
	extern RelocAddr<_ControlMap_SaveRemappings> ControlMap_SaveRemappings;

	using _ForceGamePause = void(*)(MenuControls* a_mgr);
	extern RelocAddr<_ForceGamePause> ForceGamePause;
}
