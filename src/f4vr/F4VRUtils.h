#pragma once

#include <f4se/GameTypes.h>
#include <f4se/PluginAPI.h>

#include "F4VROffsets.h"

namespace f4vr {
	static constexpr std::uint32_t KEYWORD_POWER_ARMOR = 0x4D8A1;
	static constexpr std::uint32_t KEYWORD_POWER_ARMOR_FRAME = 0x15503F;

	// UI
	void showMessagebox(const std::string& text);
	void showNotification(const std::string& text);

	// Controls
	void setControlsThumbstickEnableState(bool toEnable);

	// Weapons/Armor/Player
	void setWandsVisibility(bool show, bool leftWand);
	bool isMeleeWeaponEquipped();
	std::string getEquippedWeaponName();
	bool hasKeyword(const RE::TESObjectARMO* armor, std::uint32_t keywordFormId);
	inline bool isJumpingOrInAir() { return IsInAir(*g_player); }
	bool isInPowerArmor();
	bool isInInternalCell();

	// settings
	inline bool isLeftHandedMode() { return *iniLeftHandedMode; }
	float getIniSettingFloat(const char* name);
	void setIniSettingBool(RE::BSFixedString name, bool value);
	void setIniSettingFloat(RE::BSFixedString name, float value);
    RE::Setting* getIniSettingNative(const char* name);

	// nodes
	RE::NiNode* getNode(const char* name, RE::NiNode* fromNode);
	RE::NiNode* getNode2(const char* name, RE::NiNode* fromNode);
	RE::NiNode* getChildNode(const char* nodeName, RE::NiNode* nde);
	RE::NiNode* get1StChildNode(const char* nodeName, const RE::NiNode* nde);

	// visibility
	bool isNodeVisible(const RE::NiNode* node);
	void setNodeVisibility(RE::NiAVObject* node, bool show = true);
	void setNodeVisibilityDeep(RE::NiAVObject* node, bool show, bool updateSelf);
	void toggleVis(RE::NiNode* node, bool hide, bool updateSelf);

	// updates
	void updateDownFromRoot();
	void updateDown(RE::NiNode* nde, bool updateSelf, const char* ignoreNode = nullptr);
	void updateDownTo(RE::NiNode* toNode, RE::NiNode* fromNode, bool updateSelf);
	void updateUpTo(RE::NiNode* toNode, RE::NiNode* fromNode, bool updateSelf);
	void updateTransforms(RE::NiNode* node);
	void updateTransformsDown(RE::NiNode* nde, bool updateSelf);

	typedef bool (*RegisterFunctions)(RE::BSScript::Internal::VirtualMachine* vm);
	void registerPapyrusNativeFunctions(const F4SE::detail::F4SEInterface* f4se, RegisterFunctions callback);
}
