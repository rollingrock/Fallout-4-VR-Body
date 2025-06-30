#include <sstream>

#include "Debug.h"

#include <f4se/ScaleformValue.h>

#include "Skeleton.h"
#include "utils.h"
#include "common/CommonUtils.h"
#include "common/Logger.h"
#include "common/Matrix.h"

using namespace common;

namespace {
	void printNode(const NiNode* node, const std::string& padding) {
		const auto scale = std::fabs(node->m_localTransform.scale - node->m_worldTransform.scale) < 0.001f
			? std::format("{:.2f}", node->m_localTransform.scale)
			: std::format("{:.2f}/{:.2f}", node->m_localTransform.scale, node->m_worldTransform.scale);
		Log::infoRaw("%s%s : children(%d), hidden(%d), Local:(%.2f, %.2f, %.2f), World:(%.2f, %.2f, %.2f), Scale:(%s)",
			padding.c_str(), node->m_name.c_str(),
			node->m_children.m_emptyRunStart, node->flags & 0x1,
			node->m_localTransform.translate.x, node->m_localTransform.translate.y, node->m_localTransform.translate.z,
			node->m_worldTransform.translate.x, node->m_worldTransform.translate.y, node->m_worldTransform.translate.z,
			scale.c_str());
	}

	void printNodeChildren(NiNode* child, std::string padding) {
		printNode(child, padding);

		padding += "..";
		if (child->GetAsNiNode()) {
			for (UInt16 i = 0; i < child->m_children.m_emptyRunStart; ++i) {
				if (const auto nextNode = child->m_children.m_data[i]) {
					printNodeChildren(reinterpret_cast<NiNode*>(nextNode), padding);
				}
			}
		}
	}

	void printNodeAncestors(const NiNode* node, std::string padding) {
		while (node) {
			printNode(node, padding);
			padding += "..";
			node = node->m_parent;
		}
	}
}

namespace frik {
	void printMatrix(const Matrix44* mat) {
		Log::info("Dump matrix:");
		std::string row;
		for (auto i = 0; i < 4; i++) {
			for (auto j = 0; j < 4; j++) {
				row += std::to_string(mat->data[i][j]);
				row += " ";
			}
			Log::info("%s", row.c_str());
			row = "";
		}
	}

	void positionDiff(const Skeleton* skelly) {
		const RE::NiPoint3 firstpos = f4vr::getPlayerNodes()->HmdNode->m_worldTransform.translate;
		const RE::NiPoint3 skellypos = f4vr::getRootNode()->m_worldTransform.translate;

		Log::info("difference = %f %f %f", firstpos.x - skellypos.x, firstpos.y - skellypos.y, firstpos.z - skellypos.z);
	}

	void printAllNodes() {
		auto* node = (*g_player)->unkF0->rootNode->GetAsNiNode();
		while (node->m_parent) {
			node = node->m_parent;
		}
		printNodes(node, false);
	}

	void printNodes(NiNode* node, const bool printAncestors) {
		Log::info("Children of '%s':", node->m_name.c_str());
		printNodeChildren(node, "");
		if (printAncestors) {
			Log::info("Ancestors of '%s':", node->m_name.c_str());
			printNodeAncestors(node, "");
		}
	}

	void printNodes(NiNode* nde, const long long curTime) {
		Log::info("%d %s : children = %d %d: local %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f", curTime, nde->m_name.c_str(), nde->m_children.m_emptyRunStart, nde->flags & 0x1,
			nde->m_localTransform.rotate.arr[0], nde->m_localTransform.rotate.arr[1], nde->m_localTransform.rotate.arr[2],
			nde->m_localTransform.rotate.arr[3], nde->m_localTransform.rotate.arr[4], nde->m_localTransform.rotate.arr[5],
			nde->m_localTransform.rotate.arr[6], nde->m_localTransform.rotate.arr[7], nde->m_localTransform.rotate.arr[8],
			nde->m_localTransform.rotate.arr[9], nde->m_localTransform.rotate.arr[10], nde->m_localTransform.rotate.arr[11],
			nde->m_localTransform.translate.x, nde->m_localTransform.translate.y, nde->m_localTransform.translate.z);

		if (nde->GetAsNiNode()) {
			for (UInt16 i = 0; i < nde->m_children.m_emptyRunStart; ++i) {
				if (const auto nextNode = nde->m_children.m_data[i] ? nde->m_children.m_data[i]->GetAsNiNode() : nullptr) {
					printNodes(nextNode, curTime);
				}
			}
		}
	}

