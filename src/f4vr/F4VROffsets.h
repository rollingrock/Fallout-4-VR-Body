#pragma once

// ReSharper disable CppClangTidyClangDiagnosticReservedIdentifier
// ReSharper disable CppClangTidyBugproneReservedIdentifier

#include "F4SEVRStructs.h"
#include "MiscStructs.h"

class BSAnimationManager;

namespace f4vr
{
    template <class T>
    class StackPtr
    {
    public:
        StackPtr() { p = nullptr; }
        T p;
    };

    inline REL::Relocation testin(REL::Offset(0x5a3b888));

    using _IsInAir = bool(*)(F4SEVR::Actor* actor);
    inline REL::Relocation<_IsInAir> IsInAir(REL::Offset(0x00DC3230));

    using _GetSitState = int(*)(RE::BSScript::Internal::VirtualMachine* registry, std::uint64_t stackID, F4SEVR::Actor* actor);
    inline REL::Relocation<_GetSitState> GetSitState(REL::Offset(0x40e410));

    using _IsSneaking = bool(*)(F4SEVR::Actor* a_actor);
    inline REL::Relocation<_IsSneaking> IsSneaking(REL::Offset(0x24d20));

    using _AIProcess_getAnimationManager = void(*)(uint64_t aiProcess, StackPtr<BSAnimationManager*>& manager);
    inline REL::Relocation<_AIProcess_getAnimationManager> AIProcess_getAnimationManager(REL::Offset(0xec5400));

    using _BSAnimationManager_setActiveGraph = void(*)(BSAnimationManager* manager, int graph);
    inline REL::Relocation<_BSAnimationManager_setActiveGraph> BSAnimationManager_setActiveGraph(REL::Offset(0x1690240));

    inline REL::Relocation<uint64_t> EquippedWeaponData_vfunc(REL::Offset(0x2d7fcf8));

    using _NiNode_UpdateWorldBound = void(*)(RE::NiNode* node);
    inline REL::Relocation<_NiNode_UpdateWorldBound> NiNode_UpdateWorldBound(REL::Offset(0x1c18ab0));

    using _AIProcess_Set2DUpdateFlags = void(*)(F4SEVR::Actor::MiddleProcess* proc, uint64_t flags);
    inline REL::Relocation<_AIProcess_Set2DUpdateFlags> AIProcess_Set3DUpdateFlags(REL::Offset(0x0));

    using _CombatUtilities_IsActorUsingMelee = bool(*)(F4SEVR::Actor* a_actor);
    inline REL::Relocation<_CombatUtilities_IsActorUsingMelee> CombatUtilities_IsActorUsingMelee(REL::Offset(0x1133bb0));

    using _CombatUtilities_IsActorUsingMagic = bool(*)(F4SEVR::Actor* a_actor);
    inline REL::Relocation<_CombatUtilities_IsActorUsingMagic> CombatUtilities_IsActorUsingMagic(REL::Offset(0x1133c30));

    using _AttackBlockHandler_IsPlayerThrowingWeapon = bool(*)();
    inline REL::Relocation<_AttackBlockHandler_IsPlayerThrowingWeapon> AttackBlockHandler_IsPlayerThrowingWeapon(REL::Offset(0xfcbcd0));

    using _Actor_CanThrow = bool(*)(F4SEVR::Actor* a_actor, uint32_t equipIndex);
    inline REL::Relocation<_Actor_CanThrow> Actor_CanThrow(REL::Offset(0xe52050));
    inline REL::Relocation<uint32_t> g_equipIndex(REL::Offset(0x3706d2c));

    using _IAnimationGraphManagerHolder_NotifyAnimationGraph = void(*)(RE::IAnimationGraphManagerHolder* a_holder, const RE::BSFixedString& event);
    inline REL::Relocation<_IAnimationGraphManagerHolder_NotifyAnimationGraph> IAnimationGraphManagerHolder_NotifyAnimationGraph(REL::Offset(0x80e7f0));

    using _TESObjectREFR_UpdateAnimation = void(*)(RE::TESObjectREFR* obj, float a_delta);
    inline REL::Relocation<_TESObjectREFR_UpdateAnimation> TESObjectREFR_UpdateAnimation(REL::Offset(0x419b50));

    inline REL::Relocation<RE::NiNode*> worldRootCamera1(REL::Offset(0x6885c80));

