#include "utils.h"

#include "FRIK.h"
#include "f4sevr/PapyrusUtils.h"
#include "f4vr/F4VRUtils.h"
#include "f4vr/PlayerNodes.h"

using namespace common;

namespace frik
{
    void turnPlayerRadioOn(const bool isActive)
    {
        F4SEVR::execPapyrusGlobalFunction("Game", "TurnPlayerRadioOn", isActive);
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
        return MatrixUtils::isCameraLookingAtObject(cameraNode->world, objectNode->world, detectThresh);
    }

    /**
     * detect if the player has an armor item which uses the headlamp equipped as not to overwrite it
     */
    bool isArmorHasHeadLamp()
    {
        const auto biped = f4vr::getPlayer()->biped.get();
        if (!biped) {
            return false;
        }
        if (const auto equippedItem = biped->object[0].parent.object) {
            if (const auto torchEnabledArmor = equippedItem->As<RE::TESObjectARMO>()) {
                return f4vr::hasKeyword(torchEnabledArmor, 0xB34A6);
            }
        }
        return false;
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
     * The game's two VR settings that place the Pip-Boy, in Fallout4Prefs.ini: in front (bAttachPipboyToHMD) wins over
     * projected, and neither is the wrist. Null while the game has not loaded its settings.
     */
    static std::pair<RE::Setting*, RE::Setting*> getPipboyPlacementSettings()
    {
        static RE::Setting* projected = nullptr;
        static RE::Setting* inFront = nullptr;
        if (!projected || !inFront) {
            projected = RE::GetINISetting("bAlwaysUseProjectedPipboy:VR");
            inFront = RE::GetINISetting("bAttachPipboyToHMD:VR");
        }
        return { projected, inFront };
    }

    /**
     * Where the game puts the Pip-Boy now, as the game's settings menu reads it.
     */
    PipboyPlacement getPipboyPlacement()
    {
        const auto [projected, inFront] = getPipboyPlacementSettings();
        if (inFront && inFront->GetBinary()) {
            return PipboyPlacement::InFront;
        }
        return projected && projected->GetBinary() ? PipboyPlacement::Projected : PipboyPlacement::Wrist;
    }

    /**
     * Set where the game puts the Pip-Boy, as the game's settings menu does: write both settings, tell the game of each
     * (SettingHandlerMap::SettingUpdated runs the handlers the game installed on them), and save Fallout4Prefs.ini.
     * Game thread only. Returns false if the game's settings are not found.
     */
    bool setPipboyPlacement(const PipboyPlacement placement)
    {
        const auto [projected, inFront] = getPipboyPlacementSettings();
        if (!projected || !inFront) {
            logger::warn("Pip-Boy placement settings not found");
            return false;
        }

        // SettingHandlerMap::SettingUpdated(Setting&), ID 1267051
        using SettingUpdated_t = void (*)(RE::Setting&);
        static REL::Relocation<SettingUpdated_t> settingUpdated{ REL::Offset(0x1d11c30) };

        logger::info("Set Pip-Boy placement to {}", static_cast<int>(placement));
        projected->SetBinary(placement == PipboyPlacement::Projected);
        settingUpdated(*projected);
        inFront->SetBinary(placement == PipboyPlacement::InFront);
        settingUpdated(*inFront);

        if (const auto prefs = RE::INIPrefSettingCollection::GetSingleton(); prefs && prefs->Open(true)) {
            prefs->WriteSettings();
            prefs->Close();
        }
        return true;
    }

    /**
     * @return muzzle flash class only if it's fully loaded with fire and projectile nodes.
     */
    f4vr::MuzzleFlash* getMuzzleFlashNodes()
    {
        if (const auto equipWeaponData = f4vr::getEquippedWeaponData()) {
            const auto vfunc = reinterpret_cast<uint64_t*>(equipWeaponData);
            if ((*vfunc & 0xFFFF) == (f4vr::EquippedWeaponData_vfunc.get() & 0xFFFF)) {
                const auto muzzle = reinterpret_cast<f4vr::MuzzleFlash*>(equipWeaponData->muzzleFlash);
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
