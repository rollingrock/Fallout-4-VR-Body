#pragma once

#include <optional>

#include "api/openvr.h"
#include "common/ConfigBase.h"
#include "common/Logger.h"
#include "res/resource.h"

namespace frik {
	constexpr auto INI_SECTION_MAIN = "Fallout4VRBody";
	constexpr auto INI_SECTION_SMOOTH_MOVEMENT = "SmoothMovementVR";

	static const auto BASE_PATH = common::getRelativePathInDocuments(R"(\My Games\Fallout4VR\FRIK_Config)");
	static const auto FRIK_INI_PATH = BASE_PATH + R"(\FRIK.ini)";
	static const auto MESH_HIDE_FACE_INI_PATH = BASE_PATH + R"(\Mesh_Hide\face.ini)";
	static const auto MESH_HIDE_SKINS_INI_PATH = BASE_PATH + R"(\Mesh_Hide\skins.ini)";
	static const auto MESH_HIDE_SLOTS_INI_PATH = BASE_PATH + R"(\Mesh_Hide\slots.ini)";
	static const auto PIPBOY_HOLO_OFFSETS_PATH = BASE_PATH + R"(\Pipboy_Offsets\HoloPipboyPosition.json)";
	static const auto PIPBOY_SCREEN_OFFSETS_PATH = BASE_PATH + R"(\Pipboy_Offsets\PipboyPosition.json)";
	static const auto WEAPONS_OFFSETS_PATH = BASE_PATH + R"(\Weapons_Offsets)";

	constexpr int FRIK_INI_VERSION = 7;

	constexpr float DEFAULT_CAMERA_HEIGHT = 120.4828f;

	/**
	 * Type of weapon related position offsets.
	 */
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

	/**
	 * Holds all the configuration variables used in the mod.
	 * Most of the configuration variables are loaded from the FRIK.ini file.
	 * Some values can be changed in-game and persisted into the FRIK.ini file.
	 */
	class Config : public common::ConfigBase {
	public:
		explicit Config()
			: ConfigBase(FRIK_INI_PATH, IDR_FRIK_INI, FRIK_INI_VERSION) {}

		void loadAllConfig();
		void save() { saveIniConfig(); }

		void togglePipBoyTorchOnArm() {
			isPipBoyTorchOnArm = !isPipBoyTorchOnArm;
			saveIniConfigValue(INI_SECTION_MAIN, "PipBoyTorchOnArm", isPipBoyTorchOnArm);
		}

		void toggleIsHoloPipboy() {
			isHoloPipboy = !isHoloPipboy;
			saveIniConfigValue(INI_SECTION_MAIN, "HoloPipBoyEnabled", isHoloPipboy);
		}

		void toggleDampenPipboyScreen() {
			dampenPipboyScreen = !dampenPipboyScreen;
			saveIniConfigValue(INI_SECTION_MAIN, "DampenPipboyScreen", dampenPipboyScreen);
		}

		void togglePipBoyOpenWhenLookAt() {
			pipBoyOpenWhenLookAt = !pipBoyOpenWhenLookAt;
			saveIniConfigValue(INI_SECTION_MAIN, "PipBoyOpenWhenLookAt", pipBoyOpenWhenLookAt);
		}

		void savePipboyScale(const float pipboyScale) {
			saveIniConfigValue(INI_SECTION_MAIN, "PipboyScale", pipboyScale);
		}

		static void openInNotepad();
		NiTransform getPipboyOffset();
		void savePipboyOffset(const NiTransform& transform);
		std::optional<NiTransform> getWeaponOffsets(const std::string& name, const WeaponOffsetsMode& mode, bool inPA) const;
		void saveWeaponOffsets(const std::string& name, const NiTransform& transform, const WeaponOffsetsMode& mode, bool inPA);
		void removeWeaponOffsets(const std::string& name, const WeaponOffsetsMode& mode, bool inPA, bool replaceWithEmbedded);

