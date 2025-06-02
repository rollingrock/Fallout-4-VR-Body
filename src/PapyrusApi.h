#pragma once

#include "Config.h"
#include "FRIK.h"
#include "HandPose.h"
#include "common/Logger.h"
#include "f4se/PapyrusNativeFunctions.h"

namespace frik {
	static void openMainConfigurationMode(StaticFunctionTag* base) {
		common::Log::info("Open Main Configuration Mode...");
		g_frik.openMainConfigurationModeActive();
	}

	static void openPipboyConfigurationMode(StaticFunctionTag* base) {
		common::Log::info("Open Pipboy Configuration Mode...");
		g_frik.openPipboyConfigurationModeActive();
	}

	static void openFrikIniFile(StaticFunctionTag* base) {
		common::Log::info("Open FRIK.ini file in notepad...");
		Config::openInNotepad();
	}

	static UInt32 getWeaponRepositionMode(StaticFunctionTag* base) {
		common::Log::info("Papyrus: Get Weapon Reposition Mode");
		return g_frik.inWeaponRepositionMode() ? 1 : 0;
	}

	static UInt32 toggleWeaponRepositionMode(StaticFunctionTag* base) {
		common::Log::info("Papyrus: Toggle Weapon Reposition Mode: %s", !g_frik.inWeaponRepositionMode() ? "ON" : "OFF");
		g_frik.toggleWeaponRepositionMode();
		return getWeaponRepositionMode(base);
	}

	static bool isLeftHandedMode(StaticFunctionTag* base) {
		common::Log::info("Papyrus: Is Left Handed Mode");
		return f4vr::isLeftHandedMode();
	}

	static void setSelfieMode(StaticFunctionTag* base, const bool isSelfieMode) {
		common::Log::info("Papyrus: Set Selfie Mode: %s", isSelfieMode ? "ON" : "OFF");
		g_frik.setSelfieMode(isSelfieMode);
	}

	static void toggleSelfieMode(StaticFunctionTag* base) {
		common::Log::info("Papyrus: toggle selfie mode");
		g_frik.setSelfieMode(!g_frik.getSelfieMode());
	}

	static void moveForward(StaticFunctionTag* base) {
		common::Log::info("Papyrus: Move Forward");
		g_config.playerOffset_forward += 1.0f;
	}

	static void moveBackward(StaticFunctionTag* base) {
		common::Log::info("Papyrus: Move Backward");
		g_config.playerOffset_forward -= 1.0f;
	}

	static void setDynamicCameraHeight(StaticFunctionTag* base, const float dynamicCameraHeight) {
		common::Log::info("Papyrus: Set Dynamic Camera Height: %f", dynamicCameraHeight);
		g_frik.setDynamicCameraHeight(dynamicCameraHeight);
	}

	// Finger pose related APIs
	static void setFingerPositionScalar2(
		StaticFunctionTag* base, const bool isLeft, const float thumb, const float index, const float middle, const float ring, const float pinky) {
		common::Log::info("Papyrus: Set Finger Position Scalar '%s' (%.3f, %.3f, %.3f, %.3f, %.3f)", isLeft ? "Left" : "Right", thumb, index, middle, ring, pinky);
		setFingerPositionScalar(isLeft, thumb, index, middle, ring, pinky);
	}

	static void restoreFingerPoseControl2(StaticFunctionTag* base, const bool isLeft) {
		common::Log::info("Papyrus: Restore Finger Pose Control '%s'", isLeft ? "Left" : "Right");
		restoreFingerPoseControl(isLeft);
	}

	/**
	 * Register code for Papyrus scripts.
	 */
	static bool registerPapyrusFunctionsCallback(VirtualMachine* vm) {
		// Register code to be accessible from Settings Holotape via Papyrus scripts
		vm->RegisterFunction(new NativeFunction0("OpenMainConfigurationMode", "FRIK:FRIK", openMainConfigurationMode, vm));
		vm->RegisterFunction(new NativeFunction0("OpenPipboyConfigurationMode", "FRIK:FRIK", openPipboyConfigurationMode, vm));
		vm->RegisterFunction(new NativeFunction0("ToggleWeaponRepositionMode", "FRIK:FRIK", toggleWeaponRepositionMode, vm));
		vm->RegisterFunction(new NativeFunction0("OpenFrikIniFile", "FRIK:FRIK", openFrikIniFile, vm));
		vm->RegisterFunction(new NativeFunction0("GetWeaponRepositionMode", "FRIK:FRIK", getWeaponRepositionMode, vm));

		/// Register mod public API to be used by other mods via Papyrus scripts
		vm->RegisterFunction(new NativeFunction0("isLeftHandedMode", "FRIK:FRIK", isLeftHandedMode, vm));
		vm->RegisterFunction(new NativeFunction1("setDynamicCameraHeight", "FRIK:FRIK", setDynamicCameraHeight, vm));
		vm->RegisterFunction(new NativeFunction0("toggleSelfieMode", "FRIK:FRIK", toggleSelfieMode, vm));
		vm->RegisterFunction(new NativeFunction1("setSelfieMode", "FRIK:FRIK", setSelfieMode, vm));
		vm->RegisterFunction(new NativeFunction0("moveForward", "FRIK:FRIK", moveForward, vm));
		vm->RegisterFunction(new NativeFunction0("moveBackward", "FRIK:FRIK", moveBackward, vm));

		// Finger pose related APIs
		vm->RegisterFunction(new NativeFunction6("setFingerPositionScalar", "FRIK:FRIK", setFingerPositionScalar2, vm));
		vm->RegisterFunction(new NativeFunction1("restoreFingerPoseControl", "FRIK:FRIK", restoreFingerPoseControl2, vm));

		return true;
	}

	static void initPapyrusApis(const F4SEInterface* f4se) {
		f4vr::registerPapyrusNativeFunctions(f4se, registerPapyrusFunctionsCallback);
	}
}
