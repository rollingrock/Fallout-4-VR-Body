#pragma once

#include "f4sE_common/Relocation.h"
#include "F4SE_common/SafeWrite.h"
#include "f4se/GameReferences.h"
#include "f4se/GameObjects.h"
#include "f4se/GameForms.h"
#include "f4se/NiNodes.h"
#include "f4se/NiObjects.h"
#include "f4se/BSGeometry.h"
#include "f4se/GameSettings.h"
#include "f4se/GameMenus.h"
#include "f4se/GameData.h"
#include "NiCloneProcess.h"
#include "MiscStructs.h"

class BSAnimationManager;

template <class T>
class StackPtr  {
public:
	StackPtr() { p = nullptr; }

	T p;
};

namespace Offsets {

	extern RelocAddr<UInt64> testin;

	typedef void(*_AIProcess_getAnimationManager)(uint64_t aiProcess, StackPtr<BSAnimationManager*>& manager);
	extern RelocAddr<_AIProcess_getAnimationManager> AIProcess_getAnimationManager;

	extern RelocPtr<bool> iniLeftHandedMode;      // location of bLeftHandedMode:VR ini setting

	typedef void(*_AIProcess_getAnimationManager)(uint64_t aiProcess, StackPtr<BSAnimationManager*> &manager);
	extern RelocAddr<_AIProcess_getAnimationManager> AIProcess_getAnimationManager;

	typedef void(*_BSAnimationManager_setActiveGraph)(BSAnimationManager* manager, int graph);
	extern RelocAddr<_BSAnimationManager_setActiveGraph> BSAnimationManager_setActiveGraph;

	extern RelocAddr<uint64_t> EquippedWeaponData_vfunc;

	typedef void(*_NiNode_UpdateWorldBound)(NiNode* node);
	extern RelocAddr<_NiNode_UpdateWorldBound> NiNode_UpdateWorldBound;

	typedef void(*_AIProcess_Set2DUpdateFlags)(Actor::MiddleProcess* proc, uint64_t flags);
	extern RelocAddr<_AIProcess_Set2DUpdateFlags> AIProcess_Set3DUpdateFlags;

	typedef bool(*_CombatUtilities_IsActorUsingMelee)(Actor* a_actor);
	extern RelocAddr<_CombatUtilities_IsActorUsingMelee> CombatUtilities_IsActorUsingMelee;

	typedef bool(*_CombatUtilities_IsActorUsingMagic)(Actor* a_actor);
	extern RelocAddr<_CombatUtilities_IsActorUsingMagic> CombatUtilities_IsActorUsingMagic;

	typedef bool(*_AttackBlockHandler_IsPlayerThrowingWeapon)();
	extern RelocAddr<_AttackBlockHandler_IsPlayerThrowingWeapon> AttackBlockHandler_IsPlayerThrowingWeapon;

	typedef bool(*_Actor_CanThrow)(Actor* a_actor, uint32_t equipIndex);
	extern RelocAddr<_Actor_CanThrow> Actor_CanThrow;
	extern RelocAddr<uint32_t> g_equipIndex;

	typedef void(*_IAnimationGraphManagerHolder_NotifyAnimationGraph)(IAnimationGraphManagerHolder* a_holder, const BSFixedString& event);
	extern RelocAddr<_IAnimationGraphManagerHolder_NotifyAnimationGraph> IAnimationGraphManagerHolder_NotifyAnimationGraph;

	typedef void(*_TESObjectREFR_UpdateAnimation)(TESObjectREFR* obj, float a_delta);
	extern RelocAddr< _TESObjectREFR_UpdateAnimation> TESObjectREFR_UpdateAnimation;




	extern RelocPtr<NiNode*> worldRootCamera1;

	// loadNif native func
	//typedef int(*_loadNif)(const char* path, uint64_t parentNode, uint64_t flags);
	typedef int(*_loadNif)(uint64_t path, uint64_t mem, uint64_t flags);
	extern RelocAddr<_loadNif> loadNif;
	//RelocAddr<_loadNif> loadNif(0x1d0dd80);

