#include "Offsets.h"




namespace Offsets {

	RelocPtr<bool> iniLeftHandedMode(0x37d5e48);      // location of bLeftHandedMode:VR ini setting

	RelocAddr <UInt64> testin(0x5a3b888);

	RelocAddr<_AIProcess_getAnimationManager> AIProcess_getAnimationManager(0xec5400);
	RelocAddr<_BSAnimationManager_setActiveGraph> BSAnimationManager_setActiveGraph(0x1690240);
	RelocAddr<uint64_t> EquippedWeaponData_vfunc(0x2d7fcf8);
	RelocAddr<_NiNode_UpdateWorldBound> NiNode_UpdateWorldBound(0x1c18ab0);
	RelocAddr<_CombatUtilities_IsActorUsingMelee> CombatUtilities_IsActorUsingMelee(0x1133bb0);
	RelocAddr<_CombatUtilities_IsActorUsingMagic> CombatUtilities_IsActorUsingMagic(0x1133c30);
	RelocAddr<_AttackBlockHandler_IsPlayerThrowingWeapon> AttackBlockHandler_IsPlayerThrowingWeapon(0xfcbcd0);
	RelocAddr<_IAnimationGraphManagerHolder_NotifyAnimationGraph> IAnimationGraphManagerHolder_NotifyAnimationGraph(0x80e7f0);
	RelocAddr< _TESObjectREFR_UpdateAnimation> TESObjectREFR_UpdateAnimation(0x419b50);
	RelocAddr<_Actor_CanThrow> Actor_CanThrow(0xe52050);
	RelocAddr<uint32_t> g_equipIndex(0x3706d2c);

	RelocPtr<NiNode*> worldRootCamera1(0x6885c80);

	// loadNif native func
	RelocAddr<_loadNif> loadNif(0x1d0dee0);
	//RelocAddr<_loadNif> loadNif(0x1d0dd80);

	RelocAddr<_togglePipboyLight> togglePipboyLight(0xf27720);
	RelocAddr<_isPipboyLightOn> isPipboyLightOn(0xf27790);
	RelocAddr<_isPlayerRadioEnabled> isPlayerRadioEnabled(0xd0a9d0);
	RelocAddr<_getPlayerRadioFreq> getPlayerRadioFreq(0xd0a740);

	
	RelocAddr<_ClearAllKeywords> ClearAllKeywords(0x148270);
	RelocAddr<_AddKeyword> AddKeyword(0x148310);

	RelocAddr<_cloneNode> cloneNode(0x1c13ff0);

	RelocAddr<_addNode> addNode(0xada20);

	RelocAddr<_BSFadeNode_UpdateGeomArray> BSFadeNode_UpdateGeomArray(0x27a9690);

	RelocAddr<_BSFadeNode_UpdateDownwardPass> BSFadeNode_UpdateDownwardPass(0x27a8db0);

	RelocAddr<_BSFadeNode_MergeWorldBounds> BSFadeNode_MergeWorldBounds(0x27a9930);

	RelocAddr<_NiNode_UpdateTransformsAndBounds> NiNode_UpdateTransformsAndBounds(0x1c18ce0);

	RelocAddr<_NiNode_UpdateDownwardPass> NiNode_UpdateDownwardPass(0x1c18620);

	RelocAddr<_BSGraphics_Utility_CalcBoneMatrices> BSGraphics_Utility_CalcBoneMatrices(0x1dabc60);

	RelocAddr<_TESObjectCELL_GetLandHeight> TESObjectCell_GetLandHeight(0x039b230);

	RelocAddr<_Actor_SwitchRace> Actor_SwitchRace(0xe07850);

	RelocAddr<_Actor_Reset3D> Actor_Reset3D(0xddad60);

	RelocAddr<_PowerArmor_ActorInPowerArmor> PowerArmor_ActorInPowerArmor(0x9bf5d0);

	RelocAddr<_PowerArmor_SwitchToPowerArmor> PowerArmor_SwitchToPowerArmor(0x9bfbc0);

	RelocAddr<_AIProcess_Update3DModel> AIProcess_Update3DModel(0x0e3c9c0);

