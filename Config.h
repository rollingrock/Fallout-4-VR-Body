#pragma once

#include "VR.h"

namespace F4VRBody {

	/// <summary>
	/// Holds all the configuration variables used in the mod.
	/// Most of the configuration variables are loaded from the FRIK.ini file.
	/// Some values can be changed in-game and persisted into the FRIK.ini file.
	/// </summary>
	class Config
	{
	public:
		bool loadConfig();
		void saveSettings();

		inline void togglePipBoyTorchOnArm() {
			isPipBoyTorchOnArm = !isPipBoyTorchOnArm;
			saveBoolValue("PipBoyTorchOnArm", isPipBoyTorchOnArm);
		}
		inline void toggleIsHoloPipboy() {
			isHoloPipboy = !isHoloPipboy;
			saveBoolValue("HoloPipBoyEnabled", isHoloPipboy);
		}
		inline void toggleDampenPipboyScreen() {
			dampenPipboyScreen = !dampenPipboyScreen;
			saveBoolValue("DampenPipboyScreen", dampenPipboyScreen);
		}
		inline void togglePipBoyOpenWhenLookAt() {
			pipBoyOpenWhenLookAt = !pipBoyOpenWhenLookAt;
			saveBoolValue("PipBoyOpenWhenLookAt", pipBoyOpenWhenLookAt);
		}

		// consts
		const float selfieOutFrontDistance = 120.0f;

		// from F4 INIs
		bool leftHandedMode = false;

		// persistant in FRIK.ini
		bool verbose = false;
		float playerHeight = 0.0;
		float playerHMDHeight = 120.0;
		float shoulderToHMD = 0.0;
		float cameraHeight = 0.0;
		float PACameraHeight = 0.0;
		bool setScale = false;
		float fVrScale = 72.0;
		bool showPAHUD = true;
		float gripLetGoThreshold = 15.0f;

		// Skeleton
		bool armsOnly = false;
		bool hideHead = false;
		bool hideEquipment = false;
		bool hideSkin = false;
		float armLength = 36.74;
		float playerOffset_forward = -4.0;
		float playerOffset_up = -2.0;
		float powerArmor_forward = 0.0f;
		float powerArmor_up = 0.0f;
		float rootOffset = 0.0f;
		float PARootOffset = 0.0f;
		float handUI_X = 0.0;
		float handUI_Y = 0.0;
		float handUI_Z = 0.0;

		// Weapon
		bool staticGripping = false;
		bool enableOffHandGripping = true;
		bool enableGripButtonToGrap = true;
		bool enableGripButtonToLetGo = true;
		bool onePressGripButton = false;
		float scopeAdjustDistance = 15.0f;

		// In-game configuration
		int UISelfieButton = 2; //TODO: UNUSED! probably a bug
		int repositionButtonID = vr::EVRButtonId::k_EButton_SteamVR_Trigger; //33
		float directionalDeadzone = 0.5; // Default value of fDirectionalDeadzone, used when turning off Pipboy to restore directionial control to the player.
		bool autoFocusWindow = false;

		// Pipboy
		bool hidePipboy = false;
		bool isHoloPipboy = true; // false = Default, true = HoloPipBoy
		bool isPipBoyTorchOnArm = true; // false = Head Based Torch, true = PipBoy Based Torch
		bool leftHandedPipBoy = false;
		bool pipBoyButtonMode = false;
		bool pipBoyOpenWhenLookAt = false;
		bool pipBoyAllowMovementNotLooking = true;
		bool switchUIControltoPrimary = true; // if the player wants to switch controls or not.
		float pipBoyScale = 1.0;
		int switchTorchButton = 2; // button to switch torch from head to hand
		int pipBoyButtonArm = 0;  // 0 for left 1 for right
		int pipBoyButtonID = vr::EVRButtonId::k_EButton_Grip; // grip button is 2
		int pipBoyButtonOffArm = 0;  // 0 for left 1 for right
		int pipBoyButtonOffID = vr::EVRButtonId::k_EButton_Grip; // grip button is 2
		int gripButtonID = vr::EVRButtonId::k_EButton_Grip; // 2
		int holdDelay = 1000; // 1000 ms
		int pipBoyOffDelay = 5000; // 5000 ms
		int pipBoyOnDelay = 100; // 100 ms
		int offHandActivateButtonID = vr::EVRButtonId::k_EButton_A; // 7
		float pipBoyLookAtGate = 0.7;
		float pipboyDetectionRange = 15.0f;

		// Dampen hands
		bool dampenHands = true;
		bool dampenHandsInVanillaScope = true;
		bool dampenPipboyScreen = true;
		float dampenHandsRotation = 0.7;
		float dampenHandsTranslation = 0.7;
		float dampenHandsRotationInVanillaScope = 0.3;
		float dampenHandsTranslationInVanillaScope = 0.3;
		float dampenPipboyRotation = 0.7;
		float dampenPipboyTranslation = 0.7;

		// Smooth Movement
		bool disableSmoothMovement = false;
		float smoothingAmount = 10.0f;
		float smoothingAmountHorizontal = 0;
		float dampingMultiplier = 1.0f;
		float dampingMultiplierHorizontal = 0;
		float stoppingMultiplier = 0.2f;
		float stoppingMultiplierHorizontal = 0.2f;
		int disableInteriorSmoothing = 1;
		int disableInteriorSmoothingHorizontal = 1;

		// hide meshes
		std::vector<std::string> faceGeometry;
		std::vector<std::string> skinGeometry;
		std::vector<int> hideSlotIndexes;

	private:
		void loadHideFace();
		void loadHideSkins();
		void loadHideSlots();
		void saveBoolValue(const char* pKey, bool value);
	};

	// Not a fan of globals but it may be easiest to refactor code right now
	extern Config* g_config;

	static bool initConfig() {
		if (g_config) {
			_MESSAGE("ERROR: config already initialized");
			return false;
		}

		_MESSAGE("Init config...");
		auto config = new Config();
		auto success = config->loadConfig();
		if (success) {
			_MESSAGE("Config loaded successfully");
			g_config = config;
			return true;
		}
		else {
			_MESSAGE("ERROR: Failed to load config");
			delete config;
			return false;
		}
	}
}