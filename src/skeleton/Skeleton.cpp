#include "Skeleton.h"

#include <array>

#include "Config.h"
#include "FRIK.h"
#include "HandPose.h"
#include "common/MatrixUtils.h"
#include "common/Quaternion.h"
#include "f4vr/BSFlattenedBoneTree.h"
#include "f4vr/F4VRUtils.h"
#include "vrcf/VRControllersManager.h"

using namespace common;
using namespace f4vr;
using namespace vrcf;

namespace
{
    /**
     * Hack to handle comfort sneak affecting the height of the player without real-world body change.
     * By setting static body pitch the body position doesn't change, making it easier to handle skeleton
     * related things like Virtual Holsters.
     */
    bool isComfortSneakHackEnabled()
    {
        return frik::g_config.comfortSneakHackStaticBodyPitchAngle > 0 && isComfortSneakMode() && isPlayerSneaking();
    }
}

namespace frik
{
    constexpr float COMFORT_SNEAK_CAMERA_OFFSET_ADJUSTMENT = 0.7f;
    constexpr float COMFORT_SNEAK_BODY_OFFSET_ADJUSTMENT = 0.5f;

    RE::NiTransform Skeleton::getBoneWorldTransform(const std::string& boneName)
    {
        return getFlattenedBoneTree()->transforms[_boneTreeMap[boneName]].world;
    }

    /**
     * Get the world position of the offhand index fingertip .
     * Make small adjustment as the finger bone position is the center of the finger.
     * Would be nice to know how long the bone is instead of magic numbers, didn't find a way so far.
     */
    RE::NiPoint3 Skeleton::getIndexFingerTipWorldPosition(const bool primaryHand)
    {
        const auto indexFinger = primaryHand == isLeftHandedMode() ? "LArm_Finger23" : "RArm_Finger23";
        const auto boneTransform = getBoneWorldTransform(indexFinger);
        const auto forward = boneTransform.rotate.Transpose() * (RE::NiPoint3(1, 0, 0));
        return boneTransform.translate + forward * (_inPowerArmor ? 3 : 1.8f);
    }

    /**
     * Get the player camera height offset adjusted for power armor, sneaking, and dynamic height from external API.
     * The height needs to be adjusted for comfort sneaking because the player physical height doesn't change but
     * the player avatar does. So the camera offset has to be reduced by the same amount as the game changes the height
     * which is 70% of the normal height.
     */
    float Skeleton::getAdjustedPlayerHMDOffset()
    {
        auto offset = g_config.getPlayerHMDOffsetUp() + g_frik.getDynamicCameraHeight();
        if (isComfortSneakMode() && isPlayerSneaking()) {
            offset *= COMFORT_SNEAK_CAMERA_OFFSET_ADJUSTMENT;
        }
        return offset;
    }

    /**
     * Initialize all the skeleton nodes for quick access during frame update.
     * Setup known defaults where relevant.
     */
    void Skeleton::initializeNodes()
    {
        QueryPerformanceFrequency(&_freqCounter);
        QueryPerformanceCounter(&_timer);

        _prevSpeed = 0.0;

        _playerNodes = getPlayerNodes();

        const auto fpSkeleton = getFirstPersonSkeleton();
        _rightHand = findNode(fpSkeleton, "RArm_Hand");
        _leftHand = findNode(fpSkeleton, "LArm_Hand");
        _rightHandPrevFrame = _rightHand->world;
        _leftHandPrevFrame = _leftHand->world;

        _head = findNode(_root, "Head");
        _spine = findNode(_root, "SPINE2");
        _chest = findNode(_root, "Chest");

        // Setup Arms
        initArmsNodes();

        initSkeletonNodesDefaults();

        _handBones = handOpen;

        initBoneTreeMap();

        setBodyLen();

        initHandPoses(_inPowerArmor);
    }

    void Skeleton::initArmsNodes()
    {
        const std::vector<std::pair<RE::BSFixedString, RE::NiAVObject**>> armNodes = {
            { "RArm_Collarbone", &_rightArm.shoulder },
            { "RArm_UpperArm", &_rightArm.upper },
            { "RArm_UpperTwist1", &_rightArm.upperT1 },
            { "RArm_ForeArm1", &_rightArm.forearm1 },
            { "RArm_ForeArm2", &_rightArm.forearm2 },
            { "RArm_ForeArm3", &_rightArm.forearm3 },
            { "RArm_Hand", &_rightArm.hand },
            { "LArm_Collarbone", &_leftArm.shoulder },
            { "LArm_UpperArm", &_leftArm.upper },
            { "LArm_UpperTwist1", &_leftArm.upperT1 },
            { "LArm_ForeArm1", &_leftArm.forearm1 },
            { "LArm_ForeArm2", &_leftArm.forearm2 },
            { "LArm_ForeArm3", &_leftArm.forearm3 },
            { "LArm_Hand", &_leftArm.hand }
        };
        const auto commonNode = getCommonNode();
        for (const auto& [name, node] : armNodes) {
            *node = findAVObject(commonNode, name.c_str());
        }
    }

    /**
     * Setup default skeleton nodes collection for quick reset on every frame
     * instead of looking up the skeleton nodes every time.
     */
    void Skeleton::initSkeletonNodesDefaults()
    {
        const auto defaultBonesMap = _inPowerArmor ? _skeletonNodesDefaultTransformInPA : _skeletonNodesDefaultTransform;
        for (const auto& [boneName, defaultTransform] : defaultBonesMap) {
            if (auto node = findAVObject(_root, boneName)) {
                auto transform = node->local; // use node transform to keep scale
                transform.translate = defaultTransform.translate;
                transform.rotate = defaultTransform.rotate;
                _skeletonNodesToDefaultTransforms.emplace_back(node, transform);
            } else {
                logger::warn("Skeleton bone node not found for '{}'", boneName.c_str());
            }
        }
    }

    void Skeleton::initBoneTreeMap()
    {
        _boneTreeMap.clear();
        _boneTreeVec.clear();

        const auto rt = reinterpret_cast<BSFlattenedBoneTree*>(_root);
        for (auto i = 0; i < rt->numTransforms; i++) {
            logger::debug("BoneTree Init -> Push {} into position {}", rt->transforms[i].name.c_str(), i);
            _boneTreeMap.insert({ rt->transforms[i].name.c_str(), i });
            _boneTreeVec.emplace_back(rt->transforms[i].name.c_str());
        }
    }

    void Skeleton::setBodyLen()
    {
        _torsoLen = MatrixUtils::vec3Len(findNode(_root, "Camera")->world.translate - findNode(_root, "COM")->world.translate);
        _torsoLen *= g_config.playerHeight / DEFAULT_CAMERA_HEIGHT;

        _legLen = MatrixUtils::vec3Len(findNode(_root, "LLeg_Thigh")->world.translate - findNode(_root, "Pelvis")->world.translate);
        _legLen += MatrixUtils::vec3Len(findNode(_root, "LLeg_Calf")->world.translate - findNode(_root, "LLeg_Thigh")->world.translate);
        _legLen += MatrixUtils::vec3Len(findNode(_root, "LLeg_Foot")->world.translate - findNode(_root, "LLeg_Calf")->world.translate);
        _legLen *= g_config.playerHeight / DEFAULT_CAMERA_HEIGHT;
    }

    /**
     * Runs on every game frame to calculate and update the skeleton transform.
     */
    void Skeleton::onFrameUpdate()
    {
        setTime();

        // save last position at this time for anyone doing speed calculations
        _lastPosition = _curentPosition;
        _curentPosition = getCameraPosition();

        logger::trace("Hide Wands...");
        setWandsVisibility(false, true);
        setWandsVisibility(false, false);

        logger::trace("Restore locals of skeleton");
        restoreNodesToDefault();
        updateDownFromRoot();

        const float neckYaw = getNeckYaw();
        const float neckPitch = getNeckPitch();

        if (!g_config.hideHead || (g_frik.getSelfieMode() && g_config.selfieIgnoreHideFlags)) {
            logger::trace("Setup Head");
            setupHead(neckYaw, neckPitch);
        }

        logger::trace("Set body under HMD");
        setBodyUnderHMD(neckYaw, neckPitch);
        updateDownFromRoot(); // Do world update now so that IK calculations have proper world reference

        // Now Set up body Posture and hook up the legs
        logger::trace("Set body posture...");
        setBodyPosture(neckPitch);
        updateDownFromRoot(); // Do world update now so that IK calculations have proper world reference

        logger::trace("Set knee posture...");
        setKneePos();

        logger::trace("Set walk...");
        walk();

        logger::trace("Set legs...");
        setSingleLeg(false);
        setSingleLeg(true);

        // Do another update before setting arms
        updateDownFromRoot(); // Do world update now so that IK calculations have proper world reference

        // do arm IK - Right then Left
        logger::trace("Set Arms...");
        handleLeftHandedWeaponNodesSwitch();
        setArms(false);
        setArms(true);
        updateDownFromRoot(); // Do world update now so that IK calculations have proper world reference

        // Misc stuff to show/hide things
        logger::trace("Pipboy and Weapons...");
        hide3rdPersonWeapon();
        hideFistHelpers();
        showHidePAHud();

        logger::trace("Cull geometry...");
        _cullGeometry.cullPlayerGeometry();

        // project body out in front of the camera for debug purposes
        logger::trace("Selfie Time");
        selfieSkelly();
        updateDownFromRoot();

        logger::trace("Operate hands...");
        setHandPose();

        if (g_frik.isInScopeMenu()) {
            hideHands();
        }

        if (_inPowerArmor) {
            fixArmor();
        }
    }

    void Skeleton::setTime()
    {
        _prevTime = _timer;
        QueryPerformanceFrequency(&_freqCounter);
        QueryPerformanceCounter(&_timer);
        _frameTime = static_cast<float>(_timer.QuadPart - _prevTime.QuadPart) / _freqCounter.QuadPart;
    }

    /**
     * Restore the skeleton main 25 nodes to their default transforms.
     * To wipe out any local transform changes the game might have made since last update
     */
    void Skeleton::restoreNodesToDefault()
    {
        for (const auto& [boneNode, resetTransform] : _skeletonNodesToDefaultTransforms) {
            boneNode->local = resetTransform;
        }
    }

    /**
     * Moves head up and back out of the player view and handle head movement.
     * It's still not good enough to prevent seeing hats and stuff, but it's a step forward in case
     * someone wants to tackle it further.
     */
    void Skeleton::setupHead(const float neckYaw, const float neckPitch) const
    {
        const float headBackAdj = g_frik.getSelfieMode() && g_config.selfieIgnoreHideFlags ? 0 : g_config.headBackPositionOffset + (neckPitch > 0 ? 2 * neckPitch : 0);
        _head->local.translate -= RE::NiPoint3(headBackAdj, 2 * headBackAdj, 0);
        _head->local.rotate = _head->local.rotate * MatrixUtils::getMatrixFromEulerAngles(neckYaw, 0, neckPitch);
        RE::NiUpdateData* ud = nullptr;
        _head->UpdateWorldData(ud);
    }