    // loadNif native func
    //typedef int(*_loadNif)(const char* path, uint64_t parentNode, uint64_t flags);
    using _loadNif = int(*)(uint64_t path, uint64_t mem, uint64_t flags);
    inline REL::Relocation<_loadNif> loadNif(REL::Offset(0x1d0dee0));
    //REL::Relocation<_loadNif> loadNif(REL::Offset(0x1d0dd80));

    using _cloneNode = RE::NiNode * (*)(const RE::NiNode* node, NiCloneProcess* obj);
    inline REL::Relocation<_cloneNode> cloneNode(REL::Offset(0x1c13ff0));

    using _addNode = RE::NiNode * (*)(uint64_t attachNode, const RE::NiAVObject* node);
    inline REL::Relocation<_addNode> addNode(REL::Offset(0xada20));

    using _BSFadeNode_UpdateGeomArray = void* (*)(RE::NiNode* node, int somevar);
    inline REL::Relocation<_BSFadeNode_UpdateGeomArray> BSFadeNode_UpdateGeomArray(REL::Offset(0x27a9690));

    using _BSFadeNode_UpdateDownwardPass = void* (*)(RE::NiNode* node, RE::NiUpdateData* updateData, int somevar);
    inline REL::Relocation<_BSFadeNode_UpdateDownwardPass> BSFadeNode_UpdateDownwardPass(REL::Offset(0x27a8db0));

    using _BSFadeNode_MergeWorldBounds = void* (*)(RE::NiNode* node);
    inline REL::Relocation<_BSFadeNode_MergeWorldBounds> BSFadeNode_MergeWorldBounds(REL::Offset(0x27a9930));

    using _NiNode_UpdateTransformsAndBounds = void* (*)(RE::NiNode* node, RE::NiUpdateData* updateData);
    inline REL::Relocation<_NiNode_UpdateTransformsAndBounds> NiNode_UpdateTransformsAndBounds(REL::Offset(0x1c18ce0));

    using _NiNode_UpdateDownwardPass = void* (*)(RE::NiNode* node, RE::NiUpdateData* updateData, uint64_t unk, int somevar);
    inline REL::Relocation<_NiNode_UpdateDownwardPass> NiNode_UpdateDownwardPass(REL::Offset(0x1c18620));

    using _BSGraphics_Utility_CalcBoneMatrices = void* (*)(RE::BSSubIndexTriShape* node, uint64_t counter);
    inline REL::Relocation<_BSGraphics_Utility_CalcBoneMatrices> BSGraphics_Utility_CalcBoneMatrices(REL::Offset(0x1dabc60));

    using _TESObjectCELL_GetLandHeight = uint64_t(*)(RE::TESObjectCELL* cell, const RE::NiPoint3* coord, const float* height);
    inline REL::Relocation<_TESObjectCELL_GetLandHeight> TESObjectCell_GetLandHeight(REL::Offset(0x039b230));

    using _Actor_SwitchRace = void(*)(F4SEVR::Actor* a_actor, RE::TESRace* a_race, bool param3, bool param4);
    inline REL::Relocation<_Actor_SwitchRace> Actor_SwitchRace(REL::Offset(0xe07850));

    using _Actor_Reset3D = void(*)(F4SEVR::Actor* a_actor, double param2, uint64_t param3, bool param4, uint64_t param5);
    inline REL::Relocation<_Actor_Reset3D> Actor_Reset3D(REL::Offset(0xddad60));

    using _PowerArmor_ActorInPowerArmor = bool(*)(F4SEVR::Actor* a_actor);
    inline REL::Relocation<_PowerArmor_ActorInPowerArmor> PowerArmor_ActorInPowerArmor(REL::Offset(0x9bf5d0));

    using _PowerArmor_SwitchToPowerArmor = bool(*)(F4SEVR::Actor* a_actor, RE::TESObjectREFR* a_refr, uint64_t a_char);
    inline REL::Relocation<_PowerArmor_SwitchToPowerArmor> PowerArmor_SwitchToPowerArmor(REL::Offset(0x9bfbc0));

    using _AIProcess_Update3DModel = void(*)(F4SEVR::Actor::MiddleProcess* proc, F4SEVR::Actor* a_actor, uint64_t flags, uint64_t someNum);
    inline REL::Relocation<_AIProcess_Update3DModel> AIProcess_Update3DModel(REL::Offset(0x0e3c9c0));

