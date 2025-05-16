// From Shizof mod with permission.  Thanks Shizof!!

#include <atomic>
#include <deque>
#include <thread>
#include <f4se/GameRTTI.h>

#include "Config.h"
#include "F4VRBody.h"
#include "MenuChecker.h"
#include "SmoothMovementVR.h"
#include "common/Logger.h"
#include "f4se/NiExtraData.h"

using namespace common;

namespace SmoothMovementVR {
	std::atomic inPowerArmorFrame = false;
	std::atomic interiorCell = false;

	std::atomic usePapyrusDefaultHeight = false;

	std::atomic<float> smoothedX = 0;
	std::atomic<float> smoothedY = 0;
	std::atomic<float> smoothedZ = 0;

	LARGE_INTEGER m_hpcFrequency;
	LARGE_INTEGER m_prevTime;
	std::atomic<float> m_frameTime = 0;

	std::atomic notMoving = false;

	std::deque<NiPoint3> lastPositions;

	float defaultHeight = 0.0f;
	float powerArmorHeight = 0.0f;
	float papyrusDefaultHeight = 0.0f;

	auto distanceNoSqrt(const NiPoint3 po1, const NiPoint3 po2) -> float {
		const float x = po1.x - po2.x;
		const float y = po1.y - po2.y;
		const float z = po1.z - po2.z;
		return x * x + y * y + z * z;
	}

	static float distanceNoSqrt2d(const float x1, const float y1, const float x2, const float y2) {
		const float x = x1 - x2;
		const float y = y1 - y2;
		return x * x + y * y;
	}

	auto smoothedValue(const NiPoint3 newPosition) -> NiPoint3 {
		LARGE_INTEGER newTime;
		QueryPerformanceCounter(&newTime);
		m_frameTime.store(static_cast<float>(newTime.QuadPart - m_prevTime.QuadPart) / m_hpcFrequency.QuadPart);
		if (m_frameTime.load() > 0.05) {
			m_frameTime.store(0.05f);
		}
		m_prevTime = newTime;

		if (f4vr::IsInAir(*g_player)) {
			smoothedX.store(newPosition.x);
			smoothedY.store(newPosition.y);
			smoothedZ.store(newPosition.z);
		} else if (distanceNoSqrt(newPosition, NiPoint3(smoothedX.load(), smoothedY.load(), smoothedZ.load())) > 4000000.0f) {
			smoothedX.store(newPosition.x);
			smoothedY.store(newPosition.y);
			smoothedZ.store(newPosition.z);
		} else {
			if (frik::g_config.disableInteriorSmoothingHorizontal && interiorCell.load()) {
				smoothedX.store(newPosition.x);
				smoothedY.store(newPosition.y);
			} else {
				if (frik::g_config.dampingMultiplierHorizontal != 0 && frik::g_config.smoothingAmountHorizontal != 0) {
					const float absValX = min(50, max(0.1f, abs(newPosition.x - smoothedX.load())));
					const float absValY = min(50, max(0.1f, abs(newPosition.y - smoothedY.load())));
					smoothedX.store(
						smoothedX.load() + m_frameTime.load() * ((newPosition.x - smoothedX.load()) / (frik::g_config.smoothingAmountHorizontal * (frik::g_config.
							dampingMultiplierHorizontal / absValX) * (notMoving.load() ? frik::g_config.stoppingMultiplierHorizontal : 1.0f))));
					smoothedY.store(
						smoothedY.load() + m_frameTime.load() * ((newPosition.y - smoothedY.load()) / (frik::g_config.smoothingAmountHorizontal * (frik::g_config.
							dampingMultiplierHorizontal / absValY) * (notMoving.load() ? frik::g_config.stoppingMultiplierHorizontal : 1.0f))));
				} else {
					smoothedX.store(newPosition.x);
					smoothedY.store(newPosition.y);
				}
			}

			if (frik::g_config.disableInteriorSmoothing && interiorCell.load()) {
				smoothedZ.store(newPosition.z);
			} else {
				const float absVal = min(50, max(0.1f, abs(newPosition.z - smoothedZ.load())));
				smoothedZ.store(
					smoothedZ.load() + m_frameTime.load() * ((newPosition.z - smoothedZ.load()) / (frik::g_config.smoothingAmount * (frik::g_config.dampingMultiplier / absVal) *
						(notMoving.load() ? frik::g_config.stoppingMultiplier : 1.0f))));
			}
		}

		//LOG("CurrentPos: %g %g %g - Smoothed: %g %g %g", newPosition.x, newPosition.y, newPosition.z, smoothedX.load(), smoothedY.load(), smoothedZ.load());

		return NiPoint3(smoothedX.load(), smoothedY.load(), smoothedZ.load());
	}

	bool first = true;

	float oldX2;
	float oldY2;

	float oldX;
	float oldY;

	NiNode* getWorldRoot() {
		NiNode* node = (*g_player)->unkF0->rootNode;
		while (node && node->m_parent) {
			node = node->m_parent;
		}
		return node;
	}

	std::atomic<float> lastAppliedLocalX;
	std::atomic<float> lastAppliedLocalY;

