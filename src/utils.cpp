#include "utils.h"

#include "FRIK.h"
#include "f4sevr/PapyrusUtils.h"
#include "f4vr/F4VRUtils.h"
#include "f4vr/PlayerNodes.h"
#include "f4vr/VRControllersManager.h"

using namespace common;

namespace frik
{
    void turnPlayerRadioOn(bool isActive)
    {
        F4SEVR::execPapyrusGlobalFunction("Game", "TurnPlayerRadioOn", isActive);
    }

    void triggerShortHeptic()
    {
        f4vr::VRControllers.triggerHaptic(f4vr::Hand::Primary, 0.001f);
    }

    void turnPipBoyOn()
    {
        RE::Setting* set = RE::GetINISetting("fHMDToPipboyScaleOuterAngle:VRPipboy");
        set->SetFloat(0.0);

        set = RE::GetINISetting("fHMDToPipboyScaleInnerAngle:VRPipboy");
        set->SetFloat(0.0);

        set = RE::GetINISetting("fHMDToPipboyScaleInnerAngle:VRPipboy");
        set->SetFloat(0.0);

        set = RE::GetINISetting("fPipboyScaleOuterAngle:VRPipboy");
        set->SetFloat(0.0);

        set = RE::GetINISetting("fPipboyScaleInnerAngle:VRPipboy");
        set->SetFloat(0.0);
    }

    void turnPipBoyOff()
    {
        RE::Setting* set = RE::GetINISetting("fHMDToPipboyScaleOuterAngle:VRPipboy");
        set->SetFloat(20.0);

        set = RE::GetINISetting("fHMDToPipboyScaleInnerAngle:VRPipboy");
        set->SetFloat(5.0);

        set = RE::GetINISetting("fPipboyScaleOuterAngle:VRPipboy");
        set->SetFloat(20.0);

        set = RE::GetINISetting("fPipboyScaleInnerAngle:VRPipboy");
        set->SetFloat(5.0);
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
}
