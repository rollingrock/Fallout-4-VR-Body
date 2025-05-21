#pragma once

#include "Config.h"
#include "FRIK.h"
#include "HandPose.h"
#include "common/Logger.h"
#include "f4se/PapyrusNativeFunctions.h"
#include "f4vr/F4VROffsets.h"

using namespace common;

namespace frik {
	static void openMainConfigurationMode(StaticFunctionTag* base) {
		Log::info("Open Main Configuration Mode...");
		g_frik.openMainConfigurationModeActive();
	}

	static void openPipboyConfigurationMode(StaticFunctionTag* base) {
		Log::info("Open Pipboy Configuration Mode...");
		g_frik.openPipboyConfigurationModeActive();
	}

	static void openFrikIniFile(StaticFunctionTag* base) {
		Log::info("Open FRIK.ini file in notepad...");
		Config::openInNotepad();
	}

	static UInt32 getWeaponRepositionMode(StaticFunctionTag* base) {
		Log::info("Papyrus: Get Weapon Reposition Mode");
		return g_frik.inWeaponRepositionMode() ? 1 : 0;
	}

	static UInt32 toggleWeaponRepositionMode(StaticFunctionTag* base) {
		Log::info("Papyrus: Toggle Weapon Reposition Mode: %s", !g_frik.inWeaponRepositionMode() ? "ON" : "OFF");
		g_frik.toggleWeaponRepositionMode();
		return getWeaponRepositionMode(base);
	}

	static bool isLeftHandedMode(StaticFunctionTag* base) {
		Log::info("Papyrus: Is Left Handed Mode");
		return *f4vr::iniLeftHandedMode;
	}

	static void setSelfieMode(StaticFunctionTag* base, const bool isSelfieMode) {
		Log::info("Papyrus: Set Selfie Mode: %s", isSelfieMode ? "ON" : "OFF");
		g_frik.setSelfieMode(isSelfieMode);
	}

	static void toggleSelfieMode(StaticFunctionTag* base) {
		Log::info("Papyrus: toggle selfie mode");
		g_frik.setSelfieMode(!g_frik.getSelfieMode());
	}

	static void moveForward(StaticFunctionTag* base) {
		Log::info("Papyrus: Move Forward");
		g_config.playerOffset_forward += 1.0f;
	}

	static void moveBackward(StaticFunctionTag* base) {
		Log::info("Papyrus: Move Backward");
		g_config.playerOffset_forward -= 1.0f;
	}

	static void setDynamicCameraHeight(StaticFunctionTag* base, const float dynamicCameraHeight) {
		Log::info("Papyrus: Set Dynamic Camera Height: %f", dynamicCameraHeight);
		g_frik.setDynamicCameraHeight(dynamicCameraHeight);
	}

	// Sphere bone detection func
	static UInt32 registerBoneSphere(StaticFunctionTag* base, const float radius, const BSFixedString bone) {
		return g_frik.boneSpheres()->registerBoneSphere(radius, bone);
	}

	static UInt32 registerBoneSphereOffset(StaticFunctionTag* base, const float radius, const BSFixedString bone, VMArray<float> pos) {
		return g_frik.boneSpheres()->registerBoneSphereOffset(radius, bone, pos);
	}

	static void destroyBoneSphere(StaticFunctionTag* base, const UInt32 handle) {
		g_frik.boneSpheres()->destroyBoneSphere(handle);
	}

	static void registerForBoneSphereEvents(StaticFunctionTag* base, VMObject* thisObject) {
		g_frik.boneSpheres()->registerForBoneSphereEvents(thisObject);
	}

	static void unRegisterForBoneSphereEvents(StaticFunctionTag* base, VMObject* thisObject) {
		g_frik.boneSpheres()->unRegisterForBoneSphereEvents(thisObject);
	}

	static void toggleDebugBoneSpheres(StaticFunctionTag* base, const bool turnOn) {
		g_frik.boneSpheres()->toggleDebugBoneSpheres(turnOn);
	}

	static void toggleDebugBoneSpheresAtBone(StaticFunctionTag* base, const UInt32 handle, const bool turnOn) {
		g_frik.boneSpheres()->toggleDebugBoneSpheresAtBone(handle, turnOn);
	}

	// Finger pose related APIs
	static void setFingerPositionScalar2(
		StaticFunctionTag* base, const bool isLeft, const float thumb, const float index, const float middle, const float ring, const float pinky) {
		Log::info("Papyrus: Set Finger Position Scalar '%s' (%.3f, %.3f, %.3f, %.3f, %.3f)", isLeft ? "Left" : "Right", thumb, index, middle, ring, pinky);
		setFingerPositionScalar(isLeft, thumb, index, middle, ring, pinky);
	}

	static void restoreFingerPoseControl2(StaticFunctionTag* base, const bool isLeft) {
		Log::info("Papyrus: Restore Finger Pose Control '%s'", isLeft ? "Left" : "Right");
		restoreFingerPoseControl(isLeft);
	}

	/**
	 * Register code for Papyrus scripts.
	 */
	static bool registerPapyrusFunctions(VirtualMachine* vm) {
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

		// Bone Sphere interaction related APIs
		vm->RegisterFunction(new NativeFunction2("RegisterBoneSphere", "FRIK:FRIK", registerBoneSphere, vm));
		vm->RegisterFunction(new NativeFunction3("RegisterBoneSphereOffset", "FRIK:FRIK", registerBoneSphereOffset, vm));
		vm->RegisterFunction(new NativeFunction1("DestroyBoneSphere", "FRIK:FRIK", destroyBoneSphere, vm));
		vm->RegisterFunction(new NativeFunction1("RegisterForBoneSphereEvents", "FRIK:FRIK", registerForBoneSphereEvents, vm));
		vm->RegisterFunction(new NativeFunction1("UnRegisterForBoneSphereEvents", "FRIK:FRIK", unRegisterForBoneSphereEvents, vm));
		vm->RegisterFunction(new NativeFunction1("toggleDebugBoneSpheres", "FRIK:FRIK", toggleDebugBoneSpheres, vm));
		vm->RegisterFunction(new NativeFunction2("toggleDebugBoneSpheresAtBone", "FRIK:FRIK", toggleDebugBoneSpheresAtBone, vm));

		// Finger pose related APIs
		vm->RegisterFunction(new NativeFunction6("setFingerPositionScalar", "FRIK:FRIK", setFingerPositionScalar2, vm));
		vm->RegisterFunction(new NativeFunction1("restoreFingerPoseControl", "FRIK:FRIK", restoreFingerPoseControl2, vm));

		return true;
	}

	static void initPapyrusApis(const F4SEInterface* f4se) {
		const auto papyrusInterface = static_cast<F4SEPapyrusInterface*>(f4se->QueryInterface(kInterface_Papyrus));
		if (!papyrusInterface->Register(frik::registerPapyrusFunctions)) {
			throw std::exception("FAILED TO REGISTER PAPYRUS FUNCTIONS!!");
		}
	}
}
