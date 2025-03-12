#pragma once

#include "Debug.h"
#include "utils.h"
#include "Skeleton.h"

namespace F4VRBody {

	void positionDiff(Skeleton* skelly) {
		NiPoint3 firstpos = skelly->getPlayerNodes()->HmdNode->m_worldTransform.pos;
		NiPoint3 skellypos = skelly->getRoot()->m_worldTransform.pos;

		_MESSAGE("difference = %f %f %f", (firstpos.x - skellypos.x), (firstpos.y - skellypos.y), (firstpos.z - skellypos.z));
	}

	void printNodes(NiNode* nde) {
		// print root node info first
		_MESSAGE("%s : children = %d hidden: %d: local (%f, %f, %f)", nde->m_name.c_str(), nde->m_children.m_emptyRunStart, (nde->flags & 0x1),
			nde->m_localTransform.pos.x, nde->m_localTransform.pos.y, nde->m_localTransform.pos.z);

		std::string padding = "";

		for (auto i = 0; i < nde->m_children.m_emptyRunStart; ++i) {
			//	auto nextNode = nde->m_children.m_data[i] ? nde->m_children.m_data[i]->GetAsNiNode() : nullptr;
			auto nextNode = nde->m_children.m_data[i];
			if (nextNode) {
				printChildren((NiNode*)nextNode, padding);
			}
		}
	}

	void printChildren(NiNode* child, std::string padding) {
		padding += "....";
		_MESSAGE("%s%s : children = %d hidden: %d: local (%2f, %2f, %2f) world (%2f, %2f, %2f)", padding.c_str(), child->m_name.c_str(), child->m_children.m_emptyRunStart, (child->flags & 0x1),
			child->m_localTransform.pos.x,
			child->m_localTransform.pos.y,
			child->m_localTransform.pos.z,
			child->m_worldTransform.pos.x,
			child->m_worldTransform.pos.y,
			child->m_worldTransform.pos.z);

		//_MESSAGE("%s%s : children = %d : worldbound %f %f %f %f", padding.c_str(), child->m_name.c_str(), child->m_children.m_emptyRunStart,
		//	child->m_worldBound.m_kCenter.x, child->m_worldBound.m_kCenter.y, child->m_worldBound.m_kCenter.z, child->m_worldBound.m_fRadius);

		if (child->GetAsNiNode())
		{
			for (auto i = 0; i < child->m_children.m_emptyRunStart; ++i) {
				//auto nextNode = child->m_children.m_data[i] ? child->m_children.m_data[i]->GetAsNiNode() : nullptr;
				auto nextNode = child->m_children.m_data[i];
				if (nextNode) {
					printChildren((NiNode*)nextNode, padding);
				}
			}
		}
	}

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

	/// <summary>
	/// Dump the player body parts and whatever they are hidden.
	/// </summary>
	void dumpPlayerGeometry(BSFadeNode* rn) {
		for (auto i = 0; i < rn->kGeomArray.count; ++i) {
			auto& geometry = rn->kGeomArray[i].spGeometry;
			auto geometryName = geometry->m_name.c_str();
			_MESSAGE("Player Geometry at %d: '%s' isHidden? %d", i, geometryName, geometry->flags &= 0x1);
		}
	}

