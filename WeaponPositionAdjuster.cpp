#include "WeaponPositionAdjuster.h"
#include "Config.h"
#include "Skeleton.h"
#include "F4VRBody.h"
#include "Debug.h"

namespace F4VRBody {
	/**
	 * Enable/Disable reposition configuration mode.
	 */
	void WeaponPositionAdjuster::toggleWeaponRepositionMode() {
		_MESSAGE("Toggle Weapon Reposition Config Mode: %s", !inWeaponRepositionMode() ? "ON" : "OFF");
		_configMode = _configMode ? nullptr : std::make_unique<WeaponPositionConfigMode>(this);
		setControlsThumbstickEnableState(!inWeaponRepositionMode());
		if (!inWeaponRepositionMode()) {
			// reload offset to handle player didn't save changes
			loadStoredOffsets(_lastWeapon);
		}
	}

	/**
	 * Run every game frame to reposition (position and rotation) the weapon and the scope camera (optical scope)
	 * with regard to weapon custom offsets and/or weapon gripping by offhand (non-primary hand).
	 */
	void WeaponPositionAdjuster::onFrameUpdate() {
		const auto weapon = _skelly->getWeaponNode();
		if (!isNodeVisible(weapon) || !(*g_player)->actorState.IsWeaponDrawn() || g_configurationMode->isCalibrateModeActive()) {
			if (_configMode) {
				_configMode->onFrameUpdate(nullptr);
			}
			return;
		}

		if (isButtonReleasedOnController(true, vr::EVRButtonId::k_EButton_A)) {
			// TODO: remove after testing (quick enter/exit reposition mode)
			toggleWeaponRepositionMode();
			ShowNotification(std::format("Testing: Weapon Reposition Mode - {}", inWeaponRepositionMode() ? "Enabled" : "Disabled"));
		}

		// store original weapon transform in case we need it later
		_weaponOriginalTransform = weapon->m_localTransform;
		// override the weapon transform to the saved offset
		weapon->m_localTransform = _weaponOffsetTransform;
		// update world transform for later calculations
		updateTransforms(weapon);

		checkEquippedWeaponChanged();

		if (_configMode) {
			_configMode->onFrameUpdate(weapon);
		}

		checkIfOffhandIsGripping(weapon);

		handleWeaponGrippingRotationAdjustment(weapon);

		handleScopeCameraAdjustmentByWeaponOffset(weapon);

		handleBetterScopes(weapon);

		_skelly->updateDown(weapon, true);

		debugPrintWeaponPositionData(weapon);
	}

	/**
	 * If equipped weapon changed set offsets to stored if exists.
	 */
	void WeaponPositionAdjuster::checkEquippedWeaponChanged() {
		const auto& weaponName = getEquippedWeaponName();
		if (weaponName == _lastWeapon && _skelly->inPowerArmor() == _lastWeaponInPA) {
			// no weapon change
			return;
		}

		_lastWeapon = weaponName;
		_lastWeaponInPA = _skelly->inPowerArmor();

		// reset state
		_offHandGripping = false;

		loadStoredOffsets(weaponName);
	}

	void WeaponPositionAdjuster::loadStoredOffsets(const std::string& weaponName) {
		// Load stored offsets for the new weapon
		const auto weaponOffsetLookup = g_config->getWeaponOffsets(weaponName, _skelly->inPowerArmor() ? WeaponOffsetsMode::powerArmor : WeaponOffsetsMode::normal);
		if (weaponOffsetLookup.has_value()) {
			_weaponOffsetTransform = weaponOffsetLookup.value();
			_MESSAGE("Use weapon offset for '%s'; Pos: (%2.2f, %2.2f, %2.2f), Scale: %2.3f, InPA: %d",
				weaponName.c_str(), _weaponOffsetTransform.pos.x, _weaponOffsetTransform.pos.y, _weaponOffsetTransform.pos.z, _weaponOffsetTransform.scale,
				_skelly->inPowerArmor());
		} else {
			// No stored offset, use original weapon transform
			_weaponOffsetTransform = _weaponOriginalTransform;
		}

		// Load stored offsets for offhand for the new weapon 
		const auto offhandOffsetLookup = g_config->getWeaponOffsets(weaponName, _skelly->inPowerArmor() ? WeaponOffsetsMode::offHandwithPowerArmor : WeaponOffsetsMode::offHand);
		if (offhandOffsetLookup.has_value()) {
			_offhandOffsetPos = offhandOffsetLookup.value().pos;
			_MESSAGE("Use offHand offset for '%s'; Pos: (%2.2f, %2.2f, %2.2f), InPA: %d",
				weaponName.c_str(), _offhandOffsetPos.x, _offhandOffsetPos.y, _offhandOffsetPos.z, _skelly->inPowerArmor());
		} else {
			// No stored offset for offhand, use default values (small vertical offset)
			_offhandOffsetPos = WeaponPositionConfigMode::getDefaultOffhandTransform();
		}
	}

