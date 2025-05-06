#include "utils.h"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <shlobj_core.h>
#include <sstream>
#include <f4se/GameMenus.h>
#include "Config.h"
#include "matrix.h"
#include "f4se/PapyrusEvents.h"

#define PI 3.14159265358979323846

namespace frik {
	RelocAddr<_AIProcess_ClearMuzzleFlashes> AIProcess_ClearMuzzleFlashes(0xecc710);
	RelocAddr<_AIProcess_CreateMuzzleFlash> AIProcess_CreateMuzzleFlash(0xecc570);

	using _SettingCollectionList_GetPtr = Setting* (*)(SettingCollectionList* list, const char* name);
	RelocAddr<_SettingCollectionList_GetPtr> SettingCollectionList_GetPtr(0x501500);

	/**
	 * Get the current time in milliseconds.
	 */
	uint64_t nowMillis() {
		const auto now = std::chrono::system_clock::now();
		return std::chrono::duration_cast<std::chrono::milliseconds>(
			now.time_since_epoch()
		).count();
	}

	std::string toStringWithPrecision(const double value, const int precision) {
		std::ostringstream stream;
		stream << std::fixed << std::setprecision(precision) << value;
		return stream.str();
	}

	float vec3Len(const NiPoint3& v1) {
		return sqrt(v1.x * v1.x + v1.y * v1.y + v1.z * v1.z);
	}

	NiPoint3 vec3Norm(NiPoint3 v1) {
		const float mag = vec3Len(v1);

		if (mag < 0.000001) {
			const float maxX = abs(v1.x);
			const float maxY = abs(v1.y);
			const float maxZ = abs(v1.z);

			if (maxX >= maxY && maxX >= maxZ) {
				return v1.x >= 0 ? NiPoint3(1, 0, 0) : NiPoint3(-1, 0, 0);
			}
			if (maxY > maxZ) {
				return v1.y >= 0 ? NiPoint3(0, 1, 0) : NiPoint3(0, -1, 0);
			}
			return v1.z >= 0 ? NiPoint3(0, 0, 1) : NiPoint3(0, 0, -1);
		}

		v1.x /= mag;
		v1.y /= mag;
		v1.z /= mag;

		return v1;
	}

	float vec3Dot(const NiPoint3& v1, const NiPoint3& v2) {
		return v1.x * v2.x + v1.y * v2.y + v1.z * v2.z;
	}

	NiPoint3 vec3Cross(const NiPoint3& v1, const NiPoint3& v2) {
		return NiPoint3(
			v1.y * v2.z - v1.z * v2.y,
			v1.z * v2.x - v1.x * v2.z,
			v1.x * v2.y - v1.y * v2.x
		);
	}

	// the determinant is proportional to the sin of the angle between two vectors.   In 3d case find the sin of the angle between v1 and v2
	// along their angle of rotation with unit vector n
	// https://stackoverflow.com/questions/14066933/direct-way-of-computing-clockwise-angle-between-2-vectors/16544330#16544330
	float vec3Det(const NiPoint3 v1, const NiPoint3 v2, const NiPoint3 n) {
		return v1.x * v2.y * n.z + v2.x * n.y * v1.z + n.x * v1.y * v2.z - v1.z * v2.y * n.x - v2.z * n.y * v1.x - n.z * v1.y * v2.x;
	}

	float degreesToRads(const float deg) {
		return deg * PI / 180;
	}

	float radsToDegrees(const float rad) {
		return rad * 180 / PI;
	}

	NiPoint3 rotateXY(const NiPoint3 vec, const float angle) {
		NiPoint3 retV;

		retV.x = vec.x * cosf(angle) - vec.y * sinf(angle);
		retV.y = vec.x * sinf(angle) + vec.y * cosf(angle);
		retV.z = vec.z;

		return retV;
	}

	NiPoint3 pitchVec(const NiPoint3 vec, const float angle) {
		const auto rotAxis = NiPoint3(vec.y, -vec.x, 0);
		Matrix44 rot;

		rot.makeTransformMatrix(getRotationAxisAngle(vec3Norm(rotAxis), angle), NiPoint3(0, 0, 0));

		return rot.make43() * vec;
	}

