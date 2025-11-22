#include "utils.h"

#include "FRIK.h"
#include "f4sevr/PapyrusUtils.h"
#include "f4vr/F4VRUtils.h"
#include "f4vr/PlayerNodes.h"
#include "f4vr/VRControllersManager.h"

using namespace common;

namespace frik
{
    void turnPlayerRadioOn(const bool isActive)
    {
        F4SEVR::execPapyrusGlobalFunction("Game", "TurnPlayerRadioOn", isActive);
    }

    void triggerStrongHaptic(const f4vr::Hand hand)
    {
        f4vr::VRControllers.triggerHaptic(hand, 0.05f, 0.5f);
    }

    void triggerShortHaptic(const f4vr::Hand hand)
    {
        f4vr::VRControllers.triggerHaptic(hand, 0.001f);
    }

    /**
     * Check if ANY pipboy open by checking if pipboy menu can be found in the UI.
     * Returns true for wrist, in-front, and projected pipboy.
     */
    bool isAnyPipboyOpen()
    {
        return RE::UI::GetSingleton()->GetMenu("PipboyMenu") != nullptr;
    }

    // Function to check if the camera is looking at the object and the object is facing the camera
    bool isCameraLookingAtObject(const RE::NiAVObject* cameraNode, const RE::NiAVObject* objectNode, const float detectThresh)
    {
        return common::isCameraLookingAtObject(cameraNode->world, objectNode->world, detectThresh);
    }

    /**
     * detect if the player has an armor item which uses the headlamp equipped as not to overwrite it
     */
    bool isArmorHasHeadLamp()
    {
        if (const auto equippedItem = f4vr::getPlayer()->equipData->slots[0].item) {
            if (const auto torchEnabledArmor = dynamic_cast<F4SEVR::TESObjectARMO*>(equippedItem)) {
                return f4vr::hasKeyword(torchEnabledArmor, 0xB34A6);
            }
        }
        return false;
    }

    /**
     * @return true if BetterScopesVR mod is loaded in the game, false otherwise.
     */
    bool isBetterScopesVRModLoaded()
    {
        auto* dataHandler = RE::TESDataHandler::GetSingleton();
        const auto mod = dataHandler ? dataHandler->LookupModByName("3dscopes-replacer.esp") : nullptr;
        return mod != nullptr;
    }

    /**
     * @return true if Fallout London VR mod is loaded in the game, false otherwise.
     */
    bool isFalloutLondonVRModLoaded()
    {
        auto* dataHandler = RE::TESDataHandler::GetSingleton();
        const auto mod = dataHandler ? dataHandler->LookupModByName("Fallout London VR.esp") : nullptr;
        return mod != nullptr;
    }

    /**
     * @return muzzle flash class only if it's fully loaded with fire and projectile nodes.
     */
    f4vr::MuzzleFlash* getMuzzleFlashNodes()
    {
        if (const auto equipWeaponData = f4vr::getEquippedWeaponData()) {
            const auto vfunc = reinterpret_cast<uint64_t*>(equipWeaponData);
            if ((*vfunc & 0xFFFF) == (f4vr::EquippedWeaponData_vfunc.get() & 0xFFFF)) {
                const auto muzzle = reinterpret_cast<f4vr::MuzzleFlash*>(equipWeaponData->unk28);
                if (muzzle && muzzle->fireNode && muzzle->projectileNode) {
                    return muzzle;
                }
            }
        }
        return nullptr;
    }

    /**
     * Get adjustment value from the thumbstick but correct it with respect to deadzone and sensitivity.
     * Deadzone is used to prevent small changes in an axis the player is not changing.
     * For example: moving the weapon right should not move it forward even if the player has small forward
     * on the thumbstick.
     */
    float correctAdjustmentValue(const float value, const float sensitivityFactor)
    {
        const float adjValue = value / sensitivityFactor;
        const float deadZone = 0.5f / sensitivityFactor;
        if (std::fabs(adjValue) < deadZone) {
            return 0;
        }
        return adjValue > 0 ? adjValue - deadZone : adjValue + deadZone;
    }
}
