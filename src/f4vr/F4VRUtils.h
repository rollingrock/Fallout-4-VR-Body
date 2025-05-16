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
	bool isMeleeWeaponEquipped();
	std::string getEquippedWeaponName();
	bool hasKeyword(const TESObjectARMO* armor, UInt32 keywordFormId);
	inline bool isJumpingOrInAir() { return IsInAir(*g_player); }
	bool isInPowerArmor();

	// settings
	bool getLeftHandedMode();
	Setting* getINISettingNative(const char* name);
	void setINIBool(BSFixedString name, bool value);
	void setINIFloat(BSFixedString name, float value);

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
	void updateDown(NiNode* nde, bool updateSelf);
	void updateDownTo(NiNode* toNode, NiNode* fromNode, bool updateSelf);
	void updateUpTo(NiNode* toNode, NiNode* fromNode, bool updateSelf);
	void updateTransforms(NiNode* node);
	void updateTransformsDown(NiNode* nde, bool updateSelf);

	// persistent data
	inline static bool _controlsThumbstickEnableState = true;
	inline static float _controlsThumbstickOriginalDeadzone = 0.25f;
	inline static float _controlsThumbstickOriginalDeadzoneMax = 0.94f;
	inline static float _controlsDirectionalOriginalDeadzone = 0.5f;
}