	// Gets a rotation matrix from an axis and an angle
	NiMatrix43 getRotationAxisAngle(NiPoint3 axis, const float theta) {
		NiMatrix43 result;
		// This math was found online http://www.euclideanspace.com/maths/geometry/rotations/conversions/angleToMatrix/
		const float c = cosf(theta);
		const float s = sinf(theta);
		const float t = 1.0 - c;
		axis = vec3Norm(axis);
		result.data[0][0] = c + axis.x * axis.x * t;
		result.data[1][1] = c + axis.y * axis.y * t;
		result.data[2][2] = c + axis.z * axis.z * t;
		float tmp1 = axis.x * axis.y * t;
		float tmp2 = axis.z * s;
		result.data[1][0] = tmp1 + tmp2;
		result.data[0][1] = tmp1 - tmp2;
		tmp1 = axis.x * axis.z * t;
		tmp2 = axis.y * s;
		result.data[2][0] = tmp1 - tmp2;
		result.data[0][2] = tmp1 + tmp2;
		tmp1 = axis.y * axis.z * t;
		tmp2 = axis.x * s;
		result.data[2][1] = tmp1 + tmp2;
		result.data[1][2] = tmp1 - tmp2;
		return result.Transpose();
	}

	void updateTransforms(NiNode* node) {
		if (!node->m_parent) {
			return;
		}

		const auto& parentTransform = node->m_parent->m_worldTransform;
		const auto& localTransform = node->m_localTransform;

		// Calculate world position
		const NiPoint3 pos = parentTransform.rot * (localTransform.pos * parentTransform.scale);
		node->m_worldTransform.pos = parentTransform.pos + pos;

		// Calculate world rotation
		Matrix44 loc;
		loc.makeTransformMatrix(localTransform.rot, NiPoint3(0, 0, 0));
		node->m_worldTransform.rot = loc.multiply43Left(parentTransform.rot);

		// Calculate world scale
		node->m_worldTransform.scale = parentTransform.scale * localTransform.scale;
	}

	void updateTransformsDown(NiNode* nde, const bool updateSelf) {
		if (updateSelf) {
			updateTransforms(nde);
		}

		for (UInt16 i = 0; i < nde->m_children.m_emptyRunStart; ++i) {
			if (const auto nextNode = nde->m_children.m_data[i] ? nde->m_children.m_data[i]->GetAsNiNode() : nullptr) {
				updateTransformsDown(nextNode, true);
			} else if (const auto triNode = nde->m_children.m_data[i] ? nde->m_children.m_data[i]->GetAsBSTriShape() : nullptr) {
				updateTransforms(reinterpret_cast<NiNode*>(triNode));
			}
		}
	}

	void toggleVis(NiNode* nde, const bool hide, const bool updateSelf) {
		if (updateSelf) {
			nde->flags = hide ? nde->flags | 0x1 : nde->flags & ~0x1;
		}

		for (UInt16 i = 0; i < nde->m_children.m_emptyRunStart; ++i) {
			if (const auto nextNode = nde->m_children.m_data[i] ? nde->m_children.m_data[i]->GetAsNiNode() : nullptr) {
				toggleVis(nextNode, hide, true);
			}
		}
	}

	void setINIBool(const BSFixedString name, bool value) {
		CallGlobalFunctionNoWait2<BSFixedString, bool>("Utility", "SetINIBool", BSFixedString(name.c_str()), value);
	}

	void setINIFloat(const BSFixedString name, float value) {
		CallGlobalFunctionNoWait2<BSFixedString, float>("Utility", "SetINIFloat", BSFixedString(name.c_str()), value);
	}

	void showMessagebox(const std::string& asText) {
		CallGlobalFunctionNoWait1<BSFixedString>("Debug", "Messagebox", BSFixedString(asText.c_str()));
	}

