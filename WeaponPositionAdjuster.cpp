#include "WeaponPositionAdjuster.h"
#include "Config.h"
#include "Skeleton.h"
#include "F4VRBody.h"
#include "Debug.h"

namespace F4VRBody {

	/// <summary>
	/// Run every game frame to reposition (position and rotation) the weapon and the scope camera (optical scope)
	/// with regard to weapon custom offsets and/or weapon gripping by offhand (non-primary hand).
	/// </summary>
	void WeaponPositionAdjuster::onFrameUpdate() {
		auto weapon = _skelly->getWeaponNode();
		if (!weapon || !(*g_player)->actorState.IsWeaponDrawn()) {
			return;
		}

		// store original weapon transform in case we need it later
		_weaponOriginalTransform = weapon->m_localTransform;
		// override the weapon transform to the saved offset
		weapon->m_localTransform = _weaponOffsetTransform;
		// update world transform for later calculations
		updateTransforms(weapon);

		checkEquippedWeaponChanged();

		handleWeaponRepositionConfigMode(weapon);

		checkIfOffhandIsGripping(weapon);

		handleWeaponGrippingRotationAdjustment(weapon);

		handleScopeCameraAdjustmentByWeaponOffset(weapon);

		_skelly->updateDown(weapon, true);

		debugPrintWeaponPositionData();
	}

	/// <summary>
	/// If equipped weapon changed set offsets to stored if exists.
	/// </summary>
	void WeaponPositionAdjuster::checkEquippedWeaponChanged() {
		auto& weapname = getEquippedWeaponName();
		if (weapname == _lastWeapon && _skelly->inPowerArmor() == _lastWeaponInPA) {
			// no weapon change
			return;
		}

		_lastWeapon = weapname;
		_lastWeaponInPA = _skelly->inPowerArmor();

		// reset state
		_offHandGripping = false;

		// Load stored offsets for the new weapon
		auto weaponOffsetLookup = g_config->getWeaponOffsets(weapname, _skelly->inPowerArmor() ? WeaponOffsetsMode::powerArmor : WeaponOffsetsMode::normal);
		if (weaponOffsetLookup.has_value()) {
			_weaponOffsetTransform = weaponOffsetLookup.value();
			_MESSAGE("Use weapon offset for '%s'; Pos: (%2.2f, %2.2f, %2.2f), Scale: %2.3f, InPA: %d",
				weapname.c_str(), _weaponOffsetTransform.pos.x, _weaponOffsetTransform.pos.y, _weaponOffsetTransform.pos.z, _weaponOffsetTransform.scale, _skelly->inPowerArmor());
		}
		else {
			// No stored offset, use original weapon transform
			_weaponOffsetTransform = _weaponOriginalTransform;
		}

		// Load stored offsets for offhand for the new weapon 
		auto offhandOffsetLookup = g_config->getWeaponOffsets(weapname, _skelly->inPowerArmor() ? WeaponOffsetsMode::offHandwithPowerArmor : WeaponOffsetsMode::offHand);
		if (offhandOffsetLookup.has_value()) {
			_offhandOffsetTransform = offhandOffsetLookup.value();
			_MESSAGE("Use offHand offset for '%s'; Pos: (%2.2f, %2.2f, %2.2f), Scale: %2.3f, InPA: %d",
				weapname.c_str(), _offhandOffsetTransform.pos.x, _offhandOffsetTransform.pos.y, _offhandOffsetTransform.pos.z, _offhandOffsetTransform.scale, _skelly->inPowerArmor());
		}
		else {
			// No stored offset for offhand, use default values (small vertical offset)
			_offhandOffsetTransform = NiTransform();
			_offhandOffsetTransform.pos = NiPoint3(0, 0, 2);
		}
	}
	
