#pragma once

#include <array>
#include <cstdint>
#include <memory>

#include "CullGeometryHandler.h"
#include "HandPose.h"
#include "SelfieHandler.h"
#include "WeaponHandRecoil.h"
#include "f4vr/PlayerNodes.h"

namespace frik
{
    struct ArmNodes
    {
        RE::NiAVObject* shoulder = nullptr;
        RE::NiAVObject* upper = nullptr;
        RE::NiAVObject* upperT1 = nullptr;
        RE::NiAVObject* forearm1 = nullptr;
        RE::NiAVObject* forearm2 = nullptr;
        RE::NiAVObject* forearm3 = nullptr;
        RE::NiAVObject* hand = nullptr;
    };

    class Skeleton
    {
    public:
        /**
         * Create a fully initialized skeleton, or nullptr if the game nodes required by the skeleton
         * are not available yet. Guarantees that an existing Skeleton instance is always usable.
         */
        static std::unique_ptr<Skeleton> create(RE::NiNode* rootNode, bool inPowerArmor);

        ArmNodes getLeftArm() const
        {
            return _leftArm;
        }

        ArmNodes getRightArm() const
        {
            return _rightArm;
        }

        static float getAdjustedPlayerHMDOffset();

        void onFrameUpdate();

        bool mirrorFingerLocalTransforms(bool sourceIsLeft, const std::array<RE::NiTransform, HandPose::FINGER_BONE_COUNT>& sourceTransforms, std::uint16_t sourceEnabledMask,
            std::array<RE::NiTransform, HandPose::FINGER_BONE_COUNT>& outTargetTransforms, std::uint16_t& outTargetEnabledMask) const
        {
            return _handPose.mirrorFingerLocalTransforms(sourceIsLeft, sourceTransforms, sourceEnabledMask, outTargetTransforms, outTargetEnabledMask);
        }

    private:
        Skeleton(RE::NiNode* rootNode, const bool inPowerArmor)
            : _root(rootNode),
              _inPowerArmor(inPowerArmor),
              _handPose(inPowerArmor)
        {
            _curentPosition = RE::NiPoint3(0, 0, 0);
            _walkingState = 0;
        }

        // initialization
        bool initializeNodes();
        void initArmsNodes();
        void initSkeletonNodesDefaults();
        void setBodyLen();

        // on frame update - skeleton update
        void setTime();
        void restoreNodesToDefault();
        void setupHead(float neckYaw, float neckPitch) const;
        void setBodyUnderHMD(float neckYaw, float neckPitch);
        void setBodyPosture(float neckPitch);
        void setKneePos();
        void walk();
        void setSingleLeg(bool isLeft) const;
        void handleLeftHandedWeaponNodesSwitch();
        void setArms(bool isLeft);
        void restoreArmNodesToDefault(bool isLeft);
        bool solveArmToHandWorldTarget(bool isLeft, const RE::NiTransform& handWorldTarget);
        void dampenHand(RE::NiNode* node, bool isLeft);
        void hide3rdPersonWeapon() const;
        void hideFistHelpers() const;
        void showHidePAHud() const;
        void hideHands() const;
        void fixArmor() const;

        // Utils - Body Positioning
        float getNeckYaw() const;
        float getNeckPitch() const;
        float getBodyPitch(float neckPitch) const;
        void rotateLeg(uint32_t pos, float angle) const;

        // root node and is in power armor define the Skeleton instance
        RE::NiNode* _root;
        bool _inPowerArmor;

        // ???
        LARGE_INTEGER _freqCounter;
        LARGE_INTEGER _timer;
        LARGE_INTEGER _prevTime;
        float _frameTime;

        // handle switch of hands for left-handed mode
        bool _lastLeftHandedModeSwitch = false;

        // Camera positions
        RE::NiPoint3 _curentPosition;
        RE::NiPoint3 _lastPosition;
        inline static float _comfortSneakCameraOffsetAdjustment = -1.0f;

        // ???
        RE::NiPoint3 _forwardDir;
        RE::NiPoint3 _sidewaysRDir;
        RE::NiPoint3 _upDir;

        // skeleton nodes
        f4vr::PlayerNodes* _playerNodes;
        RE::NiNode* _rightHand;
        RE::NiNode* _leftHand;
        RE::NiNode* _head;
        RE::NiNode* _spine;
        RE::NiNode* _chest;
        float _torsoLen;
        float _legLen;
        ArmNodes _rightArm;
        ArmNodes _leftArm;

        // Default transform are used to reset the skeleton before each frame update to start from scratch
        std::vector<std::pair<RE::NiAVObject*, const RE::NiTransform>> _skeletonNodesToDefaultTransforms;
        static std::unordered_map<std::string, RE::NiTransform> getSkeletonNodesDefaultTransforms();
        static std::unordered_map<std::string, RE::NiTransform> getSkeletonNodesDefaultTransformsInPA();
        inline static const std::unordered_map<std::string, RE::NiTransform> _skeletonNodesDefaultTransform = getSkeletonNodesDefaultTransforms();
        inline static const std::unordered_map<std::string, RE::NiTransform> _skeletonNodesDefaultTransformInPA = getSkeletonNodesDefaultTransformsInPA();

        // legs walking stuff
        int _walkingState;
        float _currentStepTime;
        RE::NiPoint3 _leftFootPos;
        RE::NiPoint3 _rightFootPos;
        RE::NiPoint3 _rightFootTarget;
        RE::NiPoint3 _leftFootTarget;
        RE::NiPoint3 _rightFootStart;
        RE::NiPoint3 _leftFootStart;
        RE::NiPoint3 _leftKneePosture;
        RE::NiPoint3 _rightKneePosture;
        RE::NiPoint3 _leftKneePos;
        RE::NiPoint3 _rightKneePos;
        int _footStepping;
        RE::NiPoint3 _stepDir;
        float _prevSpeed;
        float _stepTimeinStep;
        int _delayFrame;

        RE::NiTransform _rightHandPrevFrame;
        RE::NiTransform _leftHandPrevFrame;

        WeaponHandRecoil _weaponHandRecoil;

        HandPose _handPose;

        // cull (hide) parts of the skeleton (head, equipment)
        CullGeometryHandler _cullGeometry;

        SelfieHandler _selfieHandler;
    };
}