	void showNotification(const std::string& asText) {
		CallGlobalFunctionNoWait1<BSFixedString>("Debug", "Notification", BSFixedString(asText.c_str()));
	}

	void turnPlayerRadioOn(bool isActive) {
		CallGlobalFunctionNoWait1<bool>("Game", "TurnPlayerRadioOn", isActive);
	}

	void configureGameVars() {
		setINIFloat("fPipboyMaxScale:VRPipboy", 3.0000);
		setINIFloat("fPipboyMinScale:VRPipboy", 0.0100f);
		setINIFloat("fVrPowerArmorScaleMultiplier:VR", 1.0000);
	}

	void windowFocus() {
		const HWND hwnd = ::FindWindowEx(nullptr, nullptr, "Fallout4VR", nullptr);
		if (!hwnd) {
			showMessagebox("Window Not Found");
			return;
		}
		const HWND foreground = GetForegroundWindow();
		if (foreground != hwnd) {
			{
				//PostMessage(hwnd, WM_SYSCOMMAND, SC_MINIMIZE, 0);
				//PostMessage(hwnd, WM_SYSCOMMAND, SC_RESTORE, 0);
				SendMessage(foreground, WM_SYSCOMMAND, SC_MINIMIZE, 0); // restore the minimize window
				SendMessage(hwnd, WM_SYSCOMMAND, SC_RESTORE, 0); // restore the minimize window
				SetForegroundWindow(hwnd);
				SetActiveWindow(hwnd);
				SetWindowPos(hwnd, HWND_TOP, 0, 0, 0, 0, SWP_SHOWWINDOW | SWP_NOMOVE | SWP_NOSIZE);
			}
		}
	}

	void simulateExtendedButtonPress(WORD vkey) {
		if (auto hwnd = ::FindWindowEx(nullptr, nullptr, "Fallout4VR", nullptr)) {
			HWND foreground = GetForegroundWindow();
			if (foreground && hwnd == foreground) {
				INPUT input;
				input.type = INPUT_KEYBOARD;
				input.ki.wScan = MapVirtualKey(vkey, MAPVK_VK_TO_VSC);
				input.ki.time = 0;
				input.ki.dwExtraInfo = 0;
				input.ki.wVk = vkey;
				if (vkey == VK_UP || vkey == VK_DOWN) {
					input.ki.dwFlags = KEYEVENTF_EXTENDEDKEY; //0; //KEYEVENTF_KEYDOWN
				} else {
					input.ki.dwFlags = 0;
				}
				SendInput(1, &input, sizeof(INPUT));
				Sleep(30);
				if (vkey == VK_UP || vkey == VK_DOWN) {
					input.ki.dwFlags = KEYEVENTF_KEYUP | KEYEVENTF_EXTENDEDKEY;
				} else {
					input.ki.dwFlags = KEYEVENTF_KEYUP;
				}
				SendInput(1, &input, sizeof(INPUT));
			}
		}
	}

	void turnPipBoyOn() {
		/*  From IdleHands
			Utility.SetINIFloat("fHMDToPipboyScaleOuterAngle:VRPipboy", 0.0000)
			Utility.SetINIFloat("fHMDToPipboyScaleInnerAngle:VRPipboy", 0.0000)
			Utility.SetINIFloat("fPipboyScaleOuterAngle:VRPipboy", 0.0000)
			Utility.SetINIFloat("fPipboyScaleInnerAngle:VRPipboy", 0.0000)
		*/
		Setting* set = GetINISetting("fHMDToPipboyScaleOuterAngle:VRPipboy");
		set->SetDouble(0.0);

		set = GetINISetting("fHMDToPipboyScaleInnerAngle:VRPipboy");
		set->SetDouble(0.0);

		set = GetINISetting("fPipboyScaleOuterAngle:VRPipboy");
		set->SetDouble(0.0);

		set = GetINISetting("fPipboyScaleInnerAngle:VRPipboy");
		set->SetDouble(0.0);

		if (g_config->autoFocusWindow && g_config->switchUIControltoPrimary) {
			windowFocus();
		}
	}

