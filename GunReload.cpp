#include "GunReload.h"
#include "F4VRBody.h"
#include "VR.h"

namespace F4VRBody {
	GunReload* g_gunReloadSystem = nullptr;

	float g_animDeltaTime = -1.0f;


	void printNodes(NiNode* nde, long long curTime) {
		_MESSAGE("%d %s : children = %d %d: local %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f", curTime, nde->m_name.c_str(), nde->m_children.m_emptyRunStart, (nde->flags & 0x1),
			nde->m_localTransform.rot.arr[0],
			nde->m_localTransform.rot.arr[1],
			nde->m_localTransform.rot.arr[2],
			nde->m_localTransform.rot.arr[3],
			nde->m_localTransform.rot.arr[4],
			nde->m_localTransform.rot.arr[5],
			nde->m_localTransform.rot.arr[6],
			nde->m_localTransform.rot.arr[7],
			nde->m_localTransform.rot.arr[8],
			nde->m_localTransform.rot.arr[9],
			nde->m_localTransform.rot.arr[10],
			nde->m_localTransform.rot.arr[11],
			nde->m_localTransform.pos.x, nde->m_localTransform.pos.y, nde->m_localTransform.pos.z);


		if (nde->GetAsNiNode()) {
			for (auto i = 0; i < nde->m_children.m_emptyRunStart; ++i) {
				auto nextNode = nde->m_children.m_data[i] ? nde->m_children.m_data[i]->GetAsNiNode() : nullptr;
				if (nextNode) {
					printNodes((NiNode*)nextNode, curTime);
				}
			}
		}
	}

	void GunReload::DoAnimationCapture() {
		if (!startAnimCap) {
			g_animDeltaTime = -1.0f;
			return;
		}

		auto elapsed = since(startCapTime).count();
		if (elapsed > 300) {
			if (elapsed > 2000) {
				g_animDeltaTime = -1.0f;
				Offsets::TESObjectREFR_UpdateAnimation(*g_player, 0.08f);
			}
			g_animDeltaTime = 0.0f;
		}

		NiNode* weap = getChildNode("Weapon", (*g_player)->firstPersonSkeleton);
	//	printNodes(weap, elapsed);
	}

	bool GunReload::startReloading() {
		NiNode* offhand = c_leftHandedMode ? getChildNode("LArm_Finger21", (*g_player)->unkF0->rootNode) : getChildNode("RArm_Finger21", (*g_player)->unkF0->rootNode);
		NiNode* bolt = getChildNode("WeaponBolt", (*g_player)->firstPersonSkeleton);

		if ((bolt == nullptr) || (offhand == nullptr)) {
			return false;
		}

		float dist = abs(vec3_len(offhand->m_worldTransform.pos - bolt->m_worldTransform.pos));

		uint64_t handInput = c_leftHandedMode ? VRHook::g_vrHook->getControllerState(VRHook::VRSystem::TrackerType::Right).ulButtonPressed : VRHook::g_vrHook->getControllerState(VRHook::VRSystem::TrackerType::Left).ulButtonPressed;

		return false;
	}

	void GunReload::Update() {

		switch (state) {
		case idle: 
			if (startReloading()) {
				state = reloadingStart;
			}
			break;
		
		case reloadingStart: 
			break;
		
		case newMagReady: 
			break;
		
		case magInserted: 
			break;

		default:
			state = idle;
			break;
		}
	}
}
