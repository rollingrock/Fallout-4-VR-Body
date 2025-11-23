#include "WeaponPositionAdjuster.h"

#include "Config.h"
#include "FRIK.h"
#include "utils.h"
#include "common/Quaternion.h"
#include "f4vr/DebugDump.h"
#include "f4vr/F4VRUtils.h"
#include "skeleton/HandPose.h"
#include "skeleton/Skeleton.h"
#include "vrcf/VRControllersManager.h"

using namespace common;

namespace
{
    std::string_view getGripStockName(RE::NiNode* weapon)
    {
        if (const auto gripNode = f4vr::getFirstChild(f4vr::findNode(weapon, "P-Grip"))) {
            return gripNode->name;
        }
        return "";
    }

    /**
     * Get the game name of the equipped weapon extending weapon that can be both pistol and rifle to include rifle suffix.
     * It's not critical but a nice to have a better hand grip for the weapons.
     */
    std::string getEquippedWeaponNameExtended(RE::NiNode* weapon)
    {
        const auto& weaponName = f4vr::getEquippedWeaponName();
        if (weaponName == "Plasma") {
            const auto stockName = getGripStockName(weapon);
            if (stockName.starts_with("RiotGrip") || stockName.starts_with("Sniper") || stockName.find("Rifle") != std::string_view::npos) {
                return weaponName + " Rifle";
            }
        } else if (weaponName == "Pipe" || weaponName == "Pipe Bolt-Action") {
            const auto stockName = getGripStockName(weapon);
            if (stockName.starts_with("HandmadePaddedStock") || stockName.starts_with("SpringStock") || stockName.starts_with("PipeStock")) {
                return weaponName + " Rifle";
            }
        } else if (weaponName == "Laser" || weaponName == "Institute") {
            const auto stockName = getGripStockName(weapon);
            if (stockName.find("Rifle") != std::string_view::npos) {
                return weaponName + " Rifle";
            }
        }

        return weaponName;
    }
}

namespace frik
{
    /**
     * Enable/Disable reposition configuration mode.
     */
    void WeaponPositionAdjuster::toggleWeaponRepositionMode()
    {
        logger::info("Toggle Weapon Reposition Config Mode: {}", !inWeaponRepositionMode() ? "ON" : "OFF");
        _configMode = _configMode ? nullptr : std::make_unique<WeaponPositionConfigMode>(this);
        if (!inWeaponRepositionMode()) {
            // reload offset to handle player didn't save changes
            loadStoredOffsets(_currentWeapon);
        }
    }

    /**
     * Run every game frame to do adjustments on all the weapon related nodes\elements.
     */
    void WeaponPositionAdjuster::onFrameUpdate()
    {
        // handle throwable first as it's independent of weapon
        handleThrowableWeapon();

        handlePrimaryWeapon();
    }

    /**
     * Handle adjustment of the throwable weapon position.
     * The throwable weapon exists only when the player is actively throwing it, NOT if it is equipped.
     * So all the handling needs to happen here with finding the offsets, defaults, and names.
     */
    void WeaponPositionAdjuster::handleThrowableWeapon()
    {
        const auto throwableWeapon = f4vr::getThrowableWeaponNode();
        if (!throwableWeapon) {
            // no throwable, clear name and noop
            _currentThrowableWeaponName = "";
            return;
        }

        // check throwable weapon is equipped for the first time
        if (_currentThrowableWeaponName.empty()) {
            _currentThrowableWeaponName = throwableWeapon->name.c_str();
            _throwableWeaponOriginalTransform = throwableWeapon->local;

            // get saved offset or use hard-coded global default
            const auto offsetLookup = g_config.getWeaponOffsets(_currentThrowableWeaponName, WeaponOffsetsMode::Throwable, _currentlyInPA);
            _throwableWeaponOffsetTransform = offsetLookup.has_value()
                ? offsetLookup.value()
                : WeaponPositionConfigMode::getThrowableWeaponDefaultAdjustment(_throwableWeaponOriginalTransform, _currentlyInPA);

            logger::info("Equipped Throwable Weapon changed to '{}' (InPA:{}); HasWeaponOffset:{}", _currentThrowableWeaponName.c_str(), _currentlyInPA, offsetLookup.has_value());
        }

        // use custom offset
        throwableWeapon->local = _throwableWeaponOffsetTransform;
    }