	/**
	 * Update the vanilla scope camera node to match weapon reposition.
	 * Offset is a simple adjustment of the diff between original and offset transform.
	 * Rotation is calculated by using weapon forward vector and the diff between it and straight in scope camera orientation.
	 */
	void WeaponPositionAdjuster::handleScopeCameraAdjustmentByWeaponOffset(const NiNode* weapon) const {
		const auto scopeCamera = _skelly->getPlayerNodes()->primaryWeaponScopeCamera;

		// Apply the position offset is weird because of different coordinate system
		const auto weaponPosDiff = _weaponOffsetTransform.pos - _weaponOriginalTransform.pos;
		scopeCamera->m_localTransform.pos = NiPoint3(weaponPosDiff.y, weaponPosDiff.x, -weaponPosDiff.z);

		// SUPER HACK adjustment because position offset adjustment in one axis is casing small drift in other axes!
		// The numbers were created by changing single offset axis and manually adjusting the other two axis until could hit the target
		scopeCamera->m_localTransform.pos.z += (weaponPosDiff.y > 0 ? 0.12f : 0.04f) * weaponPosDiff.y + (weaponPosDiff.x > 0 ? 0.14f : 0.04f) * weaponPosDiff.x;
		scopeCamera->m_localTransform.pos.x += (weaponPosDiff.x > 0 ? -0.1f : -0.11f) * weaponPosDiff.x + (weaponPosDiff.z > 0 ? 0.05f : 0.06f) * weaponPosDiff.z;

		if (_offHandGripping) {
			// if offhand is gripping it overrides the rotation adjustment
			return;
		}

		// need to update default transform for later world rotation use
		scopeCamera->m_localTransform.rot = _scopeCameraBaseMatrix;
		updateTransforms(scopeCamera);

		// get the "forward" vector of the weapon (direction of the bullets)
		const auto weaponForwardVec = NiPoint3(weapon->m_worldTransform.rot.data[1][0], weapon->m_worldTransform.rot.data[1][1], weapon->m_worldTransform.rot.data[1][2]);

		// Calculate the rotation adjustment using quaternion by diff between scope camera vector and straight
		Quaternion rotAdjust;
		const auto weaponForwardVecInScopeTransform = scopeCamera->m_worldTransform.rot.Transpose() * weaponForwardVec / scopeCamera->m_worldTransform.scale;
		rotAdjust.vec2vec(weaponForwardVecInScopeTransform, NiPoint3(1, 0, 0));
		scopeCamera->m_localTransform.rot = rotAdjust.getRot().multiply43Left(_scopeCameraBaseMatrix);
	}