    // below takes the two vectors from hmd to each hand and sums them to determine a center axis in which to see how much the hmd has rotated
    // A secondary angle is also calculated which is 90 degrees on the z axis up to handle when the hands are approaching the z plane of the hmd
    // this helps keep the body stable through a wide range of hand poses
    // this still struggles with hands close to the face and with one hand low and one hand high.
    // Will need to take prog advice to add weights to these positions which I'll do at a later date.
    float Skeleton::getNeckYaw() const
    {
        if (!_playerNodes) {
            logger::info("player nodes not set in neck yaw");
            return 0.0;
        }

        const RE::NiPoint3 pos = _playerNodes->UprightHmdNode->world.translate;
        const RE::NiPoint3 hmdToLeft = _playerNodes->SecondaryWandNode->world.translate - pos;
        const RE::NiPoint3 hmdToRight = _playerNodes->primaryWandNode->world.translate - pos;
        float weight = 1.0f;

        if (MatrixUtils::vec3Len(hmdToLeft) < 10.0f || MatrixUtils::vec3Len(hmdToRight) < 10.0f) {
            return 0.0;
        }

        // handle excessive angles when hand is above the head.
        if (hmdToLeft.z > 0) {
            weight = (std::max)(weight - 0.05f * hmdToLeft.z, 0.0f);
        }

        if (hmdToRight.z > 0) {
            weight = (std::max)(weight - 0.05f * hmdToRight.z, 0.0f);
        }

        // hands moving across the chest rotate too much.   try to handle with below
        // wp = parWp + parWr * lp =>   lp = (wp - parWp) * parWr'
        const RE::NiPoint3 locLeft = _playerNodes->HmdNode->world.rotate * (hmdToLeft);
        const RE::NiPoint3 locRight = _playerNodes->HmdNode->world.rotate * (hmdToRight);

        if (locLeft.x > locRight.x) {
            const float delta = locRight.x - locLeft.x;
            weight = (std::max)(weight + 0.02f * delta, 0.0f);
        }

        const RE::NiPoint3 sum = hmdToRight + hmdToLeft;

        const RE::NiPoint3 forwardDir = MatrixUtils::vec3Norm(_playerNodes->HmdNode->world.rotate * (MatrixUtils::vec3Norm(sum))); // rotate sum to local hmd space to get the proper angle
        const RE::NiPoint3 hmdForwardDir = MatrixUtils::vec3Norm(_playerNodes->HmdNode->world.rotate * (_playerNodes->HmdNode->local.translate));

        const float anglePrime = atan2f(forwardDir.x, forwardDir.y);
        const float angleSec = atan2f(forwardDir.x, forwardDir.z);

        const float pitchDiff = atan2f(hmdForwardDir.y, hmdForwardDir.z) - atan2f(forwardDir.z, forwardDir.y);

        const float angleFinal = fabs(pitchDiff) > MatrixUtils::degreesToRads(80.0f) ? angleSec : anglePrime;
        return std::clamp(-angleFinal * weight, MatrixUtils::degreesToRads(-50.0f), MatrixUtils::degreesToRads(50.0f));
    }

    float Skeleton::getNeckPitch() const
    {
        const RE::NiPoint3& lookDir = MatrixUtils::vec3Norm(_playerNodes->HmdNode->world.rotate * (_playerNodes->HmdNode->local.translate));
        return atan2f(lookDir.y, lookDir.z);
    }

    float Skeleton::getBodyPitch(const float neckPitch) const
    {
        if (isComfortSneakHackEnabled()) {
            return MatrixUtils::degreesToRads(g_config.comfortSneakHackStaticBodyPitchAngle);
        }

        constexpr float basePitch = 105.3f;
        constexpr float weight = 0.1f;

        const float curHeight = g_config.playerHeight;
        const float heightCalc = std::abs((curHeight - (_playerNodes->UprightHmdNode->local.translate.z + getAdjustedPlayerHMDOffset())) / curHeight);
        const float angle = heightCalc * (basePitch + weight * MatrixUtils::radsToDegrees(neckPitch));
        return MatrixUtils::degreesToRads(angle);
    }

    /**
     * set up the body underneath the headset in a proper scale and orientation
     */
    void Skeleton::setBodyUnderHMD(const float neckYaw, const float neckPitch)
    {
        if (g_config.disableSmoothMovement) {
            _playerNodes->playerworldnode->local.translate.z = getAdjustedPlayerHMDOffset();
            updateDown(_playerNodes->playerworldnode, true);
        }

        //		float y    = (*g_playerCamera)->cameraNode->world.rotate.data[1][1];  // Middle column is y vector.   Grab just x and y portions and make a unit vector.    This can be used to rotate body to always be orientated with the hmd.
        //	float x    = (*g_playerCamera)->cameraNode->world.rotate.data[1][0];  //  Later will use this vector as the basis for the rest of the IK
        const float z = _root->local.translate.z;
        //	float z = groundHeight;

        Quaternion qa;
        qa.setAngleAxis(-neckPitch, RE::NiPoint3(-1, 0, 0));

        const RE::NiMatrix3 newRot = qa.getMatrix() * _playerNodes->HmdNode->local.rotate;

        _forwardDir = MatrixUtils::rotateXY(RE::NiPoint3(newRot.entry[1][0], newRot.entry[1][1], 0), neckYaw * 0.7f);
        _sidewaysRDir = RE::NiPoint3(_forwardDir.y, -_forwardDir.x, 0);

        RE::NiNode* body = _root->parent;
        body->local.translate *= 0.0f;
        body->world.translate.x = _curentPosition.x;
        body->world.translate.y = _curentPosition.y;
        body->world.translate.z += _playerNodes->playerworldnode->local.translate.z;

        const RE::NiPoint3 back = MatrixUtils::vec3Norm(RE::NiPoint3(_forwardDir.x, _forwardDir.y, 0));
        const auto bodyDir = RE::NiPoint3(0, 1, 0);

        _root->local.rotate = MatrixUtils::getMatrixFromRotateVectorVec(back, bodyDir) * body->world.rotate.Transpose();
        _root->local.translate = body->world.translate - _curentPosition;
        _root->local.translate.z = z;
        //_root->local.translate *= 0.0f;
        //_root->local.translate.y = g_config.playerBodyOffsetForwardStanding - 6.0f;
        _root->local.scale = g_config.playerHeight / DEFAULT_CAMERA_HEIGHT; // set scale based off specified user height
    }

    void Skeleton::setBodyPosture(const float neckPitch)
    {
        const float bodyPitch = _inPowerArmor ? getBodyPitch(neckPitch) : getBodyPitch(neckPitch) / 1.2f;

        RE::NiNode* com = findNode(_root, "COM");
        const RE::NiNode* neck = findNode(_root, "Neck");
        RE::NiNode* spine = findNode(_root, "SPINE1");

        _leftKneePos = findNode(com, "LLeg_Calf")->world.translate;
        _rightKneePos = findNode(com, "RLeg_Calf")->world.translate;

        com->local.translate.x = 0.0;
        com->local.translate.y = 0.0;

        // comfort sneak changes the height of the avatar without the player changing height in the real world, need to adjust for it
        const float comfortSneakAdjustZ = isComfortSneakMode() && isPlayerSneaking() ? COMFORT_SNEAK_BODY_OFFSET_ADJUSTMENT : 1.0f;

        // small offset to (1) not change player height when looking up/down and (2) move the body back, especially when looking down
        const float xOffsetByNeckPitch = fmaxf(0, (isComfortSneakHackEnabled() ? 2.0f : 5.0f) * fabs(neckPitch) * _root->local.scale);
        const float zOffsetByNeckPitch = 6.0f * neckPitch * _root->local.scale;

        const float playerAdjustZ = (4 * g_config.getPlayerBodyOffsetUp() - g_config.getPlayerHMDOffsetUp()) * comfortSneakAdjustZ + zOffsetByNeckPitch;
        // if people complain about body posture we can add manual adjustment here later

        const auto neckPos = getCameraPosition() + RE::NiPoint3(
            -_forwardDir.x * (g_config.getPlayerBodyOffsetForward() / 2 - xOffsetByNeckPitch),
            -_forwardDir.y * (g_config.getPlayerBodyOffsetForward() / 2 - xOffsetByNeckPitch),
            -playerAdjustZ);

        _torsoLen = MatrixUtils::vec3Len(neck->world.translate - com->world.translate);

        const RE::NiPoint3 hmdToHip = neckPos - com->world.translate;
        const auto dir = RE::NiPoint3(-_forwardDir.x, -_forwardDir.y, 0);

        const float dist = tanf(bodyPitch) * MatrixUtils::vec3Len(hmdToHip);
        RE::NiPoint3 tmpHipPos = com->world.translate + dir * (dist / MatrixUtils::vec3Len(dir));
        tmpHipPos.z = com->world.translate.z;

        const RE::NiPoint3 hmdToNewHip = tmpHipPos - neckPos;
        const RE::NiPoint3 newHipPos = neckPos + hmdToNewHip * (_torsoLen / MatrixUtils::vec3Len(hmdToNewHip));

        const RE::NiPoint3 newPos = com->local.translate + _root->world.rotate * (newHipPos - com->world.translate);
        com->local.translate.y += newPos.y + g_config.getPlayerBodyOffsetForward() - 2 * xOffsetByNeckPitch;
        com->local.translate.z = _inPowerArmor ? newPos.z / 1.7f : newPos.z / 1.5f;

        // ???
        _root->parent->world.translate.z -= g_config.getPlayerBodyOffsetUp() + getAdjustedPlayerHMDOffset();

        const RE::NiMatrix3 mat = MatrixUtils::getMatrixFromRotateVectorVec(neckPos - tmpHipPos, hmdToHip) * spine->parent->world.rotate.Transpose();
        spine->local.rotate = spine->world.rotate * mat;
    }

    void Skeleton::setKneePos()
    {
        const auto lKnee = findNode(_root, "LLeg_Calf");
        const auto rKnee = findNode(_root, "RLeg_Calf");

        if (!lKnee || !rKnee) {
            return;
        }

        lKnee->world.translate.z = _leftKneePos.z;
        rKnee->world.translate.z = _rightKneePos.z;

        _leftKneePos = lKnee->world.translate;
        _rightKneePos = rKnee->world.translate;

        updateDown(lKnee, false);
        updateDown(rKnee, false);
    }

    // TODO: does it do anything? check if it works at all
    void Skeleton::fixArmor() const
    {
        auto lPauldron = findNode(_root, "L_Pauldron");
        auto rPauldron = findNode(_root, "R_Pauldron");

        if (!lPauldron || !rPauldron) {
            return;
        }

        //float delta = findNode("LArm_Collarbone", _root)->world.translate.z - _root->world.translate.z;
        const float delta = findNode(_root, "LArm_UpperArm")->world.translate.z - _root->world.translate.z;
        if (lPauldron) {
            lPauldron->local.translate.z = delta - 15.0f;
        }
        if (rPauldron) {
            rPauldron->local.translate.z = delta - 15.0f;
        }
    }

