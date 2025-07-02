#pragma once

#include "F4VROffsets.h"

namespace f4vr
{
    // 80808
    class StringCache
    {
    public:
        // 18+
        struct Entry
        {
            enum
            {
                kState_RefcountMask = 0x3FFF,
                kState_External = 0x4000,
                kState_Wide = 0x8000
            };

            Entry* next; // 00
            std::uint32_t state; // 08 - refcount, hash, flags
            std::uint32_t length; // 0C
            Entry* externData; // 10
            char data[0]; // 18

            bool IsWide()
            {
                Entry* iter = this;

                while (iter->state & kState_External) {
                    iter = iter->externData;
                }

                if ((iter->state & kState_Wide) == kState_Wide) {
                    return true;
                }

                return false;
            }

            template <typename T>
            T* Get()
            {
                auto iter = this;

                while (iter->state & kState_External) {
                    iter = iter->externData;
                }

                return static_cast<T*>(iter->data);
            }
        };
    };

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
            StringCache::Entry* name;
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