    using _PowerArmor_SwitchFromPowerArmorFurnitureLoaded = void(*)(F4SEVR::Actor* a_actor, uint64_t somenum);
    inline REL::Relocation<_PowerArmor_SwitchFromPowerArmorFurnitureLoaded> PowerArmor_SwitchFromPowerArmorFurnitureLoaded(REL::Offset(0x9c1450));

    inline REL::Relocation<uint64_t> g_frameCounter(REL::Offset(0x65a2b48));
    inline REL::Relocation<std::uint64_t*> cloneAddr1(REL::Offset(0x36ff560));
    inline REL::Relocation<std::uint64_t*> cloneAddr2(REL::Offset(0x36ff564));

    using _TESObjectREFR_GetWorldSpace = RE::TESWorldSpace * (*)(RE::TESObjectREFR* a_refr);
    inline REL::Relocation<_TESObjectREFR_GetWorldSpace> TESObjectREFR_GetWorldSpace(REL::Offset(0x3f75a0));

    // using _TESDataHandler_CreateReferenceAtLocation = void* (*)(DataHandler* dataHandler, void* newRefr, f4vr::NEW_REFR_DATA* refrData);
    // inline REL::Relocation<_TESDataHandler_CreateReferenceAtLocation> TESDataHandler_CreateReferenceAtLocation(REL::Offset(0x11bd80));

    using _Actor_GetCurrentWeapon = RE::TESObjectWEAP * (*)(F4SEVR::Actor* a_actor, RE::TESObjectWEAP* weap, BGSEquipIndex idx);
    inline REL::Relocation<_Actor_GetCurrentWeapon> Actor_GetCurrentWeapon(REL::Offset(0xe50da0));

    using _Actor_GetCurrentAmmo = RE::TESAmmo * (*)(F4SEVR::Actor* a_actor, BGSEquipIndex idx);
    inline REL::Relocation<_Actor_GetCurrentAmmo> Actor_GetCurrentAmmo(REL::Offset(0xe05ba0));

    using _Actor_GetWeaponEquipIndex = void(*)(F4SEVR::Actor* a_actor, BGSEquipIndex* idx, BGSObjectInstance* instance);
    inline REL::Relocation<_Actor_GetWeaponEquipIndex> Actor_GetWeaponEquipIndex(REL::Offset(0xe50e70));

    using _TESObjectREFR_Set3D = void(*)(RE::TESObjectREFR* a_refr, RE::NiAVObject* a_obj, bool unk);
    inline REL::Relocation<_TESObjectREFR_Set3D> TESObjectREFR_Set3D(REL::Offset(0x3ece40));

    using _TESObjectREFR_Set3DSimple = void(*)(RE::TESObjectREFR* a_refr, RE::NiAVObject* a_obj, bool unk);
    inline REL::Relocation<_TESObjectREFR_Set3DSimple> TESObjectREFR_Set3DSimple(REL::Offset(0x3edb20));

    using _TESObjectREFR_DropAddon3DReplacement = void(*)(RE::TESObjectREFR* a_refr, RE::NiAVObject* a_obj);
    inline REL::Relocation<_TESObjectREFR_DropAddon3DReplacement> TESObjectREFR_DropAddon3DReplacement(REL::Offset(0x3e9c70));

    using _BSPointerHandleManagerInterface_GetSmartPointer = void(*)(void* a_handle, void* a_refr);
    inline REL::Relocation<_BSPointerHandleManagerInterface_GetSmartPointer> BSPointerHandleManagerInterface_GetSmartPointer(REL::Offset(0xab60));

    using _TESObjectCell_AttachReference3D = void(*)(RE::TESObjectCELL* a_cell, RE::TESObjectREFR* a_refr, bool somebool, bool somebool2);
    inline REL::Relocation<_TESObjectCell_AttachReference3D> TESObjectCell_AttachReference3D(REL::Offset(0x3c8310));

    using _TESObjectREFR_AttachToParentRef3D = void(*)(RE::TESObjectREFR* a_refr);
    inline REL::Relocation<_TESObjectREFR_AttachToParentRef3D> TESObjectREFR_AttachToParentRef3D(REL::Offset(0x46b380));

