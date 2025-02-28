#pragma once

#include "Debug.h"
#include "utils.h"
#include "Skeleton.h"

namespace F4VRBody {

	static void positionDiff(Skeleton* skelly) {
		NiPoint3 firstpos = skelly->getPlayerNodes()->HmdNode->m_worldTransform.pos;
		NiPoint3 skellypos = skelly->getRoot()->m_worldTransform.pos;

		_MESSAGE("difference = %f %f %f", (firstpos.x - skellypos.x), (firstpos.y - skellypos.y), (firstpos.z - skellypos.z));
	}

	static void printNodes(NiNode* nde) {
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

	static void printChildren(NiNode* child, std::string padding) {
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
}