    /**
     * Reposition (position and rotation) the weapon, the scope camera (optical scope), and back-of-hand UI
     * with regard to weapon custom offsets and/or weapon gripping by offhand (non-primary hand).
     */
    void WeaponPositionAdjuster::handlePrimaryWeapon()
    {
        const auto weapon = f4vr::getWeaponNode();
        const auto backOfHand = getBackOfHandUINode();
        if (!f4vr::isNodeVisible(weapon)) {
            if (_configMode) {
                _configMode->onFrameUpdate(nullptr);
            }
            checkEquippedWeaponChanged(weapon, true);
            backOfHand->local = _backOfHandUIOffsetTransform;
            return;
        }

        // store original weapon transform in case we need it later
        _weaponOriginalTransform = weapon->local;
        _weaponOriginalWorldTransform = weapon->world;

        checkEquippedWeaponChanged(weapon, false);

        // override the back-of-hand UI transform
        backOfHand->local = _backOfHandUIOffsetTransform;

        // override the weapon transform to the saved offset
        weapon->local = _weaponOffsetTransform;
        // update world transform for later calculations
        f4vr::updateTransforms(weapon);

        if (_configMode) {
            _configMode->onFrameUpdate(weapon);
        }

        checkIfOffhandIsGripping(weapon);

        handleWeaponGrippingRotationAdjustment(weapon);

        handlePrimaryHandGripOffsetAdjustment(weapon);

        handleScopeCameraAdjustmentByWeaponOffset(weapon);

        handleSpecialWeaponFix(weapon);

        handleBetterScopes(weapon);

        f4vr::updateDown(weapon, true);

        fixMuzzleFlashPosition();

        debugPrintWeaponPositionData(weapon);
    }

    /**
     * If equipped weapon changed set offsets to stored if exists.
     */
    void WeaponPositionAdjuster::checkEquippedWeaponChanged(RE::NiNode* weapon, const bool emptyHand)
    {
        const auto& weaponName = emptyHand ? EMPTY_HAND : getEquippedWeaponNameExtended(weapon);
        const bool inPA = f4vr::isInPowerArmor();
        if (weaponName == _currentWeapon && inPA == _currentlyInPA) {
            // no weapon change
            return;
        }

        _currentWeapon = weaponName;
        _currentlyInPA = inPA;
        _isCurrentWeaponMelee = f4vr::isMeleeWeaponEquipped();

        // reset state
        _offHandGripping = false;

        loadStoredOffsets(weaponName);
    }

    /**
     * Load the stored weapon position adjustment offset for weapon and offhand.
     */
    void WeaponPositionAdjuster::loadStoredOffsets(const std::string& weaponName)
    {
        // Load stored offsets for the new weapon
        const auto weaponOffsetLookup = g_config.getWeaponOffsets(weaponName, WeaponOffsetsMode::Weapon, _currentlyInPA);
        if (weaponOffsetLookup.has_value()) {
            _weaponOffsetTransform = weaponOffsetLookup.value();
        } else {
            // No stored offset, use original weapon transform
            _weaponOffsetTransform = _isCurrentWeaponMelee ? WeaponPositionConfigMode::getMeleeWeaponDefaultAdjustment(_weaponOriginalTransform) : _weaponOriginalTransform;
        }

        // Load stored offsets for primary hand for the new weapon
        const auto primaryHandOffsetLookup = g_config.getWeaponOffsets(weaponName, WeaponOffsetsMode::PrimaryHand, _currentlyInPA);
        _hasPrimaryHandOffset = primaryHandOffsetLookup.has_value();
        if (primaryHandOffsetLookup.has_value()) {
            _primaryHandOffsetRot = primaryHandOffsetLookup.value().rotate;
        } else {
            // No stored offset, use identity for no change
            _primaryHandOffsetRot = MatrixUtils::getIdentityMatrix();
        }

        // Load stored offsets for offhand for the new weapon
        const auto offhandOffsetLookup = g_config.getWeaponOffsets(weaponName, WeaponOffsetsMode::OffHand, _currentlyInPA);
        if (offhandOffsetLookup.has_value()) {
            _offhandOffsetRot = offhandOffsetLookup.value().rotate;
        } else {
            // No stored offset, use identity for no change
            _offhandOffsetRot = MatrixUtils::getIdentityMatrix();
        }

        // Load stored offsets for back of hand UI for the new weapon
        auto backOfHandOffsetLookup = g_config.getWeaponOffsets(_currentWeapon, WeaponOffsetsMode::BackOfHandUI, _currentlyInPA);
        if (!backOfHandOffsetLookup.has_value()) {
            // Use empty hand offset as a global default that the player can adjust so they won't have to adjust it for every weapon.
            backOfHandOffsetLookup = g_config.getWeaponOffsets(EMPTY_HAND, WeaponOffsetsMode::BackOfHandUI, _currentlyInPA);
        }
        if (backOfHandOffsetLookup.has_value()) {
            _backOfHandUIOffsetTransform = backOfHandOffsetLookup.value();
            logger::debug("Use back of hand offset Pos: ({:2.2f}, {:2.2f}, {:2.2f}), Scale: {:.3f}, InPA: {}",
                _weaponOffsetTransform.translate.x, _weaponOffsetTransform.translate.y, _weaponOffsetTransform.translate.z, _weaponOffsetTransform.scale, _currentlyInPA);
        } else {
            // No stored offset, use default adjustment
            _backOfHandUIOffsetTransform = WeaponPositionConfigMode::getBackOfHandUIDefaultAdjustment(getBackOfHandUINode()->local, _currentlyInPA);
        }

        logger::info("Equipped Weapon changed to '{}' (InPA:{}); HasWeaponOffset:{}, HasPrimaryHandOffset:{}, HasOffhandOffset:{}, HasBackOfHandOffset:{}",
            _currentWeapon, _currentlyInPA,
            weaponOffsetLookup.has_value(), primaryHandOffsetLookup.has_value(), offhandOffsetLookup.has_value(), backOfHandOffsetLookup.has_value());
    }