	void turnPipBoyOff() {
		/*  From IdleHands
	Utility.SetINIFloat("fHMDToPipboyScaleOuterAngle:VRPipboy", 20.0000)
	Utility.SetINIFloat("fHMDToPipboyScaleInnerAngle:VRPipboy", 5.0000)
	Utility.SetINIFloat("fPipboyScaleOuterAngle:VRPipboy", 20.0000)
	Utility.SetINIFloat("fPipboyScaleInnerAngle:VRPipboy", 5.0000)
		*/
		Setting* set = GetINISetting("fHMDToPipboyScaleOuterAngle:VRPipboy");
		set->SetDouble(20.0);

		set = GetINISetting("fHMDToPipboyScaleInnerAngle:VRPipboy");
		set->SetDouble(5.0);

		set = GetINISetting("fPipboyScaleOuterAngle:VRPipboy");
		set->SetDouble(20.0);

		set = GetINISetting("fPipboyScaleInnerAngle:VRPipboy");
		set->SetDouble(5.0);

		rotationStickEnabledToggle(true);
	}

	/**
	 * Check if ANY pipboy open by checking if pipboy menu can be found in the UI.
	 * Returns true for wrist, in-front, and projected pipboy.
	 */
	bool isAnyPipboyOpen() {
		BSFixedString pipboyMenu("PipboyMenu");
		return (*g_ui)->GetMenu(pipboyMenu) != nullptr;
	}

	// TODO: move those to the VR handler that will handle all controls logic like press
	static bool _controlsThumbstickEnableState = true;
	static float _controlsThumbstickOriginalDeadzone = 0.25f;
	static float _controlsThumbstickOriginalDeadzoneMax = 0.94f;
	static float _controlsDirectionalOriginalDeadzone = 0.5f;

	/**
	 * If to enable/disable the use of both controllers analog thumbstick.
	 */
	void setControlsThumbstickEnableState(const bool toEnable) {
		if (_controlsThumbstickEnableState == toEnable) {
			return; // no change
		}
		_controlsThumbstickEnableState = toEnable;
		if (toEnable) {
			setINIFloat("fLThumbDeadzone:Controls", _controlsThumbstickOriginalDeadzone);
			setINIFloat("fLThumbDeadzoneMax:Controls", _controlsThumbstickOriginalDeadzoneMax);
			setINIFloat("fDirectionalDeadzone:Controls", _controlsDirectionalOriginalDeadzone);
		} else {
			_controlsThumbstickOriginalDeadzone = GetINISetting("fLThumbDeadzone:Controls")->data.f32;
			_controlsThumbstickOriginalDeadzoneMax = GetINISetting("fLThumbDeadzoneMax:Controls")->data.f32;
			_controlsDirectionalOriginalDeadzone = GetINISetting("fDirectionalDeadzone:Controls")->data.f32;
			setINIFloat("fLThumbDeadzone:Controls", 1.0);
			setINIFloat("fLThumbDeadzoneMax:Controls", 1.0);
			setINIFloat("fDirectionalDeadzone:Controls", 1.0);
		}
	}

	/**
	 * If to enable/disable the use of right stick for player rotation.
	 * Used to disable for pipboy usage and weapon repositions.
	 */
	void rotationStickEnabledToggle(const bool enable) {
		setINIFloat("fDirectionalDeadzone:Controls", enable ? g_config->directionalDeadzone : 1.0);
	}

	/**
	 * Return true if the node is visible, false if it is hidden or null.
	 */
	bool isNodeVisible(const NiNode* node) {
		return node && !(node->flags & 0x1);
	}

	/**
	 * Update the node flags to show/hide it.
	 */
	void showHideNode(NiAVObject* node, const bool toHide) {
		if (toHide) {
			node->flags |= 0x1; // hide
		} else {
			node->flags &= 0xfffffffffffffffe; // show
		}
	}