	typedef NiNode*(*_cloneNode)(NiNode* node, NiCloneProcess* obj);
	extern RelocAddr<_cloneNode> cloneNode;

	typedef NiNode* (*_addNode)(uint64_t attachNode, NiAVObject* node);
	extern RelocAddr<_addNode> addNode;

	typedef void* (*_BSFadeNode_UpdateGeomArray)(NiNode* node, int somevar);
	extern RelocAddr<_BSFadeNode_UpdateGeomArray> BSFadeNode_UpdateGeomArray;

	typedef void* (*_BSFadeNode_UpdateDownwardPass)(NiNode* node, NiAVObject::NiUpdateData* updateData, int somevar);
	extern RelocAddr<_BSFadeNode_UpdateDownwardPass> BSFadeNode_UpdateDownwardPass;

	typedef void* (*_BSFadeNode_MergeWorldBounds)(NiNode* node);
	extern RelocAddr<_BSFadeNode_MergeWorldBounds> BSFadeNode_MergeWorldBounds;

	typedef void* (*_NiNode_UpdateTransformsAndBounds)(NiNode* node, NiAVObject::NiUpdateData* updateData);
	extern RelocAddr<_NiNode_UpdateTransformsAndBounds> NiNode_UpdateTransformsAndBounds;

	typedef void* (*_NiNode_UpdateDownwardPass)(NiNode* node, NiAVObject::NiUpdateData* updateData, uint64_t unk, int somevar);
	extern RelocAddr<_NiNode_UpdateDownwardPass> NiNode_UpdateDownwardPass;

	typedef void* (*_BSGraphics_Utility_CalcBoneMatrices)(BSSubIndexTriShape* node, uint64_t counter);
	extern RelocAddr<_BSGraphics_Utility_CalcBoneMatrices> BSGraphics_Utility_CalcBoneMatrices;

	typedef uint64_t(*_TESObjectCELL_GetLandHeight)(TESObjectCELL* cell, NiPoint3* coord, float* height);
	extern RelocAddr<_TESObjectCELL_GetLandHeight> TESObjectCell_GetLandHeight;

	typedef void(*_Actor_SwitchRace)(Actor* a_actor, TESRace* a_race, bool param3, bool param4);
	extern RelocAddr<_Actor_SwitchRace> Actor_SwitchRace;

	typedef void(*_Actor_Reset3D)(Actor* a_actor, double param2, uint64_t param3, bool param4, uint64_t param5);
	extern RelocAddr<_Actor_Reset3D> Actor_Reset3D;

	typedef bool(*_PowerArmor_ActorInPowerArmor)(Actor* a_actor);
	extern RelocAddr<_PowerArmor_ActorInPowerArmor> PowerArmor_ActorInPowerArmor;

	typedef bool(*_PowerArmor_SwitchToPowerArmor)(Actor* a_actor, TESObjectREFR* a_refr, uint64_t a_char);
	extern RelocAddr<_PowerArmor_SwitchToPowerArmor> PowerArmor_SwitchToPowerArmor;

	typedef void(*_AIProcess_Update3DModel)(Actor::MiddleProcess* proc, Actor* a_actor, uint64_t flags, uint64_t someNum);
	extern RelocAddr<_AIProcess_Update3DModel> AIProcess_Update3DModel;

	typedef void(*_PowerArmor_SwitchFromPowerArmorFurnitureLoaded)(Actor* a_actor, uint64_t somenum);
	extern RelocAddr<_PowerArmor_SwitchFromPowerArmorFurnitureLoaded> PowerArmor_SwitchFromPowerArmorFurnitureLoaded;

	extern RelocAddr<uint64_t> g_frameCounter;
	extern RelocAddr<UInt64*> cloneAddr1;
	extern RelocAddr<UInt64*> cloneAddr2;

	typedef TESWorldSpace* (*_TESObjectREFR_GetWorldSpace)(TESObjectREFR* a_refr);
	extern RelocAddr<_TESObjectREFR_GetWorldSpace> TESObjectREFR_GetWorldSpace;