    /**
     * Update the vanilla scope camera node to match weapon reposition.
     * Offset is a simple adjustment of the diff between original and offset transform.
     * Rotation is calculated by using weapon forward vector and the diff between it and straight in scope camera orientation.
     */
    void WeaponPositionAdjuster::handleScopeCameraAdjustmentByWeaponOffset(const RE::NiNode* weapon) const
    {
        const auto scopeCamera = f4vr::getPlayerNodes()->primaryWeaponScopeCamera;

        // Apply the position offset is weird because of different coordinate system
        const auto weaponPosDiff = _weaponOffsetTransform.translate - _weaponOriginalTransform.translate;
        scopeCamera->local.translate = RE::NiPoint3(weaponPosDiff.y, weaponPosDiff.x, -weaponPosDiff.z);

        // SUPER HACK adjustment because position offset adjustment in one axis is casing small drift in other axes!
        // The numbers were created by changing single offset axis and manually adjusting the other two axis until could hit the target
        scopeCamera->local.translate.z += (weaponPosDiff.y > 0 ? 0.12f : 0.04f) * weaponPosDiff.y + (weaponPosDiff.x > 0 ? 0.14f : 0.04f) * weaponPosDiff.x;
        scopeCamera->local.translate.x += (weaponPosDiff.x > 0 ? -0.1f : -0.11f) * weaponPosDiff.x + (weaponPosDiff.z > 0 ? 0.05f : 0.06f) * weaponPosDiff.z;

        if (_offHandGripping) {
            // if offhand is gripping it overrides the rotation adjustment
            return;
        }

        // need to update default transform for later world rotation use
        scopeCamera->local.rotate = _scopeCameraBaseMatrix;
        f4vr::updateTransforms(scopeCamera);

        // get the "forward" vector of the weapon (direction of the bullets)
        const auto weaponForwardVec = RE::NiPoint3(weapon->world.rotate.entry[1][0], weapon->world.rotate.entry[1][1], weapon->world.rotate.entry[1][2]);

        // Calculate the rotation adjustment using quaternion by diff between scope camera vector and straight
        Quaternion rotAdjust;
        const auto weaponForwardVecInScopeTransform = scopeCamera->world.rotate * (weaponForwardVec / scopeCamera->world.scale);
        rotAdjust.vec2Vec(weaponForwardVecInScopeTransform, RE::NiPoint3(1, 0, 0));
        scopeCamera->local.rotate = rotAdjust.getMatrix() * _scopeCameraBaseMatrix;
    }

