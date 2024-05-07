// From Shizof's mod with permission.  Thanks Shizof!!

#include "SmoothMovementVR.h"
#include "f4se/NiExtraData.h"
#include "F4VRBody.h"
#include "utils.h"
#include <atomic>
#include <deque>

extern float smoothingAmount;
extern float smoothingAmountHorizontal;
extern float dampingMultiplier;
extern float dampingMultiplierHorizontal;
extern float stoppingMultiplier;
extern float stoppingMultiplierHorizontal;
extern int disableInteriorSmoothing;
extern int disableInteriorSmoothingHorizontal;

namespace SmoothMovementVR
{
	RelocAddr   <_IsInAir>                      IsInAir(0x00DC3230);

	//////////////////////////////////////////////////

	UInt32 KeywordPowerArmor = 0x4D8A1;
	UInt32 KeywordPowerArmorFrame = 0x15503F;

	std::atomic<bool> inPowerArmorFrame = false;
	std::atomic<bool> interiorCell = false;

	std::atomic<bool> usePapyrusDefaultHeight = false;


	std::atomic<float> smoothedX = 0;
	std::atomic<float> smoothedY = 0;
	std::atomic<float> smoothedZ = 0;

	LARGE_INTEGER m_hpcFrequency;
	LARGE_INTEGER m_prevTime;
	std::atomic<float> m_frameTime = 0;

	std::atomic<bool> notMoving = false;

	std::deque<NiPoint3> lastPositions;

	float defaultHeight = 0.0f;
	float powerArmorHeight = 0.0f;
	float papyrusDefaultHeight = 0.0f;

	bool checkIfJumpingOrInAir() {
		return IsInAir(*g_player);
	}

	float distanceNoSqrt(NiPoint3 po1, NiPoint3 po2)
	{
		const float x = po1.x - po2.x;
		const float y = po1.y - po2.y;
		const float z = po1.z - po2.z;
		return x * x + y * y + z * z;
	}

	float distanceNoSqrt2d(float x1, float y1, float x2, float y2)
	{
		const float x = x1 - x2;
		const float y = y1 - y2;
		return x * x + y * y;
	}