    void Skeleton::walk()
    {
        const auto lHip = findNode(_root, "LLeg_Thigh");
        const auto rHip = findNode(_root, "RLeg_Thigh");

        if (!lHip || !rHip) {
            return;
        }

        const auto lKnee = findNode(lHip, "LLeg_Calf");
        const auto rKnee = findNode(rHip, "RLeg_Calf");
        const auto lFoot = findNode(lHip, "LLeg_Foot");
        const auto rFoot = findNode(rHip, "RLeg_Foot");

        if (!lKnee || !rKnee || !lFoot || !rFoot) {
            return;
        }

        // move feet closer together
        const RE::NiPoint3 leftToRight = _inPowerArmor
            ? (rFoot->world.translate - lFoot->world.translate) * -0.15f
            : (rFoot->world.translate - lFoot->world.translate) * 0.3f;
        lFoot->world.translate += leftToRight;
        rFoot->world.translate -= leftToRight;

        // want to calculate direction vector first.     Will only concern with x-y vector to start.
        RE::NiPoint3 lastPos = _lastPosition;
        RE::NiPoint3 curPos = _curentPosition;
        curPos.z = 0;
        lastPos.z = 0;

        RE::NiPoint3 dir = curPos - lastPos;

        float curSpeed = std::clamp(abs(MatrixUtils::vec3Len(dir)) / _frameTime, 0.0f, 350.0f);
        if (_prevSpeed > 20.0f) {
            curSpeed = (curSpeed + _prevSpeed) / 2.0f;
        }

        const float stepTime = std::clamp(cos(curSpeed / 140.0f), 0.28f, 0.50f);
        dir = MatrixUtils::vec3Norm(dir);

        // if decelerating reset target
        if (curSpeed - _prevSpeed < -20.0f) {
            _walkingState = 3;
        }

        _prevSpeed = curSpeed;

        static float spineAngle = 0.0;

        // setup current walking state based on velocity and previous state
        if (!isJumpingOrInAir()) {
            switch (_walkingState) {
            case 0: {
                if (curSpeed >= 35.0) {
                    _walkingState = 1; // start walking
                    _footStepping = std::rand() % 2 + 1; // pick a random foot to take a step  // NOLINT(concurrency-mt-unsafe)
                    _stepDir = dir;
                    _stepTimeinStep = stepTime;
                    _delayFrame = 2;

                    if (_footStepping == 1) {
                        _rightFootTarget = rFoot->world.translate + _stepDir * (curSpeed * stepTime * 1.5f);
                        _rightFootStart = rFoot->world.translate;
                        _leftFootTarget = lFoot->world.translate;
                        _leftFootStart = lFoot->world.translate;
                        _leftFootPos = _leftFootStart;
                        _rightFootPos = _rightFootStart;
                    } else {
                        _rightFootTarget = rFoot->world.translate;
                        _rightFootStart = rFoot->world.translate;
                        _leftFootTarget = lFoot->world.translate + _stepDir * (curSpeed * stepTime * 1.5f);
                        _leftFootStart = lFoot->world.translate;
                        _leftFootPos = _leftFootStart;
                        _rightFootPos = _rightFootStart;
                    }
                    _currentStepTime = stepTime / 2;
                    break;
                }
                _currentStepTime = 0.0;
                _footStepping = 0;
                spineAngle = 0.0;
                break;
            }
            case 1: {
                if (curSpeed < 20.0) {
                    _walkingState = 2; // begin process to stop walking
                    _currentStepTime = 0.0;
                }
                break;
            }
            case 2: {
                if (curSpeed >= 20.0) {
                    _walkingState = 1; // resume walking
                    _currentStepTime = 0.0;
                }
                break;
            }
            case 3: {
                _stepDir = dir;
                if (_footStepping == 1) {
                    _rightFootTarget = rFoot->world.translate + _stepDir * (curSpeed * stepTime * 0.1f);
                } else {
                    _leftFootTarget = lFoot->world.translate + _stepDir * (curSpeed * stepTime * 0.1f);
                }
                _walkingState = 1;
                break;
            }
            default: {
                _walkingState = 0;
                break;
            }
            }
        } else {
            _walkingState = 0;
        }

        if (_walkingState == 0) {
            // we're standing still so just set foot positions accordingly.
            _leftFootPos = lFoot->world.translate;
            _rightFootPos = rFoot->world.translate;
            _leftFootPos.z = _root->world.translate.z;
            _rightFootPos.z = _root->world.translate.z;

            return;
        }
        if (_walkingState == 1) {
            RE::NiPoint3 dirOffset = dir - _stepDir;
            const float dot = MatrixUtils::vec3Dot(dir, _stepDir);
            const float scale = (std::min)(curSpeed * stepTime * 1.5f, 140.0f);
            dirOffset = dirOffset * scale;

            float sign = 1.0f;

            _currentStepTime += _frameTime;

            const float frameStep = _frameTime / _stepTimeinStep;
            const float interp = std::clamp(frameStep * (_currentStepTime / _frameTime), 0.0f, 1.0f);

            if (_footStepping == 1) {
                sign = -1.0f;
                if (dot < 0.9) {
                    if (!_delayFrame) {
                        _rightFootTarget += dirOffset;
                        _stepDir = dir;
                        _delayFrame = 2;
                    } else {
                        _delayFrame--;
                    }
                } else {
                    _delayFrame = _delayFrame == 2 ? _delayFrame : _delayFrame + 1;
                }
                _rightFootTarget.z = _root->world.translate.z;
                _rightFootStart.z = _root->world.translate.z;
                _rightFootPos = _rightFootStart + (_rightFootTarget - _rightFootStart) * interp;
                const float stepAmount = std::clamp(MatrixUtils::vec3Len(_rightFootTarget - _rightFootStart) / 150.0f, 0.0f, 1.0f);
                const float stepHeight = (std::max)(stepAmount * 9.0f, 1.0f);
                const float up = sinf(interp * std::numbers::pi_v<float>) * stepHeight;
                _rightFootPos.z += up;
            } else {
                if (dot < 0.9f) {
                    if (!_delayFrame) {
                        _leftFootTarget += dirOffset;
                        _stepDir = dir;
                        _delayFrame = 2;
                    } else {
                        _delayFrame--;
                    }
                } else {
                    _delayFrame = _delayFrame == 2 ? _delayFrame : _delayFrame + 1;
                }
                _leftFootTarget.z = _root->world.translate.z;
                _leftFootStart.z = _root->world.translate.z;
                _leftFootPos = _leftFootStart + (_leftFootTarget - _leftFootStart) * interp;
                const float stepAmount = std::clamp(MatrixUtils::vec3Len(_leftFootTarget - _leftFootStart) / 150.0f, 0.0f, 1.0f);
                const float stepHeight = (std::max)(stepAmount * 9.0f, 1.0f);
                const float up = sinf(interp * std::numbers::pi_v<float>) * stepHeight;
                _leftFootPos.z += up;
            }

            spineAngle = sign * sinf(interp * std::numbers::pi_v<float>) * 3.0f;

            _spine->local.rotate = MatrixUtils::getMatrixFromEulerAngles(MatrixUtils::degreesToRads(spineAngle), 0.0f, 0.0f) * _spine->local.rotate;

            if (_currentStepTime > stepTime) {
                _currentStepTime = 0.0;
                _stepDir = dir;
                _stepTimeinStep = stepTime;
                //logger::info("%2f %2f", curSpeed, stepTime);

                if (_footStepping == 1) {
                    _footStepping = 2;
                    _leftFootTarget = lFoot->world.translate + _stepDir * scale;
                    _leftFootStart = _leftFootPos;
                } else {
                    _footStepping = 1;
                    _rightFootTarget = rFoot->world.translate + _stepDir * scale;
                    _rightFootStart = _rightFootPos;
                }
            }
            return;
        }
        if (_walkingState == 2) {
            _leftFootPos = lFoot->world.translate;
            _rightFootPos = rFoot->world.translate;
            _walkingState = 0;
        }
    }

    // adapted solver from VRIK.  Thanks prog!
    void Skeleton::setSingleLeg(const bool isLeft) const
    {
        const auto footNode = isLeft ? findNode(_root, "LLeg_Foot") : findNode(_root, "RLeg_Foot");
        const auto kneeNode = isLeft ? findNode(_root, "LLeg_Calf") : findNode(_root, "RLeg_Calf");
        const auto hipNode = isLeft ? findNode(_root, "LLeg_Thigh") : findNode(_root, "RLeg_Thigh");

        const RE::NiPoint3 footPos = isLeft ? _leftFootPos : _rightFootPos;
        const RE::NiPoint3 hipPos = hipNode->world.translate;

        const RE::NiPoint3 footToHip = hipNode->world.translate - footPos;

        auto rotV = RE::NiPoint3(0, 1, 0);
        if (_inPowerArmor) {
            rotV.y = 0;
            rotV.z = isLeft ? 1.0f : -1.0f;
        }
        const RE::NiPoint3 hipDir = hipNode->world.rotate.Transpose() * (rotV);
        const RE::NiPoint3 xDir = MatrixUtils::vec3Norm(footToHip);
        const RE::NiPoint3 yDir = MatrixUtils::vec3Norm(hipDir - xDir * MatrixUtils::vec3Dot(hipDir, xDir));

        const float thighLenOrig = MatrixUtils::vec3Len(kneeNode->local.translate);
        const float calfLenOrig = MatrixUtils::vec3Len(footNode->local.translate);
        float thighLen = thighLenOrig;
        float calfLen = calfLenOrig;

        const float ftLen = (std::max)(MatrixUtils::vec3Len(footToHip), 0.1f);

        if (ftLen > thighLen + calfLen) {
            const float diff = ftLen - thighLen - calfLen;
            const float ratio = calfLen / (calfLen + thighLen);
            calfLen += ratio * diff + 0.1f;
            thighLen += (1.0f - ratio) * diff + 0.1f;
        }
        // Use the law of cosines to calculate the angle the calf must bend to reach the knee position
        // In cases where this is impossible (foot too close to thigh), then set calfLen = thighLen so
        // there is always a solution
        float footAngle = acosf((calfLen * calfLen + ftLen * ftLen - thighLen * thighLen) / (2 * calfLen * ftLen));
        if (std::isnan(footAngle) || std::isinf(footAngle)) {
            calfLen = thighLen = (thighLenOrig + calfLenOrig) / 2.0f;
            footAngle = acosf((calfLen * calfLen + ftLen * ftLen - thighLen * thighLen) / (2 * calfLen * ftLen));
        }
        // Get the desired world coordinate of the knee
        const float xDist = cosf(footAngle) * calfLen;
        const float yDist = sinf(footAngle) * calfLen;
        const RE::NiPoint3 kneePos = footPos + xDir * xDist + yDir * yDist;

        const RE::NiPoint3 pos = kneePos - hipPos;
        RE::NiPoint3 uLocalDir = hipNode->world.rotate * (MatrixUtils::vec3Norm(pos) / hipNode->world.scale);
        hipNode->local.rotate = MatrixUtils::getMatrixFromRotateVectorVec(uLocalDir, kneeNode->local.translate) * hipNode->local.rotate;

        const RE::NiMatrix3 hipWR = hipNode->local.rotate * hipNode->parent->world.rotate;

        RE::NiMatrix3 calfWR = kneeNode->local.rotate * hipWR;

        uLocalDir = calfWR * (MatrixUtils::vec3Norm(footPos - kneePos) / kneeNode->world.scale);
        kneeNode->local.rotate = MatrixUtils::getMatrixFromRotateVectorVec(uLocalDir, footNode->local.translate) * kneeNode->local.rotate;

        calfWR = kneeNode->local.rotate * hipWR;

        // Calculate Clp:  Cwp = Twp + Twr * (Clp * Tws) = kneePos   ===>   Clp = Twr' * (kneePos - Twp) / Tws
        kneeNode->local.translate = hipWR * ((kneePos - hipPos) / hipNode->world.scale);
        if (MatrixUtils::vec3Len(kneeNode->local.translate) > thighLenOrig) {
            kneeNode->local.translate = MatrixUtils::vec3Norm(kneeNode->local.translate) * thighLenOrig;
        }

        // Calculate Flp:  Fwp = Cwp + Cwr * (Flp * Cws) = footPos   ===>   Flp = Cwr' * (footPos - Cwp) / Cws
        footNode->local.translate = calfWR * ((footPos - kneePos) / kneeNode->world.scale);
        if (MatrixUtils::vec3Len(footNode->local.translate) > calfLenOrig) {
            footNode->local.translate = MatrixUtils::vec3Norm(footNode->local.translate) * calfLenOrig;
        }
    }