    /**
     * Figure out if player should be gripping the weapon with offhand, handling all the different offhand grip modes:   | enableGripButtonToGrap | onePressGripButton | enableGripButtonToLetGo |
     * Mode 1: hand automatically snap to the barrel when in range, move hand quickly to let go                          | false                  | false              | false                   |
     * Mode 2: hand automatically snap to the barrel when in range, press grip button to let go                          | false                  | false              | true                    |
     * Mode 3: holding grip button to snap to the barrel, release grib button to let go                                  | true                   | true               | false                   |
     * Mode 4: press grip button to snap to the barrel, press grib button again to let go                                | true                   | false              | true                    |
     */
    void WeaponPositionAdjuster::checkIfOffhandIsGripping(const RE::NiNode* weapon)
    {
        if (!g_config.enableOffHandGripping) {
            return;
        }

        if (_offHandGripping) {
            if (g_config.onePressGripButton && !vrcf::VRControllers.isPressHeldDown(vrcf::Hand::Offhand, g_config.gripButtonID)) {
                // Mode 3 release grip when not holding the grip button
                setOffhandGripping(false);
            }

            if (g_config.enableGripButtonToLetGo && vrcf::VRControllers.isPressed(vrcf::Hand::Offhand, g_config.gripButtonID)) {
                if (g_config.enableGripButtonToGrap || !isOffhandCloseToBarrel(weapon)) {
                    // Mode 2,4 release grip on pressing the grip button again
                    setOffhandGripping(false);
                } else {
                    // Mode 2 but close to barrel, so ignore un-grip as it will grip on next frame
                    vrcf::VRControllers.triggerHaptic(vrcf::Hand::Offhand);
                }
            }

            if (!g_config.enableGripButtonToGrap && !g_config.enableGripButtonToLetGo && !g_frik.isLookingThroughScope() && isOffhandMovedFastAway()) {
                // mode 1 release when move fast away from barrel
                setOffhandGripping(false);
            }

            // already gripping, no need for checks to grip
            return;
        }

        if (!g_config.enableGripButtonToGrap && !g_config.enableGripButtonToLetGo) {
            // mode 1 extra calculation for past 3 frames, annoying but only if mode 1 is in use
            // ReSharper disable once CppExpressionWithoutSideEffects
            isOffhandMovedFastAway();
        }

        if (!isOffhandCloseToBarrel(weapon)) {
            // not close to barrel, no need to grip
            return;
        }

        if (!g_config.enableGripButtonToGrap) {
            // Mode 1,2 grab when close to barrel
            setOffhandGripping(true);
        }
        if (!g_frik.isPipboyOn() && vrcf::VRControllers.isPressed(vrcf::Hand::Offhand, g_config.gripButtonID)) {
            // Mode 3,4 grab when pressing grip button
            setOffhandGripping(true);
        }
    }

    /**
     * Set gripping flag and offhand hand pose for gripping.
     */
    void WeaponPositionAdjuster::setOffhandGripping(const bool isGripping)
    {
        _offHandGripping = isGripping;
        setOffhandGripHandPose(isGripping);
    }

    /**
     * Rotate the primary hand if there is a primary hand offset defined for the weapon.
     * Useful for rifle type weapons where the stock grip angle may differ than regular pistol.
     * Better result to move the hand and not the weapon as moving the weapon causes issues with holding comfort
     * and scope rotation when entering scoped mode.
     */
    void WeaponPositionAdjuster::handlePrimaryHandGripOffsetAdjustment(const RE::NiNode* weapon) const
    {
        if (_hasPrimaryHandOffset) {
            const auto primaryHand = f4vr::isLeftHandedMode() ? _skelly->getLeftArm().hand : _skelly->getRightArm().hand;
            primaryHand->local.rotate = _primaryHandOffsetRot * primaryHand->local.rotate;
            f4vr::updateDown(primaryHand, true, weapon->name.c_str());
        }
    }