	/// <summary>
	/// In weapon reposition mode handle controllers input to adjust weapon position.
	/// Depending on buttons pressed we can horizontal/vertical position or rotation.
	/// </summary>
	void WeaponPositionAdjuster::handleWeaponRepositionConfigMode(NiNode* weapon) {
		if (isButtonReleasedOnController(true, vr::EVRButtonId::k_EButton_A)) {
			// TODO: remove after testing (quick enter/exit reposition mode)
			_inRepositionConfigMode = !_inRepositionConfigMode;
			rotationStickEnabledToggle(!_inRepositionConfigMode);
			std::string state = _inRepositionConfigMode ? "Enabled" : "Disabled";
			ShowNotification("Testing: Weapon Reposition Mode - " + state);
		}

		if (!_inRepositionConfigMode) {
			return;
		}

		if (checkAndClearButtonLongPressedOnController(true, vr::EVRButtonId::k_EButton_Grip)) {
			ShowNotification("Reset Weapon Position to Default");
			_vrhook->StartHaptics(g_config->leftHandedMode ? 1 : 2, 0.5, 0.4);
			_weaponOffsetTransform = _weaponOriginalTransform;
		}

		if (checkAndClearButtonLongPressedOnController(true, vr::EVRButtonId::k_EButton_A)) {
			ShowNotification("Saving Weapon Position");
			_vrhook->StartHaptics(g_config->leftHandedMode ? 1 : 2, 0.5, 0.4);
			g_config->saveWeaponOffsets(_lastWeapon, _weaponOffsetTransform, _lastWeaponInPA ? WeaponOffsetsMode::powerArmor : WeaponOffsetsMode::normal);
		}

		vr::VRControllerAxis_t axis_state = getControllerState(true).rAxis[0];
		if (axis_state.x == 0.f && axis_state.y == 0.f) {
			return;
		}
		if (isButtonPressHeldDownOnController(false, vr::EVRButtonId::k_EButton_A)) {
			Matrix44 rot;
			if (isButtonPressHeldDownOnController(false, vr::EVRButtonId::k_EButton_Grip)) {
				// roll rotation
				rot.setEulerAngles(0, -degrees_to_rads(axis_state.x / 3), 0);
			}
			else {
				// pitch and yaw rotation
				rot.setEulerAngles(-degrees_to_rads(axis_state.y / 5), 0, degrees_to_rads(axis_state.x / 5));
			}
			_weaponOffsetTransform.rot = rot.multiply43Left(_weaponOffsetTransform.rot);
		}
		else if (isButtonPressHeldDownOnController(false, vr::EVRButtonId::k_EButton_Grip)) {
			// adjust vertical (z - up/down)
			_weaponOffsetTransform.pos.z -= axis_state.y / 10;
		}
		else {
			// adjust horizontal (y - right/left, x - forward/backward) (default - no buttons pressed)
			_weaponOffsetTransform.pos.y += axis_state.x / 10;
			_weaponOffsetTransform.pos.x += axis_state.y / 10;
		}

		// update the weapon with the offset change
		weapon->m_localTransform = _weaponOffsetTransform;
	}

	/// <summary>
	/// Update the vanilla scope camera node to match weapon reposition.
	/// Offset is a simple adjustment of the diff between original and offset transform.
	/// Rotation is calculated by using weapon forward vector and the diff between it and spright in scope camera orientation.
	/// </summary>
	void WeaponPositionAdjuster::handleScopeCameraAdjustmentByWeaponOffset(NiNode* weapon) {
		auto scopeCamera = _skelly->getPlayerNodes()->primaryWeaponScopeCamera;

		// Apply the position offset is weird because of different coordinate system
		auto weaponPosDiff = _weaponOffsetTransform.pos - _weaponOriginalTransform.pos;
		scopeCamera->m_localTransform.pos = NiPoint3(weaponPosDiff.y, weaponPosDiff.x, -weaponPosDiff.z);
		
		// SUPER HACK adjustment because position offset adjustment in one axis is casing small drift in other axises!
		// The numbers were created by chaning single offset axis and manually adjusting the other two axis until could hit the target
		scopeCamera->m_localTransform.pos.z += (weaponPosDiff.y > 0 ? 0.12 : 0.04) * weaponPosDiff.y + (weaponPosDiff.x > 0 ? 0.14 : 0.04) * weaponPosDiff.x;
		scopeCamera->m_localTransform.pos.x += (weaponPosDiff.x > 0 ? -0.1 : -0.11) * weaponPosDiff.x + (weaponPosDiff.z > 0 ? 0.05 : 0.06) * weaponPosDiff.z;
		
		if (_offHandGripping) {
			// if offhand is gripping it overrides the rotation adjustment
			return;
		}
		
		// need to update default transform for later world rotation use
		scopeCamera->m_localTransform.rot = _scopeCameraBaseMatrix;
		updateTransforms(scopeCamera);

		// get the "forward" vector of the weapon (direction of the bullets)
		auto weaponForwardVec = NiPoint3(weapon->m_worldTransform.rot.data[1][0], weapon->m_worldTransform.rot.data[1][1], weapon->m_worldTransform.rot.data[1][2]);

		// Calculate the rotation adjustment using quaternion by diff between scope camera vector and straight
		Quaternion rotAdjust;
		auto weaponForwardVecInScopeTransform = scopeCamera->m_worldTransform.rot.Transpose() * weaponForwardVec / scopeCamera->m_worldTransform.scale;
		rotAdjust.vec2vec(weaponForwardVecInScopeTransform, NiPoint3(1, 0, 0));
		scopeCamera->m_localTransform.rot = rotAdjust.getRot().multiply43Left(_scopeCameraBaseMatrix);
	}

