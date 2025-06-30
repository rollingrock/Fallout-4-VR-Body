#include "F4VRUtils.h"

#include <f4se/BSGeometry.h>
#include <f4se/GameRTTI.h>
#include <f4se/GameSettings.h>

#include "PlayerNodes.h"
#include "../Config.h"
#include "../common/Matrix.h"
#include "f4se/PapyrusEvents.h"

namespace f4vr {
	void showMessagebox(const std::string& text) {
		common::logger::info("Show messagebox: '{}'", text.c_str());
		auto str = RE::BSFixedString(text.c_str());
		CallGlobalFunctionNoWait1<RE::BSFixedString>("Debug", "Messagebox", str);
	}

	void showNotification(const std::string& text) {
		common::logger::info("Show notification: '{}'", text.c_str());
		auto str = RE::BSFixedString(text.c_str());
		CallGlobalFunctionNoWait1<RE::BSFixedString>("Debug", "Notification", str);
	}

	/**
	 * Set the visibility of controller wand.
	 */
	void setWandsVisibility(const bool show, const bool leftWand) {
		const auto node = leftWand ? getPlayerNodes()->primaryWandNode : getPlayerNodes()->SecondaryWandNode;
		for (UInt16 i = 0; i < node->children.m_emptyRunStart; ++i) {
			if (const auto child = node->children.data[i]) {
				if (const auto triShape = child->GetAsBSTriShape()) {
					setNodeVisibility(triShape, show);
					break;
				}
				if (!_stricmp(child->name.c_str(), "")) {
					setNodeVisibility(child, show);
					if (const auto grandChild = child->GetAsRE::NiNode()->children.data[0]) {
						setNodeVisibility(grandChild, show);
					}
					break;
				}
			}
		}
	}

	/**
	 * @return true if the equipped weapon is a melee weapon type.
	 */
	bool isMeleeWeaponEquipped() {
		if (!CombatUtilities_IsActorUsingMelee(*g_player)) {
			return false;
		}
		const auto* inventory = (*g_player)->inventoryList;
		if (!inventory) {
			return false;
		}
		for (std::uint32_t i = 0; i < inventory->items.count; i++) {
			BGSInventoryItem item;
			inventory->items.GetNthItem(i, item);
			if (item.form && item.form->formType == kFormType_WEAP && item.stack->flags & 0x3) {
				return true;
			}
		}
		return false;
	}

	/**
	 * Get the game name of the equipped weapon.
	 */
	std::string getEquippedWeaponName() {
		const auto* equipData = (*g_player)->middleProcess->unk08->equipData;
		return equipData ? equipData->item->GetFullName() : "";
	}

	bool hasKeyword(const TESObjectARMO* armor, const std::uint32_t keywordFormId) {
		if (armor) {
			for (std::uint32_t i = 0; i < armor->keywordForm.numKeywords; i++) {
				if (armor->keywordForm.keywords[i]) {
					if (armor->keywordForm.keywords[i]->formID == keywordFormId) {
						return true;
					}
				}
			}
		}
		return false;
	}

	// Thanks Shizof and SmoothMovementVR for below code
	bool isInPowerArmor() {
		if ((*g_player)->equipData) {
			if ((*g_player)->equipData->slots[0x03].item != nullptr) {
				if (const auto equippedForm = (*g_player)->equipData->slots[0x03].item) {
					if (equippedForm->formType == TESObjectARMO::kTypeID) {
						if (const auto armor = DYNAMIC_CAST(equippedForm, RE::TESForm, TESObjectARMO)) {
							return hasKeyword(armor, KEYWORD_POWER_ARMOR) || hasKeyword(armor, KEYWORD_POWER_ARMOR_FRAME);
						}
					}
				}
			}
		}
		return false;
	}

	/**
	 * Is the player is current in an "internal cell" as inside a building, cave, etc.
	 */
	bool isInInternalCell() {
		const auto cell = (*g_player)->parentCell;
		return cell && (cell->flags & TESObjectCELL::kFlag_IsInterior) == TESObjectCELL::kFlag_IsInterior;
	}

	float getIniSettingFloat(const char* name) {
		const auto setting = getIniSettingNative(name);
		return setting ? setting->data.f32 : 0;
	}

	void setIniSettingBool(const RE::BSFixedString name, bool value) {
		auto str = RE::BSFixedString(name.c_str());
		CallGlobalFunctionNoWait2<RE::BSFixedString, bool>("Utility", "SetINIBool", str, value);
	}