    /**
     * Handle adjusting weapon and scope camera rotation when offhand is gripping the weapon.
     * Calculate the weapon to offhand vector and then adjust the weapon and scope camera rotation to match the vector.
     * This is fucking complicated code so a few pointers:
     * - Use "_weaponOriginalWorldTransform" and not "weapon->world" for vector because we repositioned the weapon
     * to grip by the stock but the original transform is where the actual hand is, and the hand is what we care about.
     * - There is a lot of moving from world space to local space, you need to pay attention to handle it for any changes.
     * - Offset pivot handling is because if we just rotate the weapon it will rotate around the original point before the
     * adjustment made to match stock to hand resulting in the stock flying away from the hand. we need to recalculate the new
     * position of the stock and then move the weapon to that position.
     */
    void WeaponPositionAdjuster::handleWeaponGrippingRotationAdjustment(RE::NiNode* weapon) const
    {
        if (!_offHandGripping && !(_configMode && _configMode->isInOffhandRepositioning())) {
            return;
        }

        Quaternion rotAdjust;

        // World-space vector from weapon to offhand
        const auto weaponToOffhandVecWorld = getOffhandPosition() - getPrimaryHandPosition();

        // Convert world-space vector into weapon space
        const auto weaponLocalVec = weapon->world.rotate * (MatrixUtils::vec3Norm(weaponToOffhandVecWorld) / weapon->world.scale);

        // Desired weapon forward direction after applying offhand offset
        const auto adjustedWeaponVec = _offhandOffsetRot.Transpose() * (weaponLocalVec);

        // Compute rotation from canonical forward (Y) to adjusted direction
        rotAdjust.vec2Vec(adjustedWeaponVec, RE::NiPoint3(0, 1, 0));

        // Compose into final local transform
        weapon->local.rotate = rotAdjust.getMatrix() * _weaponOffsetTransform.rotate;

        // -- Handle Scope:
        if (g_frik.isInScopeMenu()) {
            handleWeaponScopeCameraGrippingRotationAdjustment(weapon, rotAdjust, adjustedWeaponVec);

            // no need to move weapon/hands if we don't see them, and it's hard to calculate scope after more weapon adjustments
            return;
        }

        // -- Handle offset pivot:

        // Pivot in local space (point where weapon touches primary hand)
        const auto gripPivotLocal = weapon->world.rotate * ((getPrimaryHandPosition() - weapon->world.translate) / weapon->world.scale);

        // Recompute position so that the grip stays in place
        const auto rotatedGripPos = weapon->local.rotate.Transpose() * (gripPivotLocal);
        const auto originalGripPos = _weaponOffsetTransform.rotate.Transpose() * (gripPivotLocal);
        weapon->local.translate = _weaponOffsetTransform.translate + (originalGripPos - rotatedGripPos);

        f4vr::updateTransforms(weapon);

        // -- Handle primary hand rotation:

        // Transform the offhand offset adjusted vector from weapon space to world space so we can adjust it into hand and scope space
        const auto adjustedWeaponVecWorld = _weaponOriginalWorldTransform.rotate.Transpose() * ((adjustedWeaponVec * _weaponOriginalWorldTransform.scale));

        // Rotate the primary hand so it will stay on the weapon stock
        const auto primaryHand = (f4vr::isLeftHandedMode() ? _skelly->getLeftArm().hand : _skelly->getRightArm().hand);
        const auto handLocalVec = primaryHand->world.rotate * (adjustedWeaponVecWorld / primaryHand->world.scale);
        rotAdjust.vec2Vec(handLocalVec, RE::NiPoint3(1, 0, 0));

        // no fucking idea why it's off by specific angle
        const auto rotAdjustWithManual = _twoHandedPrimaryHandManualAdjustment * rotAdjust.getMatrix();

        primaryHand->local.rotate = rotAdjustWithManual * primaryHand->local.rotate;

        // update all the fingers to match the hand rotation
        f4vr::updateDown(primaryHand, true, weapon->name.c_str());
    }

