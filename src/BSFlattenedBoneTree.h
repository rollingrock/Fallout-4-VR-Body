#pragma once

#include "f4se/NiNodes.h"

namespace F4VRBody {
	typedef int (*_BSFlattenedBoneTree_GetBoneIndex)(NiAVObject* a_tree, BSFixedString* a_name);
	extern RelocAddr <_BSFlattenedBoneTree_GetBoneIndex> BSFlattenedBoneTree_GetBoneIndex;
	typedef NiNode* (*_BSFlattenedBoneTree_GetBoneNode)(NiAVObject* a_tree, BSFixedString* a_name);
	extern RelocAddr <_BSFlattenedBoneTree_GetBoneNode> BSFlattenedBoneTree_GetBoneNode;
	typedef NiNode* (*_BSFlattenedBoneTree_GetBoneNodeFromPos)(NiAVObject* a_tree, int a_pos);
	extern RelocAddr <_BSFlattenedBoneTree_GetBoneNodeFromPos> BSFlattenedBoneTree_GetBoneNodeFromPos;
	typedef void* (*_BSFlattenedBoneTree_UpdateBoneArray)(NiAVObject* node);
	extern RelocAddr<_BSFlattenedBoneTree_UpdateBoneArray> BSFlattenedBoneTree_UpdateBoneArray;

	class BSFlattenedBoneTree : public NiNode
	{
	public:

		struct BoneTransforms {
			NiTransform local;
			NiTransform world;
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

		int GetBoneIndex(std::string a_name)
		{
			BSFixedString* name = new BSFixedString(a_name.c_str());
			return BSFlattenedBoneTree_GetBoneIndex(this, name);
		}

		NiNode* GetBoneNode(std::string a_name)
		{
			BSFixedString* name = new BSFixedString(a_name.c_str());
			return BSFlattenedBoneTree_GetBoneNode(this, name);
		}

		NiNode* GetBoneNode(int a_pos)
		{
			return BSFlattenedBoneTree_GetBoneNodeFromPos(this, a_pos);
		}

		void UpdateBoneArray() {
			BSFlattenedBoneTree_UpdateBoneArray(this);
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