	/**
	 * Get the correct right/left-handed  config and whatever primary or secondary is requested.
	 * Example: right is primary for right-handed  mode, but left is primary for left-handed  mode.
	 */
	static VRHook::VRSystem::TrackerType getTrackerTypeForCorrectHand(const bool primary) {
		return g_config->leftHandedMode
			? primary
			? VRHook::VRSystem::TrackerType::Left
			: VRHook::VRSystem::TrackerType::Right
			: primary
			? VRHook::VRSystem::TrackerType::Right
			: VRHook::VRSystem::TrackerType::Left;
	}

	/**
	 * Get the input controller state object for the primary controller depending on left handmode.
	 * Regular primary is right hand, but if left hand mode is on then primary is left hand.
	 */
	vr::VRControllerState_t getControllerState(const bool primary) {
		const auto tracker = getTrackerTypeForCorrectHand(primary);
		return VRHook::g_vrHook->getControllerState(tracker);
	}

	/**
	 * Check if the given button is pressed AFTER NOT being pressed on the primary/secondary input controller.
	 * This will return true for ONE frame only when the button is first pressed.
	 * Regular primary is right hand, but if left hand mode is on then primary is left hand.
	 */
	bool isButtonPressedOnController(const bool primary, int buttonId) {
		const auto tracker = getTrackerTypeForCorrectHand(primary);
		const auto prevInput = VRHook::g_vrHook->getControllerPreviousState(tracker).ulButtonPressed;
		const auto input = VRHook::g_vrHook->getControllerState(tracker).ulButtonPressed;
		const auto button = vr::ButtonMaskFromId(static_cast<vr::EVRButtonId>(buttonId));
		return !(prevInput & button) && input & button;
	}

	/**
	 * Check if the given button is pressed and is HELD down on the primary/secondary input controller.
	 * This will return true for EVERY frame while the button is pressed.
	 * Regular primary is right hand, but if left hand mode is on then primary is left hand.
	 */
	bool isButtonPressHeldDownOnController(const bool primary, int buttonId) {
		const auto tracker = getTrackerTypeForCorrectHand(primary);
		const auto prevInput = VRHook::g_vrHook->getControllerPreviousState(tracker).ulButtonPressed;
		const auto input = VRHook::g_vrHook->getControllerState(tracker).ulButtonPressed;
		const auto button = vr::ButtonMaskFromId(static_cast<vr::EVRButtonId>(buttonId));
		return prevInput & button && input & button;
	}

	/**
	 * Check if the given button was released AFTER being pressed on the primary/secondary input controller.
	 * This will return true for ONE frame only when the button is first released.
	 * Regular primary is right hand, but if left hand mode is on then primary is left hand.
	 */
	bool isButtonReleasedOnController(const bool primary, int buttonId) {
		const auto tracker = getTrackerTypeForCorrectHand(primary);
		const auto prevInput = VRHook::g_vrHook->getControllerPreviousState(tracker).ulButtonPressed;
		const auto input = VRHook::g_vrHook->getControllerState(tracker).ulButtonPressed;
		const auto button = vr::ButtonMaskFromId(static_cast<vr::EVRButtonId>(buttonId));
		return prevInput & button && !(input & button);
	}

	/**
	 * Check if the given button is long pressed on the primary/secondary input controller.
	 * This will return true for EVERY frame when the button is pressed for longer then longPressDuration.
	 * Regular primary is right hand, but if left hand mode is on then primary is left hand.
	 */
	bool isButtonLongPressedOnController(const bool primary, int buttonId, const int longPressDuration) {
		const auto tracker = getTrackerTypeForCorrectHand(primary);
		const auto longPress = VRHook::g_vrHook->getControllerLongButtonPressedState(tracker);
		if (longPress.startTimeMilisec == 0 || nowMillis() - longPress.startTimeMilisec < longPressDuration) {
			return false;
		}
		const auto button = vr::ButtonMaskFromId(static_cast<vr::EVRButtonId>(buttonId));
		return longPress.ulButtonPressed & button;
	}