    /**
     * To handle scope we move the calculated two-handed vector from weapon space to world space and then into scope space to
     * adjust the scope. Yeah, it's a bit weird, but it works.
     */
    void WeaponPositionAdjuster::handleWeaponScopeCameraGrippingRotationAdjustment(const RE::NiNode* weapon, Quaternion rotAdjust, const RE::NiPoint3 adjustedWeaponVec) const
    {
        const auto scopeCamera = f4vr::getPlayerNodes()->primaryWeaponScopeCamera;

        // Adjust the position after all the calculation above using consistent coordinate system
        // Use the same coordinate transformation as the main scope function: (y, x, -z)
        const auto weaponPosDiff = weapon->local.translate - _weaponOriginalTransform.translate;
        scopeCamera->local.translate = RE::NiPoint3(weaponPosDiff.y, weaponPosDiff.x, -weaponPosDiff.z);

        // Set base camera matrix first (remaps axes from weapon to scope system)
        scopeCamera->local.rotate = _scopeCameraBaseMatrix;
        f4vr::updateTransforms(scopeCamera);

        // Transform the offhand offset adjusted vector from weapon space to world space so we can adjust it into scope space
        const auto adjustedWeaponVecWorld = weapon->world.rotate.Transpose() * ((adjustedWeaponVec * weapon->world.scale));

        // Convert it into scope space
        const auto scopeLocalVec = scopeCamera->world.rotate * (adjustedWeaponVecWorld / scopeCamera->world.scale);

        // Compute scope rotation: align local X with this direction
        rotAdjust.vec2Vec(scopeLocalVec, RE::NiPoint3(1, 0, 0));
        scopeCamera->local.rotate = rotAdjust.getMatrix() * _scopeCameraBaseMatrix;
    }

    /**
     * Return true if the angle of offhand to weapon to grip is close to barrel but far in distance from main hand
     * to prevent grabbing when two hands are just close.
     */
    bool WeaponPositionAdjuster::isOffhandCloseToBarrel(const RE::NiNode* weapon) const
    {
        const auto offhand2WeaponVec = getOffhandPosition() - getPrimaryHandPosition();
        const float distanceFromPrimaryHand = MatrixUtils::vec3Len(offhand2WeaponVec);
        const auto weaponLocalVec = weapon->world.rotate * (MatrixUtils::vec3Norm(offhand2WeaponVec) / weapon->world.scale);
        const auto adjustedWeaponVec = _offhandOffsetRot.Transpose() * (weaponLocalVec);
        const float angleDiffToWeaponVec = MatrixUtils::vec3Dot(MatrixUtils::vec3Norm(adjustedWeaponVec), RE::NiPoint3(0, 1, 0));
        return angleDiffToWeaponVec > 0.955 && distanceFromPrimaryHand > 15;
    }

    /**
     * Check if the offhand moved fast away in the last 3 frames.
     * offhandFingerBonePos, bodyPos, and avgHandV are static and therefor persist between frames.
     * It's a bit of weird calculation and TBH I don't know if players really use this mode...
     */
    bool WeaponPositionAdjuster::isOffhandMovedFastAway() const
    {
        static auto bodyPos = RE::NiPoint3(0, 0, 0);
        static auto offhandFingerBonePos = RE::NiPoint3(0, 0, 0);
        static float avgHandV[3] = { 0.0f, 0.0f, 0.0f };
        static int fc = 0;
        const auto offHandBone = f4vr::isLeftHandedMode() ? "RArm_Finger31" : "LArm_Finger31";

        const auto currentPos = f4vr::getCameraPosition();
        const float handFrameMovement = MatrixUtils::vec3Len(_skelly->getBoneWorldTransform(offHandBone).translate - offhandFingerBonePos);
        const float bodyFrameMovement = MatrixUtils::vec3Len(currentPos - bodyPos);
        avgHandV[fc] = abs(handFrameMovement - bodyFrameMovement);
        fc = (fc + 1) % 3;

        float sum = 0;
        for (const float i : avgHandV) {
            sum += i;
        }
        const float handV = sum / 3;

        bodyPos = currentPos;
        offhandFingerBonePos = _skelly->getBoneWorldTransform(offHandBone).translate;

        return handV > g_config.gripLetGoThreshold;
    }

    /**
     * Get the world coordinates of the primary hand holding the weapon.
     */
    RE::NiPoint3 WeaponPositionAdjuster::getPrimaryHandPosition() const
    {
        const auto primaryHandNode = f4vr::isLeftHandedMode() ? _skelly->getLeftArm().hand : _skelly->getRightArm().hand;
        return primaryHandNode->world.translate;
    }

    /**
     * Get the world coordinates of the offhand.
     */
    RE::NiPoint3 WeaponPositionAdjuster::getOffhandPosition() const
    {
        const auto offHandBone = f4vr::isLeftHandedMode() ? "RArm_Finger31" : "LArm_Finger31";
        return _skelly->getBoneWorldTransform(offHandBone).translate;
    }

