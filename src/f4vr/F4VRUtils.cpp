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
		common::Log::info("Show messagebox: '%s'", text.c_str());
		auto str = BSFixedString(text.c_str());
		CallGlobalFunctionNoWait1<BSFixedString>("Debug", "Messagebox", str);
	}

	void showNotification(const std::string& text) {
		common::Log::info("Show notification: '%s'", text.c_str());
		auto str = BSFixedString(text.c_str());
		CallGlobalFunctionNoWait1<BSFixedString>("Debug", "Notification", str);
	}

	/**
	 * If to enable/disable the use of both controllers analog thumbstick.
	 */
	void setControlsThumbstickEnableState(const bool toEnable) {
		if (_controlsThumbstickEnableState == toEnable) {
			return; // no change
		}
		_controlsThumbstickEnableState = toEnable;
		if (toEnable) {
			setIniSettingFloat("fLThumbDeadzone:Controls", _controlsThumbstickOriginalDeadzone);
			setIniSettingFloat("fLThumbDeadzoneMax:Controls", _controlsThumbstickOriginalDeadzoneMax);
			setIniSettingFloat("fDirectionalDeadzone:Controls", _controlsDirectionalOriginalDeadzone);
		} else {
			const auto controlsThumbstickOriginalDeadzone = getIniSettingFloat("fLThumbDeadzone:Controls");
			if (controlsThumbstickOriginalDeadzone < 1) {
				_controlsThumbstickOriginalDeadzone = controlsThumbstickOriginalDeadzone;
				_controlsThumbstickOriginalDeadzoneMax = getIniSettingFloat("fLThumbDeadzoneMax:Controls");
				_controlsDirectionalOriginalDeadzone = getIniSettingFloat("fDirectionalDeadzone:Controls");
			} else {
				common::Log::warn("Controls thumbstick deadzone is already set to 1.0, not changing it.");
			}
			setIniSettingFloat("fLThumbDeadzone:Controls", 1.0);
			setIniSettingFloat("fLThumbDeadzoneMax:Controls", 1.0);
			setIniSettingFloat("fDirectionalDeadzone:Controls", 1.0);
		}
	}

	/**
	 * Set the visibility of controller wand.
	 */
	void setWandsVisibility(const bool show, const bool leftWand) {
		const auto node = leftWand ? getPlayerNodes()->primaryWandNode : getPlayerNodes()->SecondaryWandNode;
		for (UInt16 i = 0; i < node->m_children.m_emptyRunStart; ++i) {
			if (const auto child = node->m_children.m_data[i]) {
				if (const auto triShape = child->GetAsBSTriShape()) {
					setVisibility(triShape, show);
					break;
				}
				if (!_stricmp(child->m_name.c_str(), "")) {
					setVisibility(child, show);
					if (const auto grandChild = child->GetAsNiNode()->m_children.m_data[0]) {
						setVisibility(grandChild, show);
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
		for (UInt32 i = 0; i < inventory->items.count; i++) {
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

	bool hasKeyword(const TESObjectARMO* armor, const UInt32 keywordFormId) {
		if (armor) {
			for (UInt32 i = 0; i < armor->keywordForm.numKeywords; i++) {
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
						if (const auto armor = DYNAMIC_CAST(equippedForm, TESForm, TESObjectARMO)) {
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

	void setIniSettingBool(const BSFixedString name, bool value) {
		auto str = BSFixedString(name.c_str());
		CallGlobalFunctionNoWait2<BSFixedString, bool>("Utility", "SetINIBool", str, value);
	}

	void setIniSettingFloat(const BSFixedString name, float value) {
		auto str = BSFixedString(name.c_str());
		CallGlobalFunctionNoWait2<BSFixedString, float>("Utility", "SetINIFloat", str, value);
	}

	Setting* getIniSettingNative(const char* name) {
		Setting* setting = SettingCollectionList_GetPtr(*g_iniSettings, name);
		if (!setting) {
			setting = SettingCollectionList_GetPtr(*g_iniPrefSettings, name);
		}

		return setting;
	}

	/**
	 * Find a node by the given name in the tree under the other given node recursively.
	 */
	NiNode* getNode(const char* name, NiNode* fromNode) {
		if (!fromNode || !fromNode->m_name) {
			return nullptr;
		}

		if (_stricmp(name, fromNode->m_name.c_str()) == 0) {
			return fromNode;
		}

		for (UInt16 i = 0; i < fromNode->m_children.m_emptyRunStart; ++i) {
			if (const auto nextNode = fromNode->m_children.m_data[i] ? fromNode->m_children.m_data[i]->GetAsNiNode() : nullptr) {
				if (const auto ret = getNode(name, nextNode)) {
					return ret;
				}
			}
		}

		return nullptr;
	}

	/**
	 * Find a node by the given name in the tree under the other given node recursively.
	 * This one handles not only NiNode but also BSTriShape.
	 */
	NiNode* getNode2(const char* name, NiNode* fromNode) {
		if (!fromNode || !fromNode->m_name) {
			return nullptr;
		}

		if (_stricmp(name, fromNode->m_name.c_str()) == 0) {
			return fromNode;
		}

		if (!fromNode->m_children.m_data) {
			return nullptr;
		}

		// TODO: use better code
		for (UInt16 i = 0; i < fromNode->m_children.m_emptyRunStart && fromNode->m_children.m_emptyRunStart < 5000; ++i) {
			if (const auto nextNode = dynamic_cast<NiNode*>(fromNode->m_children.m_data[i])) {
				if (const auto ret = getNode2(name, nextNode)) {
					return ret;
				}
			}
		}

		return nullptr;
	}

	NiNode* getChildNode(const char* nodeName, NiNode* nde) {
		if (!nde->m_name) {
			return nullptr;
		}

		if (!_stricmp(nodeName, nde->m_name.c_str())) {
			return nde;
		}

		for (UInt16 i = 0; i < nde->m_children.m_emptyRunStart; ++i) {
			if (const auto nextNode = nde->m_children.m_data[i] ? nde->m_children.m_data[i]->GetAsNiNode() : nullptr) {
				if (const auto ret = getChildNode(nodeName, nextNode)) {
					return ret;
				}
			}
		}

		return nullptr;
	}

	NiNode* get1StChildNode(const char* nodeName, const NiNode* nde) {
		for (UInt16 i = 0; i < nde->m_children.m_emptyRunStart; ++i) {
			if (const auto nextNode = nde->m_children.m_data[i] ? nde->m_children.m_data[i]->GetAsNiNode() : nullptr) {
				if (!_stricmp(nodeName, nextNode->m_name.c_str())) {
					return nextNode;
				}
			}
		}
		return nullptr;
	}

	/**
	 * Return true if the node is visible, false if it is hidden or null.
	 */
	bool isNodeVisible(const NiNode* node) {
		return node && !(node->flags & 0x1);
	}

	/**
	 * Update the node flags to show/hide it.
	 */
	void showHideNode(NiAVObject* node, const bool toHide) {
		if (toHide) {
			node->flags |= 0x1; // hide
		} else {
			node->flags &= 0xfffffffffffffffe; // show
		}
	}

	/**
	 * Change flags to show or hide a node
	 */
	void setVisibility(NiAVObject* nde, const bool show) {
		if (nde) {
			nde->flags = show ? nde->flags & ~0x1 : nde->flags | 0x1;
		}
	}

	void toggleVis(NiNode* nde, const bool hide, const bool updateSelf) {
		if (updateSelf) {
			nde->flags = hide ? nde->flags | 0x1 : nde->flags & ~0x1;
		}

		for (UInt16 i = 0; i < nde->m_children.m_emptyRunStart; ++i) {
			if (const auto nextNode = nde->m_children.m_data[i] ? nde->m_children.m_data[i]->GetAsNiNode() : nullptr) {
				toggleVis(nextNode, hide, true);
			}
		}
	}

	// TODO: this feels an overkill on how much it is called
	void updateDownFromRoot() {
		updateDown(getRootNode(), true);
	}

	void updateDown(NiNode* nde, const bool updateSelf, const char* ignoreNode) {
		if (!nde) {
			return;
		}

		NiAVObject::NiUpdateData* ud = nullptr;

		if (updateSelf) {
			nde->UpdateWorldData(ud);
		}

		for (UInt16 i = 0; i < nde->m_children.m_emptyRunStart; ++i) {
			if (const auto nextNode = nde->m_children.m_data[i]) {
				if (ignoreNode && _stricmp(nextNode->m_name.c_str(), ignoreNode) == 0) {
					continue; // skip this node
				}
				if (const auto niNode = nextNode->GetAsNiNode()) {
					updateDown(niNode, true);
				} else if (const auto triNode = nextNode->GetAsBSGeometry()) {
					triNode->UpdateWorldData(ud);
				}
			}
		}
	}

	void updateDownTo(NiNode* toNode, NiNode* fromNode, const bool updateSelf) {
		if (!toNode || !fromNode) {
			return;
		}

		if (updateSelf) {
			NiAVObject::NiUpdateData* ud = nullptr;
			fromNode->UpdateWorldData(ud);
		}

		if (_stricmp(toNode->m_name.c_str(), fromNode->m_name.c_str()) == 0) {
			return;
		}

		for (UInt16 i = 0; i < fromNode->m_children.m_emptyRunStart; ++i) {
			if (const auto nextNode = fromNode->m_children.m_data[i]) {
				if (const auto niNode = nextNode->GetAsNiNode()) {
					updateDownTo(toNode, niNode, true);
				}
			}
		}
	}

	void updateUpTo(NiNode* toNode, NiNode* fromNode, const bool updateSelf) {
		if (!toNode || !fromNode) {
			return;
		}

		NiAVObject::NiUpdateData* ud = nullptr;

		if (_stricmp(toNode->m_name.c_str(), fromNode->m_name.c_str()) == 0) {
			if (updateSelf) {
				fromNode->UpdateWorldData(ud);
			}
			return;
		}

		fromNode->UpdateWorldData(ud);
		if (const auto parent = fromNode->m_parent ? fromNode->m_parent->GetAsNiNode() : nullptr) {
			updateUpTo(toNode, parent, true);
		}
	}

	void updateTransforms(NiNode* node) {
		if (!node->m_parent) {
			return;
		}

		const auto& parentTransform = node->m_parent->m_worldTransform;
		const auto& localTransform = node->m_localTransform;

		// Calculate world position
		const NiPoint3 pos = parentTransform.rot * (localTransform.pos * parentTransform.scale);
		node->m_worldTransform.pos = parentTransform.pos + pos;

		// Calculate world rotation
		common::Matrix44 loc;
		loc.makeTransformMatrix(localTransform.rot, NiPoint3(0, 0, 0));
		node->m_worldTransform.rot = loc.multiply43Left(parentTransform.rot);

		// Calculate world scale
		node->m_worldTransform.scale = parentTransform.scale * localTransform.scale;
	}

	void updateTransformsDown(NiNode* nde, const bool updateSelf) {
		if (updateSelf) {
			updateTransforms(nde);
		}

		for (UInt16 i = 0; i < nde->m_children.m_emptyRunStart; ++i) {
			if (const auto nextNode = nde->m_children.m_data[i] ? nde->m_children.m_data[i]->GetAsNiNode() : nullptr) {
				updateTransformsDown(nextNode, true);
			} else if (const auto triNode = nde->m_children.m_data[i] ? nde->m_children.m_data[i]->GetAsBSTriShape() : nullptr) {
				updateTransforms(reinterpret_cast<NiNode*>(triNode));
			}
		}
	}
}