	/**
	 * Print the local transform data of nodes tree.
	 */
	void printNodesTransform(NiNode* node, std::string padding) {
		Log::info("%s%s child=%d, %s, Pos:(%2.3f, %2.3f, %2.3f), Rot:[[%2.4f, %2.4f, %2.4f][%2.4f, %2.4f, %2.4f][%2.4f, %2.4f, %2.4f]]",
			padding.c_str(), node->m_name.c_str(),
			node->m_children.m_emptyRunStart, node->flags & 0x1 ? "hidden" : "visible",
			node->m_localTransform.translate.x,
			node->m_localTransform.translate.y,
			node->m_localTransform.translate.z,
			node->m_localTransform.rotate.data[0][0], node->m_localTransform.rotate.data[1][0], node->m_localTransform.rotate.data[2][0],
			node->m_localTransform.rotate.data[0][1], node->m_localTransform.rotate.data[1][1], node->m_localTransform.rotate.data[2][1],
			node->m_localTransform.rotate.data[0][2], node->m_localTransform.rotate.data[1][2], node->m_localTransform.rotate.data[2][2]);
		if (!node->GetAsNiNode()) {
			return; // no children for non NiNodes
		}

		padding += "..";
		for (UInt16 i = 0; i < node->m_children.m_emptyRunStart; ++i) {
			if (const auto nextNode = node->m_children.m_data[i]) {
				printNodesTransform(reinterpret_cast<NiNode*>(nextNode), padding);
			}
		}
	}

	void printTransform(const std::string& name, const RE::NiTransform& transform, const bool sample) {
		const auto frm = "Transform '" + name + "' Pos:(%.2f, %.2f, %.2f), Rot:[[%.2f, %.2f, %.2f][%.2f, %.2f, %.2f][%.2f, %.2f, %.2f]], Scale:(%.2f)";
		if (sample) {
			Log::sample(frm.c_str(),
				transform.translate.x, transform.translate.y, transform.translate.z,
				transform.rotate.data[0][0], transform.rotate.data[1][0], transform.rotate.data[2][0],
				transform.rotate.data[0][1], transform.rotate.data[1][1], transform.rotate.data[2][1],
				transform.rotate.data[0][2], transform.rotate.data[1][2], transform.rotate.data[2][2],
				transform.scale);
		} else {
			Log::info(frm.c_str(),
				transform.translate.x, transform.translate.y, transform.translate.z,
				transform.rotate.data[0][0], transform.rotate.data[1][0], transform.rotate.data[2][0],
				transform.rotate.data[0][1], transform.rotate.data[1][1], transform.rotate.data[2][1],
				transform.rotate.data[0][2], transform.rotate.data[1][2], transform.rotate.data[2][2],
				transform.scale);
		}
	}

	void printPosition(const std::string& name, const RE::NiPoint3& pos, const bool sample) {
		const auto frm = "Transform '" + name + "' Pos: (%.2f, %.2f, %.2f)";
		if (sample) {
			Log::sample(frm.c_str(), pos.x, pos.y, pos.z);
		} else {
			Log::info(frm.c_str(), pos.x, pos.y, pos.z);
		}
	}

	/**
	 * Dump the player body parts and whatever they are hidden.
	 */
	void dumpPlayerGeometry(BSFadeNode* rn) {
		for (auto i = 0; i < rn->kGeomArray.count; ++i) {
			const auto& geometry = rn->kGeomArray[i].spGeometry;
			Log::info("Geometry[%d] = '%s' (%s)", i, geometry->m_name.c_str(), geometry->flags & 0x1 ? "Hidden" : "Visible");
		}
	}

