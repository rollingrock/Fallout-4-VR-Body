#include "WeaponPositionAdjuster.h"
#include "Config.h"
#include "Skeleton.h"
#include "F4VRBody.h"
#include "Debug.h"

namespace F4VRBody {
	// use as weapon name when no weapon in equipped. specifically for default back of hand UI offset.
	constexpr auto EMPTY_HAND = "EmptyHand";

	/**
	 * Enable/Disable reposition configuration mode.
	 */
	void WeaponPositionAdjuster::toggleWeaponRepositionMode() {
		_MESSAGE("Toggle Weapon Reposition Config Mode: %s", !inWeaponRepositionMode() ? "ON" : "OFF");
		_configMode = _configMode ? nullptr : std::make_unique<WeaponPositionConfigMode>(this);
		setControlsThumbstickEnableState(!inWeaponRepositionMode());
		if (!inWeaponRepositionMode()) {
			// reload offset to handle player didn't save changes
			loadStoredOffsets(_currentWeapon);
		}
	}

	/**
	 * Run every game frame to do adjustments on all the weapon related nodes\elements.
	 */
	void WeaponPositionAdjuster::onFrameUpdate() {
		// handle throwable first as it's independent of weapon
		handleThrowableWeapon();

		handlePrimaryWeapon();
	}

	/**
	 * Handle adjustment of the throwable weapon position.
	 * The throwable weapon exists only when the player is actively throwing it, NOT if it is equipped.
	 * So all the handling needs to happen here with finding the offsets, defaults, and names.
	 */
	void WeaponPositionAdjuster::handleThrowableWeapon() {
		const auto throwableWeapon = _skelly->getThrowableWeaponNode();
		if (!throwableWeapon) {
			// no throwable, clear name and noop
			_currentThrowableWeaponName = "";
			return;
		}

		// check throwable weapon is equipped for the first time
		if (_currentThrowableWeaponName.empty()) {
			_currentThrowableWeaponName = throwableWeapon->m_name.c_str();
			_throwableWeaponOriginalTransform = throwableWeapon->m_localTransform;

			// get saved offset or use hard-coded global default
			const auto offsetLookup = g_config->getWeaponOffsets(_currentThrowableWeaponName, WeaponOffsetsMode::Throwable, _currentlyInPA);
			_throwableWeaponOffsetTransform = offsetLookup.has_value()
				? offsetLookup.value()
				: WeaponPositionConfigMode::getThrowableWeaponDefaultAdjustment(_throwableWeaponOriginalTransform, _currentlyInPA);

			_MESSAGE("Equipped Throwable Weapon changed to '%s' (InPA:%d); HasWeaponOffset:%d", _currentThrowableWeaponName.c_str(), _currentlyInPA, offsetLookup.has_value());
		}

		// use custom offset
		throwableWeapon->m_localTransform = _throwableWeaponOffsetTransform;
	}

