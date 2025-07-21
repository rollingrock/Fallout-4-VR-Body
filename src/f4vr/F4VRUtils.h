#pragma once
#include "F4VROffsets.h"

namespace f4vr
{
    static constexpr std::uint32_t KEYWORD_POWER_ARMOR = 0x4D8A1;
    static constexpr std::uint32_t KEYWORD_POWER_ARMOR_FRAME = 0x15503F;

    // UI
    void showMessagebox(const std::string& text);
    void showNotification(const std::string& text);

    // Controls
    void setControlsThumbstickEnableState(bool toEnable);
    void closeFavoriteMenu();

    // Weapons/Armor/Player
    void setWandsVisibility(bool show, bool leftWand);
    bool isMeleeWeaponEquipped();
    std::string getEquippedWeaponName();
    bool hasKeyword(const F4SEVR::TESObjectARMO* armor, std::uint32_t keywordFormId);
    bool isJumpingOrInAir();
    bool isInPowerArmor();
    bool isInInternalCell();
    bool isSwimming(const RE::PlayerCharacter* player = RE::PlayerCharacter::GetSingleton());
    bool isUnderwater(const RE::PlayerCharacter* player = RE::PlayerCharacter::GetSingleton());
    bool isMovementSafe(RE::PlayerCharacter* player, const RE::NiPoint3& currentPos, const RE::NiPoint3& targetPos);

    // settings
    bool isLeftHandedMode();
    bool useWandDirectionalMovement();
    RE::Setting* getIniSetting(const char* name, bool addNew = false);

    // nodes
    RE::NiAVObject* findAVObject(RE::NiAVObject* node, const std::string& name, const int maxDepth = 999);
    RE::NiNode* findNode(RE::NiAVObject* node, const char* name, const int maxDepth = 999);
    RE::NiNode* find1StChildNode(RE::NiAVObject* node, const char* name);

    // visibility
    bool isNodeVisible(const RE::NiNode* node);
    void setNodeVisibility(RE::NiAVObject* node, bool show);
    void setNodeVisibilityDeep(RE::NiAVObject* node, bool show, bool updateSelf = true);

    // updates
    void updateDownFromRoot();
    void updateDown(RE::NiAVObject* node, bool updateSelf, const char* ignoreNode = nullptr);
    void updateDownTo(RE::NiNode* toNode, RE::NiNode* fromNode, bool updateSelf);
    void updateUpTo(RE::NiNode* toNode, RE::NiNode* fromNode, bool updateSelf);
    void updateTransforms(RE::NiNode* node);
    void updateTransformsDown(RE::NiNode* node, bool updateSelf);

    void registerPapyrusNativeFunctions(F4SE::PapyrusInterface::RegisterFunctions callback);

    // CommonLib migration
    RE::NiNode* loadNifFromFile(const std::string& path);
    RE::NiNode* getClonedNiNodeForNifFile(const std::string& path, const std::string& name = "");
    void attachChildToNode(RE::NiNode* node, RE::NiAVObject* child, bool firstAvail = true);
    void detachChildFromNode(RE::NiNode* node, RE::NiAVObject* child, RE::NiPointer<RE::NiAVObject>& out);
    void removeChildFromNode(RE::NiNode* node, RE::NiAVObject* child);
    void removeChildAtFromNode(RE::NiNode* node, int childIndex);
    void updateNodeWorldData(RE::NiAVObject* node);
    F4SEVR::NiNode* getF4SEVRNode(RE::NiNode* node);
}