	NiPoint3 smoothedValue(NiPoint3 newPosition)
	{
		LARGE_INTEGER newTime;
		QueryPerformanceCounter(&newTime);
		m_frameTime.store((float)(newTime.QuadPart - m_prevTime.QuadPart) / m_hpcFrequency.QuadPart);
		if (m_frameTime.load() > 0.05)
		{
			m_frameTime.store(0.05);
		}
		m_prevTime = newTime;

		if (IsInAir(*g_player))
		{
			smoothedX.store(newPosition.x);
			smoothedY.store(newPosition.y);
			smoothedZ.store(newPosition.z);
		}
		else if (distanceNoSqrt(newPosition, NiPoint3(smoothedX.load(), smoothedY.load(), smoothedZ.load())) > 4000000.0f)
		{
			smoothedX.store(newPosition.x);
			smoothedY.store(newPosition.y);
			smoothedZ.store(newPosition.z);
		}
		else
		{
			if (disableInteriorSmoothingHorizontal && interiorCell.load())
			{
				smoothedX.store(newPosition.x);
				smoothedY.store(newPosition.y);
			}
			else
			{
				if (dampingMultiplierHorizontal != 0 && smoothingAmountHorizontal != 0)
				{
					float absValX = abs(newPosition.x - smoothedX.load());
					if (absValX < 0.1f)
					{
						absValX = 0.1f;
					}
					float absValY = abs(newPosition.y - smoothedY.load());
					if (absValY < 0.1f)
					{
						absValY = 0.1f;
					}
					smoothedX.store(smoothedX.load() + m_frameTime.load() * ((newPosition.x - smoothedX.load()) / (smoothingAmountHorizontal * (dampingMultiplierHorizontal / absValX) * (notMoving.load() ? (stoppingMultiplierHorizontal) : 1.0f))));
					smoothedY.store(smoothedY.load() + m_frameTime.load() * ((newPosition.y - smoothedY.load()) / (smoothingAmountHorizontal * (dampingMultiplierHorizontal / absValY) * (notMoving.load() ? (stoppingMultiplierHorizontal) : 1.0f))));
				}
				else
				{
					smoothedX.store(newPosition.x);
					smoothedY.store(newPosition.y);
				}
			}

			if (disableInteriorSmoothing && interiorCell.load())
			{
				smoothedZ.store(newPosition.z);
			}
			else
			{
				float absVal = abs(newPosition.z - smoothedZ.load());
				if (absVal < 0.1f)
				{
					absVal = 0.1f;
				}
				smoothedZ.store(smoothedZ.load() + m_frameTime.load() * ((newPosition.z - smoothedZ.load()) / (smoothingAmount * (dampingMultiplier / absVal) * (notMoving.load() ? (stoppingMultiplier) : 1.0f))));
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

	NiNode* getWorldRoot()
	{
		NiNode* node = (*g_player)->unkF0->rootNode;
		while (node && node->m_parent)
		{
			node = node->m_parent;
		}
		return node;
	}

	std::atomic<float> lastAppliedLocalX;
	std::atomic<float> lastAppliedLocalY;

	void everyFrame()
	{
		if ((*g_player) && (*g_player)->unkF0 && (*g_player)->unkF0->rootNode)
		{
			if ((*g_player) && (*g_player)->unkF0 && (*g_player)->unkF0->rootNode)
			{
				BSFixedString playerWorld("PlayerWorldNode");
				BSFixedString Hmd("HmdNode");
				BSFixedString room("RoomNode");
				NiNode* worldNiNode = getWorldRoot();
				if (worldNiNode)
				{
					NiAVObject* worldNiAV = worldNiNode;

					if (worldNiAV)
					{
						NiAVObject* playerWorldNode = CALL_MEMBER_FN(worldNiAV, GetAVObjectByName)(&playerWorld, true, true);
						NiAVObject* hmdNode = CALL_MEMBER_FN(worldNiAV, GetAVObjectByName)(&Hmd, true, true);
						//NiAVObject * roomNode = CALL_MEMBER_FN(worldNiAV, GetAVObjectByName)(&room, true, true);

						if (playerWorldNode && hmdNode)
						{
							const NiPoint3 curPos = (*g_player)->pos;

							if (first && curPos.z != 0)
							{
								smoothedX.store(curPos.x);
								smoothedY.store(curPos.y);
								smoothedZ.store(curPos.z);
								first = false;
							}
							if (lastPositions.size() >= 4)
							{
								NiPoint3 pos = lastPositions.at(0);
								bool same = true;
								for (unsigned int i = 1; i < lastPositions.size(); i++)
								{
									if (!(lastPositions.at(i).x == pos.x && lastPositions.at(i).y == pos.y))
									{
										same = false;
										break;
									}
								}
								if (same)
								{
									notMoving.store(true);
								}
								else
								{
									notMoving.store(false);
								}
							}
							else
							{
								notMoving.store(false);
							}

							lastPositions.emplace_back((*g_player)->pos);
							if (lastPositions.size() > 5)
							{
								lastPositions.pop_front();
							}

							NiPoint3 newPos = smoothedValue(curPos);

							if (notMoving.load() && distanceNoSqrt2d(newPos.x - curPos.x, newPos.y - curPos.y, lastAppliedLocalX.load(), lastAppliedLocalY.load()) > 100)
							{
								newPos.x = curPos.x;
								newPos.y = curPos.y;
								newPos.z = curPos.z;
								smoothedX.store(newPos.x);
								smoothedY.store(newPos.y);
								playerWorldNode->m_localTransform.pos.z = newPos.z - curPos.z;
							}
							else
							{
								playerWorldNode->m_localTransform.pos.x = newPos.x - curPos.x;
								playerWorldNode->m_localTransform.pos.y = newPos.y - curPos.y;
								playerWorldNode->m_localTransform.pos.z = newPos.z - curPos.z;

								lastAppliedLocalX.store(playerWorldNode->m_localTransform.pos.x);
								lastAppliedLocalY.store(playerWorldNode->m_localTransform.pos.y);
							}
							//_MESSAGE("playerPos: %g %g %g  --newPos:  %g %g %g  --appliedLocal: %g %g %g", curPos.x, curPos.y, curPos.z, newPos.x, newPos.y, newPos.z, playerWorldNode->m_localTransform.pos.x, playerWorldNode->m_localTransform.pos.y, playerWorldNode->m_localTransform.pos.z);

						//	_MESSAGE("playerWorldNode: %g %g %g", playerWorldNode->m_localTransform.pos.x, playerWorldNode->m_localTransform.pos.y, playerWorldNode->m_localTransform.pos.z);

								playerWorldNode->m_localTransform.pos.z += inPowerArmorFrame.load() ? (F4VRBody::c_PACameraHeight + F4VRBody::c_cameraHeight) : F4VRBody::c_cameraHeight;
								F4VRBody::updateTransformsDown((NiNode*)playerWorldNode, true);
						}
						else
						{
							_MESSAGE("Cannot get PlayerWorldNode...");
						}
					}
					else
					{
						_MESSAGE("Cannot get worldNiAV...");
					}
				}
				else
				{
					_MESSAGE("Cannot get worldNiNode...");
				}
			}
		}
	}

	bool HasKeyword(TESObjectARMO* armor, UInt32 keywordFormId)
	{
		if (armor)
		{
			for (UInt32 i = 0; i < armor->keywordForm.numKeywords; i++)
			{
				if (armor->keywordForm.keywords[i])
				{
					if (armor->keywordForm.keywords[i]->formID == keywordFormId)
					{
						return true;
					}
				}
			}
		}
		return false;
	}

	bool firstRun = true;

	void ArmorCheck()
	{
		while (true)
		{
			if (!(*g_player) || !(*g_player)->unkF0)
			{
				Sleep(5000);
				continue;
			}

			if (isGameStoppedNoDialogue())
			{
				Sleep(3000);
			}
			else
			{
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
				TESObjectCELL* cell = (*g_player)->parentCell;
				if (cell)
				{
					if ((cell->flags & TESObjectCELL::kFlag_IsInterior) == TESObjectCELL::kFlag_IsInterior) //Interior cell
					{
						if (!interiorCell.load())
						{
							interiorCell.store(true);
						}
					}
					else
					{
						if (interiorCell.load())
						{
							interiorCell.store(false);
						}
					}
				}
				if ((*g_player)->equipData)
				{
					if ((*g_player)->equipData->slots[0x03].item != nullptr)
					{
						TESForm* equippedForm = (*g_player)->equipData->slots[0x03].item;
						if (equippedForm)
						{
							if (equippedForm->formType == TESObjectARMO::kTypeID)
							{
								TESObjectARMO* armor = DYNAMIC_CAST(equippedForm, TESForm, TESObjectARMO);

								if (armor)
								{
									if (HasKeyword(armor, KeywordPowerArmor) || HasKeyword(armor, KeywordPowerArmorFrame))
									{
										if (!inPowerArmorFrame.load())
										{
											inPowerArmorFrame.store(true);
										}
									}
									else
									{
										if (inPowerArmorFrame.load())
										{
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

	void StartFunctions()
	{
		QueryPerformanceFrequency(&m_hpcFrequency);
		QueryPerformanceCounter(&m_prevTime);


		_MESSAGE("Starting armor thread");

		std::thread t6(ArmorCheck);
		t6.detach();

		_MESSAGE("Armor Thread Started");
	}

}
