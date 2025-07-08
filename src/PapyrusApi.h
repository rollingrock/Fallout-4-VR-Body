#pragma once

#include "Config.h"
#include "FRIK.h"
#include "HandPose.h"
#include "common/Logger.h"

namespace frik
{
    // Static functions for Papyrus - CommonLib F4 style (no parameters for static functions without arguments)
    static void openMainConfigurationMode(std::monostate)
    {
        common::logger::info("Open Main Configuration Mode...");
        g_frik.openMainConfigurationModeActive();
    }

    static void openPipboyConfigurationMode(std::monostate)
    {
        common::logger::info("Open Pipboy Configuration Mode...");
        g_frik.openPipboyConfigurationModeActive();
    }

    static void openFrikIniFile(std::monostate)
    {
        common::logger::info("Open FRIK.ini file in notepad...");
        Config::openInNotepad();
    }

    static std::uint32_t getWeaponRepositionMode(std::monostate)
    {
        common::logger::info("Papyrus: Get Weapon Reposition Mode");
        return g_frik.inWeaponRepositionMode() ? 1 : 0;
    }

    static std::uint32_t toggleWeaponRepositionMode(std::monostate)
    {
        common::logger::info("Papyrus: Toggle Weapon Reposition Mode: {}", !g_frik.inWeaponRepositionMode() ? "ON" : "OFF");
        g_frik.toggleWeaponRepositionMode();
        return g_frik.inWeaponRepositionMode() ? 1 : 0;
    }

    static bool isLeftHandedMode(std::monostate)
    {
        common::logger::info("Papyrus: Is Left Handed Mode");
        return f4vr::isLeftHandedMode();
    }

    static void setSelfieMode(std::monostate, const bool isSelfieMode)
    {
        common::logger::info("Papyrus: Set Selfie Mode: {}", isSelfieMode ? "ON" : "OFF");
        g_frik.setSelfieMode(isSelfieMode);
    }

    static void toggleSelfieMode(std::monostate)
    {
        common::logger::info("Papyrus: toggle selfie mode");
        g_frik.setSelfieMode(!g_frik.getSelfieMode());
    }

    static void moveForward(std::monostate)
    {
        common::logger::info("Papyrus: Move Forward");
        g_config.playerOffset_forward += 1.0f;
    }

    static void moveBackward(std::monostate)
    {
        common::logger::info("Papyrus: Move Backward");
        g_config.playerOffset_forward -= 1.0f;
    }

    static void setDynamicCameraHeight(std::monostate, const float dynamicCameraHeight)
    {
        common::logger::info("Papyrus: Set Dynamic Camera Height: {}", dynamicCameraHeight);
        g_frik.setDynamicCameraHeight(dynamicCameraHeight);
    }

    // Finger pose related APIs
    static void setFingerPositionScalar2(std::monostate, const bool isLeft, const float thumb, const float index, const float middle, const float ring, const float pinky)
    {
        common::logger::info("Papyrus: Set Finger Position Scalar '{}' ({:.3f}, {:.3f}, {:.3f}, {:.3f}, {:.3f})",
            isLeft ? "Left" : "Right", thumb, index, middle, ring, pinky);
        setFingerPositionScalar(isLeft, thumb, index, middle, ring, pinky);
    }

    static void restoreFingerPoseControl2(std::monostate, const bool isLeft)
    {
        common::logger::info("Papyrus: Restore Finger Pose Control '{}'", isLeft ? "Left" : "Right");
        restoreFingerPoseControl(isLeft);
    }

    static void initPapyrusApis()
    {
        const auto vm = RE::GameVM::GetSingleton()->GetVM();

        // Register code to be accessible from Settings Holotape via Papyrus scripts
        vm->BindNativeMethod("FRIK:FRIK", "OpenMainConfigurationMode", openMainConfigurationMode);
        vm->BindNativeMethod("FRIK:FRIK", "OpenPipboyConfigurationMode", openPipboyConfigurationMode);
        vm->BindNativeMethod("FRIK:FRIK", "ToggleWeaponRepositionMode", toggleWeaponRepositionMode);
        vm->BindNativeMethod("FRIK:FRIK", "OpenFrikIniFile", openFrikIniFile);
        vm->BindNativeMethod("FRIK:FRIK", "GetWeaponRepositionMode", getWeaponRepositionMode);

        /// Register mod public API to be used by other mods via Papyrus scripts
        vm->BindNativeMethod("FRIK:FRIK", "isLeftHandedMode", isLeftHandedMode);
        vm->BindNativeMethod("FRIK:FRIK", "setDynamicCameraHeight", setDynamicCameraHeight);
        vm->BindNativeMethod("FRIK:FRIK", "toggleSelfieMode", toggleSelfieMode);
        vm->BindNativeMethod("FRIK:FRIK", "setSelfieMode", setSelfieMode);
        vm->BindNativeMethod("FRIK:FRIK", "moveForward", moveForward);
        vm->BindNativeMethod("FRIK:FRIK", "moveBackward", moveBackward);

        // Finger pose related APIs
        vm->BindNativeMethod("FRIK:FRIK", "setFingerPositionScalar", setFingerPositionScalar2);
        vm->BindNativeMethod("FRIK:FRIK", "restoreFingerPoseControl", restoreFingerPoseControl2);
    }
}