    using _TESObjectREFR_AttachAllChildRef3D = void(*)(RE::TESObjectREFR* a_refr);
    inline REL::Relocation<_TESObjectREFR_AttachAllChildRef3D> TESObjectREFR_AttachAllChildRef3D(REL::Offset(0x46b8d0));

    using _TESObjectREFR_InitHavokForCollisionObject = void(*)(RE::TESObjectREFR* a_refr);
    inline REL::Relocation<_TESObjectREFR_InitHavokForCollisionObject> TESObjectREFR_InitHavokForCollisionObject(REL::Offset(0x3eee60));

    using _bhkUtilFunctions_MoveFirstCollisionObjectToRoot = void(*)(RE::NiAVObject* root, RE::NiAVObject* child);
    inline REL::Relocation<_bhkUtilFunctions_MoveFirstCollisionObjectToRoot> bhkUtilFunctions_MoveFirstCollisionObjectToRoot(REL::Offset(0x1e17050));

    using _bhkUtilFunctions_SetLayer = void(*)(RE::NiAVObject* root, std::uint32_t collisionLayer);
    inline REL::Relocation<_bhkUtilFunctions_SetLayer> bhkUtilFunctions_SetLayer(REL::Offset(0x1e16180));

    using _bhkWorld_RemoveObject = void(*)(RE::NiAVObject* root, bool a_bool, bool a_bool2);
    inline REL::Relocation<_bhkWorld_RemoveObject> bhkWorld_RemoveObject(REL::Offset(0x1df9480));

    using _bhkWorld_SetMotion = void(*)(RE::NiAVObject* root, hknpMotionPropertiesId::Preset preset, bool bool1, bool bool2, bool bool3);
    inline REL::Relocation<_bhkWorld_SetMotion> bhkWorld_SetMotion(REL::Offset(0x1df95b0));

    using _bhkNPCollisionObject_AddToWorld = void(*)(RE::bhkNPCollisionObject* a_obj, RE::bhkWorld* a_world);
    inline REL::Relocation<_bhkNPCollisionObject_AddToWorld> bhkNPCollisionObject_AddToWorld(REL::Offset(0x1e07be0));

    using _TESObjectCell_GetbhkWorld = RE::bhkWorld * (*)(RE::TESObjectCELL* a_cell);
    inline REL::Relocation<_TESObjectCell_GetbhkWorld> TESObjectCell_GetbhkWorld(REL::Offset(0x39b070));

    using _Actor_GetAmmoClipPercentage = float(*)(F4SEVR::Actor* a_actor, BGSEquipIndex a_idx);
    inline REL::Relocation<_Actor_GetAmmoClipPercentage> Actor_GetAmmoClipPercentage(REL::Offset(0xddf6c0));

    using _Actor_GetCurrentAmmoCount = float(*)(F4SEVR::Actor* a_actor, BGSEquipIndex a_idx);
    inline REL::Relocation<_Actor_GetCurrentAmmoCount> Actor_GetCurrentAmmoCount(REL::Offset(0xddf690));

    using _Actor_SetCurrentAmmoCount = float(*)(F4SEVR::Actor* a_actor, BGSEquipIndex a_idx, int a_count);
    inline REL::Relocation<_Actor_SetCurrentAmmoCount> Actor_SetCurrentAmmoCount(REL::Offset(0xddf790));

    using _ExtraDataList_setAmmoCount = void(*)(RE::ExtraDataList* a_list, int a_count);
    inline REL::Relocation<_ExtraDataList_setAmmoCount> ExtraDataList_setAmmoCount(REL::Offset(0x980d0));

    using _ExtraDataList_setCount = void(*)(RE::ExtraDataList* a_list, int a_count);
    inline REL::Relocation<_ExtraDataList_setCount> ExtraDataList_setCount(REL::Offset(0x88fe0));

    using _ExtraDataListExtraDataList = void(*)(RE::ExtraDataList* a_list);
    inline REL::Relocation<_ExtraDataListExtraDataList> ExtraDataListExtraDataList(REL::Offset(0x81360));

    using _ExtraDataListSetPersistentCell = void(*)(RE::ExtraDataList* a_list, int a_int);
    inline REL::Relocation<_ExtraDataListSetPersistentCell> ExtraDataListSetPersistentCell(REL::Offset(0x87bc0));

    // using _MemoryManager_Allocate = void* (*)(Heap* manager, uint64_t size, uint32_t someint, bool somebool);
    // inline REL::Relocation<_MemoryManager_Allocate> MemoryManager_Allocate(REL::Offset(0x1b91950));

