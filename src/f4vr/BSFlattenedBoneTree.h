#pragma once

#include "F4VROffsets.h"
#include "f4se/NiNodes.h"

namespace f4vr {
	class BSFlattenedBoneTree : public NiNode {
	public:
		struct BoneTransforms {
			RE::NiTransform local;
			RE::NiTransform world;
			short parPos;
			short childPos;
			uint32_t unk8c;
			NiNode* refNode;
			BSFixedString name;
			uint64_t unk98;
		};

		struct BoneNodePosition {
			StringCache::Entry* name;
			int position;
			uint32_t pad;
			uintptr_t unk;
		};

		int GetBoneIndex(const std::string& a_name) {
			auto name = new BSFixedString(a_name.c_str());
			return f4vr::BSFlattenedBoneTree_GetBoneIndex(this, name);
		}

		NiNode* GetBoneNode(const std::string& a_name) {
			auto name = new BSFixedString(a_name.c_str());
			return f4vr::BSFlattenedBoneTree_GetBoneNode(this, name);
		}

		NiNode* GetBoneNode(const int a_pos) {
			return f4vr::BSFlattenedBoneTree_GetBoneNodeFromPos(this, a_pos);
		}

		void UpdateBoneArray() {
			f4vr::BSFlattenedBoneTree_UpdateBoneArray(this);
		}

		int numTransforms;
		uint32_t pad0;
		BoneTransforms* transforms;
		uint64_t unk190;
		uint64_t unk198;
		uint64_t unk1a0;
		uint64_t unk1a8;
		uint64_t unk1b0;
		BoneNodePosition* bonePositions;
	};

	STATIC_ASSERT(sizeof(BSFlattenedBoneTree) == 0x1C0);
}
