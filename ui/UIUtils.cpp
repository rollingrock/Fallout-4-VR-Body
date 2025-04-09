#pragma once

#include "UIUtils.h"

// TODO: refactor to move this dependancy to common code
#include "../Offsets.h"
#include "../Config.h"

namespace ui {

	/// <summary>
	/// Get struct with usefull NiNodes references related to player.
	/// </summary>
	F4VRBody::PlayerNodes* getPlayerNodes() {
		return reinterpret_cast<F4VRBody::PlayerNodes*>(reinterpret_cast<char*>(*g_player) + 0x6E0);
	}

	/// <summary>
	/// Update the node flags to show/hide it.
	/// </summary>
	void setNodeVisibility(NiNode* node, bool visible) {
		if (visible)
			node->flags &= 0xfffffffffffffffe; // show
		else
			node->flags |= 0x1; // hide
	}

	/// <summary>
	/// Get a NiNode that can be used in game UI for the given .nif file.
	/// Why is just loading not enough?
	/// </summary>
	NiNode* getClonedNiNodeForNifFile(const std::string& path) {
		auto& normPath = path._Starts_with("Data") ? path : "Data/Meshes/" + path;
		NiNode* nifNode = loadNifFromFile(normPath.c_str());
		NiCloneProcess proc;
		proc.unk18 = Offsets::cloneAddr1;
		proc.unk48 = Offsets::cloneAddr2;
		auto uiNode = Offsets::cloneNode(nifNode, &proc);
        uiNode->m_name = BSFixedString(path.c_str());
		return uiNode;
	}

	/// <summary>
	/// Load .nif file from the filesystem and return the root node.
	/// </summary>
	NiNode* loadNifFromFile(const char* path) {
		uint64_t flags[2] = { 0x0, 0xed };
		uint64_t mem = 0;
		int ret = Offsets::loadNif((uint64_t) & (*path), (uint64_t)&mem, (uint64_t)&flags);
		return (NiNode*)mem;
	}

	/// <summary>
	/// Find node by name in the given sub-tree of the given root node.
	/// </summary>
	NiNode* findNode(const char* nodeName, NiNode* node) {
		if (!node || !node->m_name) {
			return nullptr;
		}

		if (_stricmp(nodeName, node->m_name.c_str()) == 0) {
			return node;
		}

		if (node->GetAsNiNode()) {
			for (auto i = 0; i < node->m_children.m_emptyRunStart; ++i) {
				auto nextNode = node->m_children.m_data[i];
				if (nextNode) {
					if (auto ret = findNode(nodeName, (NiNode*)nextNode)) {
						return ret;
					}
				}
			}
		}
		return nullptr;
	}

	/// <summary>
	/// Attach the given node to the primary wand node to be rendered near it.
	/// </summary>
	void attachNodeToPrimaryWand(const NiNode* node) {
		auto primaryWandNode = findNode("world_primaryWand.nif", getPlayerNodes()->primaryUIAttachNode);
		if(primaryWandNode)
			primaryWandNode->AttachChild((NiAVObject*)node, true);
		else
			_WARNING("Primary wand node not found!");
	}
}
