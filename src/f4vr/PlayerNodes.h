#pragma once

#include "BSFlattenedBoneTree.h"
#include "F4SEVRStructs.h"
#include "F4VRUtils.h"

namespace f4vr
{
    // part of PlayerCharacter object but making useful struct below since not mapped in F4SE
    struct PlayerNodes
    {
        RE::NiNode* playerworldnode; //0x06E0
        RE::NiNode* roomnode; //0x06E8
        RE::NiNode* primaryWandNode; //0x06F0
        RE::NiNode* primaryWandandTouchPad; //0x06F8
        RE::NiNode* primaryUIAttachNode; //0x0700
        RE::NiNode* primaryWeapontoWeaponNode; //0x0708
        RE::NiNode* primaryWeaponKickbackRecoilNode; //0x0710
        RE::NiNode* primaryWeaponOffsetNOde; //0x0718
        RE::NiNode* primaryWeaponScopeCamera; //0x0720
        RE::NiNode* primaryVertibirdMinigunOffNOde; //0x0728
        RE::NiNode* primaryMeleeWeaponOffsetNode; //0x0730
        RE::NiNode* primaryUnarmedPowerArmorWeaponOffsetNode; //0x0738
        RE::NiNode* primaryWandLaserPointer; //0x0740
        RE::NiNode* PrimaryWandLaserPointerAdjuster; //0x0748
        RE::NiNode* unk750; //0x0750
        RE::NiNode* PrimaryMeleeWeaponOffsetNode; //0x0758
        RE::NiNode* SecondaryMeleeWeaponOffsetNode; //0x0760
        RE::NiNode* SecondaryWandNode; //0x0768
        RE::NiNode* Point002Node; //0x0770
        RE::NiNode* WorkshopPalletNode; //0x0778
        RE::NiNode* WorkshopPallenSlide; //0x0780
        RE::NiNode* SecondaryUIOffsetNode; //0x0788
        RE::NiNode* SecondaryMeleeWeaponOffsetNode2; //0x0790
        RE::NiNode* SecondaryUnarmedPowerArmorWeaponOffsetNode; //0x0798
        RE::NiNode* SecondaryAimNode; //0x07A0
        RE::NiNode* PipboyParentNode; //0x07A8
        RE::NiNode* PipboyRoot_nif_only_node; //0x07B0
        RE::NiNode* ScreenNode; //0x07B8
        RE::NiNode* PipboyLightParentNode; //0x07C0
        RE::NiNode* unk7c8; //0x07C8
        RE::NiNode* ScopeParentNode; //0x07D0
        RE::NiNode* unk7d8; //0x07D8
        RE::NiNode* HmdNode; //0x07E0
        RE::NiNode* OffscreenHmdNode; //0x07E8
        RE::NiNode* UprightHmdNode; //0x07F0
        RE::NiNode* UprightHmdLagNode; //0x07F8
        RE::NiNode* BlackSphereNode; //0x0800
        RE::NiNode* HeadLightParentNode; //0x0808
        RE::NiNode* unk810; //0x0810
        RE::NiNode* WeaponLeftNode; //0x0818
        RE::NiNode* unk820; //0x0820
        RE::NiNode* LockPickParentNode; //0x0828
    };

    inline F4SEVR::PlayerCharacter* getPlayer()
    {
        const auto player = RE::PlayerCharacter::GetSingleton();
        return reinterpret_cast<F4SEVR::PlayerCharacter*>(player);
    }

    inline PlayerNodes* getPlayerNodes()
    {
        const auto player = RE::PlayerCharacter::GetSingleton();
        auto* nodes = reinterpret_cast<PlayerNodes*>(reinterpret_cast<std::uintptr_t>(player) + 0x6E0);
        return nodes;
    }

    inline RE::NiNode* getWorldRootNode()
    {
        const auto g_player = getPlayer();
        return g_player ? g_player->unkF0->rootNode : nullptr;
    }

    inline RE::BSFadeNode* getRootNode()
    {
        const auto niAVObject = getPlayer()->unkF0->rootNode->children[0];
        return niAVObject ? niAVObject->IsFadeNode() : nullptr;
    }

    // is it the 3rd-person bone tree?
    inline BSFlattenedBoneTree* getFlattenedBoneTree()
    {
        const auto g_player = getPlayer();
        return reinterpret_cast<BSFlattenedBoneTree*>(g_player->unkF0->rootNode->children[0]->IsNode());
    }

    inline RE::NiNode* getFirstPersonSkeleton()
    {
        return getPlayer()->firstPersonSkeleton;
    }

    inline BSFlattenedBoneTree* getFirstPersonBoneTree()
    {
        const auto g_player = getPlayer();
        return reinterpret_cast<BSFlattenedBoneTree*>(g_player->firstPersonSkeleton->children[0]->IsNode());
    }

    inline F4SEVR::EquippedWeaponData* getEquippedWeaponData()
    {
        const auto g_player = getPlayer();
        return g_player->middleProcess->unk08 && g_player->middleProcess->unk08->equipData
            ? g_player->middleProcess->unk08->equipData->equippedData
            : nullptr;
    }

    inline RE::NiNode* getCommonNode()
    {
        return dynamic_cast<RE::NiNode*>(getNode("COM", getRootNode()));
    }

    inline RE::NiNode* getWeaponNode()
    {
        return dynamic_cast<RE::NiNode*>(getNode("Weapon", getPlayer()->firstPersonSkeleton));
    }

    inline RE::NiNode* getPrimaryWandNode()
    {
        return dynamic_cast<RE::NiNode*>(getNode("world_primaryWand.nif", getPlayerNodes()->primaryUIAttachNode));
    }

    /**
     * The throwable weapon is attached to the melee node but only exists if the player is actively throwing the weapon.
     * @return found throwable node or nullptr if not
     */
    inline RE::NiNode* getThrowableWeaponNode()
    {
        const auto meleeNode = getPlayerNodes()->primaryMeleeWeaponOffsetNode;
        return !meleeNode->children.empty() ? meleeNode->children[0]->IsNode() : nullptr;
    }

    inline RE::NiPoint3 getCameraPosition()
    {
        // TODO: commonlibf4 migration
        return { 0, 0, 0 };
        // return (*g_playerCamera)->cameraNode->world.translate;
    }
}
