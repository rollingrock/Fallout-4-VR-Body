#pragma once

#include <map>
#include <openvr.h>

#include "CullGeometryHandler.h"
#include "common/CommonUtils.h"
#include "f4vr/PlayerNodes.h"

namespace frik
{
    struct ArmNodes
    {
        RE::NiAVObject* shoulder;
        RE::NiAVObject* upper;
        RE::NiAVObject* upperT1;
        RE::NiAVObject* forearm1;
        RE::NiAVObject* forearm2;
        RE::NiAVObject* forearm3;
        RE::NiAVObject* hand;
    };

    class Skeleton
    {
    public:
        Skeleton(RE::NiNode* rootNode, const bool inPowerArmor) :
            _root(rootNode), _inPowerArmor(inPowerArmor)
        {
            _curentPosition = RE::NiPoint3(0, 0, 0);
            _walkingState = 0;
            initializeNodes();
        }

        ArmNodes getLeftArm() const
        {
            return _leftArm;
        }

        ArmNodes getRightArm() const
        {
            return _rightArm;
        }

        RE::NiTransform getBoneWorldTransform(const std::string& boneName);

        RE::NiPoint3 getIndexFingerTipWorldPosition(bool primaryHand);

        void onFrameUpdate();

    private:
        // initialization
        void initializeNodes();
        void initArmsNodes();
        void initSkeletonNodesDefaults();
        void initBoneTreeMap();
        void setBodyLen();

        // on frame update - skeleton update
        void setTime();
        void restoreNodesToDefault();
        void setupHead() const;
        void setBodyUnderHMD();
        void setBodyPosture();
        void setKneePos();
        void walk();
        void setSingleLeg(bool isLeft) const;
        void handleLeftHandedWeaponNodesSwitch();
        void setArms(bool isLeft);
        void dampenHand(RE::NiNode* node, bool isLeft);
        void hide3rdPersonWeapon() const;
        void hideFistHelpers() const;
        void showHidePAHud() const;
        void selfieSkelly() const;
        void showOnlyArms() const;
        void setHandPose();
        void hideHands() const;
        void fixArmor() const;

        // Utils
        void calculateHandPose(const std::string& bone, float gripProx, bool thumbUp, bool isLeft);
        void copy1StPerson(const std::string& bone);

        // Utils - Body Positioning
        float getNeckYaw() const;
        float getNeckPitch() const;
        float getBodyPitch() const;
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

        std::map<std::string, RE::NiTransform, common::CaseInsensitiveComparator> _handBones;
        std::map<std::string, bool, common::CaseInsensitiveComparator> _closedHand;
        static std::unordered_map<std::string, vr::EVRButtonId> getHandBonesButtonMap();
        inline static const std::unordered_map<std::string, vr::EVRButtonId> _handBonesButton = getHandBonesButtonMap();

        RE::NiTransform _rightHandPrevFrame;
        RE::NiTransform _leftHandPrevFrame;

        // bones
        std::map<std::string, int> _boneTreeMap;
        std::vector<std::string> _boneTreeVec;
        static std::map<std::string, std::pair<std::string, std::string>> makeFingerRelations();
        inline static const std::map<std::string, std::pair<std::string, std::string>> _fingerRelations = makeFingerRelations();

        // cull (hide) parts of the skeleton (head, equipment)
        CullGeometryHandler _cullGeometry;
    };
}