	typedef void*(*_TESDataHandler_CreateReferenceAtLocation)(DataHandler* dataHandler, void* newRefr, F4VRBody::NEW_REFR_DATA* refrData);
	extern RelocAddr<_TESDataHandler_CreateReferenceAtLocation> TESDataHandler_CreateReferenceAtLocation;
	
	typedef TESObjectWEAP* (*_Actor_GetCurrentWeapon)(Actor* a_actor, TESObjectWEAP* weap, F4VRBody::BGSEquipIndex idx);
	extern RelocAddr<_Actor_GetCurrentWeapon> Actor_GetCurrentWeapon;

	typedef TESAmmo* (*_Actor_GetCurrentAmmo)(Actor* a_actor, F4VRBody::BGSEquipIndex idx);
	extern RelocAddr<_Actor_GetCurrentAmmo> Actor_GetCurrentAmmo;

	typedef void(*_Actor_GetWeaponEquipIndex)(Actor* a_actor, F4VRBody::BGSEquipIndex* idx, F4VRBody::BGSObjectInstance* instance);
	extern RelocAddr<_Actor_GetWeaponEquipIndex> Actor_GetWeaponEquipIndex;

	typedef void(*_TESObjectREFR_Set3D)(TESObjectREFR* a_refr, NiAVObject* a_obj, bool unk);
	extern RelocAddr<_TESObjectREFR_Set3D> TESObjectREFR_Set3D;

	typedef void(*_TESObjectREFR_Set3DSimple)(TESObjectREFR* a_refr, NiAVObject* a_obj, bool unk);
	extern RelocAddr<_TESObjectREFR_Set3DSimple> TESObjectREFR_Set3DSimple;

	typedef void(*_TESObjectREFR_DropAddon3DReplacement)(TESObjectREFR* a_refr, NiAVObject* a_obj);
	extern RelocAddr<_TESObjectREFR_DropAddon3DReplacement> TESObjectREFR_DropAddon3DReplacement;

	typedef void(*_BSPointerHandleManagerInterface_GetSmartPointer)(void* a_handle, void* a_refr);
	extern RelocAddr<_BSPointerHandleManagerInterface_GetSmartPointer> BSPointerHandleManagerInterface_GetSmartPointer;

	typedef void(*_TESObjectCell_AttachReference3D)(TESObjectCELL* a_cell, TESObjectREFR* a_refr, bool somebool, bool somebool2);
	extern RelocAddr<_TESObjectCell_AttachReference3D> TESObjectCell_AttachReference3D;

	typedef void(*_TESObjectREFR_AttachToParentRef3D)(TESObjectREFR* a_refr);
	extern RelocAddr<_TESObjectREFR_AttachToParentRef3D> TESObjectREFR_AttachToParentRef3D;

	typedef void(*_TESObjectREFR_AttachAllChildRef3D)(TESObjectREFR* a_refr);
	extern RelocAddr<_TESObjectREFR_AttachAllChildRef3D> TESObjectREFR_AttachAllChildRef3D;

	typedef void(*_TESObjectREFR_InitHavokForCollisionObject)(TESObjectREFR* a_refr);
	extern RelocAddr<_TESObjectREFR_InitHavokForCollisionObject> TESObjectREFR_InitHavokForCollisionObject;

	typedef void(*_bhkUtilFunctions_MoveFirstCollisionObjectToRoot)(NiAVObject* root, NiAVObject* child);
	extern RelocAddr<_bhkUtilFunctions_MoveFirstCollisionObjectToRoot> bhkUtilFunctions_MoveFirstCollisionObjectToRoot;

	typedef void(*_bhkUtilFunctions_SetLayer)(NiAVObject* root, std::uint32_t collisionLayer);
	extern RelocAddr<_bhkUtilFunctions_SetLayer> bhkUtilFunctions_SetLayer;

