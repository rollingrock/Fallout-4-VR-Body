#include "F4VRBody.h"
#include "Skeleton.h"
#include "f4se/GameSettings.h"

#define PI 3.14159265358979323846

bool firstTime = true;

namespace F4VRBody {

	Skeleton* playerSkelly = nullptr;

	bool isLoaded = false;

	uint64_t updateCounter = 0;
	uint64_t prevCounter = 0;
	uint64_t localCounter = 0;

	float c_playerHeight = 0.0;
	bool  c_setScale = false;
	float c_fVrScale = 70.0;
	float c_playerOffset_forward = -4.0;
	float c_playerOffset_up = -2.0;
	float c_pipboyDetectionRange = 15.0f;
	float c_armLength = 36.74;

	

	bool loadConfig() {
		INIReader ini(".\\Data\\F4SE\\plugins\\Fallout4VR_Body.ini");

		if (ini.ParseError() < 0) {
			_MESSAGE("ERROR: cannot read Fallout4VR_Body.ini");
			return false;
		}

		c_playerHeight =         ini.GetReal("Fallout4VRBody", "PlayerHeight", 120.4828f);
		c_setScale     =         ini.GetBoolean("Fallout4VRBody", "setScale", false);
		c_fVrScale     =         ini.GetReal("Fallout4VRBody", "fVrScale", 70.0);
		c_playerOffset_forward = ini.GetReal("Fallout4VRBody", "playerOffset_forward", -4.0);
		c_playerOffset_up =      ini.GetReal("Fallout4VRBody", "playerOffset_up", -2.0);
		c_pipboyDetectionRange = ini.GetReal("Fallout4VRBody", "pipboyDetectionRange", 15.0);
		c_armLength =            ini.GetReal("FalloutVRBody", "armLength", 36.74);

		return true;
	}

	bool setSkelly() {
		if ((*g_player)->unkF0 && (*g_player)->unkF0->rootNode) {
			playerSkelly = new Skeleton((BSFadeNode*)(*g_player)->unkF0->rootNode->m_children.m_data[0]->GetAsNiNode());
			_MESSAGE("skeleton = %016I64X", playerSkelly->getRoot());
			playerSkelly->setNodes();
			playerSkelly->setDirection();
		    playerSkelly->swapPipboy();

			if (c_setScale) {
				Setting* set = GetINISetting("fVrScale:VR");
				set->SetDouble(c_fVrScale);
			}

			playerSkelly->setBodyLen();
			_MESSAGE("initialized");
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
//		updateCounter++;
		//_MESSAGE("mainLoopCounter = %d; UpdateCounter = %d", g_mainLoopCounter, updateCounter);
	//	_MESSAGE("start of update");

		if (!isLoaded) {
			return;
		}

		if (!playerSkelly) {
			if (firstTime) {
				if (!setSkelly()) {
					return;
				}
			}

			firstTime = false;
		//	playerSkelly->getRoot()->UpdateDownwardPass(nullptr, 0);   // update BSFlattenedBoneTree render buffer
			return;
			//		playerSkelly->printNodes(playerSkelly->getRoot());

		}

		// do stuff now

		//_MESSAGE("start update");

		// first restore locals to a default state to wipe out any local transform changes the game might have made since last update
		playerSkelly->restoreLocals(playerSkelly->getRoot());
		playerSkelly->updateDown(playerSkelly->getRoot(), true);

		//_MESSAGE("locals saved");

		//playerSkelly->printNodes();

		// moves head up and back out of the player view.   doing this instead of hiding with a small scale setting since it preserves neck shape
		NiNode* headNode = playerSkelly->getNode("Head", playerSkelly->getRoot());
		playerSkelly->setupHead(headNode);

	//	_MESSAGE("head setup");

		//// set up the body underneath the headset in a proper scale and orientation
		playerSkelly->setUnderHMD();
		playerSkelly->updateDown(playerSkelly->getRoot(), true);  // Do world update now so that IK calculations have proper world reference
		//_MESSAGE("body init");

		// Now Set up body Posture and hook up the legs
		playerSkelly->setBodyPosture();
		playerSkelly->updateDown(playerSkelly->getRoot(), true);  // Do world update now so that IK calculations have proper world reference

	//	_MESSAGE("posture");

		playerSkelly->setLegs();

		// Do another update before setting arms
		playerSkelly->updateDown(playerSkelly->getRoot(), true);  // Do world update now so that IK calculations have proper world reference

	//	_MESSAGE("legs");
		// do arm IK - Right then Left
		playerSkelly->setArms(false);
		playerSkelly->setArms(true);
		playerSkelly->updateDown(playerSkelly->getRoot(), true);  // Do world update now so that IK calculations have proper world reference

	//	_MESSAGE("arms");
		// Misc stuff to show/hide things and also setup the wrist pipboy
		playerSkelly->hideWeapon();
	//	_MESSAGE("hide weapon");
		playerSkelly->positionPipboy();
//		_MESSAGE("pip boy position");
		playerSkelly->fixMelee();
//		_MESSAGE("melee");
		playerSkelly->hideWands();
	//	_MESSAGE("wands hidden");
		playerSkelly->hideFistHelpers();
	//	_MESSAGE("helpers hidden");

		playerSkelly->operatePipBoy();
	//	_MESSAGE("pip boy operated");

		playerSkelly->updateDown(playerSkelly->getRoot(), true);  // Last world update before exit.    Probably not necessary.




		// project body out in front of the camera for debug purposes
		//playerSkelly->projectSkelly(120.0f);
		//playerSkelly->updateDown(playerSkelly->getRoot(), true);  // Last world update before exit.    Probably not necessary.
	}


	void startUp() {
		_MESSAGE("Starting up F4Body");
		isLoaded = true;
		return;
	}
}