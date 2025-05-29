#pragma once

#include <map>

#include "CullGeometryHandler.h"
#include "api/openvr.h"
#include "common/CommonUtils.h"
#include "f4se/GameReferences.h"
#include "f4se/NiNodes.h"
#include "f4vr/PlayerNodes.h"
#include "include/SimpleIni.h"

namespace frik {
	struct ArmNodes {
		NiAVObject* shoulder;
		NiAVObject* upper;
		NiAVObject* upperT1;
		NiAVObject* forearm1;
		NiAVObject* forearm2;
		NiAVObject* forearm3;
		NiAVObject* hand;
	};

	class Skeleton {
	public:
		Skeleton(BSFadeNode* rootNode, const bool inPowerArmor)
			: _root(rootNode), _inPowerArmor(inPowerArmor) {
			_curentPosition = NiPoint3(0, 0, 0);
			_walkingState = 0;
			initializeNodes();
		}

		ArmNodes getLeftArm() const {
			return _leftArm;
		}

		ArmNodes getRightArm() const {
			return _rightArm;
		}

		NiTransform getBoneWorldTransform(const std::string& boneName);

		NiPoint3 getOffhandIndexFingerTipWorldPosition();

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
		void dampenHand(NiNode* node, bool isLeft);
		void leftHandedModePipboy() const;
		void hide3rdPersonWeapon() const;
		void positionPipboy() const;
		void hidePipboy() const;
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
		BSFadeNode* _root;
		bool _inPowerArmor;

		// ???
		LARGE_INTEGER _freqCounter;
		LARGE_INTEGER _timer;
		LARGE_INTEGER _prevTime;
		double _frameTime;

		// handle switch of hands for left-handed mode
		bool _lastLeftHandedModeSwitch = false;

		// Camera positions
		NiPoint3 _curentPosition;
		NiPoint3 _lastPosition;

		// ???
		NiPoint3 _forwardDir;
		NiPoint3 _sidewaysRDir;
		NiPoint3 _upDir;

		// skeleton nodes
		f4vr::PlayerNodes* _playerNodes;
		NiNode* _rightHand;
		NiNode* _leftHand;
		NiNode* _head;
		NiNode* _spine;
		NiNode* _chest;
		float _torsoLen;
		float _legLen;
		ArmNodes _rightArm;
		ArmNodes _leftArm;

		// Default transform are used to reset the skeleton before each frame update to start from scratch
		std::vector<std::pair<NiAVObject*, const NiTransform>> _skeletonNodesToDefaultTransforms;
		static std::unordered_map<std::string, NiTransform> getSkeletonNodesDefaultTransforms();
		static std::unordered_map<std::string, NiTransform> getSkeletonNodesDefaultTransformsInPA();
		inline static const std::unordered_map<std::string, NiTransform> _skeletonNodesDefaultTransform = getSkeletonNodesDefaultTransforms();
		inline static const std::unordered_map<std::string, NiTransform> _skeletonNodesDefaultTransformInPA = getSkeletonNodesDefaultTransformsInPA();

		// legs walking stuff
		int _walkingState;
		double _currentStepTime;
		NiPoint3 _leftFootPos;
		NiPoint3 _rightFootPos;
		NiPoint3 _rightFootTarget;
		NiPoint3 _leftFootTarget;
		NiPoint3 _rightFootStart;
		NiPoint3 _leftFootStart;
		NiPoint3 _leftKneePosture;
		NiPoint3 _rightKneePosture;
		NiPoint3 _leftKneePos;
		NiPoint3 _rightKneePos;
		int _footStepping;
		NiPoint3 _stepDir;
		double _prevSpeed;
		double _stepTimeinStep;
		int _delayFrame;

		std::map<std::string, NiTransform, common::CaseInsensitiveComparator> _handBones;
		std::map<std::string, bool, common::CaseInsensitiveComparator> _closedHand;
		static std::unordered_map<std::string, vr::EVRButtonId> getHandBonesButtonMap();
		inline static const std::unordered_map<std::string, vr::EVRButtonId> _handBonesButton = getHandBonesButtonMap();

		NiTransform _rightHandPrevFrame;
		NiTransform _leftHandPrevFrame;

		// bones
		std::map<std::string, int> _boneTreeMap;
		std::vector<std::string> _boneTreeVec;
		static std::map<std::string, std::pair<std::string, std::string>> makeFingerRelations();
		inline static const std::map<std::string, std::pair<std::string, std::string>> _fingerRelations = makeFingerRelations();

		// cull (hide) parts of the skeleton (head, equipment)
		CullGeometryHandler _cullGeometry;
	};
}