    void Skeleton::rotateLeg(const uint32_t pos, const float angle) const
    {
        const auto rt = reinterpret_cast<BSFlattenedBoneTree*>(_root);

        auto& transform = rt->transforms[pos];
        transform.local.rotate = MatrixUtils::getMatrixFromEulerAngles(MatrixUtils::degreesToRads(angle), 0, 0) * transform.local.rotate;

        const auto& parentTransform = rt->transforms[transform.parPos];
        const RE::NiPoint3 p = parentTransform.world.rotate * (transform.local.translate * parentTransform.world.scale);
        transform.world.translate = parentTransform.world.translate + p;

        transform.world.rotate = transform.local.rotate * parentTransform.world.rotate;
    }

    /**
     * Hide the 3rd-person weapon that comes with the skeleton as we are using the 1st-person weapon model.
     */
    void Skeleton::hide3rdPersonWeapon() const
    {
        if (RE::NiAVObject* weapon = find1StChildNode(_rightArm.hand, "Weapon")) {
            setNodeVisibility(weapon, false);
        }
    }

    void Skeleton::hideFistHelpers() const
    {
        if (!isLeftHandedMode()) {
            RE::NiAVObject* node = findNode(_playerNodes->primaryWandNode, "fist_M_Right_HELPER");
            if (node != nullptr) {
                node->flags.flags |= 0x1; // first bit sets the cull flag so it will be hidden;
            }

            node = findNode(_playerNodes->primaryWandNode, "fist_F_Right_HELPER");
            if (node != nullptr) {
                node->flags.flags |= 0x1;
            }

            node = findNode(_playerNodes->primaryWandNode, "PA_fist_R_HELPER");
            if (node != nullptr) {
                node->flags.flags |= 0x1;
            }

            node = findNode(_playerNodes->SecondaryWandNode, "fist_M_Left_HELPER");
            if (node != nullptr) {
                node->flags.flags |= 0x1; // first bit sets the cull flag so it will be hidden;
            }

            node = findNode(_playerNodes->SecondaryWandNode, "fist_F_Left_HELPER");
            if (node != nullptr) {
                node->flags.flags |= 0x1;
            }

            node = findNode(_playerNodes->SecondaryWandNode, "PA_fist_L_HELPER");
            if (node != nullptr) {
                node->flags.flags |= 0x1;
            }
        } else {
            RE::NiAVObject* node = findNode(_playerNodes->SecondaryWandNode, "fist_M_Right_HELPER");
            if (node != nullptr) {
                node->flags.flags |= 0x1; // first bit sets the cull flag so it will be hidden;
            }

            node = findNode(_playerNodes->SecondaryWandNode, "fist_F_Right_HELPER");
            if (node != nullptr) {
                node->flags.flags |= 0x1;
            }

            node = findNode(_playerNodes->SecondaryWandNode, "PA_fist_R_HELPER");
            if (node != nullptr) {
                node->flags.flags |= 0x1;
            }

            node = findNode(_playerNodes->primaryWandNode, "fist_M_Left_HELPER");
            if (node != nullptr) {
                node->flags.flags |= 0x1; // first bit sets the cull flag so it will be hidden;
            }

            node = findNode(_playerNodes->primaryWandNode, "fist_F_Left_HELPER");
            if (node != nullptr) {
                node->flags.flags |= 0x1;
            }

            node = findNode(_playerNodes->primaryWandNode, "PA_fist_L_HELPER");
            if (node != nullptr) {
                node->flags.flags |= 0x1;
            }
        }

        if (const auto uiNode = findNode(_playerNodes->SecondaryWandNode, "Point002")) {
            uiNode->local.scale = 0.0;
        }
    }

    void Skeleton::showHidePAHud() const
    {
        if (const auto hud = findNode(_playerNodes->roomnode, "PowerArmorHelmetRoot")) {
            hud->local.scale = g_config.showPAHUD ? 1.0f : 0.0f;
        }
    }

    /**
     * Switch right and left weapon nodes if left-handed mode is enabled to correctly the hands.
     * Remember the setting to set back if settings change while game is running.
     */
    void Skeleton::handleLeftHandedWeaponNodesSwitch()
    {
        if (_lastLeftHandedModeSwitch == isLeftHandedMode()) {
            return;
        }

        _lastLeftHandedModeSwitch = isLeftHandedMode();
        logger::warn("Left-handed mode weapon nodes switch (LeftHanded:{})", _lastLeftHandedModeSwitch);

        RE::NiNode* rightWeapon = getWeaponNode();
        RE::NiNode* leftWeapon = _playerNodes->WeaponLeftNode;
        const auto rHand = findNode(getFirstPersonSkeleton(), "RArm_Hand");
        const auto lHand = findNode(getFirstPersonSkeleton(), "LArm_Hand");

        if (!rightWeapon || !rHand || !leftWeapon || !lHand) {
            logger::sample("Cannot set up weapon nodes for left-handed mode switch");
            _lastLeftHandedModeSwitch = isLeftHandedMode();
            return;
        }

        rHand->DetachChild(rightWeapon);
        rHand->DetachChild(leftWeapon);
        lHand->DetachChild(rightWeapon);
        lHand->DetachChild(leftWeapon);

        if (isLeftHandedMode()) {
            rHand->AttachChild(leftWeapon, true);
            lHand->AttachChild(rightWeapon, true);
        } else {
            rHand->AttachChild(rightWeapon, true);
            lHand->AttachChild(leftWeapon, true);
        }
    }

