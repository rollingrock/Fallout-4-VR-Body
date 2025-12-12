#include "Flashlight.h"

#include "Config.h"
#include "FRIK.h"
#include "utils.h"
#include "vrcf/VRControllersManager.h"

using namespace common;

namespace frik
{
    Flashlight::Flashlight(Skeleton* skelly) :
        _skelly(skelly) {}

    /**
     * Executed every frame to update to handle flashlight location and moving between hand and head.
     */
    void Flashlight::onFrameUpdate()
    {
        if (g_config.removeFlashlight || !f4vr::isPipboyLightOn(f4vr::getPlayer())) {
            return;
        }

        checkSwitchingFlashlightHeadHand();

        adjustFlashlightTransformToHandOrHead();
    }

    /**
     * Switch between Pipboy flashlight on head or right/left hand based if player switches using button press of the hand near head.
     */
    void Flashlight::checkSwitchingFlashlightHeadHand()
    {
        // check a bit higher than the HMD to allow hand close to the lower part of the face
        const auto hmdPos = f4vr::getPlayerNodes()->HmdNode->world.translate + RE::NiPoint3(0, 0, 4);
        const auto isLeftHandCloseToHMD = MatrixUtils::vec3Len(_skelly->getLeftArm().hand->world.translate - hmdPos) < 12;
        const auto isRightHandCloseToHMD = MatrixUtils::vec3Len(_skelly->getRightArm().hand->world.translate - hmdPos) < 12;

        if (isLeftHandCloseToHMD && (g_config.flashlightLocation == FlashlightLocation::Head || g_config.flashlightLocation == FlashlightLocation::LeftArm)) {
            if (!_flashlightHapticActivated) {
                _flashlightHapticActivated = true;
                triggerStrongHaptic(vrcf::Hand::Left);
            }
        } else if (isRightHandCloseToHMD && (g_config.flashlightLocation == FlashlightLocation::Head || g_config.flashlightLocation == FlashlightLocation::RightArm)) {
            if (!_flashlightHapticActivated) {
                _flashlightHapticActivated = true;
                triggerStrongHaptic(vrcf::Hand::Right);
            }
        } else {
            _flashlightHapticActivated = false;
            return;
        }

        const bool isLeftHandGrab = isLeftHandCloseToHMD && vrcf::VRControllers.isReleasedShort(vrcf::Hand::Left, g_config.switchTorchButton);
        const bool isRightHandGrab = isRightHandCloseToHMD && vrcf::VRControllers.isReleasedShort(vrcf::Hand::Right, g_config.switchTorchButton);
        if (!isLeftHandGrab && !isRightHandGrab) {
            return;
        }

        if (g_config.flashlightLocation == FlashlightLocation::Head) {
            g_config.setFlashlightLocation(isLeftHandGrab ? FlashlightLocation::LeftArm : FlashlightLocation::RightArm);
        } else if ((g_config.flashlightLocation == FlashlightLocation::LeftArm && isLeftHandGrab) ||
            (g_config.flashlightLocation == FlashlightLocation::RightArm && isRightHandGrab)) {
            g_config.setFlashlightLocation(FlashlightLocation::Head);
        }
    }

    /**
     * Adjust the position of the light node to the hand that is holding it or revert to head position.
     * It is safer than moving the node as that can result in game crash.
     */
    void Flashlight::adjustFlashlightTransformToHandOrHead() const
    {
        const auto lightNode = f4vr::getFirstChild(f4vr::getPlayerNodes()->HeadLightParentNode);
        if (!lightNode) {
            return;
        }

        // revert to original transform
        lightNode->local.rotate = MatrixUtils::getIdentityMatrix();
        lightNode->local.translate = RE::NiPoint3(0, 0, 0);

        if (g_config.flashlightLocation != FlashlightLocation::Head) {
            // update world transforms after reverting to original
            f4vr::updateTransforms(lightNode);

            // use the right arm node
            const auto armNode = g_config.flashlightLocation == FlashlightLocation::LeftArm
                ? f4vr::findNode(_skelly->getLeftArm().shoulder, "LArm_Hand")
                : f4vr::findNode(_skelly->getRightArm().shoulder, "RArm_Hand");

            // calculate relocation transform and set to local
            lightNode->local = MatrixUtils::calculateRelocation(lightNode, armNode);

            // small adjustment to prevent light on the fingers and shadows from them
            const float offsetX = f4vr::isInPowerArmor() ? 16.0f : 12.0f;
            const float offsetY = g_config.flashlightLocation == FlashlightLocation::LeftArm ? -5.0f : 5.0f;
            lightNode->local.translate += RE::NiPoint3(offsetX, offsetY, 5);
        }
    }
}