	/**
	 * Figure out if player should be gripping the weapon with offhand, handling all the different offhand grip modes:   | enableGripButtonToGrap | onePressGripButton | enableGripButtonToLetGo |
	 * Mode 1: hand automatically snap to the barrel when in range, move hand quickly to let go                          | false                  | false              | false                   |
	 * Mode 2: hand automatically snap to the barrel when in range, press grip button to let go                          | false                  | false              | true                    |
	 * Mode 3: holding grip button to snap to the barrel, release grib button to let go                                  | true                   | true               | false                   |
	 * Mode 4: press grip button to snap to the barrel, press grib button again to let go                                | true                   | false              | true                    |
	 */
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
				} else {
					// Mode 2 but close to barrel, so ignore un-grip as it will grip on next frame
					_vrHook->StartHaptics(g_config->leftHandedMode ? 0 : 1, 0.05f, 0.3f);
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
			// ReSharper disable once CppExpressionWithoutSideEffects
			isOffhandMovedFastAway();
		}

		if (!isOffhandCloseToBarrel(weapon)) {
			// not close to barrel, no need to grip
			return;
		}

		if (!g_config->enableGripButtonToGrap) {
			// Mode 1,2 grab when close to barrel
			_offHandGripping = true;
		}
		if (!g_pipboy->status() && isButtonPressedOnController(false, g_config->gripButtonID)) {
			// Mode 3,4 grab when pressing grip button
			_offHandGripping = true;
		}
	}

	/**
	 * Handle adjusting weapon and scope camera rotation when offhand is gripping the weapon.
	 * Calculate the weapon to offhand vector and then adjust the weapon and scope camera rotation to match the vector.
	 */
	void WeaponPositionAdjuster::handleWeaponGrippingRotationAdjustment(NiNode* weapon) const {
		if (!_offHandGripping && !(_configMode && _configMode->isInOffhandRepositioning())) {
			return;
		}

		// calculate the vector from weapon to offhand in world coordinates
		// This vector will be used to adjust the rotation of the weapon and scope camera
		auto weaponToOffhandVec = getOffhandPosition(weapon) - weapon->m_worldTransform.pos;

		// adjust by the saved offhand offset
		weaponToOffhandVec += _offhandOffsetPos;

		// Handle weapon:
		// Calculate the rotation adjustment using quaternion by diff between weapon vector and straight
		Quaternion rotAdjust;
		const auto offhandNormVecInWeaponTransform = weapon->m_worldTransform.rot.Transpose() * vec3_norm(weaponToOffhandVec) / weapon->m_worldTransform.scale;
		rotAdjust.vec2vec(offhandNormVecInWeaponTransform, NiPoint3(0, 1, 0));
		weapon->m_localTransform.rot = rotAdjust.getRot().multiply43Left(_weaponOffsetTransform.rot);

		// Handle scope camera:
		// need to adjust the scope camera and update world transform so we can use it for scope vector
		const auto scopeCamera = _skelly->getPlayerNodes()->primaryWeaponScopeCamera;
		scopeCamera->m_localTransform.rot = _scopeCameraBaseMatrix;
		updateTransforms(scopeCamera);

		// Calculate the rotation adjustment using quaternion by diff between scope camera vector and straight
		const auto weaponToOffhandScopeTransform = scopeCamera->m_worldTransform.rot.Transpose() * vec3_norm(weaponToOffhandVec) / scopeCamera->m_worldTransform.scale;
		rotAdjust.vec2vec(weaponToOffhandScopeTransform, NiPoint3(1, 0, 0));
		scopeCamera->m_localTransform.rot = rotAdjust.getRot().multiply43Left(_scopeCameraBaseMatrix);
	}

	/**
	 * Return true if the angle of offhand to weapon to grip is close to barrel but far in distance from main hand 
	 * to prevent grabbing when two hands are just close.
	 */
	bool WeaponPositionAdjuster::isOffhandCloseToBarrel(NiNode* weapon) const {
		const auto offhand2WeaponVec = getOffhandPosition(weapon) - weapon->m_worldTransform.pos;
		const float distanceFromPrimaryHand = vec3_len(offhand2WeaponVec);
		const auto normalizedVec = weapon->m_worldTransform.rot.Transpose() * vec3_norm(offhand2WeaponVec) / weapon->m_worldTransform.scale;
		const float angleDiffToWeaponVec = vec3_dot(vec3_norm(normalizedVec), NiPoint3(0, 1, 0));
		return angleDiffToWeaponVec > 0.955 && distanceFromPrimaryHand > 15;
	}

	/**
	 * Check if the offhand moved fast away in the last 3 frames.
	 * offhandFingerBonePos, bodyPos, and avgHandV are static and therefor persist between frames.
	 * It's a bit of weird calculation and TBH I don't know if players really use this mode...
	 */
	bool WeaponPositionAdjuster::isOffhandMovedFastAway() const {
		static auto bodyPos = NiPoint3(0, 0, 0);
		static auto offhandFingerBonePos = NiPoint3(0, 0, 0);
		static float avgHandV[3] = {0.0f, 0.0f, 0.0f};
		static int fc = 0;
		const auto offHandBone = g_config->leftHandedMode ? "RArm_Finger31" : "LArm_Finger31";

		const auto rt = reinterpret_cast<BSFlattenedBoneTree*>(_skelly->getRoot());
		const float handFrameMovement = vec3_len(rt->transforms[_skelly->getBoneInMap(offHandBone)].world.pos - offhandFingerBonePos);

		const float bodyFrameMovement = vec3_len(_skelly->getCurrentBodyPos() - bodyPos);
		avgHandV[fc] = abs(handFrameMovement - bodyFrameMovement);
		fc = (fc + 1) % 3;

		float sum = 0;
		for (const float i : avgHandV) {
			sum += i;
		}
		const float handV = sum / 3;

		bodyPos = _skelly->getCurrentBodyPos();
		offhandFingerBonePos = rt->transforms[_skelly->getBoneInMap(offHandBone)].world.pos;

		return handV > g_config->gripLetGoThreshold;
	}

	/**
	 * Get the world coordinates of the offhand.
	 */
	NiPoint3 WeaponPositionAdjuster::getOffhandPosition(NiNode* weapon) const {
		const auto rt = reinterpret_cast<BSFlattenedBoneTree*>(_skelly->getRoot());
		const auto offHandBone = g_config->leftHandedMode ? "RArm_Finger31" : "LArm_Finger31";
		return rt->transforms[_skelly->getBoneInMap(offHandBone)].world.pos;
	}

	/**
	 * Handle toggling of scope zoom for BetterScopesVR mod.
	 * Toggle only when player presses offhand X/A button and the hand is close to the weapon scope.
	 */
	void WeaponPositionAdjuster::handleBetterScopes(NiNode* weapon) const {
		if (!isButtonPressedOnController(false, vr::EVRButtonId::k_EButton_A)) {
			// fast return not to make additional calculations, checking button is cheap
			return;
		}

		static BSFixedString reticleNodeName = "ReticleNode";
		NiAVObject* scopeRet = weapon->GetObjectByName(&reticleNodeName);
		if (!scopeRet) {
			return;
		}
		const auto reticlePos = scopeRet->GetAsNiNode()->m_worldTransform.pos;
		const auto offhandPos = getOffhandPosition(weapon);
		const auto offset = vec3_len(reticlePos - offhandPos);

		// is hand near scope
		if (offset < g_config->scopeAdjustDistance) {
			// Zoom toggling
			_MESSAGE("Zoom Toggle pressed; sending message to switch zoom state");
			g_messaging->Dispatch(g_pluginHandle, 16, nullptr, 0, "FO4VRBETTERSCOPES");
			_vrHook->StartHaptics(g_config->leftHandedMode ? 0 : 1, 0.1f, 0.3f);
		}
	}

	void WeaponPositionAdjuster::debugPrintWeaponPositionData(NiNode* weapon) {
		if (!g_config->checkDebugDumpDataOnceFor("weapon_pos"))
			return;

		_MESSAGE("Weapon: %s, InPA: %d", _lastWeapon.c_str(), _lastWeaponInPA);
		printTransform("Weapon Original: ", _weaponOriginalTransform);
		printTransform("Weapon Offset  : ", _weaponOffsetTransform);
		printTransform("Scope Offset   : ", _skelly->getPlayerNodes()->primaryWeaponScopeCamera->m_localTransform);
		printNodes(weapon);
		printNodesTransform(weapon);
	}
}