    /**
     * Special handling for the "Laser Musket" weapon as it's "loaded" state beam doesn't follow the parent weapon rotation.
     * Fix by forcing the rotation on it from calculating the diff from original to the current after all other calculation including offhand gripping.
     * Also works on Automatic laser specific modification that has laser beam in it.
     * The BeamMesh can have different suffixes so using starts with to find it.
     */
    void WeaponPositionAdjuster::handleSpecialWeaponFix(RE::NiNode* weapon) const
    {
        if (_currentWeapon.starts_with("Laser")) {
            if (const auto beamNode = f4vr::findNodeStartsWith(f4vr::findNode(weapon, "P-Barrel"), "BeamMesh")) {
                beamNode->local.rotate = MatrixUtils::getIdentityMatrix() * (weapon->local.rotate * _weaponOriginalTransform.rotate);
            }
        }
    }

    /**
     * Handle toggling of scope zoom for BetterScopesVR mod.
     * Toggle only when player presses offhand X/A button and the hand is close to the weapon scope.
     */
    void WeaponPositionAdjuster::handleBetterScopes(RE::NiNode* weapon) const
    {
        if (!vrcf::VRControllers.isPressed(vrcf::Hand::Offhand, vr::EVRButtonId::k_EButton_A)) {
            // fast return not to make additional calculations, checking button is cheap
            return;
        }

        const RE::NiAVObject* scopeRet = f4vr::findAVObject(weapon, "ReticleNode");
        if (!scopeRet) {
            return;
        }
        const auto reticlePos = scopeRet->world.translate;
        const auto offhandPos = getOffhandPosition();
        const auto offset = MatrixUtils::vec3Len(reticlePos - offhandPos);

        // is hand near scope
        if (offset < g_config.scopeAdjustDistance) {
            // Zoom toggling
            logger::info("Zoom Toggle pressed; sending message to switch zoom state");
            g_frik.dispatchMessageToBetterScopesVR(16, nullptr, 0);
            vrcf::VRControllers.triggerHaptic(vrcf::Hand::Offhand);
        }
    }

    /**
     * Fixes the position of the muzzle flash to be at the projectile node.
     * Required for two-handed weapon, otherwise the flash is where the weapon was before two-handed moved it.
     */
    void WeaponPositionAdjuster::fixMuzzleFlashPosition()
    {
        const auto muzzle = getMuzzleFlashNodes();
        if (!muzzle) {
            return;
        }

        // shockingly the projectile world location is exactly what we need to set of the muzzle flash node
        muzzle->fireNode->local = muzzle->projectileNode->world;

        // small optimization to only run world update if the fireNode is visible
        // note, setting fireNode local transform must happen ALWAYS or some artifact is visible in old location for some weapons
        if (f4vr::isNodeVisible(muzzle->fireNode)) {
            // critical to move all muzzle flash effects to the projectile node
            f4vr::updateDown(muzzle->fireNode, true);
        }
    }

    /**
     * Get the game node for the back of hand UI.
     * Using Primary Wand Node instead of "BackOfHand" node because wand node local transform is changed by the game by equipped weapons.
     * Probably because the original UI is to the left of the weapon and need to be offset by how big the weapon is.
     * Because we put the UI on the back of the hand we don't need to offset by the weapon size. And with it, it looks like we don't need per
     * weapon adjustment.
     */
    RE::NiNode* WeaponPositionAdjuster::getBackOfHandUINode()
    {
        return f4vr::getPrimaryWandNode();
    }

    void WeaponPositionAdjuster::debugPrintWeaponPositionData(RE::NiNode* weapon) const
    {
        if (!g_config.checkDebugDumpDataOnceFor("weapon_pos")) {
            return;
        }

        logger::info("Weapon: {}, InPA: {}", _currentWeapon.c_str(), _currentlyInPA);
        f4vr::DebugDump::printTransform("Weapon Original: ", _weaponOriginalTransform);
        f4vr::DebugDump::printTransform("Weapon Offset  : ", _weaponOffsetTransform);
        f4vr::DebugDump::printTransform("Back of Hand UI: ", _backOfHandUIOffsetTransform);
        f4vr::DebugDump::printTransform("Scope Offset   : ", f4vr::getPlayerNodes()->primaryWeaponScopeCamera->local);
        f4vr::DebugDump::printNodes(weapon);
        f4vr::DebugDump::printNodesTransform(weapon);
    }
}
