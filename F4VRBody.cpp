#include "F4VRBody.h"


namespace F4VRBody {
	bool isLoaded = false;
	BSFadeNode* skelly;

	bool setSkelly() {
		if ((*g_player)->unkF0 && (*g_player)->unkF0->rootNode) {
			skelly = (BSFadeNode*)(*g_player)->unkF0->rootNode;	
			_MESSAGE("skeleton = %016I64X", skelly);
			return true;
		}
		else {
			return false;
		}
	}

	void update() {
		if (!isLoaded) {
			return;
		}

		if (!skelly) {
			if (!setSkelly()) {
				return;
			}
		}

		// do stuff now
	}


	void startUp() {
		_MESSAGE("Starting up F4Body");
		PlayerCharacter* pc = *g_player;
		isLoaded = true;

		if (pc) {
			if (pc->unkF0 && pc->unkF0->rootNode) {
				isLoaded = true;
				skelly = (BSFadeNode*)pc->unkF0->rootNode;
			}
		}
		return;
	}
}