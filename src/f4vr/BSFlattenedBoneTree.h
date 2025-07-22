#pragma once

#include "F4VROffsets.h"
#include "f4sevr/Common.h"

namespace f4vr
{
    class BSFlattenedBoneTree : public RE::NiNode
    {
    public:
        struct BoneTransforms
        {
            RE::NiTransform local;
            RE::NiTransform world;
            short parPos;
            short childPos;
            uint32_t unk8c;
            RE::NiNode* refNode;
            RE::BSFixedString name;
            uint64_t unk98;
        };

        struct BoneNodePosition
        {
            F4SEVR::StringCache::Entry* name;
            int position;
            uint32_t pad;
            uintptr_t unk;
        };

        int GetBoneIndex(const std::string& a_name)
        {
            auto name = new RE::BSFixedString(a_name.c_str());
            return BSFlattenedBoneTree_GetBoneIndex(this, name);
        }

        RE::NiNode* GetBoneNode(const std::string& a_name)
        {
            auto name = new RE::BSFixedString(a_name.c_str());
            return BSFlattenedBoneTree_GetBoneNode(this, name);
        }

        RE::NiNode* GetBoneNode(const int a_pos)
        {
            return BSFlattenedBoneTree_GetBoneNodeFromPos(this, a_pos);
        }

        void UpdateBoneArray()
        {
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

    static_assert(sizeof(BSFlattenedBoneTree) == 0x1C0);
}