	/**
	 * Check if the given button is long pressed on the primary/secondary input controller and clear the state if it is.
	 * This will return true for ONE frame when the button is pressed for longer then longPressDuration. But if the
	 * player continues to hold the button it will return true again after longPressDuration passed again.
	 * Regular primary is right hand, but if left hand mode is on then primary is left hand.
	 */
	bool checkAndClearButtonLongPressedOnController(const bool primary, const int buttonId, const int longPressDuration) {
		const auto isButtonLongPressed = isButtonLongPressedOnController(primary, buttonId, longPressDuration);
		if (isButtonLongPressed) {
			VRHook::g_vrHook->clearControllerLongButtonPressedState(getTrackerTypeForCorrectHand(primary));
		}
		return isButtonLongPressed;
	}

	// Function to check if the camera is looking at the object and the object is facing the camera
	bool isCameraLookingAtObject(const NiAVObject* cameraNode, const NiAVObject* objectNode, const float detectThresh) {
		// Get the position of the camera and the object
		const NiPoint3 cameraPos = cameraNode->m_worldTransform.pos;
		const NiPoint3 objectPos = objectNode->m_worldTransform.pos;

		// Calculate the direction vector from the camera to the object
		const NiPoint3 direction = vec3Norm(NiPoint3(objectPos.x - cameraPos.x, objectPos.y - cameraPos.y, objectPos.z - cameraPos.z));

		// Get the forward vector of the camera (assuming it's the y-axis)
		const NiPoint3 cameraForward = vec3Norm(cameraNode->m_worldTransform.rot * NiPoint3(0, 1, 0));

		// Get the forward vector of the object (assuming it's the y-axis)
		const NiPoint3 objectForward = vec3Norm(objectNode->m_worldTransform.rot * NiPoint3(0, 1, 0));

		// Check if the camera is looking at the object
		const float cameraDot = vec3Dot(cameraForward, direction);
		const bool isCameraLooking = cameraDot > detectThresh; // Adjust the threshold as needed

		// Check if the object is facing the camera
		const float objectDot = vec3Dot(objectForward, direction);
		const bool isObjectFacing = objectDot > detectThresh; // Adjust the threshold as needed

		return isCameraLooking && isObjectFacing;
	}

	std::string getEquippedWeaponName() {
		const auto* equipData = (*g_player)->middleProcess->unk08->equipData;
		return equipData ? equipData->item->GetFullName() : "";
	}

	/**
	 * @return true if the equipped weapon is a melee weapon type.
	 */
	bool isMeleeWeaponEquipped() {
		if (!Offsets::CombatUtilities_IsActorUsingMelee(*g_player)) {
			return false;
		}
		const auto* inventory = (*g_player)->inventoryList;
		if (!inventory) {
			return false;
		}
		for (UInt32 i = 0; i < inventory->items.count; i++) {
			BGSInventoryItem item;
			inventory->items.GetNthItem(i, item);
			if (item.form && item.form->formType == kFormType_WEAP && item.stack->flags & 0x3) {
				return true;
			}
		}
		return false;
	}

	bool getLeftHandedMode() {
		const Setting* set = GetINISetting("bLeftHandedMode:VR");

		return set->data.u8;
	}

	NiNode* getChildNode(const char* nodeName, NiNode* nde) {
		if (!nde->m_name) {
			return nullptr;
		}

		if (!_stricmp(nodeName, nde->m_name.c_str())) {
			return nde;
		}

		for (UInt16 i = 0; i < nde->m_children.m_emptyRunStart; ++i) {
			if (const auto nextNode = nde->m_children.m_data[i] ? nde->m_children.m_data[i]->GetAsNiNode() : nullptr) {
				if (const auto ret = getChildNode(nodeName, nextNode)) {
					return ret;
				}
			}
		}

		return nullptr;
	}