	typedef void(*_bhkWorld_RemoveObject)(NiAVObject* root, bool a_bool, bool a_bool2);
	extern RelocAddr<_bhkWorld_RemoveObject> bhkWorld_RemoveObject;

	typedef void(*_bhkWorld_SetMotion)(NiAVObject* root, F4VRBody::hknpMotionPropertiesId::Preset preset, bool bool1, bool bool2, bool bool3);
	extern RelocAddr<_bhkWorld_SetMotion> bhkWorld_SetMotion;

	typedef void(*_bhkNPCollisionObject_AddToWorld)(bhkNPCollisionObject* a_obj, bhkWorld* a_world);
	extern RelocAddr<_bhkNPCollisionObject_AddToWorld> bhkNPCollisionObject_AddToWorld;

	typedef bhkWorld*(*_TESObjectCell_GetbhkWorld)(TESObjectCELL* a_cell);
	extern RelocAddr<_TESObjectCell_GetbhkWorld> TESObjectCell_GetbhkWorld;

	typedef float(*_Actor_GetAmmoClipPercentage)(Actor* a_actor, F4VRBody::BGSEquipIndex a_idx);
	extern RelocAddr<_Actor_GetAmmoClipPercentage> Actor_GetAmmoClipPercentage;

	typedef float(*_Actor_GetCurrentAmmoCount)(Actor* a_actor, F4VRBody::BGSEquipIndex a_idx);
	extern RelocAddr<_Actor_GetCurrentAmmoCount> Actor_GetCurrentAmmoCount;

	typedef float(*_Actor_SetCurrentAmmoCount)(Actor* a_actor, F4VRBody::BGSEquipIndex a_idx, int a_count);
	extern RelocAddr<_Actor_SetCurrentAmmoCount> Actor_SetCurrentAmmoCount;

	typedef void(*_ExtraDataList_setAmmoCount)(ExtraDataList* a_list, int a_count);
	extern RelocAddr<_ExtraDataList_setAmmoCount> ExtraDataList_setAmmoCount;

	typedef void(*_ExtraDataList_setCount)(ExtraDataList* a_list, int a_count);
	extern RelocAddr<_ExtraDataList_setCount> ExtraDataList_setCount;

	typedef void(*_ExtraDataList_ExtraDataList)(ExtraDataList* a_list);
	extern RelocAddr<_ExtraDataList_ExtraDataList> ExtraDataList_ExtraDataList;

	typedef void(*_ExtraDataListSetPersistentCell)(ExtraDataList* a_list, int a_int);
	extern RelocAddr<_ExtraDataListSetPersistentCell> ExtraDataListSetPersistentCell;

	typedef void* (*_MemoryManager_Allocate)(Heap* manager, uint64_t size, uint32_t someint, bool somebool);
	extern RelocAddr<_MemoryManager_Allocate> MemoryManager_Allocate;
	typedef void* (*_togglePipboyLight)(Actor* a_actor);
	extern RelocAddr<_togglePipboyLight> togglePipboyLight;

	typedef bool* (*_isPipboyLightOn)(Actor* a_actor);
	extern RelocAddr<_isPipboyLightOn> isPipboyLightOn;

	typedef uint64_t (*_isPlayerRadioEnabled)();
	extern RelocAddr<_isPlayerRadioEnabled> isPlayerRadioEnabled;

	typedef float (*_getPlayerRadioFreq)();
	extern RelocAddr<_getPlayerRadioFreq> getPlayerRadioFreq;


	typedef void (*_ClearAllKeywords)(TESForm* form);
	extern RelocAddr<_ClearAllKeywords> ClearAllKeywords;

	typedef void (*_AddKeyword)(TESForm* form, BGSKeyword* keyword);
	extern RelocAddr<_AddKeyword> AddKeyword;
	typedef void(*_ControlMap_SaveRemappings)(InputManager* a_mgr);
	extern RelocAddr<_ControlMap_SaveRemappings> ControlMap_SaveRemappings;

	typedef void(*_ForceGamePause)(MenuControls* a_mgr);
	extern RelocAddr<_ForceGamePause> ForceGamePause;

}
