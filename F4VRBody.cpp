#include "F4VRBody.h"
#include "Skeleton.h"

#define PI 3.14159265358979323846

namespace F4VRBody {

	Skeleton* playerSkelly = nullptr;

	bool isLoaded = false;

	uint64_t updateCounter = 0;
	uint64_t prevCounter = 0;
	uint64_t localCounter = 0;

	bool setSkelly() {
		if ((*g_player)->unkF0 && (*g_player)->unkF0->rootNode) {
			playerSkelly = new Skeleton((BSFadeNode*)(*g_player)->unkF0->rootNode->m_children.m_data[0]->GetAsNiNode());
			_MESSAGE("skeleton = %016I64X", playerSkelly->getRoot());
			playerSkelly->setNodes();
			playerSkelly->setDirection();
			return true;
		}
		else {
			return false;
		}
	}

	void update() {
		//if (prevCounter != g_mainLoopCounter) {
		//	prevCounter = g_mainLoopCounter;
		//	localCounter = 0;
		//	return;
		//}
		//else if (localCounter != 3) {
		//	localCounter++;
		//	return;
		//}

	//	localCounter++;
		updateCounter++;
		//_MESSAGE("mainLoopCounter = %d; UpdateCounter = %d", g_mainLoopCounter, updateCounter);

		if (!isLoaded) {
			return;
		}

		if (!playerSkelly) {
			if (!setSkelly()) {
				return;
			}
			//		playerSkelly->printNodes(playerSkelly->getRoot());

		}

		// do stuff now

		//playerSkelly->updateZ((NiNode*)playerSkelly->getRoot());

		//playerSkelly->printNodes();

	//	playerSkelly->projectSkelly(120.0f);   // project out in front of the players view by 30 units

		NiNode* headNode = playerSkelly->getNode("Head", playerSkelly->getRoot());

		playerSkelly->setupHead(headNode);

		playerSkelly->setUnderHMD();
	//	playerSkelly->setHandPos();
	//	playerSkelly->removeHands();

		// do arm IK - Right then Left
		playerSkelly->setArms(false);
//		playerSkelly->setArms(true);


		playerSkelly->getRoot()->UpdateDownwardPass(nullptr, 0);   // update BSFlattenedBoneTree render buffer
	}


	void startUp() {
		_MESSAGE("Starting up F4Body");
		isLoaded = true;
		return;
	}
}