    using _togglePipboyLight = void* (*)(F4SEVR::Actor* a_actor);
    inline REL::Relocation<_togglePipboyLight> togglePipboyLight(REL::Offset(0xf27720));

    using _isPipboyLightOn = bool* (*)(F4SEVR::Actor* a_actor);
    inline REL::Relocation<_isPipboyLightOn> isPipboyLightOn(REL::Offset(0xf27790));

    using _isPlayerRadioEnabled = uint64_t(*)();
    inline REL::Relocation<_isPlayerRadioEnabled> isPlayerRadioEnabled(REL::Offset(0xd0a9d0));

    using _getPlayerRadioFreq = float(*)();
    inline REL::Relocation<_getPlayerRadioFreq> getPlayerRadioFreq(REL::Offset(0xd0a740));

    using _ClearAllKeywords = void(*)(RE::TESForm* form);
    inline REL::Relocation<_ClearAllKeywords> ClearAllKeywords(REL::Offset(0x148270));

    using _AddKeyword = void(*)(RE::TESForm* form, RE::BGSKeyword* keyword);
    inline REL::Relocation<_AddKeyword> AddKeyword(REL::Offset(0x148310));

    // using _ControlMap_SaveRemappings = void(*)(InputManager* a_mgr);
    // inline REL::Relocation<_ControlMap_SaveRemappings> ControlMap_SaveRemappings(REL::Offset(0x1bb3f00));

    using _ForceGamePause = void(*)(RE::MenuControls* a_mgr);
    inline REL::Relocation<_ForceGamePause> ForceGamePause(REL::Offset(0x1323370));

    using _AIProcess_ClearMuzzleFlashes = void* (*)(F4SEVR::Actor::MiddleProcess* middleProcess);
    inline REL::Relocation<_AIProcess_ClearMuzzleFlashes> AIProcess_ClearMuzzleFlashes(REL::Offset(0xecc710));

    using _AIProcess_CreateMuzzleFlash = void* (*)(F4SEVR::Actor::MiddleProcess* middleProcess, uint64_t projectile, F4SEVR::Actor* actor);
    inline REL::Relocation<_AIProcess_CreateMuzzleFlash> AIProcess_CreateMuzzleFlash(REL::Offset(0xecc570));

    // using _SettingCollectionList_GetPtr = RE::Setting * (*)(SettingCollectionList* list, const char* name);
    // inline REL::Relocation<_SettingCollectionList_GetPtr> SettingCollectionList_GetPtr(REL::Offset(0x501500));

    using _BSFlattenedBoneTree_GetBoneIndex = int(*)(RE::NiAVObject* a_tree, RE::BSFixedString* a_name);
    inline REL::Relocation<_BSFlattenedBoneTree_GetBoneIndex> BSFlattenedBoneTree_GetBoneIndex(REL::Offset(0x1c20c80));

    using _BSFlattenedBoneTree_GetBoneNode = RE::NiNode * (*)(RE::NiAVObject* a_tree, RE::BSFixedString* a_name);
    inline REL::Relocation<_BSFlattenedBoneTree_GetBoneNode> BSFlattenedBoneTree_GetBoneNode(REL::Offset(0x1c21070));

    using _BSFlattenedBoneTree_GetBoneNodeFromPos = RE::NiNode * (*)(RE::NiAVObject* a_tree, int a_pos);
    inline REL::Relocation<_BSFlattenedBoneTree_GetBoneNodeFromPos> BSFlattenedBoneTree_GetBoneNodeFromPos(REL::Offset(0x1c21030));

    using _BSFlattenedBoneTree_UpdateBoneArray = void* (*)(RE::NiAVObject* node);
    inline REL::Relocation<_BSFlattenedBoneTree_UpdateBoneArray> BSFlattenedBoneTree_UpdateBoneArray(REL::Offset(0x1c214b0));

    // Native function that takes the 1st person skeleton weapon node and calculates the skeleton from upper-arm down based off the offsetNode
    using _Update1StPersonArm = void* (*)(const RE::PlayerCharacter* pc, RE::NiNode** weapon, RE::NiNode** offsetNode);
    inline REL::Relocation<_Update1StPersonArm> Update1StPersonArm(REL::Offset(0xef6280));
}
