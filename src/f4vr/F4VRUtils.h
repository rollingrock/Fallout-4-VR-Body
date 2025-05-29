#pragma once

#include <f4se/GameTypes.h>

#include "F4VROffsets.h"

namespace f4vr {
	static constexpr UInt32 KEYWORD_POWER_ARMOR = 0x4D8A1;
	static constexpr UInt32 KEYWORD_POWER_ARMOR_FRAME = 0x15503F;

	// UI
	void showMessagebox(const std::string& text);
	void showNotification(const std::string& text);

	// Controls
	void setControlsThumbstickEnableState(bool toEnable);

	// Weapons/Armor/Player
	void setWandsVisibility(const bool show, const bool leftWand);
	bool isMeleeWeaponEquipped();
	std::string getEquippedWeaponName();
	bool hasKeyword(const TESObjectARMO* armor, UInt32 keywordFormId);
	inline bool isJumpingOrInAir() { return IsInAir(*g_player); }
	bool isInPowerArmor();
	bool isInInternalCell();

	// settings
	inline bool isLeftHandedMode() { return *iniLeftHandedMode; }
	float getIniSettingFloat(const char* name);
	void setIniSettingBool(BSFixedString name, bool value);
	void setIniSettingFloat(BSFixedString name, float value);
	Setting* getIniSettingNative(const char* name);

	// nodes
	NiNode* getNode(const char* name, NiNode* fromNode);
	NiNode* getNode2(const char* name, NiNode* fromNode);
	NiNode* getChildNode(const char* nodeName, NiNode* nde);
	NiNode* get1StChildNode(const char* nodeName, const NiNode* nde);

	// visibility
	bool isNodeVisible(const NiNode* node);
	void showHideNode(NiAVObject* node, bool toHide);
	void setVisibility(NiAVObject* nde, bool show = true);
	void toggleVis(NiNode* nde, bool hide, bool updateSelf);

	// updates
	void updateDownFromRoot();
	void updateDown(NiNode* nde, bool updateSelf, const char* ignoreNode = nullptr);
	void updateDownTo(NiNode* toNode, NiNode* fromNode, bool updateSelf);
	void updateUpTo(NiNode* toNode, NiNode* fromNode, bool updateSelf);
	void updateTransforms(NiNode* node);
	void updateTransformsDown(NiNode* nde, bool updateSelf);

	// persistent data
	inline bool _controlsThumbstickEnableState = true;
	inline float _controlsThumbstickOriginalDeadzone = 0.25f;
	inline float _controlsThumbstickOriginalDeadzoneMax = 0.94f;
	inline float _controlsDirectionalOriginalDeadzone = 0.5f;
}
