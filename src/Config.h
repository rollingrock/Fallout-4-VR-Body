#pragma once

#include "VR.h"
#include <optional>

namespace F4VRBody {
	constexpr auto INI_SECTION_MAIN = "Fallout4VRBody";
	constexpr auto INI_SECTION_DEBUG = "Debug";
	constexpr auto INI_SECTION_SMOOTH_MOVEMENT = "SmoothMovementVR";

	/// <summary>
	/// Type of weapon related position offsets.
	/// </summary>
	enum class WeaponOffsetsMode : uint8_t {
		// The weapon offset in the primary hand.
		Weapon = 0,
		// The secondary hand gripping position offset.
		OffHand,
		// The secondary hand gripping position offset.
		Throwable,
		// Back of hand UI (HP,Ammo,etc.) offset on hand.
		BackOfHandUI,
	};

	/// <summary>
	/// Holds all the configuration variables used in the mod.
	/// Most of the configuration variables are loaded from the FRIK.ini file.
	/// Some values can be changed in-game and persisted into the FRIK.ini file.
	/// </summary>
	class Config {
	public:
		void load();
		void save() const { saveFrikINI(); }
		void onUpdateFrame();
		bool checkDebugDumpDataOnceFor(const char* name);

		inline void togglePipBoyTorchOnArm() {
			isPipBoyTorchOnArm = !isPipBoyTorchOnArm;
			saveFrikIniValue(INI_SECTION_MAIN, "PipBoyTorchOnArm", isPipBoyTorchOnArm);
		}

		inline void toggleIsHoloPipboy() {
			isHoloPipboy = !isHoloPipboy;
			saveFrikIniValue(INI_SECTION_MAIN, "HoloPipBoyEnabled", isHoloPipboy);
		}

		inline void toggleDampenPipboyScreen() {
			dampenPipboyScreen = !dampenPipboyScreen;
			saveFrikIniValue(INI_SECTION_MAIN, "DampenPipboyScreen", dampenPipboyScreen);
		}

		inline void togglePipBoyOpenWhenLookAt() {
			pipBoyOpenWhenLookAt = !pipBoyOpenWhenLookAt;
			saveFrikIniValue(INI_SECTION_MAIN, "PipBoyOpenWhenLookAt", pipBoyOpenWhenLookAt);
		}

		inline void savePipboyScale(double pipboyScale) {
			saveFrikIniValue(INI_SECTION_MAIN, "PipboyScale", pipboyScale);
		}

		inline int getAutoReloadConfigInterval() const {
			return _reloadConfigInterval;
		}

		inline void toggleAutoReloadConfig() {
			_reloadConfigInterval = _reloadConfigInterval == 0 ? 5 : 0;
			saveFrikIniValue(INI_SECTION_DEBUG, "ReloadConfigInterval", std::to_string(_reloadConfigInterval).c_str());
		}

		NiTransform getPipboyOffset();
		void savePipboyOffset(const NiTransform& transform);
		std::optional<NiTransform> getWeaponOffsets(const std::string& name, const WeaponOffsetsMode& mode, bool inPA) const;
		void saveWeaponOffsets(const std::string& name, const NiTransform& transform, const WeaponOffsetsMode& mode, bool inPA);
		void removeWeaponOffsets(const std::string& name, const WeaponOffsetsMode& mode, bool inPA, bool replaceWithEmbedded);
		void OpenInNotepad() const;

		// from F4 INIs
		bool leftHandedMode = false;

		// persistant in FRIK.ini
		float playerHeight = 0.0;
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

		// Weapon
		bool enableOffHandGripping = true;
		bool enableGripButtonToGrap = true;
		bool enableGripButtonToLetGo = true;
		bool onePressGripButton = false;
		float scopeAdjustDistance = 15.0f;

		// In-game configuration
		int repositionButtonID = vr::EVRButtonId::k_EButton_SteamVR_Trigger; //33
		float directionalDeadzone = 0.5; // Default value of fDirectionalDeadzone, used when turning off Pipboy to restore directional control to the player.
		bool autoFocusWindow = false;
		float selfieOutFrontDistance = 120.0f;
		bool selfieIgnoreHideFlags = false;
		int offHandActivateButtonID = vr::EVRButtonId::k_EButton_A; // 7

		// Pipboy
		bool hidePipboy = false;
		bool isHoloPipboy = true; // false = Default, true = HoloPipBoy
		bool isPipBoyTorchOnArm = true; // false = Head Based Torch, true = PipBoy Based Torch
		bool isPipBoyTorchRightArmMode = false; // false = torch on left arm, true = right hand
		bool leftHandedPipBoy = false;
		bool pipBoyOpenWhenLookAt = false;
		bool pipBoyAllowMovementNotLooking = true;
		bool switchUIControltoPrimary = true; // if the player wants to switch controls or not.
		float pipBoyScale = 1.0;
		int switchTorchButton = 2; // button to switch torch from head to hand
		int pipBoyButtonArm = 0; // 0 for left 1 for right
		int pipBoyButtonID = vr::EVRButtonId::k_EButton_Grip; // grip button is 2
		int pipBoyButtonOffArm = 0; // 0 for left 1 for right
		int pipBoyButtonOffID = vr::EVRButtonId::k_EButton_Grip; // grip button is 2
		int gripButtonID = vr::EVRButtonId::k_EButton_Grip; // 2
		int holdDelay = 1000; // 1000 ms
		int pipBoyOffDelay = 5000; // 5000 ms
		int pipBoyOnDelay = 100; // 100 ms
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
		std::vector<int> hideEquipSlotIndexes;

		// Can be used to test things at runtime during development
		// i.e. check "debugFlowFlag==1" somewhere in code and use config reload to change the value at runtime.
		float debugFlowFlag1 = 0;
		float debugFlowFlag2 = 0;
		float debugFlowFlag3 = 0;

	private:
		void loadFrikINI();
		void saveFrikINI() const;
		void updateLoggerLogLevel() const;
		void updateFrikINIVersion() const;
		void loadHideMeshes();
		void loadHideEquipmentSlots();
		void saveFrikIniValue(const char* section, const char* pKey, bool value);
		void saveFrikIniValue(const char* section, const char* pKey, double value);
		void saveFrikIniValue(const char* section, const char* pKey, const char* value);
		void loadPipboyOffsets();
		void loadWeaponsOffsetsFromEmbedded();
		void loadWeaponsOffsetsFromFilesystem();
		void saveOffsetsToJsonFile(const std::string& name, const NiTransform& transform, const std::string& file) const;
		static void setupFolders();
		static void migrateConfigFilesIfNeeded();

		// Reload config interval in seconds (0 - no reload)
		int _reloadConfigInterval = 0;
		time_t lastReloadTime = 0;
		// The log level to set for the logger
		int logLevel = 0;
		// The FRIK.ini version to handle updates/migrations
		int version = 0;

		std::map<std::string, NiTransform> _pipboyOffsets;
		std::map<std::string, NiTransform> _weaponsOffsets;
		std::map<std::string, NiTransform> _weaponsEmbeddedOffsets;

		std::string _debugDumpDataOnceNames;
	};

	// Not a fan of globals but it may be easiest to refactor code right now
	extern Config* g_config;

	static void initConfig() {
		if (g_config) {
			throw std::exception("Config already initialized");
		}

		auto config = new Config();
		config->load();
		g_config = config;
		_VMESSAGE("Config loaded successfully");
	}
}