		// persistent in FRIK.ini
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
		float armLength = 36.74f;
		float playerOffset_forward = -4.0;
		float playerOffset_up = -2.0;
		float powerArmor_forward = 0.0f;
		float powerArmor_up = 0.0f;
		float rootOffset = 0.0f;
		float PARootOffset = 0.0f;

		// Weapon offhand grip
		bool enableOffHandGripping = true;
		bool enableGripButtonToGrap = true;
		bool enableGripButtonToLetGo = true;
		bool onePressGripButton = false;
		float scopeAdjustDistance = 15.0f;

		// In-game configuration
		float selfieOutFrontDistance = 120.0f;
		bool selfieIgnoreHideFlags = false;

		// Pipboy
		bool hidePipboy = false;
		bool isHoloPipboy = true; // false = Default, true = HoloPipBoy
		bool isPipBoyTorchOnArm = true; // false = Head Based Torch, true = PipBoy Based Torch
		bool isPipBoyTorchRightArmMode = false; // false = torch on left arm, true = right hand
		bool leftHandedPipBoy = false;
		bool pipBoyOpenWhenLookAt = false;
		bool pipBoyAllowMovementNotLooking = true;
		bool enablePrimaryControllerPipboyUse = true;
		float pipBoyScale = 1.0;
		int switchTorchButton = 2; // button to switch torch from head to hand
		int pipBoyButtonArm = 0; // 0 for left 1 for right
		int pipBoyButtonID = vr::EVRButtonId::k_EButton_Grip; // grip button is 2
		int pipBoyButtonOffArm = 0; // 0 for left 1 for right
		int pipBoyButtonOffID = vr::EVRButtonId::k_EButton_Grip; // grip button is 2
		int gripButtonID = vr::EVRButtonId::k_EButton_Grip; // 2
		int pipBoyOffDelay = 5000; // 5000 ms
		int pipBoyOnDelay = 100; // 100 ms
		float pipBoyLookAtGate = 0.7f;
		float pipboyDetectionRange = 15.0f;

		// Dampen hands
		bool dampenHands = true;
		bool dampenHandsInVanillaScope = true;
		bool dampenPipboyScreen = true;
		float dampenHandsRotation = 0.7f;
		float dampenHandsTranslation = 0.7f;
		float dampenHandsRotationInVanillaScope = 0.3f;
		float dampenHandsTranslationInVanillaScope = 0.3f;
		float dampenPipboyRotation = 0.7f;
		float dampenPipboyTranslation = 0.7f;

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
		const std::vector<std::string>& faceGeometry() const { return _faceGeometry; }
		const std::vector<std::string>& skinGeometry() const { return _skinGeometry; }
		const std::vector<int>& hideEquipSlotIndexes() const { return _hideEquipSlotIndexes; }

	protected:
		virtual void loadIniConfigInternal(const CSimpleIniA& ini) override;
		virtual void saveIniConfigInternal(CSimpleIniA& ini) const override;

	private:
		void loadHideMeshes();
		void loadHideEquipmentSlots();
		void loadPipboyOffsets();
		void loadWeaponsOffsets();
		static void setupFolders();
		static void migrateConfigFilesIfNeeded();
		static std::string getWeaponNameWithMode(const std::string& name, const WeaponOffsetsMode& mode, bool inPA, bool leftHanded);

		// hide meshes
		std::vector<std::string> _faceGeometry;
		std::vector<std::string> _skinGeometry;
		std::vector<int> _hideEquipSlotIndexes;

		// offsets
		std::unordered_map<std::string, NiTransform> _pipboyOffsets;
		std::unordered_map<std::string, NiTransform> _weaponsOffsets;
		std::unordered_map<std::string, NiTransform> _weaponsEmbeddedOffsets;
	};

	// Not a fan of globals but it may be easiest to refactor code right now
	inline Config g_config;
}