	void everyFrame() {
		if (*g_player && (*g_player)->unkF0 && (*g_player)->unkF0->rootNode) {
			if (*g_player && (*g_player)->unkF0 && (*g_player)->unkF0->rootNode) {
				BSFixedString playerWorld("PlayerWorldNode");
				BSFixedString hmd("HmdNode");
				BSFixedString room("RoomNode");
				if (const auto worldNiNode = getWorldRoot()) {
					if (NiAVObject* worldNiAV = worldNiNode) {
						NiAVObject* playerWorldNode = CALL_MEMBER_FN(worldNiAV, GetAVObjectByName)(&playerWorld, true, true);
						const NiAVObject* hmdNode = CALL_MEMBER_FN(worldNiAV, GetAVObjectByName)(&hmd, true, true);
						//NiAVObject * roomNode = CALL_MEMBER_FN(worldNiAV, GetAVObjectByName)(&room, true, true);

						if (playerWorldNode && hmdNode) {
							const NiPoint3 curPos = (*g_player)->pos;

							if (first && curPos.z != 0) {
								smoothedX.store(curPos.x);
								smoothedY.store(curPos.y);
								smoothedZ.store(curPos.z);
								first = false;
							}
							if (lastPositions.size() >= 4) {
								const NiPoint3 pos = lastPositions.at(0);
								bool same = true;
								for (unsigned int i = 1; i < lastPositions.size(); i++) {
									if (!(lastPositions.at(i).x == pos.x && lastPositions.at(i).y == pos.y)) {
										same = false;
										break;
									}
								}
								if (same) {
									notMoving.store(true);
								} else {
									notMoving.store(false);
								}
							} else {
								notMoving.store(false);
							}

							lastPositions.emplace_back((*g_player)->pos);
							if (lastPositions.size() > 5) {
								lastPositions.pop_front();
							}

							NiPoint3 newPos = smoothedValue(curPos);

							if (notMoving.load() && distanceNoSqrt2d(newPos.x - curPos.x, newPos.y - curPos.y, lastAppliedLocalX.load(), lastAppliedLocalY.load()) > 100) {
								newPos.x = curPos.x;
								newPos.y = curPos.y;
								newPos.z = curPos.z;
								smoothedX.store(newPos.x);
								smoothedY.store(newPos.y);
								playerWorldNode->m_localTransform.pos.z = newPos.z - curPos.z;
							} else {
								playerWorldNode->m_localTransform.pos.x = newPos.x - curPos.x;
								playerWorldNode->m_localTransform.pos.y = newPos.y - curPos.y;
								playerWorldNode->m_localTransform.pos.z = newPos.z - curPos.z;

								lastAppliedLocalX.store(playerWorldNode->m_localTransform.pos.x);
								lastAppliedLocalY.store(playerWorldNode->m_localTransform.pos.y);
							}
							//Log::info("playerPos: %g %g %g  --newPos:  %g %g %g  --appliedLocal: %g %g %g", curPos.x, curPos.y, curPos.z, newPos.x, newPos.y, newPos.z, playerWorldNode->m_localTransform.pos.x, playerWorldNode->m_localTransform.pos.y, playerWorldNode->m_localTransform.pos.z);

							//	Log::info("playerWorldNode: %g %g %g", playerWorldNode->m_localTransform.pos.x, playerWorldNode->m_localTransform.pos.y, playerWorldNode->m_localTransform.pos.z);

							playerWorldNode->m_localTransform.pos.z += inPowerArmorFrame.load()
								? frik::g_config.PACameraHeight + frik::g_config.cameraHeight + frik::c_dynamicCameraHeight
								: frik::g_config.cameraHeight + frik::c_dynamicCameraHeight;
						} else {
							Log::info("Cannot get PlayerWorldNode...");
						}
					} else {
						Log::info("Cannot get worldNiAV...");
					}
				} else {
					Log::info("Cannot get worldNiNode...");
				}
			}
		}
	}

	bool firstRun = true;

	static void armorCheck() {
		while (true) {
			if (!*g_player || !(*g_player)->unkF0) {
				Sleep(5000);
				continue;
			}

			if (isGameStoppedNoDialogue()) {
				Sleep(3000);
			} else {
				/*if(firstRun)
				{
					Setting * scale = GetINISetting("fVrHMDMovementThreshold:VR");
					if (scale)
					{
						scale->SetDouble(100);
					}
					firstRun = false;
				}
				*/
				if (const TESObjectCELL* cell = (*g_player)->parentCell) {
					if ((cell->flags & TESObjectCELL::kFlag_IsInterior) == TESObjectCELL::kFlag_IsInterior) //Interior cell
					{
						if (!interiorCell.load()) {
							interiorCell.store(true);
						}
					} else {
						if (interiorCell.load()) {
							interiorCell.store(false);
						}
					}
				}
				// TODO: refactor code to use isInPowerArmor() common code
				if ((*g_player)->equipData) {
					if ((*g_player)->equipData->slots[0x03].item == nullptr) {} else {
						if (TESForm* equippedForm = (*g_player)->equipData->slots[0x03].item) {
							if (equippedForm->formType == TESObjectARMO::kTypeID) {
								if (const auto armor = DYNAMIC_CAST(equippedForm, TESForm, TESObjectARMO)) {
									if (f4vr::hasKeyword(armor, f4vr::KeywordPowerArmor) || f4vr::hasKeyword(armor, f4vr::KeywordPowerArmorFrame)) {
										if (!inPowerArmorFrame.load()) {
											inPowerArmorFrame.store(true);
										}
									} else {
										if (inPowerArmorFrame.load()) {
											inPowerArmorFrame.store(false);
										}
									}
								}
							}
						}
					}
				}

				Sleep(500);
			}
		}
	}

	void startFunctions() {
		QueryPerformanceFrequency(&m_hpcFrequency);
		QueryPerformanceCounter(&m_prevTime);

		Log::info("Starting armor thread");

		std::thread t6(armorCheck);
		t6.detach();

		Log::info("Armor Thread Started");
	}
}