	/**
	 * Reposition (position and rotation) the weapon, the scope camera (optical scope), and back-of-hand UI
	 * with regard to weapon custom offsets and/or weapon gripping by offhand (non-primary hand).
	 */
	void WeaponPositionAdjuster::handlePrimaryWeapon() {
		const auto weapon = _skelly->getWeaponNode();
		const auto backOfHand = getBackOfHandUINode();
		if (!isNodeVisible(weapon) || g_configurationMode->isCalibrateModeActive()) {
			if (_configMode) {
				_configMode->onFrameUpdate(nullptr);
			}
			checkEquippedWeaponChanged(true);
			backOfHand->m_localTransform = _backOfHandUIOffsetTransform;
			return;
		}

		// store original weapon transform in case we need it later
		_weaponOriginalTransform = weapon->m_localTransform;

		checkEquippedWeaponChanged(false);

		// override the back-of-hand UI transform
		backOfHand->m_localTransform = _backOfHandUIOffsetTransform;

		// override the weapon transform to the saved offset
		weapon->m_localTransform = _weaponOffsetTransform;
		// update world transform for later calculations
		updateTransforms(weapon);

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
	void WeaponPositionAdjuster::checkEquippedWeaponChanged(const bool emptyHand) {
		const auto& weaponName = emptyHand ? EMPTY_HAND : getEquippedWeaponName();
		if (weaponName == _currentWeapon && _skelly->inPowerArmor() == _currentlyInPA) {
			// no weapon change
			return;
		}

		_currentWeapon = weaponName;
		_currentlyInPA = _skelly->inPowerArmor();

		// reset state
		_offHandGripping = false;

		loadStoredOffsets(weaponName);
	}

	/**
	 * Load the stored weapon position adjustment offset for weapon and offhand.
	 */
	void WeaponPositionAdjuster::loadStoredOffsets(const std::string& weaponName) {
		// Load stored offsets for the new weapon
		const auto weaponOffsetLookup = g_config->getWeaponOffsets(weaponName, WeaponOffsetsMode::Weapon, _currentlyInPA);
		if (weaponOffsetLookup.has_value()) {
			_weaponOffsetTransform = weaponOffsetLookup.value();
		} else {
			// No stored offset, use original weapon transform
			_weaponOffsetTransform = isMeleeWeaponEquipped() ? WeaponPositionConfigMode::getMeleeWeaponDefaultAdjustment(_weaponOriginalTransform) : _weaponOriginalTransform;
		}

		// Load stored offsets for offhand for the new weapon 
		const auto offhandOffsetLookup = g_config->getWeaponOffsets(weaponName, WeaponOffsetsMode::OffHand, _currentlyInPA);
		if (offhandOffsetLookup.has_value()) {
			_offhandOffsetRot = offhandOffsetLookup.value().rot;
		} else {
			// No stored offset for offhand, use identity for no change
			_offhandOffsetRot = Matrix44::getIdentity43();
		}

		// Load stored offsets for back of hand UI for the new weapon 
		auto backOfHandOffsetLookup = g_config->getWeaponOffsets(_currentWeapon, WeaponOffsetsMode::BackOfHandUI, _currentlyInPA);
		if (!backOfHandOffsetLookup.has_value()) {
			// Use empty hand offset as a global default that the player can adjust so they won't have to adjust it for every weapon.
			backOfHandOffsetLookup = g_config->getWeaponOffsets(EMPTY_HAND, WeaponOffsetsMode::BackOfHandUI, _currentlyInPA);
		}
		if (backOfHandOffsetLookup.has_value()) {
			_backOfHandUIOffsetTransform = backOfHandOffsetLookup.value();
			_VMESSAGE("Use back of hand offset Pos: (%2.2f, %2.2f, %2.2f), Scale: %1.3f, InPA: %d",
				_currentWeapon.c_str(), _weaponOffsetTransform.pos.x, _weaponOffsetTransform.pos.y, _weaponOffsetTransform.pos.z, _weaponOffsetTransform.scale, _currentlyInPA);
		} else {
			// No stored offset, use default adjustment
			_backOfHandUIOffsetTransform = WeaponPositionConfigMode::getBackOfHandUIDefaultAdjustment(getBackOfHandUINode()->m_localTransform, _currentlyInPA);
		}

		_MESSAGE("Equipped Weapon changed to '%s' (InPA:%d); HasWeaponOffset:%d, HasOffhandOffset:%d, HasBackOfHandOffset:%d",
			_currentWeapon.c_str(), _currentlyInPA, weaponOffsetLookup.has_value(), offhandOffsetLookup.has_value(), backOfHandOffsetLookup.has_value());
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
	void WeaponPositionAdjuster::checkIfOffhandIsGripping(const NiNode* weapon) {
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

		Quaternion rotAdjust;

		// World-space vector from weapon to offhand
		const auto weaponToOffhandVecWorld = getOffhandPosition() - weapon->m_worldTransform.pos;

		// Convert world-space vector into weapon space
		const auto weaponLocalVec = weapon->m_worldTransform.rot.Transpose() * vec3_norm(weaponToOffhandVecWorld) / weapon->m_worldTransform.scale;

		// Desired weapon forward direction after applying offhand offset
		const auto adjustedWeaponVec = _offhandOffsetRot * weaponLocalVec;

		// Compute rotation from canonical forward (Y) to adjusted direction
		rotAdjust.vec2vec(adjustedWeaponVec, NiPoint3(0, 1, 0));

		// Compose into final local transform
		weapon->m_localTransform.rot = rotAdjust.getRot().multiply43Left(_weaponOffsetTransform.rot);

		// Handle Scope:
		// Set base camera matrix first (remaps axes from weapon to scope system)
		const auto scopeCamera = _skelly->getPlayerNodes()->primaryWeaponScopeCamera;
		scopeCamera->m_localTransform.rot = _scopeCameraBaseMatrix;
		updateTransforms(scopeCamera);

		// Transform the offhand offset adjusted vector from weapon space to world space so we can adjust it into scope space
		const auto adjustedVecWorld = weapon->m_worldTransform.rot * (adjustedWeaponVec * weapon->m_worldTransform.scale);

		// Convert it into scope space
		const auto scopeLocalVec = scopeCamera->m_worldTransform.rot.Transpose() * adjustedVecWorld / scopeCamera->m_worldTransform.scale;

		// Compute scope rotation: align local X with this direction
		rotAdjust.vec2vec(scopeLocalVec, NiPoint3(1, 0, 0));
		scopeCamera->m_localTransform.rot = rotAdjust.getRot().multiply43Left(_scopeCameraBaseMatrix);
	}


	/**
	 * Return true if the angle of offhand to weapon to grip is close to barrel but far in distance from main hand 
	 * to prevent grabbing when two hands are just close.
	 */
	bool WeaponPositionAdjuster::isOffhandCloseToBarrel(const NiNode* weapon) const {
		const auto offhand2WeaponVec = getOffhandPosition() - weapon->m_worldTransform.pos;
		const float distanceFromPrimaryHand = vec3_len(offhand2WeaponVec);
		const auto weaponLocalVec = weapon->m_worldTransform.rot.Transpose() * vec3_norm(offhand2WeaponVec) / weapon->m_worldTransform.scale;
		const auto adjustedWeaponVec = _offhandOffsetRot * weaponLocalVec;
		const float angleDiffToWeaponVec = vec3_dot(vec3_norm(adjustedWeaponVec), NiPoint3(0, 1, 0));
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
	NiPoint3 WeaponPositionAdjuster::getOffhandPosition() const {
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
		const auto offhandPos = getOffhandPosition();
		const auto offset = vec3_len(reticlePos - offhandPos);

		// is hand near scope
		if (offset < g_config->scopeAdjustDistance) {
			// Zoom toggling
			_MESSAGE("Zoom Toggle pressed; sending message to switch zoom state");
			g_messaging->Dispatch(g_pluginHandle, 16, nullptr, 0, "FO4VRBETTERSCOPES");
			_vrHook->StartHaptics(g_config->leftHandedMode ? 0 : 1, 0.1f, 0.3f);
		}
	}

	/**
	 * Get the game node for the back of hand UI.
	 * Using Primary Wand Node instead of "BackOfHand" node because wand node local transform is changed by the game by equipped weapons.
	 * Probably because the original UI is to the left of the weapon and need to be offset by how big the weapon is.
	 * Because we put the UI on the back of the hand we don't need to offset by the weapon size. And with it, it looks like we don't need per
	 * weapon adjustment.
	 */
	NiNode* WeaponPositionAdjuster::getBackOfHandUINode() const {
		return _skelly->getPrimaryWandNode();
	}

	void WeaponPositionAdjuster::debugPrintWeaponPositionData(NiNode* weapon) {
		if (!g_config->checkDebugDumpDataOnceFor("weapon_pos"))
			return;

		_MESSAGE("Weapon: %s, InPA: %d", _currentWeapon.c_str(), _currentlyInPA);
		printTransform("Weapon Original: ", _weaponOriginalTransform);
		printTransform("Weapon Offset  : ", _weaponOffsetTransform);
		printTransform("Back of Hand UI: ", _backOfHandUIOffsetTransform);
		printTransform("Scope Offset   : ", _skelly->getPlayerNodes()->primaryWeaponScopeCamera->m_localTransform);
		printNodes(weapon);
		printNodesTransform(weapon);
	}
}