	void setIniSettingFloat(const RE::BSFixedString name, float value) {
		auto str = RE::BSFixedString(name.c_str());
		CallGlobalFunctionNoWait2<RE::BSFixedString, float>("Utility", "SetINIFloat", str, value);
	}

    RE::Setting* getIniSettingNative(const char* name) {
        RE::Setting* setting = SettingCollectionList_GetPtr(*g_iniSettings, name);
		if (!setting) {
			setting = SettingCollectionList_GetPtr(*g_iniPrefSettings, name);
		}

		return setting;
	}

	/**
	 * Find a node by the given name in the tree under the other given node recursively.
	 */
	RE::NiNode* getNode(const char* name, RE::NiNode* fromNode) {
		if (!fromNode || !fromNode->name) {
			return nullptr;
		}

		if (_stricmp(name, fromNode->name.c_str()) == 0) {
			return fromNode;
		}

		for (UInt16 i = 0; i < fromNode->children.m_emptyRunStart; ++i) {
			if (const auto nextNode = fromNode->children.data[i] ? fromNode->children.data[i]->GetAsRE::NiNode() : nullptr) {
				if (const auto ret = getNode(name, nextNode)) {
					return ret;
				}
			}
		}

		return nullptr;
	}

	/**
	 * Find a node by the given name in the tree under the other given node recursively.
	 * This one handles not only RE::NiNode but also BSTriShape.
	 */
	RE::NiNode* getNode2(const char* name, RE::NiNode* fromNode) {
		if (!fromNode || !fromNode->name) {
			return nullptr;
		}

		if (_stricmp(name, fromNode->name.c_str()) == 0) {
			return fromNode;
		}

		if (!fromNode->children.data) {
			return nullptr;
		}

		// TODO: use better code
		for (UInt16 i = 0; i < fromNode->children.m_emptyRunStart && fromNode->children.m_emptyRunStart < 5000; ++i) {
			if (const auto nextNode = dynamic_cast<RE::NiNode*>(fromNode->children.data[i])) {
				if (const auto ret = getNode2(name, nextNode)) {
					return ret;
				}
			}
		}

		return nullptr;
	}

	RE::NiNode* getChildNode(const char* nodeName, RE::NiNode* nde) {
		if (!nde->name) {
			return nullptr;
		}

		if (!_stricmp(nodeName, nde->name.c_str())) {
			return nde;
		}

		for (UInt16 i = 0; i < nde->children.m_emptyRunStart; ++i) {
			if (const auto nextNode = nde->children.data[i] ? nde->children.data[i]->GetAsRE::NiNode() : nullptr) {
				if (const auto ret = getChildNode(nodeName, nextNode)) {
					return ret;
				}
			}
		}

		return nullptr;
	}

	RE::NiNode* get1StChildNode(const char* nodeName, const RE::NiNode* nde) {
		for (UInt16 i = 0; i < nde->children.m_emptyRunStart; ++i) {
			if (const auto nextNode = nde->children.data[i] ? nde->children.data[i]->GetAsRE::NiNode() : nullptr) {
				if (!_stricmp(nodeName, nextNode->name.c_str())) {
					return nextNode;
				}
			}
		}
		return nullptr;
	}

	/**
	 * Return true if the node is visible, false if it is hidden or null.
	 */
	bool isNodeVisible(const RE::NiNode* node) {
		return node && !(node->flags & 0x1);
	}

	/**
	 * Change flags to show or hide a node
	 */
	void setNodeVisibility(RE::NiAVObject* node, const bool show) {
		if (node) {
			node->flags = show ? node->flags & ~0x1 : node->flags | 0x1;
		}
	}

	void setNodeVisibilityDeep(RE::NiAVObject* node, const bool show, const bool updateSelf) {
		if (node && updateSelf) {
			node->flags = show ? node->flags & ~0x1 : node->flags | 0x1;
		}
		if (const auto niNode = node->GetAsRE::NiNode()) {
			for (UInt16 i = 0; i < niNode->children.m_emptyRunStart; ++i) {
				setNodeVisibilityDeep(niNode->children.data[i], show, true);
			}
		}
	}

	// TODO: check removing this in favor of setNodeVisibilityDeep
	void toggleVis(RE::NiNode* node, const bool hide, const bool updateSelf) {
		if (updateSelf) {
			node->flags = hide ? node->flags | 0x1 : node->flags & ~0x1;
		}

		for (UInt16 i = 0; i < node->children.m_emptyRunStart; ++i) {
			if (const auto nextNode = node->children.data[i] ? node->children.data[i]->GetAsRE::NiNode() : nullptr) {
				toggleVis(nextNode, hide, true);
			}
		}
	}