	void debug(Skeleton* skelly) {

		static std::uint64_t fc = 0;

		//Offsets::ForceGamePause(*g_menuControls);

		BSFadeNode* rn = static_cast<BSFadeNode*>(skelly->getRoot()->m_parent);
		//_MESSAGE("newrun");

		//for (int i = 0; i < 44; i++) {
		//	if ((*g_player)->equipData->slots[i].item != nullptr) {
		//		std::string name = (*g_player)->equipData->slots[i].item->GetFullName();
		//		auto form_type = (*g_player)->equipData->slots[i].item->GetFormType();
		//		_MESSAGE("%s formType = %d", name.c_str(), form_type);
		//		if (form_type == FormType::kFormType_ARMO) {
		//			auto form = reinterpret_cast<TESObjectARMO*>((*g_player)->equipData->slots[i].item);
		//			auto bipedslot = form->bipedObject.data.parts;
		//			_MESSAGE("biped slot = %d", bipedslot);
		//		}
		//	}
		//}



		//static bool runTimer = false;
		//static auto startTime = std::chrono::high_resolution_clock::now();

		//if (fc > 400) {
		//	fc = 0;
		//	BSFixedString event("reloadStart");
		//	IAnimationGraphManagerHolder_NotifyAnimationGraph(&(*g_player)->animGraphHolder, event);
		//	runTimer = true;
		//	startTime = std::chrono::high_resolution_clock::now();
		//}

		//if (runTimer) {
		//	auto elapsed = std::chrono::high_resolution_clock::now() - startTime;
		//	if (300 < std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count()) {

		//		BSFixedString event("reloadComplete");
		//		IAnimationGraphManagerHolder_NotifyAnimationGraph(&(*g_player)->animGraphHolder, event);
		//		TESObjectREFR_UpdateAnimation((*g_player), 5.0f);

		//		runTimer = false;
		//	}
		//}

		//_MESSAGE("throwing-> %d", Actor_CanThrow(*g_player, g_equipIndex));

		//for (auto i = 0; i < rn->kGeomArray.capacity-1; ++i) {
		//	BSFadeNode::FlattenedGeometryData data = rn->kGeomArray[i];
		//	_MESSAGE("%s", data.spGeometry->m_name.c_str());
		//}

		//_playerNodes->ScopeParentNode->flags &= 0xfffffffffffffffe;
		//updateDown(dynamic_cast<NiNode*>(_playerNodes->ScopeParentNode), true);


	//	BSFixedString name("LArm_Hand");
	////	NiAVObject* node = (*g_player)->firstPersonSkeleton->GetObjectByName(&name);
	//	NiAVObject* node = _root->GetObjectByName(&name);
	//	if (!node) { return; }
	//	_MESSAGE("%d %f %f %f %f %f %f", node->flags & 0xF, node->m_localTransform.pos.x,
	//									 node->m_localTransform.pos.y,
	//									 node->m_localTransform.pos.z,
	//									 node->m_worldTransform.pos.x,
	//									 node->m_worldTransform.pos.y,
	//									 node->m_worldTransform.pos.z
	//									);

	//	for (auto i = 0; i < (*g_player)->inventoryList->items.count; i++) {
		//	_MESSAGE("%d,%d,%x,%x,%s", fc, i, (*g_player)->inventoryList->items[i].form->formID, (*g_player)->inventoryList->items[i].form->formType, (*g_player)->inventoryList->items[i].form->GetFullName());
		//}

		//if (fc < 1) {
		//	tHashSet<ObjectModMiscPair, BGSMod::Attachment::Mod*> *map = g_modAttachmentMap.GetPtr();
		//	map->Dump();
		//}

		BSFlattenedBoneTree* rt = (BSFlattenedBoneTree*)skelly->getRoot();

		//for (auto i = 0; i < rt->numTransforms; i++) {

		//	//if (rt->transforms[i].refNode) {
		//	//	_MESSAGE("%d,%s,%d,%d", fc, rt->transforms[i].refNode->m_name.c_str(), rt->transforms[i].childPos, rt->transforms[i].parPos);
		//	//}
		//	//else {
		//	//	_MESSAGE("%d,%s,%d,%d", fc, "", rt->transforms[i].childPos, rt->transforms[i].parPos);
		//	//}
		//		_MESSAGE("%d,%d,%s", fc, i, rt->transforms[i].name.c_str());
		//}
		//
		//for (auto i = 0; i < rt->numTransforms; i++) {
		//	int pos = rt->bonePositions[i].position;
		//	if (rt->bonePositions[i].name && ((uint64_t)rt->bonePositions[i].name > 0x1000)) {
		//		_MESSAGE("%d,%d,%s", fc, pos, rt->bonePositions[i].name->data);
		//	}
		//	else {
		//		_MESSAGE("%d,%d", fc, pos);
		//	}
		//}

		//	if (rt->bonePositions[i].name && (rt->bonePositions[i].position != 0) && ((uint64_t)rt->bonePositions[i].name > 0x1000)) {
		//		int pos = rt->bonePositions[i].position;
		//		if (pos > rt->numTransforms) {
		//			continue;
		//		}
		//		_MESSAGE("%d,%s,%d,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f", fc, rt->bonePositions[i].name->data,
		//							rt->bonePositions[i].position,
		//							rt->transforms[pos].local.rot.arr[0],
		//							rt->transforms[pos].local.rot.arr[1],
		//							rt->transforms[pos].local.rot.arr[2],
		//							rt->transforms[pos].local.rot.arr[3],
		//							rt->transforms[pos].local.rot.arr[4],
		//							rt->transforms[pos].local.rot.arr[5],
		//							rt->transforms[pos].local.rot.arr[6],
		//							rt->transforms[pos].local.rot.arr[7],
		//							rt->transforms[pos].local.rot.arr[8],
		//							rt->transforms[pos].local.rot.arr[9],
		//							rt->transforms[pos].local.rot.arr[10],
		//							rt->transforms[pos].local.rot.arr[11],
		//							rt->transforms[pos].local.pos.x,
		//							rt->transforms[pos].local.pos.y,
		//							rt->transforms[pos].local.pos.z,
		//							rt->transforms[pos].world.rot.arr[0],
		//							rt->transforms[pos].world.rot.arr[1],
		//							rt->transforms[pos].world.rot.arr[2],
		//							rt->transforms[pos].world.rot.arr[3],
		//							rt->transforms[pos].world.rot.arr[4],
		//							rt->transforms[pos].world.rot.arr[5],
		//							rt->transforms[pos].world.rot.arr[6],
		//							rt->transforms[pos].world.rot.arr[7],
		//							rt->transforms[pos].world.rot.arr[8],
		//							rt->transforms[pos].world.rot.arr[9],
		//							rt->transforms[pos].world.rot.arr[10],
		//							rt->transforms[pos].world.rot.arr[11],
		//							rt->transforms[pos].world.pos.x,
		//							rt->transforms[pos].world.pos.y,
		//							rt->transforms[pos].world.pos.z
		//		);


		//	//	if(strstr(rt->bonePositions[i].name->data, "Finger")) {
		//	//		Matrix44 rot;
		//	//		rot.makeIdentity();
		//	//		rt->transforms[pos].local.rot = rot.make43();

		//	//		if (rt->transforms[pos].refNode) {
		//	//			rt->transforms[pos].refNode->m_localTransform.rot = rot.make43();
		//	//		}

		//	//		rot.makeTransformMatrix(rt->transforms[pos].local.rot, NiPoint3(0, 0, 0));

		//	//		short parent = rt->transforms[pos].parPos;
		//	//		rt->transforms[pos].world.rot = rot.multiply43Left(rt->transforms[parent].world.rot);

		//	//		if (rt->transforms[pos].refNode) {
		//	//			rt->transforms[pos].refNode->m_worldTransform.rot = rt->transforms[pos].world.rot;
		//	//		}

		//	//	}
		//	}

		//	//rt->UpdateWorldBound();
		//}

		fc++;
	}
}