	NiNode* get1StChildNode(const char* nodeName, const NiNode* nde) {
		for (UInt16 i = 0; i < nde->m_children.m_emptyRunStart; ++i) {
			if (const auto nextNode = nde->m_children.m_data[i] ? nde->m_children.m_data[i]->GetAsNiNode() : nullptr) {
				if (!_stricmp(nodeName, nextNode->m_name.c_str())) {
					return nextNode;
				}
			}
		}
		return nullptr;
	}

	Setting* getINISettingNative(const char* name) {
		Setting* setting = SettingCollectionList_GetPtr(*g_iniSettings, name);
		if (!setting) {
			setting = SettingCollectionList_GetPtr(*g_iniPrefSettings, name);
		}

		return setting;
	}

	std::string str_tolower(std::string s) {
		std::transform(s.begin(), s.end(), s.begin(),
			[](const unsigned char c) { return std::tolower(c); }
		);
		return s;
	}

	std::string ltrim(std::string s) {
		s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](const unsigned char ch) {
			return !std::isspace(ch);
		}));
		return s;
	}

	std::string rtrim(std::string s) {
		s.erase(std::find_if(s.rbegin(), s.rend(), [](const unsigned char ch) {
			return !std::isspace(ch);
		}).base(), s.end());
		return s;
	}

	std::string trim(const std::string& s) {
		return ltrim(rtrim(s));
	}

	/**
	 * Find dll embedded resource by id and return its data as string if exists.
	 * Return null if the resource is not found.
	 */
	std::optional<std::string> getEmbeddedResourceAsStringIfExists(const WORD resourceId) {
		// Must specify the dll to read its resources and not the exe
		const HMODULE hModule = GetModuleHandle("FRIK.dll");
		const HRSRC hRes = FindResource(hModule, MAKEINTRESOURCE(resourceId), RT_RCDATA);
		if (!hRes) {
			return std::nullopt;
		}

		const HGLOBAL hResData = LoadResource(hModule, hRes);
		if (!hResData) {
			return std::nullopt;
		}

		const DWORD dataSize = SizeofResource(hModule, hRes);
		const void* pData = LockResource(hResData);
		if (!pData) {
			return std::nullopt;
		}

		return std::string(static_cast<const char*>(pData), dataSize);
	}

	/**
	 * Find dll embedded resource by id and return its data as string.
	 */
	std::string getEmbededResourceAsString(const WORD resourceId) {
		// Must specify the dll to read its resources and not the exe
		const HMODULE hModule = GetModuleHandle("FRIK.dll");
		const HRSRC hRes = FindResource(hModule, MAKEINTRESOURCE(resourceId), RT_RCDATA);
		if (!hRes) {
			throw std::runtime_error("Resource not found for id: " + std::to_string(resourceId));
		}

		const HGLOBAL hResData = LoadResource(hModule, hRes);
		if (!hResData) {
			throw std::runtime_error("Failed to load resource for id: " + std::to_string(resourceId));
		}

		const DWORD dataSize = SizeofResource(hModule, hRes);
		const void* pData = LockResource(hResData);
		if (!pData) {
			throw std::runtime_error("Failed to lock resource for id: " + std::to_string(resourceId));
		}

		return std::string(static_cast<const char*>(pData), dataSize);
	}

	/**
	 * Get a simple string of the current time in HH:MM:SS format.
	 */
	std::string getCurrentTimeString() {
		const std::time_t now = std::time(nullptr);
		std::tm localTime;
		localtime_s(&localTime, &now);
		char buffer[9];
		std::strftime(buffer, sizeof(buffer), "%H:%M:%S", &localTime);
		return std::string(buffer);
	}

	/**
	 * Loads a list of string values from a file.
	 * Each value is expected to be on a new line.
	 */
	std::vector<std::string> loadListFromFile(const std::string& filePath) {
		std::ifstream input;
		input.open(filePath);

		std::vector<std::string> list;
		if (input.is_open()) {
			while (input) {
				std::string lineStr;
				input >> lineStr;
				if (!lineStr.empty()) {
					list.push_back(trim(str_tolower(lineStr)));
				}
			}
		}

		input.close();
		return list;
	}

	/**
	 * Create a folder structure if it doesn't exist.
	 * Check if the given path ends with a file name and if so, remove it.
	 */
	void createDirDeep(const std::string& pathStr) {
		auto path = std::filesystem::path(pathStr);
		if (path.has_extension()) {
			path = path.parent_path();
		}
		if (!std::filesystem::exists(path)) {
			_MESSAGE("Creating directory: %s", path.string().c_str());
			std::filesystem::create_directories(path);
		}
	}

	/**
	 * If file at a given path doesn't exist then create it from the embedded resource.
	 */
	void createFileFromResourceIfNotExists(const std::string& filePath, const WORD resourceId, const bool fixNewline) {
		if (std::filesystem::exists(filePath)) {
			return;
		}

		_MESSAGE("Creating '%s' file from resource id: %d...", filePath.c_str(), resourceId);
		auto data = getEmbededResourceAsString(resourceId);

		if (fixNewline) {
			// Remove all \r to ensure it uses only \n for new lines as ini library creates empty lines
			std::erase(data, '\r');
		}

		std::ofstream outFile(filePath, std::ios::trunc);
		if (!outFile) {
			throw std::runtime_error("Failed to create '" + filePath + "' file");
		}
		if (!outFile.write(data.data(), data.size())) {
			outFile.close();
			std::remove(filePath.c_str());
			throw std::runtime_error("Failed to write to '" + filePath + "' file");
		}
		outFile.close();

		_VMESSAGE("File '%s' created successfully (size: %d)", filePath.c_str(), data.size());
	}

	/**
	 * Get path in the My Documents folder.
	 */
	std::string getRelativePathInDocuments(const std::string& relPath) {
		char documentsPath[MAX_PATH];
		if (SUCCEEDED(SHGetFolderPath(NULL, CSIDL_MYDOCUMENTS, NULL, 0, documentsPath))) {
			return std::string(documentsPath) + relPath;
		}
		throw std::runtime_error("Failed to get My Documents folder path");
	}

	/**
	 * Safely (no override, no errors) move a file from fromPath to toPath.
	 */
	void moveFileSafe(const std::string& fromPath, const std::string& toPath) {
		try {
			if (!std::filesystem::exists(fromPath)) {
				return;
			}
			if (std::filesystem::exists(toPath)) {
				_MESSAGE("Moving '%s' to '%s' failed, file already exists", fromPath.c_str(), toPath.c_str());
				return;
			}
			_MESSAGE("Moving '%s' to '%s'", fromPath.c_str(), toPath.c_str());
			std::filesystem::rename(fromPath, toPath);
		} catch (const std::exception& e) {
			_ERROR("Failed to move file to new location: %s", e.what());
		}
	}

	/**
	 * Safely (no override, no errors) move all files (and files only) in the fromPath to the toPath.
	 */
	void moveAllFilesInFolderSafe(const std::string& fromPath, const std::string& toPath) {
		if (!std::filesystem::exists(fromPath)) {
			return;
		}
		for (const auto& entry : std::filesystem::directory_iterator(fromPath)) {
			if (entry.is_regular_file()) {
				const auto& sourcePath = entry.path();
				const auto destinationPath = toPath / sourcePath.filename();
				moveFileSafe(sourcePath.string(), destinationPath.string());
			}
		}
	}

	/**
	 * @return true if BetterScopesVR mod is loaded in the game, false otherwise.
	 */
	bool isBetterScopesVRModLoaded() {
		return isModLoaded("FO4VRBETTERSCOPES");
	}

	/**
	 * @return true if a mod by specific name is loaded in the game, false otherwise.
	 */
	bool isModLoaded(const char* modName) {
		const DataHandler* dataHandler = *g_dataHandler;
		if (!dataHandler) {
			return false;
		}
		for (auto i = 0; i < dataHandler->modList.loadedModCount; ++i) {
			const auto modInfo = dataHandler->modList.loadedMods[i];
			if (_stricmp(modInfo->name, modName) == 0) {
				return true;
			}
		}
		return false;
	}
}