	/// <summary>
	/// Figure out if player should be gripping the weapon with offhand, handling all the different offhand grip modes:   | enableGripButtonToGrap | onePressGripButton | enableGripButtonToLetGo |
	/// Mode 1: hand automatically snap to the barrel when in range, move hand quickly to let go                          | false                  | false              | false                   |
	/// Mode 2: hand automatically snap to the barrel when in range, press grip button to let go                          | false                  | false              | true                    |
	/// Mode 3: holding grip button to snap to the barrel, release grib button to let go                                  | true                   | true               | false                   |
	/// Mode 4: press grip button to snap to the barrel, press grib button again to let go                                | true                   | false              | true                    |
	/// </summary>
	void WeaponPositionAdjuster::checkIfOffhandIsGripping(NiNode* weapon) {
		if (!g_config->enableOffHandGripping) {
			return;
		}

		if (_offHandGripping) {
			if (g_config->onePressGripButton && !isButtonPressHeldDownOnController(false, g_config->gripButtonID)) {
				// Mode 3 release grip when not holding the grip button
				_offHandGripping = false;
			}

			if (g_config->enableGripButtonToLetGo && isButtonPressedOnController(false, g_config->gripButtonID)) {
				if (g_config->enableGripButtonToGrap || !isOffhandCloseToBarrel(weapon)) {
					// Mode 2,4 release grip on pressing the grip button again
					_offHandGripping = false;
				}
				else {
					// Mode 2 but close to barrel, so ignore ungrip as it will grip on next frame
					_vrhook->StartHaptics(g_config->leftHandedMode ? 0 : 1, 0.05, 0.3);
				}
			}

			if (!g_config->enableGripButtonToGrap && !g_config->enableGripButtonToLetGo && !c_isLookingThroughScope && isOffhandMovedFastAway()) {
				// mode 1 release when move fast away from barrel
				_offHandGripping = false;
			}

			// already gripping, no need for checks to grip
			return;
		}

		if (!g_config->enableGripButtonToGrap && !g_config->enableGripButtonToLetGo) {
			// mode 1 extra calculation for past 3 frames, annoying but only if mode 1 is in use
			isOffhandMovedFastAway();
		}

		if (!isOffhandCloseToBarrel(weapon)) {
			// not close to barrel, no need to grip
			return;
		}

		if (!g_config->enableGripButtonToGrap) {
			// Mode 1,2 grab when close to barrel
			_offHandGripping = true;
		} else if (!g_pipboy->status() && isButtonPressedOnController(false, g_config->gripButtonID)) {
			// Mode 3,4 grab when pressing grip button
			_offHandGripping = true;
		}
	}

	/// <summary>
	/// Handle adjusting weapon and scope camera rotation when offhand is gripping the weapon.
	/// Calculate the weapon to offhand vector and then adjust the weapon and scope camera rotation to match the vector.
	/// </summary>
	void WeaponPositionAdjuster::handleWeaponGrippingRotationAdjustment(NiNode* weapon) {
		if (!g_config->enableOffHandGripping || !_offHandGripping) {
			return;
		}

		// calculate the vector from weapon to offhand in world coordinates
		// This vector will be used to adjust the rotation of the weapon and scope camera
		auto weaponToOffhandVec = getOffhand2WeaponVector(weapon);
		
		// adjust by the saved offhand offset
		weaponToOffhandVec += _offhandOffsetTransform.pos;
		
		// Handle weapon:
		// Calculate the rotation adjustment using quaternion by diff between weapon vector and straight
		Quaternion rotAdjust;
		auto offhandNormVecInWeaponTransform = weapon->m_worldTransform.rot.Transpose() * vec3_norm(weaponToOffhandVec) / weapon->m_worldTransform.scale;
		rotAdjust.vec2vec(offhandNormVecInWeaponTransform, NiPoint3(0, 1, 0));
		weapon->m_localTransform.rot = rotAdjust.getRot().multiply43Left(_weaponOffsetTransform.rot);

		// Handle scope camera:
		// need to adjust the scope camera and update world transform so we can use it for scope vector
		auto scopeCamera = _skelly->getPlayerNodes()->primaryWeaponScopeCamera;
		scopeCamera->m_localTransform.rot = _scopeCameraBaseMatrix;
		updateTransforms(scopeCamera);

		// Calculate the rotation adjustment using quaternion by diff between scope camera vector and spright
		auto weaponToOffhandVecinScopeTransform = scopeCamera->m_worldTransform.rot.Transpose() * vec3_norm(weaponToOffhandVec) / scopeCamera->m_worldTransform.scale;
		rotAdjust.vec2vec(weaponToOffhandVecinScopeTransform, NiPoint3(1, 0, 0));
		scopeCamera->m_localTransform.rot = rotAdjust.getRot().multiply43Left(_scopeCameraBaseMatrix);
	}

