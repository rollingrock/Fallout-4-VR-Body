#pragma once

#include "f4se/NiNodes.h"

class BSFlattenedBoneTree : public NiNode
{
public:

	struct BoneTransforms {
		NiTransform local;
		NiTransform world;
		short parPos;
		short pad;
		uint32_t unk8c;
		NiNode* refNode;
		uint64_t unk90;
		uint64_t unk98;
	};

	struct BoneNodePosition {
		StringCache::Entry* name;
		int position;
		uint32_t pad;
		uintptr_t unk;
	};
	
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