    // This is the main arm IK solver function - Algo credit to prog from SkyrimVR VRIK mod - what a beast!
    void Skeleton::setArms(bool isLeft)
    {
        // This first part is to handle the game calculating the first person hand based off two offset nodes
        // PrimaryWeaponOffset and PrimaryMeleeOffset
        // Unfortunately neither of these two nodes are that close to each other so when you equip a melee or ranged weapon
        // the hand will jump which completely messes up the solver and looks bad to boot.
        // So this code below does a similar operation as the in game function that solves the first person arm by forcing
        // everything to go to the PrimaryWeaponNode.  I have hardcoded a rotation below based off one of the guns that
        // matches my real life hand pose with an index controller very well.   I use this as the baseline for everything

        if (getFirstPersonSkeleton() == nullptr) {
            return;
        }

        RE::NiNode* rightWeapon = getWeaponNode();
        //RE::NiNode* rightWeapon = _playerNodes->primaryWandNode;
        RE::NiNode* leftWeapon = _playerNodes->WeaponLeftNode; // "WeaponLeft" can return incorrect node for left-handed with throwable weapons

        // handle the NON-primary hand (i.e. the hand that is NOT holding the weapon)
        bool handleOffhand = isLeftHandedMode() ^ isLeft;

        RE::NiNode* weaponNode = handleOffhand ? leftWeapon : rightWeapon;
        RE::NiNode* offsetNode = handleOffhand ? _playerNodes->SecondaryMeleeWeaponOffsetNode2 : _playerNodes->primaryWeaponOffsetNOde;

        if (handleOffhand) {
            _playerNodes->SecondaryMeleeWeaponOffsetNode2->local = _playerNodes->primaryWeaponOffsetNOde->local;
            _playerNodes->SecondaryMeleeWeaponOffsetNode2->local.rotate =
                _playerNodes->SecondaryMeleeWeaponOffsetNode2->local.rotate * MatrixUtils::getMatrixFromEulerAngles(0, MatrixUtils::degreesToRads(180.0f), 0);
            _playerNodes->SecondaryMeleeWeaponOffsetNode2->local.translate = RE::NiPoint3(-2, -9, 2);
            updateTransforms(_playerNodes->SecondaryMeleeWeaponOffsetNode2);
        }

        weaponNode->local.rotate = !isLeftHandedMode()
            ? MatrixUtils::getMatrix(-0.122f, 0.987f, 0.100f, 0.990f, 0.114f, 0.081f, 0.069f, 0.109f, -0.992f)
            : MatrixUtils::getMatrix(-0.122f, 0.987f, 0.100f, -0.990f, -0.114f, -0.081f, -0.069f, -0.109f, 0.992f);

        if (handleOffhand) {
            weaponNode->local.rotate = weaponNode->local.rotate * MatrixUtils::getMatrixFromEulerAngles(0, MatrixUtils::degreesToRads(isLeft ? 45.0f : -45.0f), 0);
        }

        weaponNode->local.translate = isLeftHandedMode()
            ? (isLeft ? RE::NiPoint3(3.389f, -2.099f, 3.133f) : RE::NiPoint3(0, -4.8f, 0))
            : isLeft
            ? RE::NiPoint3(0, 0, 0)
            : RE::NiPoint3(4.389f, -1.899f, -3.133f);

        dampenHand(offsetNode, isLeft);

        weaponNode->IncRefCount();
        Update1StPersonArm(RE::PlayerCharacter::GetSingleton(), &weaponNode, &offsetNode);

        RE::NiPoint3 handPos = isLeft ? _leftHand->world.translate : _rightHand->world.translate;
        RE::NiMatrix3 handRot = isLeft ? _leftHand->world.rotate : _rightHand->world.rotate;

        const auto arm = isLeft ? _leftArm : _rightArm;

        // Detect if the 1st person hand position is invalid. This can happen when a controller loses tracking.
        // If it is, do not handle IK and let Fallout use its normal animations for that arm instead.
        if (isnan(handPos.x) || isnan(handPos.y) || isnan(handPos.z) ||
            isinf(handPos.x) || isinf(handPos.y) || isinf(handPos.z) ||
            MatrixUtils::vec3Len(arm.upper->world.translate - handPos) > 200.0) {
            return;
        }

        float adjustedArmLength = g_config.armLength / 36.74f;

        // Shoulder IK is done in a very simple way

        RE::NiPoint3 shoulderToHand = handPos - arm.upper->world.translate;
        float armLength = g_config.armLength;
        float adjustAmount = (std::clamp)(MatrixUtils::vec3Len(shoulderToHand) - armLength * 0.5f, 0.0f, armLength * 0.85f) / (armLength * 0.85f);
        RE::NiPoint3 shoulderOffset = MatrixUtils::vec3Norm(shoulderToHand) * (adjustAmount * armLength * 0.08f);

        RE::NiPoint3 clavicalToNewShoulder = arm.upper->world.translate + shoulderOffset - arm.shoulder->world.translate;

        RE::NiPoint3 sLocalDir = arm.shoulder->world.rotate * (clavicalToNewShoulder / arm.shoulder->world.scale);

        RE::NiMatrix3 result = MatrixUtils::getMatrixFromRotateVectorVec(sLocalDir, RE::NiPoint3(1, 0, 0)) * arm.shoulder->local.rotate;
        arm.shoulder->local.rotate = result;

        updateDown(arm.shoulder, true);

        // The bend of the arm depends on its distance to the body.  Its distance as well as the lengths of
        // the upper arm and forearm define the sides of a triangle:
        //                 ^
        //                /|\         Let a,b be the arm lengths, c be the distance from hand-to-shoulder
        //               /^| \        Let A be the total angle at which the wrist must bend
        //              / ||  \       Let x be the width of the right triangle
        //            a/  y|   \  b   Let y be the height of the right triangle
        //            /   ||    \
        //           /    v|<-x->\
        // Shoulder /______|_____A\ Hand
        //                c
        // Law of cosines: Wrist angle A = acos( (b^2 + c^2 - a^2) / (2*b*c) )
        // The wrist angle is used to calculate x and y, which are used to position the elbow

        float negLeft = isLeft ? -1.0f : 1.0f;

        float originalUpperLen = MatrixUtils::vec3Len(arm.forearm1->local.translate);
        float originalForearmLen;

        if (_inPowerArmor) {
            originalForearmLen = MatrixUtils::vec3Len(arm.hand->local.translate);
        } else {
            originalForearmLen = MatrixUtils::vec3Len(arm.hand->local.translate) + MatrixUtils::vec3Len(arm.forearm2->local.translate) + MatrixUtils::vec3Len(arm.forearm3->local.translate);
        }
        float upperLen = originalUpperLen * adjustedArmLength;
        float forearmLen = originalForearmLen * adjustedArmLength;

        RE::NiPoint3 Uwp = arm.upper->world.translate;
        RE::NiPoint3 handToShoulder = Uwp - handPos;
        float hsLen = (std::max)(MatrixUtils::vec3Len(handToShoulder), 0.1f);

        if (hsLen > (upperLen + forearmLen) * 2.25f) {
            return;
        }

        // Stretch the upper arm and forearm proportionally when the hand distance exceeds the arm length
        if (hsLen > upperLen + forearmLen) {
            float diff = hsLen - upperLen - forearmLen;
            float ratio = forearmLen / (forearmLen + upperLen);
            forearmLen += ratio * diff + 0.1f;
            upperLen += (1.0f - ratio) * diff + 0.1f;
        }

        RE::NiPoint3 forwardDir = MatrixUtils::vec3Norm(_forwardDir);
        RE::NiPoint3 sidewaysDir = MatrixUtils::vec3Norm(_sidewaysRDir * negLeft);

        // The primary twist angle comes from the direction the wrist is pointing into the forearm
        RE::NiPoint3 handBack = handRot.Transpose() * (RE::NiPoint3(-1, 0, 0));
        float twistAngle = asinf((std::clamp)(handBack.z, -0.999f, 0.999f));

        // The second twist angle comes from a side vector pointing "outward" from the side of the wrist
        RE::NiPoint3 handSide = handRot.Transpose() * (RE::NiPoint3(0, -1, 0));
        RE::NiPoint3 handInSide = handSide * negLeft;
        float twistAngle2 = -1 * asinf((std::clamp)(handSide.z, -0.599f, 0.999f));

        // Blend the two twist angles together, using the primary angle more when the wrist is pointing downward
        //float interpTwist = (std::clamp)((handBack.z + 0.866f) * 1.155f, 0.25f, 0.8f); // 0 to 1 as hand points 60 degrees down to horizontal
        float interpTwist = (std::clamp)((handBack.z + 0.866f) * 1.155f, 0.45f, 0.8f); // 0 to 1 as hand points 60 degrees down to horizontal
        //		logger::info("%2f %2f %2f", rads_to_degrees(twistAngle), rads_to_degrees(twistAngle2), interpTwist);
        twistAngle = twistAngle + interpTwist * (twistAngle2 - twistAngle);
        // Wonkiness is bad.  Interpolate twist angle towards zero to correct it when the angles are pointed a certain way.
        /*	float fixWonkiness1 = (std::clamp)(vec3_dot(handSide, vec3_norm(-sidewaysDir - forwardDir * 0.25f + RE::NiPoint3(0, 0, -0.25f))), 0.0f, 1.0f);
            float fixWonkiness2 = 1.0f - (std::clamp)(vec3_dot(handBack, vec3_norm(forwardDir + sidewaysDir)), 0.0f, 1.0f);
            twistAngle = twistAngle + fixWonkiness1 * fixWonkiness2 * (-PI / 2.0f - twistAngle);*/

        //		logger::info("final angle %2f", rads_to_degrees(twistAngle));

        // Smooth out sudden changes in the twist angle over time to reduce elbow shake
        static std::array<float, 2> prevAngle = { 0, 0 };
        twistAngle = prevAngle[isLeft ? 0 : 1] + (twistAngle - prevAngle[isLeft ? 0 : 1]) * 0.25f;
        prevAngle[isLeft ? 0 : 1] = twistAngle;

        // Calculate the hand's distance behind the body - It will increase the minimum elbow rotation angle
        float size = 1.0;
        float behindD = -(forwardDir.x * arm.shoulder->world.translate.x + forwardDir.y * arm.shoulder->world.translate.y) - 10.0f;
        float handBehindDist = -(handPos.x * forwardDir.x + handPos.y * forwardDir.y + behindD);
        float behindAmount = (std::clamp)(handBehindDist / (40.0f * size), 0.0f, 1.0f);

        // Holding hands in front of chest increases the minimum elbow rotation angle (elbows lift) and decreases the maximum angle
        RE::NiPoint3 planeDir = MatrixUtils::rotateXY(forwardDir, negLeft * MatrixUtils::degreesToRads(135));
        float planeD = -(planeDir.x * arm.shoulder->world.translate.x + planeDir.y * arm.shoulder->world.translate.y) + 16.0f * size;
        float armCrossAmount = (std::clamp)((handPos.x * planeDir.x + handPos.y * planeDir.y + planeD) / (20.0f * size), 0.0f, 1.0f);

        // The arm lift limits how much the crossing amount can influence minimum elbow rotation
        // The maximum rotation is also decreased as hands lift higher (elbows point further downward)
        float armLiftLimitZ = _chest->world.translate.z * size;
        float armLiftThreshold = 60.0f * size;
        float armLiftLimit = (std::clamp)((armLiftLimitZ + armLiftThreshold - handPos.z) / armLiftThreshold, 0.0f, 1.0f); // 1 at bottom, 0 at top
        float upLimit = (std::clamp)((1.0f - armLiftLimit) * 1.4f, 0.0f, 1.0f); // 0 at bottom, 1 at a much lower top

        // Determine overall amount the elbows minimum rotation will be limited
        float adjustMinAmount = (std::max)(behindAmount, (std::min)(armCrossAmount, armLiftLimit));

        // Get the minimum and maximum angles at which the elbow is allowed to twist
        float twistMinAngle = MatrixUtils::degreesToRads(-85.0) + MatrixUtils::degreesToRads(50) * adjustMinAmount;
        float twistMaxAngle = MatrixUtils::degreesToRads(55.0) - (std::max)(MatrixUtils::degreesToRads(90) * armCrossAmount, MatrixUtils::degreesToRads(70) * upLimit);

        // Twist angle ranges from -PI/2 to +PI/2; map that range to go from the minimum to the maximum instead
        float twistLimitAngle = twistMinAngle + (twistAngle + std::numbers::pi_v<float> / 2.0f) / std::numbers::pi_v<float> * (twistMaxAngle - twistMinAngle);

        //logger::info("{} {} {} {}", rads_to_degrees(twistAngle), rads_to_degrees(twistAngle2), rads_to_degrees(twistAngle), rads_to_degrees(twistLimitAngle));
        // The bendDownDir vector points in the direction the player faces, and bends up/down with the final elbow angle
        RE::NiMatrix3 rot = MatrixUtils::getRotationAxisAngle(sidewaysDir * negLeft, twistLimitAngle);
        RE::NiPoint3 bendDownDir = rot.Transpose() * (forwardDir);

        // Get the "X" direction vectors pointing to the shoulder
        RE::NiPoint3 xDir = MatrixUtils::vec3Norm(handToShoulder);

        // Get the final "Y" vector, perpendicular to "X", and pointing in elbow direction (as in the diagram above)
        float sideD = -(sidewaysDir.x * arm.shoulder->world.translate.x + sidewaysDir.y * arm.shoulder->world.translate.y) - 1.0f * 8.0f;
        float acrossAmount = -(handPos.x * sidewaysDir.x + handPos.y * sidewaysDir.y + sideD) / (16.0f * 1.0f);
        float handSideTwistOutward = MatrixUtils::vec3Dot(handSide, MatrixUtils::vec3Norm(sidewaysDir + forwardDir * 0.5f));
        float armTwist = (std::clamp)(handSideTwistOutward - (std::max)(0.0f, acrossAmount + 0.25f), 0.0f, 1.0f);

        if (acrossAmount < 0) {
            acrossAmount *= 0.2f;
        }

        float handBehindHead = (std::clamp)((handBehindDist + 0.0f * size) / (15.0f * size), 0.0f, 1.0f) * (std::clamp)(upLimit * 1.2f, 0.0f, 1.0f);
        float elbowsTwistForward = (std::max)(acrossAmount * MatrixUtils::degreesToRads(90), handBehindHead * MatrixUtils::degreesToRads(120));
        RE::NiPoint3 elbowDir = MatrixUtils::rotateXY(bendDownDir, -negLeft * (MatrixUtils::degreesToRads(150) - armTwist * MatrixUtils::degreesToRads(25) - elbowsTwistForward));
        RE::NiPoint3 yDir = elbowDir - xDir * MatrixUtils::vec3Dot(elbowDir, xDir);
        yDir = MatrixUtils::vec3Norm(yDir);

        // Get the angle wrist must bend to reach elbow position
        // In cases where this is impossible (hand too close to shoulder), then set forearmLen = upperLen so there is always a solution
        float wristAngle = acosf((forearmLen * forearmLen + hsLen * hsLen - upperLen * upperLen) / (2 * forearmLen * hsLen));
        if (isnan(wristAngle) || isinf(wristAngle)) {
            forearmLen = upperLen = (originalUpperLen + originalForearmLen) / 2.0f * adjustedArmLength;
            wristAngle = acosf((forearmLen * forearmLen + hsLen * hsLen - upperLen * upperLen) / (2 * forearmLen * hsLen));
        }

        // Get the desired world coordinate of the elbow
        float xDist = cosf(wristAngle) * forearmLen;
        float yDist = sinf(wristAngle) * forearmLen;
        RE::NiPoint3 elbowWorld = handPos + xDir * xDist + yDir * yDist;

        // This code below rotates and positions the upper arm, forearm, and hand bones
        // Notation: C=Clavicle, U=Upper arm, F=Forearm, H=hand   w=world, l=local   p=position, r=rotation, s=scale
        //    Rules: World position = Parent world pos + Parent world rot * (Local pos * Parent World scale)
        //           World Rotation = Parent world rotation * Local Rotation
        // ---------------------------------------------------------------------------------------------------------

        // The upper arm bone must be rotated from its forward vector to its shoulder-to-elbow vector in its local space
        // Calculate Ulr:  baseUwr * rotTowardElbow = Cwr * Ulr   ===>   Ulr = Cwr' * baseUwr * rotTowardElbow
        RE::NiMatrix3 Uwr = arm.upper->world.rotate;
        RE::NiPoint3 pos = elbowWorld - Uwp;
        RE::NiPoint3 uLocalDir = Uwr * (MatrixUtils::vec3Norm(pos) / arm.upper->world.scale);

        arm.upper->local.rotate = MatrixUtils::getMatrixFromRotateVectorVec(uLocalDir, arm.forearm1->local.translate) * arm.upper->local.rotate;

        Uwr = arm.upper->local.rotate * arm.shoulder->world.rotate;

        // Find the angle of the forearm twisted around the upper arm and twist the upper arm to align it
        //    Uwr * twist = Cwr * Ulr   ===>   Ulr = Cwr' * Uwr * twist
        pos = handPos - elbowWorld;
        RE::NiPoint3 uLocalTwist = Uwr * (MatrixUtils::vec3Norm(pos));
        uLocalTwist.x = 0;
        RE::NiPoint3 upperSide = arm.upper->world.rotate.Transpose() * (RE::NiPoint3(0, 1, 0));
        RE::NiPoint3 uloc = arm.shoulder->world.rotate * (upperSide);
        uloc.x = 0;
        float upperAngle = acosf(MatrixUtils::vec3Dot(MatrixUtils::vec3Norm(uLocalTwist), MatrixUtils::vec3Norm(uloc))) * (uLocalTwist.z > 0 ? 1.f : -1.f);

        arm.upper->local.rotate = MatrixUtils::getMatrixFromEulerAngles(-upperAngle, 0, 0) * arm.upper->local.rotate;

        Uwr = arm.upper->local.rotate * arm.shoulder->world.rotate;

        arm.forearm1->local.rotate = MatrixUtils::getMatrixFromEulerAngles(-upperAngle, 0, 0) * arm.forearm1->local.rotate;

        // The forearm arm bone must be rotated from its forward vector to its elbow-to-hand vector in its local space
        // Calculate Flr:  Fwr * rotTowardHand = Uwr * Flr   ===>   Flr = Uwr' * Fwr * rotTowardHand
        RE::NiMatrix3 Fwr = arm.forearm1->local.rotate * Uwr;
        RE::NiPoint3 elbowHand = handPos - elbowWorld;
        RE::NiPoint3 fLocalDir = Fwr * (MatrixUtils::vec3Norm(elbowHand));

        arm.forearm1->local.rotate = MatrixUtils::getMatrixFromRotateVectorVec(fLocalDir, RE::NiPoint3(1, 0, 0)) * arm.forearm1->local.rotate;
        Fwr = arm.forearm1->local.rotate * Uwr;

        RE::NiMatrix3 Fwr3;

        if (!_inPowerArmor && arm.forearm2 != nullptr && arm.forearm3 != nullptr) {
            auto Fwr2 = arm.forearm2->local.rotate * Fwr;
            Fwr3 = arm.forearm3->local.rotate * Fwr2;

            // Find the angle the wrist is pointing and twist forearm3 appropriately
            //    Fwr * twist = Uwr * Flr   ===>   Flr = (Uwr' * Fwr) * twist = (Flr) * twist

            RE::NiPoint3 wLocalDir = Fwr3 * (MatrixUtils::vec3Norm(handInSide));
            wLocalDir.x = 0;
            RE::NiPoint3 forearm3Side = Fwr3.Transpose() * (RE::NiPoint3(0, 0, -1));
            // forearm is rotated 90 degrees already from hand so need this vector instead of 0,-1,0
            RE::NiPoint3 floc = Fwr2 * (MatrixUtils::vec3Norm(forearm3Side));
            floc.x = 0;
            float fcos = MatrixUtils::vec3Dot(MatrixUtils::vec3Norm(wLocalDir), MatrixUtils::vec3Norm(floc));
            float fsin = MatrixUtils::vec3Det(MatrixUtils::vec3Norm(wLocalDir), MatrixUtils::vec3Norm(floc), RE::NiPoint3(-1, 0, 0));
            float forearmAngle = -1 * negLeft * atan2f(fsin, fcos);

            arm.forearm2->local.rotate = MatrixUtils::getMatrixFromEulerAngles(negLeft * forearmAngle / 2, 0, 0) * arm.forearm2->local.rotate;
            arm.forearm3->local.rotate = MatrixUtils::getMatrixFromEulerAngles(negLeft * forearmAngle / 2, 0, 0) * arm.forearm3->local.rotate;

            Fwr2 = arm.forearm2->local.rotate * Fwr;
            Fwr3 = arm.forearm3->local.rotate * Fwr2;
        }

        // Calculate Hlr:  Fwr * Hlr = handRot   ===>   Hlr = Fwr' * handRot
        arm.hand->local.rotate = handRot * (_inPowerArmor ? Fwr : Fwr3).Transpose();

        // Calculate Flp:  Fwp = Uwp + Uwr * (Flp * Uws) = elbowWorld   ===>   Flp = Uwr' * (elbowWorld - Uwp) / Uws
        arm.forearm1->local.translate = Uwr * ((elbowWorld - Uwp) / arm.upper->world.scale);

        float origEHLen = MatrixUtils::vec3Len(arm.hand->world.translate - arm.forearm1->world.translate);
        float forearmRatio = forearmLen / origEHLen * _root->local.scale;

        if (arm.forearm2 && !_inPowerArmor) {
            arm.forearm2->local.translate *= forearmRatio;
            arm.forearm3->local.translate *= forearmRatio;
        }
        arm.hand->local.translate *= forearmRatio;
    }

