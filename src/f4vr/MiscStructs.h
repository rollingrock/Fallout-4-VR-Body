#pragma once

namespace f4vr
{
    constexpr std::uint32_t PipboyAA = 0x0001ED3D;
    constexpr std::uint32_t PipboyArmor = 0x00021B3B;
    constexpr std::uint32_t MiningHelmet = 0x0022DD1F;
    constexpr std::uint32_t PALightKW = 0x000B34A6;

    inline REL::Relocation NEW_REFR_DATA_VTABLE(REL::Offset(0x2c8d080));

    using _BGSObjectInstance_CTOR = void* (*)(void* instance, RE::TESForm* a_form, RE::TBO_InstanceData* a_instanceData);
    inline REL::Relocation<_BGSObjectInstance_CTOR> BGSObjectInstance_CTOR(REL::Offset(0x2dd930));

    class BGSObjectInstance
    {
    public:
        BGSObjectInstance(RE::TESForm* a_object, RE::TBO_InstanceData* a_instanceData)
        {
            ctor(a_object, a_instanceData);
        }

        // members
        RE::TESForm* object{ nullptr }; // 00
        RE::TBO_InstanceData* instanceData; // 08

    private:
        BGSObjectInstance* ctor(RE::TESForm* aObject, RE::TBO_InstanceData* aInstanceData)
        {
            return static_cast<BGSObjectInstance*>(BGSObjectInstance_CTOR(static_cast<void*>(this), aObject, aInstanceData));
        }
    };

    static_assert(sizeof(BGSObjectInstance) == 0x10);

    struct NiCloneProcess
    {
        std::uint64_t unk00 = 0;
        std::uint64_t unk08 = 0; // Start of clone list 1?
        std::uint64_t unk10 = 0;
        std::uint64_t* unk18; // initd to RelocAddr(0x36ff560)
        std::uint64_t unk20 = 0;
        std::uint64_t unk28 = 0;
        std::uint64_t unk30 = 0;
        std::uint64_t unk38 = 0; // Start of clone list 2?
        std::uint64_t unk40 = 0;
        std::uint64_t* unk48; // initd to RelocAddr(0x36ff564)
        std::uint64_t unk50 = 0;
        std::uint64_t unk58 = 0;
        std::uint8_t copyType = 1; // 60 - CopyType - default 1
        std::uint8_t m_eAffectedNodeRelationBehavior = 0; // 61 - CloneRelationBehavior - default 0
        std::uint8_t m_eDynamicEffectRelationBehavior = 0; // 62 - CloneRelationBehavior - default 0
        char m_cAppendChar = '$'; // 64 - default '$'
        RE::NiPoint3 scale = { 1.0f, 1.0f, 1.0f }; // 0x68 - default {1, 1, 1}
    };

    class MuzzleFlash
    {
    public:
        uint64_t unk00;
        uint64_t unk08;
        RE::NiNode* fireNode;
        RE::NiNode* projectileNode;
    };
}