	RelocAddr<_PowerArmor_SwitchFromPowerArmorFurnitureLoaded> PowerArmor_SwitchFromPowerArmorFurnitureLoaded(0x9c1450);

	RelocAddr<uint64_t> g_frameCounter(0x65a2b48);
	RelocAddr<UInt64*> cloneAddr1(0x36ff560);
	RelocAddr<UInt64*> cloneAddr2(0x36ff564);

	RelocAddr<_TESObjectREFR_GetWorldSpace> TESObjectREFR_GetWorldSpace(0x3f75a0);
	RelocAddr<_TESDataHandler_CreateReferenceAtLocation> TESDataHandler_CreateReferenceAtLocation(0x11bd80);
	RelocAddr<_TESObjectREFR_Set3D> TESObjectREFR_Set3D(0x3ece40);
	RelocAddr<_TESObjectREFR_Set3DSimple> TESObjectREFR_Set3DSimple(0x3edb20);

	RelocAddr< _Actor_GetCurrentWeapon> Actor_GetCurrentWeapon(0xe50da0);
	RelocAddr< _Actor_GetCurrentAmmo> Actor_GetCurrentAmmo(0xe05ba0);
	RelocAddr<_Actor_GetWeaponEquipIndex> Actor_GetWeaponEquipIndex(0xe50e70);
	RelocAddr<_BSPointerHandleManagerInterface_GetSmartPointer> BSPointerHandleManagerInterface_GetSmartPointer(0xab60);
	RelocAddr<_TESObjectCell_AttachReference3D> TESObjectCell_AttachReference3D(0x3c8310);
	RelocAddr<_TESObjectREFR_AttachToParentRef3D> TESObjectREFR_AttachToParentRef3D(0x46b380);
	RelocAddr<_TESObjectREFR_AttachAllChildRef3D> TESObjectREFR_AttachAllChildRef3D(0x46b8d0);
	RelocAddr<_TESObjectREFR_DropAddon3DReplacement> TESObjectREFR_DropAddon3DReplacement(0x3e9c70);
	RelocAddr<_TESObjectREFR_InitHavokForCollisionObject> TESObjectREFR_InitHavokForCollisionObject(0x3eee60);
	RelocAddr<_bhkUtilFunctions_MoveFirstCollisionObjectToRoot> bhkUtilFunctions_MoveFirstCollisionObjectToRoot(0x1e17050);
	RelocAddr<_bhkUtilFunctions_SetLayer> bhkUtilFunctions_SetLayer(0x1e16180);
	RelocAddr<_bhkWorld_SetMotion> bhkWorld_SetMotion(0x1df95b0);
	RelocAddr<_bhkWorld_RemoveObject> bhkWorld_RemoveObject(0x1df9480);
	RelocAddr<_bhkNPCollisionObject_AddToWorld> bhkNPCollisionObject_AddToWorld(0x1e07be0);
	RelocAddr<_TESObjectCell_GetbhkWorld> TESObjectCell_GetbhkWorld(0x39b070);
	RelocAddr<_Actor_GetAmmoClipPercentage> Actor_GetAmmoClipPercentage(0xddf6c0);
	RelocAddr<_Actor_GetCurrentAmmoCount> Actor_GetCurrentAmmoCount(0xddf690);
	RelocAddr<_Actor_SetCurrentAmmoCount> Actor_SetCurrentAmmoCount(0xddf790);
	RelocAddr<_ExtraDataList_setCount> ExtraDataList_setCount(0x88fe0);
	RelocAddr<_ExtraDataList_ExtraDataList> ExtraDataList_ExtraDataList(0x81360);
	RelocAddr<_ExtraDataList_setAmmoCount> ExtraDataList_setAmmoCount(0x980d0);
	RelocAddr<_ExtraDataListSetPersistentCell> ExtraDataListSetPersistentCell(0x87bc0);
	RelocAddr<_MemoryManager_Allocate> MemoryManager_Allocate(0x1b91950);
	RelocAddr<_ControlMap_SaveRemappings> ControlMap_SaveRemappings(0x1bb3f00);
	RelocAddr<_ForceGamePause> ForceGamePause(0x1323370); 
}