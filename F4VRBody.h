#pragma once
#include "f4se/GameReferences.h"
#include "f4se/NiNodes.h"
#include "include/INIReader.h"

extern uint64_t g_mainLoopCounter;
namespace F4VRBody {

	extern float c_playerHeight;
	extern float c_playerOffset;
	extern float c_pipboyDetectionRange;
	extern float c_armLength;

	bool loadConfig();

	void update();
	void startUp();
}