	void debug(const Skeleton* skelly) {
		static std::uint64_t fc = 0;

		//Offsets::ForceGamePause(*g_menuControls);

		auto rn = static_cast<BSFadeNode*>(f4vr::getRootNode()->m_parent);
		//Log::info("newrun");

		//for (int i = 0; i < 44; i++) {
		//	if ((*g_player)->equipData->slots[i].item != nullptr) {
		//		std::string name = (*g_player)->equipData->slots[i].item->GetFullName();
		//		auto form_type = (*g_player)->equipData->slots[i].item->GetFormType();
		//		Log::info("%s formType = %d", name.c_str(), form_type);
		//		if (form_type == FormType::kFormType_ARMO) {
		//			auto form = reinterpret_cast<TESObjectARMO*>((*g_player)->equipData->slots[i].item);
		//			auto bipedslot = form->bipedObject.data.parts;
		//			Log::info("biped slot = %d", bipedslot);
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

		//Log::info("throwing-> %d", Actor_CanThrow(*g_player, g_equipIndex));

		//for (auto i = 0; i < rn->kGeomArray.capacity-1; ++i) {
		//	BSFadeNode::FlattenedGeometryData data = rn->kGeomArray[i];
		//	Log::info("%s", data.spGeometry->m_name.c_str());
		//}

		//_playerNodes->ScopeParentNode->flags &= 0xfffffffffffffffe;
		//updateDown(dynamic_cast<NiNode*>(_playerNodes->ScopeParentNode), true);

		//	BSFixedString name("LArm_Hand");
		////	NiAVObject* node = (*g_player)->firstPersonSkeleton->GetObjectByName(&name);
		//	NiAVObject* node = _root->GetObjectByName(&name);
		//	if (!node) { return; }
		//	Log::info("%d %f %f %f %f %f %f", node->flags & 0xF, node->m_localTransform.translate.x,
		//									 node->m_localTransform.translate.y,
		//									 node->m_localTransform.translate.z,
		//									 node->m_worldTransform.translate.x,
		//									 node->m_worldTransform.translate.y,
		//									 node->m_worldTransform.translate.z
		//									);

		//	for (auto i = 0; i < (*g_player)->inventoryList->items.count; i++) {
		//	Log::info("%d,%d,%x,%x,%s", fc, i, (*g_player)->inventoryList->items[i].form->formID, (*g_player)->inventoryList->items[i].form->formType, (*g_player)->inventoryList->items[i].form->GetFullName());
		//}

		//if (fc < 1) {
		//	tHashSet<ObjectModMiscPair, BGSMod::Attachment::Mod*> *map = g_modAttachmentMap.GetPtr();
		//	map->Dump();
		//}

		//for (auto i = 0; i < rt->numTransforms; i++) {

		//if (rt->transforms[i].refNode) {
		//	Log::info("%d,%s,%d,%d", fc, rt->transforms[i].refNode->m_name.c_str(), rt->transforms[i].childPos, rt->transforms[i].parPos);
		//}
		//else {
		//	Log::info("%d,%s,%d,%d", fc, "", rt->transforms[i].childPos, rt->transforms[i].parPos);
		//}
		//		Log::info("%d,%d,%s", fc, i, rt->transforms[i].name.c_str());
		//}
		//
		//for (auto i = 0; i < rt->numTransforms; i++) {
		//	int pos = rt->bonePositions[i].translateition;
		//	if (rt->bonePositions[i].name && ((uint64_t)rt->bonePositions[i].name > 0x1000)) {
		//		Log::info("%d,%d,%s", fc, pos, rt->bonePositions[i].name->data);
		//	}
		//	else {
		//		Log::info("%d,%d", fc, pos);
		//	}
		//}

		//	if (rt->bonePositions[i].name && (rt->bonePositions[i].translateition != 0) && ((uint64_t)rt->bonePositions[i].name > 0x1000)) {
		//		int pos = rt->bonePositions[i].translateition;
		//		if (pos > rt->numTransforms) {
		//			continue;
		//		}
		//		Log::info("%d,%s,%d,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f", fc, rt->bonePositions[i].name->data,
		//							rt->bonePositions[i].translateition,
		//							rt->transforms[pos].local.rotate.arr[0],
		//							rt->transforms[pos].local.rotate.arr[1],
		//							rt->transforms[pos].local.rotate.arr[2],
		//							rt->transforms[pos].local.rotate.arr[3],
		//							rt->transforms[pos].local.rotate.arr[4],
		//							rt->transforms[pos].local.rotate.arr[5],
		//							rt->transforms[pos].local.rotate.arr[6],
		//							rt->transforms[pos].local.rotate.arr[7],
		//							rt->transforms[pos].local.rotate.arr[8],
		//							rt->transforms[pos].local.rotate.arr[9],
		//							rt->transforms[pos].local.rotate.arr[10],
		//							rt->transforms[pos].local.rotate.arr[11],
		//							rt->transforms[pos].local.translate.x,
		//							rt->transforms[pos].local.translate.y,
		//							rt->transforms[pos].local.translate.z,
		//							rt->transforms[pos].world.rotate.arr[0],
		//							rt->transforms[pos].world.rotate.arr[1],
		//							rt->transforms[pos].world.rotate.arr[2],
		//							rt->transforms[pos].world.rotate.arr[3],
		//							rt->transforms[pos].world.rotate.arr[4],
		//							rt->transforms[pos].world.rotate.arr[5],
		//							rt->transforms[pos].world.rotate.arr[6],
		//							rt->transforms[pos].world.rotate.arr[7],
		//							rt->transforms[pos].world.rotate.arr[8],
		//							rt->transforms[pos].world.rotate.arr[9],
		//							rt->transforms[pos].world.rotate.arr[10],
		//							rt->transforms[pos].world.rotate.arr[11],
		//							rt->transforms[pos].world.translate.x,
		//							rt->transforms[pos].world.translate.y,
		//							rt->transforms[pos].world.translate.z
		//		);

		//	if(strstr(rt->bonePositions[i].name->data, "Finger")) {
		//		Matrix44 rot;
		//		rot.makeIdentity();
		//		rt->transforms[pos].local.rotate = rot.make43();

		//		if (rt->transforms[pos].refNode) {
		//			rt->transforms[pos].refNode->m_localTransform.rotate = rot.make43();
		//		}

		//		rot.makeTransformMatrix(rt->transforms[pos].local.rotate, RE::NiPoint3(0, 0, 0));

		//		short parent = rt->transforms[pos].parPos;
		//		rt->transforms[pos].world.rotate = rot.multiply43Left(rt->transforms[parent].world.rotate);

		//		if (rt->transforms[pos].refNode) {
		//			rt->transforms[pos].refNode->m_worldTransform.rotate = rt->transforms[pos].world.rotate;
		//		}

		//	}
		//	}

		//rt->UpdateWorldBound();
		//}

		fc++;
	}