	// TODO: this feels an overkill on how much it is called
	void updateDownFromRoot() {
		updateDown(getRootNode(), true);
	}

	void updateDown(RE::NiNode* nde, const bool updateSelf, const char* ignoreNode) {
		if (!nde) {
			return;
		}

		RE::NiAVObject::NiUpdateData* ud = nullptr;

		if (updateSelf) {
			nde->UpdateWorldData(ud);
		}

		for (UInt16 i = 0; i < nde->children.m_emptyRunStart; ++i) {
			if (const auto nextNode = nde->children.data[i]) {
				if (ignoreNode && _stricmp(nextNode->name.c_str(), ignoreNode) == 0) {
					continue; // skip this node
				}
				if (const auto niNode = nextNode->GetAsRE::NiNode()) {
					updateDown(niNode, true);
				} else if (const auto triNode = nextNode->GetAsBSGeometry()) {
					triNode->UpdateWorldData(ud);
				}
			}
		}
	}

	void updateDownTo(RE::NiNode* toNode, RE::NiNode* fromNode, const bool updateSelf) {
		if (!toNode || !fromNode) {
			return;
		}

		if (updateSelf) {
			RE::NiAVObject::NiUpdateData* ud = nullptr;
			fromNode->UpdateWorldData(ud);
		}

		if (_stricmp(toNode->name.c_str(), fromNode->name.c_str()) == 0) {
			return;
		}

		for (UInt16 i = 0; i < fromNode->children.m_emptyRunStart; ++i) {
			if (const auto nextNode = fromNode->children.data[i]) {
				if (const auto niNode = nextNode->GetAsRE::NiNode()) {
					updateDownTo(toNode, niNode, true);
				}
			}
		}
	}

	void updateUpTo(RE::NiNode* toNode, RE::NiNode* fromNode, const bool updateSelf) {
		if (!toNode || !fromNode) {
			return;
		}

		RE::NiAVObject::NiUpdateData* ud = nullptr;

		if (_stricmp(toNode->name.c_str(), fromNode->name.c_str()) == 0) {
			if (updateSelf) {
				fromNode->UpdateWorldData(ud);
			}
			return;
		}

		fromNode->UpdateWorldData(ud);
		if (const auto parent = fromNode->parent ? fromNode->parent->GetAsRE::NiNode() : nullptr) {
			updateUpTo(toNode, parent, true);
		}
	}

	void updateTransforms(RE::NiNode* node) {
		if (!node->parent) {
			return;
		}

		const auto& parentTransform = node->parent->world;
		const auto& localTransform = node->local;

		// Calculate world position
		const RE::NiPoint3 pos = parentTransform.rotate * (localTransform.translate * parentTransform.scale);
		node->world.translate = parentTransform.translate + pos;

		// Calculate world rotation
		common::Matrix44 loc;
		loc.makeTransformMatrix(localTransform.rotate, RE::NiPoint3(0, 0, 0));
		node->world.rotate = loc.multiply43Left(parentTransform.rotate);

		// Calculate world scale
		node->world.scale = parentTransform.scale * localTransform.scale;
	}

	void updateTransformsDown(RE::NiNode* nde, const bool updateSelf) {
		if (updateSelf) {
			updateTransforms(nde);
		}

		for (UInt16 i = 0; i < nde->children.m_emptyRunStart; ++i) {
			if (const auto nextNode = nde->children.data[i] ? nde->children.data[i]->GetAsRE::NiNode() : nullptr) {
				updateTransformsDown(nextNode, true);
			} else if (const auto triNode = nde->children.data[i] ? nde->children.data[i]->GetAsBSTriShape() : nullptr) {
				updateTransforms(reinterpret_cast<RE::NiNode*>(triNode));
			}
		}
	}

	/**
	 * Run a callback to register papyrus native functions.
	 * Functions that papyrus can call into this mod c++ code.
	 */
	void registerPapyrusNativeFunctions(const F4SE::detail::F4SEInterface* f4se, const RegisterFunctions callback) {
		const auto papyrusInterface = static_cast<F4SE::detail::F4SEPapyrusInterface*>(f4se->QueryInterface(kInterface_Papyrus));
		if (!papyrusInterface->Register(callback)) {
			throw std::exception("Failed to register papyrus functions");
		}
	}
}