    void Skeleton::hideHands() const
    {
        const RE::NiPoint3 rwp = _rightArm.shoulder->world.translate;
        _root->local.scale = 0.00001f;
        updateTransforms(_root);
        _root->world.translate += _forwardDir * -10.0f;
        _root->world.translate.z = rwp.z;
        updateDown(_root, false);
    }

    void Skeleton::calculateHandPose(const std::string& bone, const float gripProx, const bool thumbUp, const bool isLeft)
    {
        Quaternion qc, qt;
        const float sign = isLeft ? -1.0f : 1.0f;

        // if a mod is using the papyrus interface to manually set finger poses
        if (handPapyrusHasControl[bone]) {
            qt.fromMatrix(handOpen[bone].rotate);
            Quaternion qo;
            qo.fromMatrix(handClosed[bone].rotate);
            qo.slerp(std::clamp(handPapyrusPose[bone], -1.0f, 2.0f), qt);
            qt = qo;
        }
        // thumbUp pose
        else if (thumbUp && bone.find("Finger1") != std::string::npos) {
            if (bone.find("Finger11") != std::string::npos) {
                RE::NiMatrix3 wr = handOpen[bone].rotate;
                wr = MatrixUtils::getMatrixFromEulerAngles(sign * 0.5f, sign * 0.4f, -0.3f) * wr;
                qt.fromMatrix(wr);
            } else if (bone.find("Finger13") != std::string::npos) {
                RE::NiMatrix3 wr = handOpen[bone].rotate;
                wr = MatrixUtils::getMatrixFromEulerAngles(0, 0, MatrixUtils::degreesToRads(-35.0f)) * wr;
                qt.fromMatrix(wr);
            }
        } else if (_closedHand[bone]) {
            qt.fromMatrix(handClosed[bone].rotate);
        } else {
            qt.fromMatrix(handOpen[bone].rotate);
            if (_handBonesButton.at(bone) == k_EButton_Grip) {
                Quaternion qo;
                qo.fromMatrix(handClosed[bone].rotate);
                qo.slerp(1.0f - gripProx, qt);
                qt = qo;
            }
        }

        qc.fromMatrix(_handBones[bone].rotate);
        const float blend = std::clamp(_frameTime * 7, -1.0f, 2.0f);
        qc.slerp(blend, qt);
        _handBones[bone].rotate = qc.getMatrix();
    }

    /**
     * Copy the 1st-person bone position for the given hand bone.
     * Useful for different weapons holding hand poses.
     */
    void Skeleton::copy1StPerson(const std::string& bone)
    {
        const auto fpTree = getFirstPersonBoneTree();
        const int pos = fpTree->GetBoneIndex(bone);
        if (pos >= 0) {
            if (fpTree->transforms[pos].refNode) {
                _handBones[bone] = fpTree->transforms[pos].refNode->local;
            } else {
                _handBones[bone] = fpTree->transforms[pos].local;
            }
        }
    }

    /**
     * In left-handed mode the 1st-person skeleton is not using the correct hand so we can't use "copy1StPerson" method.
     * Instead, we just force a specific hand pose that makes sense.
     */
    void Skeleton::setPredefinedHandPose(const std::string& bone)
    {
        Quaternion qo, qt;
        qt.fromMatrix(handOpen[bone].rotate);
        qo.fromMatrix(handClosed[bone].rotate);
        qo.slerp(std::clamp(getHandBonePose(bone, g_frik.isMeleeWeaponDrawn()), -1.0f, 2.0f), qt);
        _handBones[bone].rotate = qo.getMatrix();
    }