	void printScaleFormElements(GFxValue* elm, const std::string& padding) {
		GFxValue childrenCountVal;
		elm->GetMember("numChildren", &childrenCountVal);
		const int childrenCount = childrenCountVal.GetType() == GFxValue::kType_Int ? childrenCountVal.GetInt() : 0;

		GFxValue nameVal;
		elm->GetMember("name", &nameVal);
		const auto name = nameVal.IsString() ? nameVal.GetString() : "Unknown";

		GFxValue visibleVal;
		elm->GetMember("visible", &visibleVal);
		const int visible = visibleVal.IsBool() ? visibleVal.GetBool() : false;

		GFxValue toStringVal;
		elm->Invoke("toString", &toStringVal, nullptr, 0);
		const auto toString = toStringVal.IsString() ? toStringVal.GetString() : "";

		GFxValue textVal;
		elm->GetMember("text", &textVal);
		GFxValue buttonTextVal;
		elm->GetMember("ButtonText", &buttonTextVal);
		const auto text = textVal.IsString() ? textVal.GetString() : "";

		Log::infoRaw("%s%s : children(%d), visible(%d), toString:(%s), text:(%s)", padding.c_str(), name, childrenCount, visible, toString, text);

		for (int i = 0; i < childrenCount; ++i) {
			GFxValue child;
			GFxValue args[1];
			args[0].SetInt(i);
			if (elm->Invoke("getChildAt", &child, args, 1)) {
				printScaleFormElements(&child, padding + "..");
			} else {
				Log::warn("Failed to get child at index %d.", i);
			}
		}
	}
}