	/// <summary>
	/// Return true if the angle of offhand to weapon to grip is close to barrel but far in distance from main hand 
	/// to prevent grabbing when two hands are just close.
	/// </summary>
	bool WeaponPositionAdjuster::isOffhandCloseToBarrel(NiNode* weapon) {
		auto offhand2WeaponVec = getOffhand2WeaponVector(weapon);
		float distanceFromPrimaryHand = vec3_len(offhand2WeaponVec);
		auto NormalizedVec = weapon->m_worldTransform.rot.Transpose() * vec3_norm(offhand2WeaponVec) / weapon->m_worldTransform.scale;
		float angleDiffToWeaponVec = vec3_dot(vec3_norm(NormalizedVec), NiPoint3(0, 1, 0));
		return angleDiffToWeaponVec > 0.955 && distanceFromPrimaryHand > 15;
	}

	/// <summary>
	/// Check if the offhand moved fast away in the last 3 frames.
	/// offhandFingerBonePos, bodyPos, and avgHandV are static and therefor persist between frames.
	/// It's a bit weird calculation and TBH I don't know if players really use this mode...
	/// </summary>
	bool WeaponPositionAdjuster::isOffhandMovedFastAway() {
		static NiPoint3 bodyPos = NiPoint3(0, 0, 0);
		static NiPoint3 offhandFingerBonePos = NiPoint3(0, 0, 0);
		static float avgHandV[3] = { 0.0f, 0.0f, 0.0f };
		static int fc = 0;
		float handV = 0.0f;
		auto offHandBone = g_config->leftHandedMode ? "RArm_Finger31" : "LArm_Finger31";

		BSFlattenedBoneTree* rt = (BSFlattenedBoneTree*)_skelly->getRoot();
		float handFrameMovement = vec3_len(rt->transforms[_skelly->getBoneInMap(offHandBone)].world.pos - offhandFingerBonePos);

		float bodyFrameMovement = vec3_len(_skelly->getCurrentBodyPos() - bodyPos);
		avgHandV[fc] = abs(handFrameMovement - bodyFrameMovement);
		fc = (fc + 1) % 3;

		float sum = 0;
		for (int i = 0; i < 3; i++) {
			sum += avgHandV[i];
		}
		handV = sum / 3;

		bodyPos = _skelly->getCurrentBodyPos();
		offhandFingerBonePos = rt->transforms[_skelly->getBoneInMap(offHandBone)].world.pos;

		return handV > g_config->gripLetGoThreshold;
	}

	/// <summary>
	/// calculate the vector from weapon to offhand in world coordinates
	/// This vector will be used to adjust the rotation of the weapon and scope camera
	/// </summary>
	NiPoint3 WeaponPositionAdjuster::getOffhand2WeaponVector(NiNode* weapon) {
		BSFlattenedBoneTree* rt = (BSFlattenedBoneTree*)_skelly->getRoot();
		auto offHandBone = g_config->leftHandedMode ? "RArm_Finger31" : "LArm_Finger31";
		return rt->transforms[_skelly->getBoneInMap(offHandBone)].world.pos - weapon->m_worldTransform.pos;
	}

	void WeaponPositionAdjuster::debugPrintWeaponPositionData() {
		if (!g_config->checkDebugDumpDataOnceFor("weapon_pos"))
			return;

		printTransform("Weapon Original: ", _weaponOriginalTransform);
		printTransform("Weapon Offset  : ", _weaponOffsetTransform);
		printTransform("Scope Offset   : ", _skelly->getPlayerNodes()->primaryWeaponScopeCamera->m_localTransform);
		auto weapon = _skelly->getWeaponNode();
		printNodes(weapon);
		printNodesTransform(weapon);
	}
}