    void Skeleton::setHandPose()
    {
        const auto rt = reinterpret_cast<BSFlattenedBoneTree*>(_root);
        for (auto pos = 0; pos < rt->numTransforms; pos++) {
            std::string name = _boneTreeVec[pos];
            auto found = _fingerRelations.find(name);
            if (found != _fingerRelations.end()) {
                const bool isLeft = name[0] == 'L';
                const uint64_t reg = isLeft
                    ? VRControllers.getControllerState_DEPRECATED(TrackerType::Left).ulButtonTouched
                    : VRControllers.getControllerState_DEPRECATED(TrackerType::Right).ulButtonTouched;
                const float gripProx = isLeft
                    ? VRControllers.getControllerState_DEPRECATED(TrackerType::Left).rAxis[2].x
                    : VRControllers.getControllerState_DEPRECATED(TrackerType::Right).rAxis[2].x;
                const bool thumbUp = reg & ButtonMaskFromId(k_EButton_Grip)
                    && reg & ButtonMaskFromId(k_EButton_SteamVR_Trigger)
                    && !(reg & ButtonMaskFromId(k_EButton_SteamVR_Touchpad));
                _closedHand[name] = reg & ButtonMaskFromId(_handBonesButton.at(name));

                if (IsWeaponDrawn()
                    && (isLeftHandedMode() || (!g_frik.isPipboyOn() && !g_frik.isPipboyOperatingWithFinger())) // left-handed has pipboy on the hand with the weapon
                    && !(isLeft ^ isLeftHandedMode())) {
                    if (isLeftHandedMode()) {
                        setPredefinedHandPose(name);
                    } else {
                        // use the game hand position for the weapon in hand
                        copy1StPerson(name);
                    }
                } else {
                    // use the forced hand position
                    calculateHandPose(name, gripProx, thumbUp, isLeft);
                }

                const RE::NiTransform trans = _handBones[name];

                rt->transforms[pos].local.rotate = trans.rotate;
                rt->transforms[pos].local.translate = handOpen[name.c_str()].translate;

                if (rt->transforms[pos].refNode) {
                    rt->transforms[pos].refNode->local = rt->transforms[pos].local;
                }
            }

            if (rt->transforms[pos].refNode) {
                rt->transforms[pos].world = rt->transforms[pos].refNode->world;
            } else {
                const short parent = rt->transforms[pos].parPos;
                RE::NiPoint3 p = rt->transforms[pos].local.translate;
                p = rt->transforms[parent].world.rotate.Transpose() * ((p * rt->transforms[parent].world.scale));

                rt->transforms[pos].world.translate = rt->transforms[parent].world.translate + p;

                rt->transforms[pos].world.rotate = rt->transforms[pos].local.rotate * rt->transforms[parent].world.rotate;
            }
        }
    }

    void Skeleton::selfieSkelly() const
    {
        // Projects the 3rd person body out in front of the player by offset amount
        if (!g_frik.getSelfieMode() || !_root) {
            return;
        }

        const float z = _root->local.translate.z;
        const RE::NiNode* body = _root->parent;

        const RE::NiPoint3 back = MatrixUtils::vec3Norm(RE::NiPoint3(-_forwardDir.x, -_forwardDir.y, 0));
        const auto bodyDir = RE::NiPoint3(0, 1, 0);

        _root->local.rotate = MatrixUtils::getMatrixFromRotateVectorVec(back, bodyDir) * body->world.rotate.Transpose();
        _root->local.translate = body->world.translate - _curentPosition;
        _root->local.translate.y += g_config.selfieOutFrontDistance;
        _root->local.translate.z = z;
    }

    void Skeleton::dampenHand(RE::NiNode* node, const bool isLeft)
    {
        if (!g_config.dampenHands) {
            if (g_frik.isMainConfigurationModeActive()) {
                // small hack to prevent jarring effect when dampen is enabled for the first time via main config
                if (isLeft) {
                    _leftHandPrevFrame = node->world;
                } else {
                    _rightHandPrevFrame = node->world;
                }
            }
            return;
        }

        const bool isInScopeMenu = g_frik.isInScopeMenu();
        if (isInScopeMenu && !g_config.dampenHandsInVanillaScope) {
            return;
        }

        // Get the previous frame transform
        const RE::NiTransform& prevFrame = isLeft ? _leftHandPrevFrame : _rightHandPrevFrame;

        // Spherical interpolation between previous frame and current frame for the world rotation matrix
        Quaternion rq, rt;
        rq.fromMatrix(prevFrame.rotate);
        rt.fromMatrix(node->world.rotate);
        rq.slerp(1 - (isInScopeMenu ? g_config.dampenHandsRotationInVanillaScope : g_config.dampenHandsRotation), rt);
        node->world.rotate = rq.getMatrix();

        // Linear interpolation between the position from the previous frame to current frame
        const RE::NiPoint3 dir = _curentPosition - _lastPosition; // Offset the player movement from this interpolation
        RE::NiPoint3 deltaPos = node->world.translate - prevFrame.translate - dir; // Add in player velocity
        deltaPos *= isInScopeMenu ? g_config.dampenHandsTranslationInVanillaScope : g_config.dampenHandsTranslation;
        node->world.translate -= deltaPos;

        // Update the previous frame transform
        if (isLeft) {
            _leftHandPrevFrame = node->world;
        } else {
            _rightHandPrevFrame = node->world;
        }

        updateDown(node, false);
    }

    /**
     * Default skeleton nodes position and rotation to be used for resetting skeleton before each frame update manipulations.
     * Required because loading a game does NOT reset the skeleton nodes resulting in incorrect positions/rotations.
     * Entering/Existing power-armor fixes the skeleton but loading the game over and over makes it worse.
     * By forcing the hardcoded default values the issue is prevented as we always start with the same initial values.
     * The values were collected by reading them from the skeleton nodes on first load of a saved game before any manipulations.
     */
    std::unordered_map<std::string, RE::NiTransform> Skeleton::getSkeletonNodesDefaultTransforms()
    {
        return std::unordered_map<std::string, RE::NiTransform>{
            { "Root", MatrixUtils::getTransform(0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f) },
            { "COM", MatrixUtils::getTransform(0.0f, 0.0f, 68.91130f, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f) },
            { "Pelvis", MatrixUtils::getTransform(0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f) },
            { "LLeg_Thigh", MatrixUtils::getTransform(0.0f, 0.00040f, 6.61510f, -0.99112f, -0.00017f, -0.13297f, -0.03860f, 0.95730f, 0.28650f, 0.12725f, 0.28909f, -0.94881f, 1.0f) },
            { "LLeg_Calf", MatrixUtils::getTransform(31.59520f, 0.0f, 0.0f, 0.99210f, 0.12266f, -0.02618f, -0.12266f, 0.99245f, 0.00159f, 0.02617f, 0.00164f, 0.99966f, 1.0f) },
            { "LLeg_Foot", MatrixUtils::getTransform(31.94290f, 0.0f, 0.0f, 0.45330f, -0.88555f, -0.10159f, 0.88798f, 0.45855f, -0.03499f, 0.07757f, -0.07435f, 0.99421f, 1.0f) },
            { "RLeg_Thigh", MatrixUtils::getTransform(0.0f, 0.00040f, -6.61510f, -0.99307f, 0.00520f, 0.11741f, -0.02903f, 0.95721f, -0.28795f, -0.11389f, -0.28936f, -0.95042f, 1.0f) },
            { "RLeg_Calf", MatrixUtils::getTransform(31.59510f, 0.0f, 0.0f, 0.99108f, 0.13329f, 0.00011f, -0.13329f, 0.99108f, 0.00139f, 0.00007f, -0.00140f, 1.0f, 1.0f) },
            { "RLeg_Foot", MatrixUtils::getTransform(31.94260f, 0.0f, 0.0f, 0.44741f, -0.88731f, 0.11181f, 0.89061f, 0.45344f, 0.03463f, -0.08143f, 0.08409f, 0.99313f, 1.0f) },
            { "SPINE1", MatrixUtils::getTransform(3.792f, -0.00290f, 0.0f, 0.99246f, -0.12254f, 0.0f, 0.12254f, 0.99246f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f) },
            { "SPINE2", MatrixUtils::getTransform(8.70470f, 0.0f, 0.0f, 0.98463f, 0.17464f, 0.0f, -0.17464f, 0.98463f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f) },
            { "Chest", MatrixUtils::getTransform(9.95630f, 0.0f, 0.0f, 0.99983f, -0.01837f, 0.0f, 0.01837f, 0.99983f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f) },
            { "LArm_Collarbone",
              MatrixUtils::getTransform(19.15320f, -0.51040f, 1.69510f, -0.40489f, -0.00599f, -0.91434f, -0.26408f, 0.95813f, 0.11066f, 0.87540f, 0.28627f, -0.38952f, 1.0f) },
            { "LArm_UpperArm", MatrixUtils::getTransform(12.53660f, 0.0f, 0.0f, 0.91617f, -0.25279f, -0.31102f, 0.25328f, 0.96658f, -0.03954f, 0.31062f, -0.04255f, 0.94958f, 1.0f) },
            { "LArm_ForeArm1", MatrixUtils::getTransform(17.96830f, 0.0f, 0.0f, 0.85511f, -0.51462f, -0.06284f, 0.51548f, 0.85690f, -0.00289f, 0.05534f, -0.02992f, 0.99802f, 1.0f) },
            { "LArm_ForeArm2", MatrixUtils::getTransform(6.15160f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.99999f, -0.00536f, 0.0f, 0.00536f, 0.99999f, 1.0f) },
            { "LArm_ForeArm3", MatrixUtils::getTransform(6.15160f, -0.00010f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.99999f, -0.00536f, 0.0f, 0.00536f, 0.99999f, 1.0f) },
            { "LArm_Hand", MatrixUtils::getTransform(6.15160f, 0.0f, -0.00010f, 0.98845f, 0.14557f, -0.04214f, 0.04136f, 0.00839f, 0.99911f, 0.14579f, -0.98931f, 0.00227f, 1.0f) },
            {
                "RArm_Collarbone",
                MatrixUtils::getTransform(19.15320f, -0.51040f, -1.69510f, -0.40497f, -0.00602f, 0.91431f, -0.26413f, 0.95811f, -0.11069f, -0.87535f, -0.28632f, -0.38960f, 1.0f)
            },
            { "RArm_UpperArm", MatrixUtils::getTransform(12.53430f, 0.0f, 0.0f, 0.91620f, -0.25314f, 0.31064f, 0.25365f, 0.96649f, 0.03947f, -0.31022f, 0.04263f, 0.94971f, 1.0f) },
            { "RArm_ForeArm1", MatrixUtils::getTransform(17.97050f, 0.00010f, -0.00010f, 0.85532f, -0.51419f, 0.06360f, 0.51507f, 0.85714f, 0.00288f, -0.05599f, 0.03030f, 0.99797f, 1.0f) },
            { "RArm_ForeArm2", MatrixUtils::getTransform(6.15280f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.99999f, 0.00536f, 0.0f, -0.00536f, 0.99999f, 1.0f) },
            { "RArm_ForeArm3", MatrixUtils::getTransform(6.15290f, 0.0f, -0.00010f, 1.0f, 0.0f, 0.0f, 0.0f, 0.99999f, 0.00536f, 0.0f, -0.00536f, 0.99999f, 1.0f) },
            { "RArm_Hand", MatrixUtils::getTransform(6.15290f, 0.0f, 0.0f, 0.98845f, 0.14557f, 0.04214f, 0.04136f, 0.00839f, -0.99911f, -0.14579f, 0.98931f, 0.00227f, 1.0f) },
            { "Neck", MatrixUtils::getTransform(22.084f, -3.767f, 0.0f, 0.91268f, -0.40867f, -0.00003f, 0.40867f, 0.91268f, 0.0f, 0.00002f, -0.00001f, 1.0f, 1.0f) },
            { "Head", MatrixUtils::getTransform(8.22440f, 0.0f, 0.0f, 0.94872f, 0.31613f, 0.00002f, -0.31613f, 0.94872f, -0.00001f, -0.00003f, 0.0f, 1.0f, 1.0f) },
        };
    }

