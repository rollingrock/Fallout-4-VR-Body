#include "F4VRBody.h"
#include "Skeleton.h"

#define PI 3.14159265358979323846



namespace F4VRBody {

	Skeleton *playerSkelly = nullptr;
	
	bool isLoaded = false;

	bool setSkelly() {
		if ((*g_player)->unkF0 && (*g_player)->unkF0->rootNode) {
			playerSkelly = new Skeleton((BSFadeNode*)(*g_player)->unkF0->rootNode);
			_MESSAGE("skeleton = %016I64X", playerSkelly->getRoot());
			return true;
		}
		else {
			return false;
		}
	}

	void update(uint64_t a_addr) {
		if (!isLoaded) {
			return;
		}

		if (!playerSkelly) {
			if (!setSkelly()) {
				return;
			}
			playerSkelly->printNodes();
			
		}

		// do stuff now

		//playerSkelly->updateZ((NiNode*)playerSkelly->getRoot());

		//playerSkelly->printNodes();

	//	playerSkelly->projectSkelly(30.0f);   // project out in front of the players view by 30 units

		playerSkelly->rotateWorld(playerSkelly->getRoot());
	}


	void startUp() {
		_MESSAGE("Starting up F4Body");
		isLoaded = true;
		return;
	}
}