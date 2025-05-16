#pragma once
#include "BSFlattenedBoneTree.h"

namespace f4vr {
	// part of PlayerCharacter object but making useful struct below since not mapped in F4SE
	struct PlayerNodes {
		NiNode* playerworldnode; //0x06E0
		NiNode* roomnode; //0x06E8
		NiNode* primaryWandNode; //0x06F0
		NiNode* primaryWandandTouchPad; //0x06F8
		NiNode* primaryUIAttachNode; //0x0700
		NiNode* primaryWeapontoWeaponNode; //0x0708
		NiNode* primaryWeaponKickbackRecoilNode; //0x0710
		NiNode* primaryWeaponOffsetNOde; //0x0718
		NiNode* primaryWeaponScopeCamera; //0x0720
		NiNode* primaryVertibirdMinigunOffNOde; //0x0728
		NiNode* primaryMeleeWeaponOffsetNode; //0x0730
		NiNode* primaryUnarmedPowerArmorWeaponOffsetNode; //0x0738
		NiNode* primaryWandLaserPointer; //0x0740
		NiNode* PrimaryWandLaserPointerAdjuster; //0x0748
		NiNode* unk750; //0x0750
		NiNode* PrimaryMeleeWeaponOffsetNode; //0x0758
		NiNode* SecondaryMeleeWeaponOffsetNode; //0x0760
		NiNode* SecondaryWandNode; //0x0768
		NiNode* Point002Node; //0x0770
		NiNode* WorkshopPalletNode; //0x0778
		NiNode* WorkshopPallenSlide; //0x0780
		NiNode* SecondaryUIOffsetNode; //0x0788
		NiNode* SecondaryMeleeWeaponOffsetNode2; //0x0790
		NiNode* SecondaryUnarmedPowerArmorWeaponOffsetNode; //0x0798
		NiNode* SecondaryAimNode; //0x07A0
		NiNode* PipboyParentNode; //0x07A8
		NiNode* PipboyRoot_nif_only_node; //0x07B0
		NiNode* ScreenNode; //0x07B8
		NiNode* PipboyLightParentNode; //0x07C0
		NiNode* unk7c8; //0x07C8
		NiNode* ScopeParentNode; //0x07D0
		NiNode* unk7d8; //0x07D8
		NiNode* HmdNode; //0x07E0
		NiNode* OffscreenHmdNode; //0x07E8
		NiNode* UprightHmdNode; //0x07F0
		NiNode* UprightHmdLagNode; //0x07F8
		NiNode* BlackSphereNode; //0x0800
		NiNode* HeadLightParentNode; //0x0808
		NiNode* unk810; //0x0810
		NiNode* WeaponLeftNode; //0x0818
		NiNode* unk820; //0x0820
		NiNode* LockPickParentNode; //0x0828
	};

	inline PlayerNodes* getPlayerNodes() {
		return reinterpret_cast<PlayerNodes*>(reinterpret_cast<char*>(*g_player) + 0x6E0);
	}

	inline BSFadeNode* getRootNode() {
		return reinterpret_cast<BSFadeNode*>((*g_player)->unkF0->rootNode->m_children.m_data[0]->GetAsNiNode());
	}

	// is it the 3rd-person bone tree?
	inline BSFlattenedBoneTree* getFlattenedBoneTree() {
		return reinterpret_cast<BSFlattenedBoneTree*>((*g_player)->unkF0->rootNode->m_children.m_data[0]->GetAsNiNode());
	}

	inline BSFlattenedBoneTree* getFirstPersonBoneTree() {
		return reinterpret_cast<BSFlattenedBoneTree*>((*g_player)->firstPersonSkeleton->m_children.m_data[0]->GetAsNiNode());
	}

	inline NiNode* getHeadNode() {
		return getNode("Head", getRootNode());
	}

	inline NiNode* getWeaponNode() {
		return getNode("Weapon", (*g_player)->firstPersonSkeleton);
	}

	inline NiNode* getPrimaryWandNode() {
		return getNode("world_primaryWand.nif", getPlayerNodes()->primaryUIAttachNode);
	}

	/**
	 * The throwable weapon is attached to the melee node but only exists if the player is actively throwing the weapon.
	 * @return found throwable node or nullptr if not
	 */
	inline NiNode* getThrowableWeaponNode() {
		const auto meleeNode = getPlayerNodes()->primaryMeleeWeaponOffsetNode;
		return meleeNode->m_children.m_emptyRunStart > 0
			? meleeNode->m_children.m_data[0]->GetAsNiNode()
			: nullptr;
	}
}