    // See "getSkeletonNodesDefaultTransforms" above
    std::unordered_map<std::string, RE::NiTransform> Skeleton::getSkeletonNodesDefaultTransformsInPA()
    {
        return std::unordered_map<std::string, RE::NiTransform>{
            { "Root", MatrixUtils::getTransform(0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f) },
            { "COM", MatrixUtils::getTransform(0.0f, -3.74980f, 89.41950f, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f) },
            { "Pelvis", MatrixUtils::getTransform(0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f) },
            { "LLeg_Thigh", MatrixUtils::getTransform(4.54870f, -1.33f, 6.90830f, -0.98736f, 0.14491f, 0.06416f, 0.06766f, 0.01940f, 0.99752f, 0.14331f, 0.98925f, -0.02896f, 1.0f) },
            { "LLeg_Calf", MatrixUtils::getTransform(34.298f, 0.0f, 0.0f, 0.99681f, -0.00145f, 0.07983f, 0.00170f, 0.99999f, -0.00305f, -0.07982f, 0.00318f, 0.99680f, 1.0f) },
            { "LLeg_Foot", MatrixUtils::getTransform(52.54120f, 0.0f, 0.0f, 0.63109f, -0.76168f, -0.14685f, -0.07775f, 0.12624f, -0.98895f, 0.77180f, 0.63554f, 0.02045f, 1.0f) },
            { "RLeg_Thigh", MatrixUtils::getTransform(4.54760f, -1.32430f, -6.898f, -0.98732f, 0.14533f, -0.06381f, 0.06732f, 0.01938f, -0.99754f, -0.14374f, -0.98919f, -0.02892f, 1.0f) },
            { "RLeg_Calf", MatrixUtils::getTransform(34.29790f, 0.0f, 0.0f, 0.99684f, -0.00096f, -0.07937f, 0.00120f, 0.99999f, 0.00307f, 0.07937f, -0.00316f, 0.99684f, 1.0f) },
            { "RLeg_Foot", MatrixUtils::getTransform(52.54080f, 0.0f, 0.0f, 0.63118f, -0.76162f, 0.14677f, -0.07771f, 0.12618f, 0.98896f, -0.77173f, -0.63562f, 0.02046f, 1.0f) },
            { "SPINE1", MatrixUtils::getTransform(5.75050f, -0.00290f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f) },
            { "SPINE2", MatrixUtils::getTransform(5.62550f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f) },
            { "Chest", MatrixUtils::getTransform(5.53660f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f) },
            { "LArm_Collarbone", MatrixUtils::getTransform(22.192f, 0.34820f, 1.00420f, -0.34818f, -0.05435f, -0.93585f, -0.26919f, 0.96207f, 0.04428f, 0.89794f, 0.26734f, -0.34961f, 1.0f) },
            { "LArm_UpperArm", MatrixUtils::getTransform(14.59840f, 0.00010f, 0.00010f, 0.77214f, -0.19393f, -0.60514f, 0.08574f, 0.97538f, -0.20318f, 0.62964f, 0.10499f, 0.76976f, 1.0f) },
            { "LArm_ForeArm1", MatrixUtils::getTransform(19.53690f, 0.41980f, 0.04580f, 0.92233f, -0.38166f, -0.06030f, 0.38176f, 0.92420f, -0.01042f, 0.05971f, -0.01341f, 0.99813f, 1.0f) },
            { "LArm_ForeArm2", MatrixUtils::getTransform(0.00020f, 0.00020f, 0.00020f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f) },
            { "LArm_ForeArm3", MatrixUtils::getTransform(10.000494f, 0.000162f, -0.000004f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f) },
            { "LArm_Hand", MatrixUtils::getTransform(26.96440f, 0.00020f, 0.00040f, 0.98604f, 0.16503f, 0.02218f, 0.00691f, -0.17364f, 0.98479f, 0.16638f, -0.97088f, -0.17236f, 1.0f) },
            { "RArm_Collarbone",
              MatrixUtils::getTransform(22.19190f, 0.34810f, -1.004f, -0.34818f, -0.06482f, 0.93518f, -0.26918f, 0.96251f, -0.03351f, -0.89795f, -0.26340f, -0.35257f, 1.0f) },
            { "RArm_UpperArm", MatrixUtils::getTransform(14.59880f, 0.0f, 0.0f, 0.77213f, -0.19339f, 0.60533f, 0.09277f, 0.97667f, 0.19369f, -0.62866f, -0.09340f, 0.77205f, 1.0f) },
            { "RArm_ForeArm1", MatrixUtils::getTransform(19.53660f, 0.41990f, -0.04620f, 0.92233f, -0.38166f, 0.06029f, 0.38171f, 0.92422f, 0.01129f, -0.06003f, 0.01260f, 0.99812f, 1.0f) },
            { "RArm_ForeArm2", MatrixUtils::getTransform(-0.00010f, -0.00010f, -0.00010f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f) },
            { "RArm_ForeArm3", MatrixUtils::getTransform(10.00050f, -0.00010f, 0.00010f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f) },
            { "RArm_Hand", MatrixUtils::getTransform(26.96460f, 0.00010f, 0.00120f, 0.98604f, 0.16503f, -0.02218f, 0.00691f, -0.17364f, -0.98479f, -0.16638f, 0.97088f, -0.17236f, 1.0f) },
            { "Neck", MatrixUtils::getTransform(24.29350f, -2.84160f, 0.0f, 0.92612f, -0.37723f, -0.00002f, 0.37723f, 0.92612f, 0.00001f, 0.00002f, -0.00002f, 1.0f, 1.0f) },
            { "Head", MatrixUtils::getTransform(8.22440f, 0.0f, 0.0f, 0.94891f, 0.31555f, 0.00002f, -0.31555f, 0.94891f, 0.0f, -0.00002f, -0.00001f, 1.0f, 1.0f) },
        };
    }

    std::map<std::string, std::pair<std::string, std::string>> Skeleton::makeFingerRelations()
    {
        std::map<std::string, std::pair<std::string, std::string>> map;

        auto addFingerRelations = [&](const std::string& hand, const std::string& finger1, const std::string& finger2,
            const std::string& finger3) {
            map.insert({ finger1, { hand, finger2 } });
            map.insert({ finger2, { finger1, finger3 } });
            map.insert({ finger3, { finger2, std::string() } });
        };

        //left hand
        addFingerRelations("LArm_Hand", "LArm_Finger11", "LArm_Finger12", "LArm_Finger13");
        addFingerRelations("LArm_Hand", "LArm_Finger21", "LArm_Finger22", "LArm_Finger23");
        addFingerRelations("LArm_Hand", "LArm_Finger31", "LArm_Finger32", "LArm_Finger33");
        addFingerRelations("LArm_Hand", "LArm_Finger41", "LArm_Finger42", "LArm_Finger43");
        addFingerRelations("LArm_Hand", "LArm_Finger51", "LArm_Finger52", "LArm_Finger53");

        //right hand
        addFingerRelations("RArm_Hand", "RArm_Finger11", "RArm_Finger12", "RArm_Finger13");
        addFingerRelations("RArm_Hand", "RArm_Finger21", "RArm_Finger22", "RArm_Finger23");
        addFingerRelations("RArm_Hand", "RArm_Finger31", "RArm_Finger32", "RArm_Finger33");
        addFingerRelations("RArm_Hand", "RArm_Finger41", "RArm_Finger42", "RArm_Finger43");
        addFingerRelations("RArm_Hand", "RArm_Finger51", "RArm_Finger52", "RArm_Finger53");

        return map;
    }

    /**
     * setup hand bones to openvr button mapping
     */
    std::unordered_map<std::string, VRButtonId> Skeleton::getHandBonesButtonMap()
    {
        return std::unordered_map<std::string, VRButtonId>{
            { "LArm_Finger11", k_EButton_SteamVR_Touchpad },
            { "LArm_Finger12", k_EButton_SteamVR_Touchpad },
            { "LArm_Finger13", k_EButton_SteamVR_Touchpad },
            { "LArm_Finger21", k_EButton_SteamVR_Trigger },
            { "LArm_Finger22", k_EButton_SteamVR_Trigger },
            { "LArm_Finger23", k_EButton_SteamVR_Trigger },
            { "LArm_Finger31", k_EButton_Grip },
            { "LArm_Finger32", k_EButton_Grip },
            { "LArm_Finger33", k_EButton_Grip },
            { "LArm_Finger41", k_EButton_Grip },
            { "LArm_Finger42", k_EButton_Grip },
            { "LArm_Finger43", k_EButton_Grip },
            { "LArm_Finger51", k_EButton_Grip },
            { "LArm_Finger52", k_EButton_Grip },
            { "LArm_Finger53", k_EButton_Grip },
            { "RArm_Finger11", k_EButton_SteamVR_Touchpad },
            { "RArm_Finger12", k_EButton_SteamVR_Touchpad },
            { "RArm_Finger13", k_EButton_SteamVR_Touchpad },
            { "RArm_Finger21", k_EButton_SteamVR_Trigger },
            { "RArm_Finger22", k_EButton_SteamVR_Trigger },
            { "RArm_Finger23", k_EButton_SteamVR_Trigger },
            { "RArm_Finger31", k_EButton_Grip },
            { "RArm_Finger32", k_EButton_Grip },
            { "RArm_Finger33", k_EButton_Grip },
            { "RArm_Finger41", k_EButton_Grip },
            { "RArm_Finger42", k_EButton_Grip },
            { "RArm_Finger43", k_EButton_Grip },
            { "RArm_Finger51", k_EButton_Grip },
            { "RArm_Finger52", k_EButton_Grip },
            { "RArm_Finger53", k_EButton_Grip }
        };
    }
}
