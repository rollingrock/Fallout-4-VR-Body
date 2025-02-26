#include "Pipboy.h"

namespace F4VRBody {

	Pipboy* g_pipboy = nullptr;

	/// <summary>
	/// Run Pipboy mesh replacement if not already done (or forced) to the configured meshes either holo or screen.
	/// </summary>
	/// <param name="force">true - run mesh replace, false - only if not previously replaced</param>
	void Pipboy::replaceMeshes(bool force) {
		if (force || !meshesReplaced) {
			if (c_IsHoloPipboy == 0) {
				replaceMeshes("HoloEmitter", "Screen");
			}
			else if (c_IsHoloPipboy == 1) {
				replaceMeshes("Screen", "HoloEmitter");
			}
		}
	}

	/// <summary>
	/// Hnalde replacing of Pipboy meshes on the arm with either screen or holo emitter.
	/// </summary>
	void Pipboy::replaceMeshes(std::string itemHide, std::string itemShow) {

		auto pn = playerSkelly->getPlayerNodes();
		NiNode* ui = pn->primaryUIAttachNode;
		NiNode* wand = get1stChildNode("world_primaryWand.nif", ui);
		NiNode* retNode = loadNifFromFile("Data/Meshes/FRIK/_primaryWand.nif");
		if (retNode) {
			// ui->RemoveChild(wand);
			// ui->AttachChild(retNode, true);
		}

		wand = pn->SecondaryWandNode;
		NiNode* pipParent = get1stChildNode("PipboyParent", wand);

		if (!pipParent) {
			meshesReplaced = false;
			return;
		}
		wand = get1stChildNode("PipboyRoot_NIF_ONLY", pipParent);
		c_IsHoloPipboy ? retNode = loadNifFromFile("Data/Meshes/FRIK/HoloPipboyVR.nif") : retNode = loadNifFromFile("Data/Meshes/FRIK/PipboyVR.nif");
		if (retNode && wand) {
			BSFixedString screenName("Screen:0");
			NiAVObject* newScreen = retNode->GetObjectByName(&screenName)->m_parent;

			if (!newScreen) {
				meshesReplaced = false;
				return;
			}

			pipParent->RemoveChild(wand);
			pipParent->AttachChild(retNode, true);

			pn->ScreenNode->RemoveChildAt(0);
			// using native function here to attach the new screen as too lazy to fully reverse what it's doing and it works fine.
			NiNode* rn = Offsets::addNode((uint64_t)&pn->ScreenNode, newScreen);
			pn->PipboyRoot_nif_only_node = retNode;
		}

		meshesReplaced = true;

		// Cylons Code Start >>>>
		auto lookup = g_weaponOffsets->getOffset("PipboyPosition", Mode::normal);
		if (c_IsHoloPipboy == true) {
			lookup = g_weaponOffsets->getOffset("HoloPipboyPosition", Mode::normal);
		}
		if (lookup.has_value()) {
			NiTransform pbTransform = lookup.value();
			static BSFixedString wandPipName("PipboyRoot");
			NiAVObject* pbRoot = pn->SecondaryWandNode->GetObjectByName(&wandPipName);
			if (pbRoot) {
				pbRoot->m_localTransform = pbTransform;
			}
		}
		pn->PipboyRoot_nif_only_node->m_localTransform.scale = 0.0; //prevents the VRPipboy screen from being displayed on first load whilst PB is off.
		NiNode* _HideNode = getChildNode(itemHide.c_str(), (*g_player)->unkF0->rootNode);
		if (_HideNode) {
			_HideNode->flags |= 0x1;
			_HideNode->m_localTransform.scale = 0;
		}
		NiNode* _ShowNode = getChildNode(itemShow.c_str(), (*g_player)->unkF0->rootNode);
		if (_ShowNode) {
			_ShowNode->flags &= 0xfffffffffffffffe;
			_ShowNode->m_localTransform.scale = 1;
		}
		// <<<< Cylons Code End

		_MESSAGE("Pipboy Meshes replaced! Hide: %s, Show: %s", itemHide, itemShow);

	}
}