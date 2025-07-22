#pragma once

// ReSharper disable CppClangTidyPerformanceEnumSize
// ReSharper disable CppClangTidyBugproneVirtualNearMiss
// ReSharper disable CppEnforceOverridingFunctionStyle

namespace F4SEVR
{
    class NiNode;
    class BSGeometry;
    class TESObjectARMA;

    struct DamageTypes
    {
        RE::BGSDamageType* damageType; // 00
        std::uint32_t value; // 08
    };

    struct ValueModifier
    {
        RE::ActorValueInfo* avInfo; // 00
        std::uint32_t unk08; // 08
    };

    class IAliasFunctor
    {
    public:
        virtual ~IAliasFunctor() {}

        virtual std::uint32_t Visit(RE::BGSRefAlias* alias) = 0;
    };

    // ??
    class ActorEquipData
    {
    public:
        std::uint64_t unk00; // 00
        RE::NiNode* flattenedBoneTree; // 08

        enum
        {
            kMaxSlots = 44
        };

        // 58
        struct SlotData
        {
            RE::TESForm* item; // 00
            RE::TBO_InstanceData* instanceData; // 08
            RE::BGSObjectInstanceExtra* extraData; // 10
            RE::TESForm* model; // 18 - ARMA for ARMO and WEAP for WEAP
            RE::BGSModelMaterialSwap* modelMatSwap; // 20
            RE::BGSTextureSet* textureSet; // 28
            RE::NiAVObject* node; // 30
            void* unk38; // 38
            RE::IAnimationGraphManagerHolder* unk40; // 40
            std::uint64_t unk48; // 48
            std::uint32_t unk50; // 50
            std::uint32_t unk54; // 54
        };

        SlotData slots[kMaxSlots];
    };

    // 08
    class BaseFormComponent
    {
    public:
        BaseFormComponent();
        virtual ~BaseFormComponent();

        virtual void Unk_01(void);
        virtual void Unk_02(void);
        virtual void Unk_03(void);
        virtual void Unk_04(void);
        virtual void Unk_05(void);
        virtual void Unk_06(void);

        //	void	** _vtbl;	// 00
    };

    // 10
    class BGSTypedKeywordValueArray
    {
    public:
        void* unk00; // 00
        void* unk08; // 08
    };

    // 18
    class BGSAttachParentArray : public BaseFormComponent
    {
    public:
        BGSTypedKeywordValueArray unk08;
    };

    // 20
    class TESForm : public BaseFormComponent
    {
    public:
        enum
        {
            kTypeID = 0
        }; // special-case

        virtual void Unk_07();
        virtual void Unk_08();
        virtual void Unk_09();
        virtual void Unk_0A();
        virtual void Unk_0B();
        virtual void Unk_0C();
        virtual bool MarkChanged(std::uint64_t changed);
        virtual void Unk_0E();
        virtual void Unk_0F();
        virtual void Unk_10();
        virtual void Unk_11(); // Serialize
        virtual void Unk_12();
        virtual void Unk_13();
        virtual void Unk_14();
        virtual void Unk_15();
        virtual void Unk_16();
        virtual std::uint64_t GetLastModifiedMod(); // 17 - Returns the ModInfo* of the mod that last modified the form.
        virtual std::uint64_t GetLastModifiedMod_2(); // 18 - Returns the ModInfo* of the mod that last modified the form. Calls a helper function to do so.
        virtual std::uint8_t GetFormType(); // 19
        virtual void Unk_1A(); // 1A - GetDebugName(TESForm * form, char * destBuffer, unsigned int bufferSize);
        virtual bool GetPlayerKnows(); // 1B - Gets flag bit 6.
        virtual void Unk_1C();
        virtual void Unk_1D();
        virtual void Unk_1E();
        virtual void Unk_1F();
        virtual void Unk_20();
        virtual void Unk_21();
        virtual void Unk_22();
        virtual void Unk_23();
        virtual void Unk_24();
        virtual void Unk_25();
        virtual void Unk_26();
        virtual void Unk_27();
        virtual void Unk_28();
        virtual void Unk_29();
        virtual void Unk_2A();
        virtual void Unk_2B();
        virtual void Unk_2C();
        virtual void Unk_2D();
        virtual void Unk_2E();
        virtual void Unk_2F();
        virtual void Unk_30();
        virtual void Unk_31();
        virtual void Unk_32();
        virtual void Unk_33();
        virtual void Unk_34();
        virtual const char* GetFullName(); // 35
        virtual void Unk_36();
        virtual void Unk_37();
        virtual void Unk_38();
        virtual void Unk_39();
        virtual const char* GetEditorID(); // Only returns string for things that actually store the EDID ingame
        virtual void Unk_3B();
        virtual void Unk_3C();
        virtual void Unk_3D();
        virtual void Unk_3E();
        virtual void Unk_3F();
        virtual void Unk_40();
        virtual void Unk_41();
        virtual void Unk_42();
        virtual void Unk_43();
        virtual void Unk_44();
        virtual void Unk_45();
        virtual void Unk_46();
        virtual void Unk_47();

        enum
        {
            kFlag_IsDeleted = 1 << 5,
            kFlag_PlayerKnows = 1 << 6,
            kFlag_Persistent = 1 << 10,
            kFlag_IsDisabled = 1 << 11,
            kFlag_NoHavok = 1 << 29,
        };

        struct Data
        {
            std::uint64_t entries; // array of ModInfo* - mods that change this form.
            std::uint64_t size;
        };

        Data* unk08; // 08
        std::uint32_t flags; // 10
        std::uint32_t formID; // 14
        std::uint16_t unk18; // 18
        std::uint8_t formType; // 1A
        std::uint8_t unk1B; // 1B
        std::uint32_t pad1C; // 1C
    };

    // 110
    class TESObjectREFR : public TESForm
    {
    public:
        virtual void Unk_48();
        virtual void Unk_49();
        virtual void Unk_4A();
        virtual void Unk_4B();
        virtual void Unk_4C();
        virtual void Unk_4D();
        virtual void Unk_4E();
        virtual void Unk_4F();
        virtual void Unk_50();
        virtual void Unk_51();
        virtual void Unk_52();
        virtual void Unk_53();
        virtual void Unk_54();
        virtual void Unk_55();
        virtual void Unk_56();
        virtual void Unk_57();
        virtual void Unk_58();
        virtual void Unk_59();
        virtual void Unk_5A();
        virtual void Unk_5B();
        virtual RE::BGSScene* GetCurrentScene(); // 5C  Returns the Scene this reference is currently participating in, or NULL if it isn't in a scene.
        virtual void Unk_5D();
        virtual void Unk_5E();
        virtual void Unk_5F();
        virtual void Unk_60();
        virtual void Unk_61();
        virtual void Unk_62();
        virtual void Unk_63();
        virtual void Unk_64();
        virtual void Unk_65();
        virtual void Unk_66();
        virtual void Unk_67();
        virtual void Unk_68();
        virtual void Unk_69();
        virtual void Unk_6A();
        virtual void Unk_6B();
        virtual void Unk_6C();
        virtual void Unk_6D();
        virtual void Unk_6E();
        virtual void Unk_6F();
        virtual void Unk_70();
        virtual void Unk_71();
        virtual void Unk_72();
        virtual void Unk_73();
        virtual void Unk_74();
        virtual void Unk_75();
        virtual void Unk_76();
        virtual void Unk_77();
        virtual void Unk_78();
        virtual void Unk_79();
        virtual void Unk_7A();
        virtual void GetMarkerPosition(RE::NiPoint3* pos);
        virtual void Unk_7C();
        virtual void Unk_7D();
        virtual void Unk_7E();
        virtual void Unk_7F();
        virtual void Unk_80();
        virtual void Unk_81();
        virtual void Unk_82();
        virtual void Unk_83();
        virtual void Unk_84();
        virtual void Unk_85();
        virtual void Unk_86();
        virtual void Unk_87();
        virtual void Unk_88();
        virtual void Unk_89();
        virtual void Unk_8A();
        virtual RE::NiNode* GetActorRootNode(bool firstPerson); // 8B - Returns either first person or third person
        virtual RE::NiNode* GetObjectRootNode(); // 8C - Returns the 3rd person skeleton
        virtual void Unk_8D();
        virtual void Unk_8E();
        virtual void Unk_8F();
        virtual void Unk_90();
        virtual RE::TESRace* GetActorRace(); // 91
        virtual void Unk_92();
        virtual void Unk_93();
        virtual void Unk_94();
        virtual void Unk_95();
        virtual void Unk_96();
        virtual void Unk_97();
        virtual void Unk_98();
        virtual void Unk_99();
        virtual void Unk_9A();
        virtual void Unk_9B();
        virtual void Unk_9C();
        virtual void Unk_9D();
        virtual void Unk_9E();
        virtual void Unk_9F();
        virtual ActorEquipData** GetEquipData(bool bFirstPerson);
        virtual void Unk_A1();
        virtual void Unk_A2();
        virtual void Unk_A3();
        virtual void Unk_A4();
        virtual void Unk_A5();
        virtual void Unk_A6();
        virtual void Unk_A7();
        virtual void Unk_A8();
        virtual void Unk_A9();
        virtual void Unk_AA();
        virtual void Unk_AB();
        virtual void Unk_AC();
        virtual void Unk_AD();
        virtual void Unk_AE();
        virtual void Unk_AF();
        virtual void Unk_B0();
        virtual void Unk_B1();
        virtual void Unk_B2();
        virtual void Unk_B3();
        virtual void Unk_B4();
        virtual void Unk_B5();
        virtual void Unk_B6();
        virtual void Unk_B7();
        virtual void Unk_B8();
        virtual void Unk_B9();
        virtual void Unk_BA();
        virtual void Unk_BB();
        virtual void Unk_BC();
        virtual void Unk_BD();
        virtual void Unk_BE();
        virtual void Unk_BF();
        virtual void Unk_C0();
        virtual void Unk_C1();
        virtual void Unk_C2();
        virtual void Unk_C3();

        enum
        {
            kTypeID = RE::ENUM_FORM_ID::kREFR
        };

        // parents
        RE::BSHandleRefObject handleRefObject; // 20
        std::uint64_t activeGraphIfInactive; // 30
        std::uint64_t animGraphEventSink; // 38
        std::uint64_t inventoryListSink; // 40
        std::uint64_t animGraphHolder; // 48
        std::uint64_t keywordFormBase; // 50
        RE::ActorValueOwner actorValueOwner; // 58
        void* unk60; // 60
        void* unk68; // 68
        std::uint32_t unk70; // 70
        std::uint32_t unk74; // 74
        std::uint32_t unk78; // 78
        std::uint32_t unk7C; // 7C
        std::uint64_t unk80; // 80
        std::uint64_t unk88; // 88
        std::uint64_t unk90; // 90
        std::uint64_t unk98; // 98
        std::uint64_t unkA0; // A0
        std::uint64_t unkA8; // A8
        std::uint64_t unkB0; // B0
        RE::TESObjectCELL* parentCell; // B8
        RE::NiPoint3 rot; // C0, C4, C8 - Probably quat?
        float unkCC;
        RE::NiPoint3 pos; // D0, D4, D8
        float unkDC;
        RE::TESForm* baseForm; // E0
        void* unkE8; // E8

        struct LoadedData
        {
            std::uint64_t unk00;
            RE::NiNode* rootNode;
            std::uint64_t unk10;
            std::uint64_t unk18;

            enum
            {
                kFlag_PhysicsInitialized = 1
            };

            std::uint64_t flags;
            // ...
        };

        LoadedData* unkF0; // F0 - Root node at 0x08
        RE::BGSInventoryList* inventoryList; // F8
        RE::ExtraDataList* extraDataList; // 100 - ExtraData?
        std::uint32_t unk104; // 104
        std::uint32_t unk108; // 108

        std::uint32_t CreateRefHandle(void);

        // MEMBER_FN_PREFIX(TESObjectREFR);
        // DEFINE_MEMBER_FN(GetReferenceName, const char*, 0x003F3A70);
        // DEFINE_MEMBER_FN(GetWorldspace, RE::TESWorldSpace*, 0x003F75A0);
        // DEFINE_MEMBER_FN(GetInventoryWeight, float, 0x003E8770);
        // DEFINE_MEMBER_FN(GetCarryWeight, float, 0x00DD7C90);
        // 7055D6CB4B64E11E63908512704F8871CEC025D3+11E
        // DEFINE_MEMBER_FN_1(ForEachAlias, void, 0x003DFC00, IAliasFunctor* functor);
    };

    static_assert(offsetof(TESObjectREFR, parentCell) == 0xB8);
    static_assert(offsetof(TESObjectREFR, baseForm) == 0xE0);
    static_assert(sizeof(TESObjectREFR) == 0x110);

    // 08
    struct IMovementInterface
    {
        virtual ~IMovementInterface();
    };

    // 08
    class IMovementState : public IMovementInterface
    {
    public:
        virtual void Unk_01() = 0;
        virtual void Unk_02() = 0;
        virtual void Unk_03(RE::NiPoint3& pos) = 0;
        virtual void Unk_04() = 0;
        virtual void Unk_05() = 0;
        virtual void Unk_06() = 0;
        virtual void Unk_07() = 0;
        virtual void Unk_08() = 0;
        virtual void Unk_09() = 0;
        virtual void Unk_0A() = 0;
        virtual void Unk_0B() = 0;
        virtual void Unk_0C() = 0;
        virtual void Unk_0D() = 0;
        virtual void Unk_0E() = 0;
        virtual void Unk_0F() = 0;
        virtual void Unk_10() = 0;
        virtual void Unk_11() = 0;
        virtual void Unk_12() = 0;
        virtual void Unk_13() = 0;
        virtual void Unk_14() = 0;
        virtual int Unk_15() = 0;
        virtual void Unk_16() = 0;
        virtual void Unk_17() = 0;
        virtual void Unk_18() = 0;
        virtual void Unk_19() = 0;
        virtual void Unk_1A() = 0;
        virtual void Unk_1B() = 0;
        virtual void Unk_1C() = 0;
        virtual void Unk_1D() = 0;
        virtual void Unk_1E() = 0;
        virtual void Unk_1F() = 0;
        virtual void Unk_20() = 0;
    };

    // 08
    class EquippedItemData : public RE::NiRefObject
    {
    public:
        virtual ~EquippedItemData() override;
    };

    // 38
    class EquippedWeaponData : public EquippedItemData
    {
    public:
        virtual ~EquippedWeaponData() override;

        RE::TESAmmo* ammo; // 10
        std::uint64_t unk18; // 18
        void* unk20; // 20
        std::uint64_t unk28; // 28
        RE::NiAVObject* object; // 30
    };

    // 10
    class ActorState : public IMovementState
    {
    public:
        virtual void Unk_01();
        virtual void Unk_02();
        virtual void Unk_03(RE::NiPoint3& pos);
        virtual void Unk_04();
        virtual void Unk_05();
        virtual void Unk_06();
        virtual void Unk_07();
        virtual void Unk_08();
        virtual void Unk_09();
        virtual void Unk_0A();
        virtual void Unk_0B();
        virtual void Unk_0C();
        virtual void Unk_0D();
        virtual void Unk_0E();
        virtual void Unk_0F();
        virtual void Unk_10();
        virtual void Unk_11();
        virtual void Unk_12();
        virtual void Unk_13();
        virtual void Unk_14();
        virtual int Unk_15();
        virtual void Unk_16();
        virtual void Unk_17();
        virtual void Unk_18();
        virtual void Unk_19();
        virtual void Unk_1A();
        virtual void Unk_1B();
        virtual void Unk_1C();
        virtual void Unk_1D();
        virtual void Unk_1E();
        virtual void Unk_1F();
        virtual void Unk_20();

        enum Flags
        {
            kUnk1 = 0x80000,
            kUnk2 = 0x40000,

            kWeaponStateShift = 1,
            kWeaponStateMask = 0x07,

            kWeaponState_Drawn = 0x03,
        };

        std::uint32_t unk08; // 08
        std::uint32_t flags; // 0C

        std::uint32_t GetWeaponState() { return (flags >> kWeaponStateShift) & kWeaponStateMask; }
        bool IsWeaponDrawn() { return GetWeaponState() >= kWeaponState_Drawn; }
    };

    // 18
    class MagicTarget
    {
    public:
        virtual ~MagicTarget();

        virtual void Unk_01(void);
        virtual void Unk_02(void);
        virtual void Unk_03(void);
        virtual void Unk_04(void);
        virtual void Unk_05(void);
        virtual void Unk_06(void);
        virtual void Unk_07(void);
        virtual void Unk_08(void);
        virtual void Unk_09(void);
        virtual void Unk_0A(void);
        virtual void Unk_0B(void);
        virtual void Unk_0C(void);

        void* unk08; // 08
        void* unk10; // 10
    };

    // 490
    class Actor : public TESObjectREFR
    {
    public:
        virtual void Unk_C4();
        virtual void Unk_C5();
        virtual void Unk_C6();
        virtual void Unk_C7();
        virtual void Unk_C8();
        virtual void Unk_C9();
        virtual void Unk_CA();
        virtual void Unk_CB();
        virtual void Unk_CC();
        virtual void Unk_CD();
        virtual void Unk_CE();
        virtual void Update(float delta);
        virtual void Unk_D0();
        virtual void Unk_D1();
        virtual void Unk_D2();
        virtual void Unk_D3();
        virtual void Unk_D4();
        virtual void Unk_D5();
        virtual void Unk_D6();
        virtual void Unk_D7();
        virtual void Unk_D8();
        virtual void Unk_D9();
        virtual void Unk_DA();
        virtual void Unk_DB();
        virtual void Unk_DC();
        virtual void Unk_DD();
        virtual void Unk_DE();
        virtual void Unk_DF();
        virtual void Unk_E0();
        virtual void Unk_E1();
        virtual void Unk_E2();
        virtual void Unk_E3();
        virtual void Unk_E4();
        virtual void Unk_E5();
        virtual void Unk_E6();
        virtual void Unk_E7();
        virtual void Unk_E8();
        virtual void Unk_E9();
        virtual void Unk_EA();
        virtual void Unk_EB();
        virtual void Unk_EC();
        virtual void Unk_ED();
        virtual void Unk_EE();
        virtual void Unk_EF();
        virtual void Unk_F0();
        virtual void Unk_F1();
        virtual void Unk_F2();
        virtual void Unk_F3();
        virtual void Unk_F4();
        virtual void Unk_F5();
        virtual void Unk_F6();
        virtual void Unk_F7();
        virtual void Unk_F8();
        virtual void Unk_F9();
        virtual void Unk_FA();
        virtual void Unk_FB();
        virtual void Unk_FC();
        virtual void Unk_FD();
        virtual bool IsInCombat(std::uint64_t unk1 = 0, std::uint64_t unk2 = 0);
        virtual void Unk_FF();
        virtual void Unk_100();
        virtual void Unk_101();
        virtual void Unk_102();
        virtual void Unk_103();
        virtual void Unk_104();
        virtual void Unk_105();
        virtual void Unk_106();
        virtual void Unk_107();
        virtual void Unk_108();
        virtual void Unk_109();
        virtual void Unk_10A();
        virtual void Unk_10B();
        virtual void Unk_10C();
        virtual void Unk_10D();
        virtual void Unk_10E();
        virtual void Unk_10F();
        virtual void Unk_110();
        virtual void Unk_111();
        virtual void Unk_112();
        virtual void Unk_113();
        virtual void Unk_114();
        virtual void Unk_115();
        virtual void Unk_116();
        virtual void Unk_117();
        virtual void Unk_118();
        virtual void Unk_119();
        virtual void Unk_11A();
        virtual void Unk_11B();
        virtual void Unk_11C();
        virtual void Unk_11D();
        virtual void Unk_11E();
        virtual void Unk_11F();
        virtual void Unk_120();
        virtual void Unk_121();
        virtual void Unk_122();
        virtual void Unk_123();
        virtual void Unk_124();
        virtual void Unk_125();
        virtual void Unk_126();
        virtual void Unk_127();
        virtual void Unk_128();
        virtual void Unk_129();
        virtual void Unk_12A();
        virtual void Unk_12B();
        virtual void Unk_12C();
        virtual void Unk_12D();
        virtual void Unk_12E();
        virtual void Unk_12F();
        virtual void Unk_130();

        enum
        {
            kTypeID = RE::ENUM_FORM_ID::kACHR
        };

        MagicTarget magicTarget; // 110
        ActorState actorState; // 128
        std::uint64_t movementDataChanged; // 138
        std::uint64_t transformDelta; // 140
        std::uint64_t subGraphActivation; // 148
        std::uint64_t characterMoveFinished; // 150
        std::uint64_t nonSupportContact; // 158
        std::uint64_t characterStateChanged; // 160

        std::uint64_t unk168[(0x2D0 - 0x168) / 8]; // 168
        std::uint32_t actorFlags; // 2D0

        enum ActorFlags
        {
            kFlag_Teammate = (1 << 26)
        };

        std::uint32_t unk2D4;
        std::uint64_t unk2D8[(0x300 - 0x2D8) / 8]; // 2D8

        // Lots of misc data goes here, equipping, perks, etc
        struct MiddleProcess
        {
            void* unk00; // 00

            struct Data08
            {
                std::uint64_t unk00[0x288 >> 3]; // 000

                std::uint64_t lock; // 280

                struct EquipData
                {
                    TESForm* item; // 00
                    RE::TBO_InstanceData* instanceData; // 08
                    RE::BGSEquipSlot* equipSlot; // 10
                    std::uint64_t unk18; // 18
                    EquippedWeaponData* equippedData; // 20
                };

                EquipData* equipData; // 288

                std::uint64_t unk290[(0x3A0 - 0x290) >> 3];
                std::uint32_t unk3A0; // 3A0
                std::uint32_t furnitureHandle1; // 3A4
                std::uint32_t furnitureHandle2; // 3A8
                std::uint32_t unk3AC; // 3AC
                std::uint64_t unk3B0[(0x490 - 0x3B0) >> 3];

                enum
                {
                    kDirtyHeadParts = 0x04,
                    kDirtyGender = 0x20
                };

                std::uint32_t unk490; // 490
                std::uint16_t unk494; // 494
                std::uint16_t unk496; // 496 - some kind of dirty flag?
            };

            Data08* unk08; // 08

            // MEMBER_FN_PREFIX(MiddleProcess);
            // DEFINE_MEMBER_FN(UpdateEquipment, void, 0x00EB4B40, Actor* actor, std::uint32_t flags);
        };

        MiddleProcess* middleProcess; // 300
        std::uint64_t unk308[(0x338 - 0x308) / 8];

        struct ActorValueData
        {
            std::uint32_t avFormID; // 00
            float value; // 04
        };

        RE::BSTArray<ActorValueData> actorValueData; // 338

        struct Data350 // ActorValue related, not sure what the 3 values are
        {
            std::uint32_t avFormID; // 00
            float unk04; // 04
            float unk08; // 08
            float unk0C; // 0C
        };

        RE::BSTArray<Data350> unk350; // 350
        std::uint64_t unk368[(0x418 - 0x368) / 8];
        RE::TESRace* race; // 418
        std::uint64_t unk420; // 420
        ActorEquipData* equipData; // 428
        std::uint64_t unk430[(0x490 - 0x430) / 8]; // 430

        bool IsPlayerTeammate() { return (actorFlags & kFlag_Teammate) == kFlag_Teammate; }
        bool GetEquippedExtraData(std::uint32_t slotIndex, RE::ExtraDataList** extraData);

        // MEMBER_FN_PREFIX(Actor);
        // DEFINE_MEMBER_FN(QueueUpdate, void, 0x00DDAD60, bool bDoFaceGen, std::uint32_t unk2, bool DoQueue, std::uint32_t flags); // 0, 0, 1, 0
        // DEFINE_MEMBER_FN(IsHostileToActor, bool, 0x00DE1CD0, Actor* actor);
        // DEFINE_MEMBER_FN(UpdateEquipment, void, 0x003F0580);
    };

    static_assert(offsetof(Actor, equipData) == 0x428);
    static_assert(offsetof(Actor::MiddleProcess::Data08, equipData) == 0x290);

    class PlayerCharacter : public Actor
    {
    public:
        enum
        {
            kTypeID = RE::ENUM_FORM_ID::kACHR
        };

        virtual void Unk_131();
        virtual void Unk_132();
        virtual void Unk_133();
        virtual void Unk_134();

        std::uint64_t menuOpenClose; // 490
        std::uint64_t menuModeChange; // 498
        std::uint64_t userEventEnabled; // 4A0
        std::uint64_t hitEvent; // 4A8
        std::uint64_t perkValueEvents; // 4B0
        std::uint64_t movementControlsFilter; // 4B8

        std::uint8_t unk4C0[0x7D8 - 0x4C0]; // 4C0

        struct Objective
        {
            void* unk00; // 0
            RE::TESQuest* owner; // 8
            // ...
        };

        struct ObjectiveData
        {
            Objective* data; // 0
            std::uint32_t instance; // 4
            std::uint32_t number; // 8
        };

        RE::BSTArray<ObjectiveData> objData; // 7D8
        std::uint64_t unk458[(0xB70 - 0x7F0) / 8]; // 7F0
        std::uint64_t unkb76[(0xFE0 - 0xB70) / 8];
        ActorEquipData* playerEquipData; // B70 - First person?
        RE::NiNode* firstPersonSkeleton; // B78   VR looks to be FE8
        std::uint64_t unkB68[(0xD00 - 0xB80) / 8]; // B78
        RE::BSTArray<RE::BGSCharacterTint::Entry*>* tints; // D00
        std::uint64_t unkC90[(0xE10 - 0xCF8) / 8]; // CF8
    };

    static_assert(offsetof(PlayerCharacter, menuOpenClose) == 0x490);
    static_assert(offsetof(PlayerCharacter, playerEquipData) == 0xFE0);
    static_assert(offsetof(PlayerCharacter, tints) == 0x1170);

    // 20+
    class TESObject : public TESForm
    {
    public:
        virtual void Unk_48();
        virtual void Unk_49();
        virtual void Unk_4A();
        virtual void Unk_4B();
        virtual void Unk_4C();
        virtual void Unk_4D();
        virtual void Unk_4E();
        virtual void Unk_4F();
        virtual void Unk_50();
        virtual void Unk_51();
        virtual void Unk_52();
        virtual void Unk_53();
    };

    // 68
    class TESBoundObject : public TESObject
    {
    public:
        virtual void Unk_54();
        virtual void Unk_55();
        virtual RE::TBO_InstanceData* CloneInstanceData(RE::TBO_InstanceData* other);
        virtual void Unk_57();
        virtual void Unk_58();
        virtual void Unk_59();
        virtual void Unk_5A();
        virtual void Unk_5B();
        virtual void Unk_5C();
        virtual void Unk_5D();
        virtual void Unk_5E();
        virtual void Unk_5F();
        virtual void Unk_60();
        virtual void Unk_61();
        virtual void Unk_62();
        virtual void Unk_63();
        virtual void Unk_64();

        struct Bound
        {
            std::uint16_t x;
            std::uint16_t y;
            std::uint16_t z;
        };

        Bound bounds1; // 20
        Bound bounds2; // 26
        RE::BGSMod::Template::Items templateItems; // 30
        RE::BGSPreviewTransform previewTransform; // 50
        RE::BGSSoundTagComponent soundTagComponent; // 60
    };

    static_assert(offsetof(TESBoundObject, templateItems) == 0x30);
    static_assert(offsetof(TESBoundObject, previewTransform) == 0x50);
    static_assert(offsetof(TESBoundObject, soundTagComponent) == 0x60);
    static_assert(sizeof(TESBoundObject) == 0x68);

    // 18
    class TESEnchantableForm : public BaseFormComponent
    {
    public:
        virtual std::uint16_t Unk_07(void); // return unk10

        RE::EnchantmentItem* enchantment; // 08 EnchantmentItem
        std::uint16_t unk10; // 10
        std::uint16_t maxCharge; // 12
        std::uint16_t pad14[2]; // 14
    };

    // 10
    class TESFullName : public BaseFormComponent
    {
    public:
        ~TESFullName() override;

        virtual void Unk_07(void);
        virtual void Unk_08(void);

        RE::BSFixedString name; // 08
    };

    // 10
    class TESRaceForm : public BaseFormComponent
    {
    public:
        RE::TESRace* race; // 08
    };

    // 10
    class BGSDestructibleObjectForm : public BaseFormComponent
    {
    public:
        void* data; // 08
    };

    // 18
    class BGSPickupPutdownSounds : public BaseFormComponent
    {
    public:
        RE::BGSSoundDescriptorForm* pickUp; // 08 BGSSoundDescriptorForm
        RE::BGSSoundDescriptorForm* putDown; // 10 BGSSoundDescriptorForm
    };

    // 108
    class TESBipedModelForm : public BaseFormComponent
    {
    public:
        std::uint64_t unk[(0x108 - 0x8) / 8];
    };

    static_assert(sizeof(TESBipedModelForm) == 0x108);

    // 10
    class BGSEquipType : public BaseFormComponent
    {
    public:
        virtual RE::BGSEquipSlot* GetEquipSlot(void);
        virtual void SetEquipSlot(RE::BGSEquipSlot* type);

        RE::BGSEquipSlot* equipSlot; // 08
    };

    // 10
    class BGSInstanceNamingRulesForm : public BaseFormComponent
    {
    public:
        RE::BGSInstanceNamingRules* rules; // 08
    };

    // 48
    class BGSListForm : public TESForm
    {
    public:
        enum
        {
            kTypeID = RE::ENUM_FORM_ID::kFLST
        };

        RE::BSTArray<TESForm*> forms; // 20
        RE::BSTArray<std::uint32_t>* tempForms; // 38
        std::uint32_t scriptAdded; // 40 - Amount on the end of the tArray that is script-added
        std::uint32_t unk44; // 44
    };

    static_assert(sizeof(BGSListForm) == 0x48);

    // 18
    class BGSBlockBashData : public BaseFormComponent
    {
    public:
        RE::BGSImpactDataSet* impactDataSet; // 08	BGSImpactDataSet*
        RE::BGSMaterialType* materialType; // 10	BGSMaterialType *
    };

    // 10
    class BGSBipedObjectForm : public BaseFormComponent
    {
    public:
        // applicable to DefaultRace
        enum
        {
            kPart_Head = 1 << 0,
            kPart_Hair = 1 << 1,
            kPart_Body = 1 << 2,
            kPart_Hands = 1 << 3,
            kPart_Forearms = 1 << 4,
            kPart_Amulet = 1 << 5,
            kPart_Ring = 1 << 6,
            kPart_Feet = 1 << 7,
            kPart_Calves = 1 << 8,
            kPart_Shield = 1 << 9,
            kPart_Unnamed10 = 1 << 10,
            kPart_LongHair = 1 << 11,
            kPart_Circlet = 1 << 12,
            kPart_Ears = 1 << 13,
            kPart_Unnamed14 = 1 << 14,
            kPart_Unnamed15 = 1 << 15,
            kPart_Unnamed16 = 1 << 16,
            kPart_Unnamed17 = 1 << 17,
            kPart_Unnamed18 = 1 << 18,
            kPart_Unnamed19 = 1 << 19,
            kPart_Unnamed20 = 1 << 20,
            kPart_Unnamed21 = 1 << 21,
            kPart_Unnamed22 = 1 << 22,
            kPart_Unnamed23 = 1 << 23,
            kPart_Unnamed24 = 1 << 24,
            kPart_Unnamed25 = 1 << 25,
            kPart_Unnamed26 = 1 << 26,
            kPart_Unnamed27 = 1 << 27,
            kPart_Unnamed28 = 1 << 28,
            kPart_Unnamed29 = 1 << 29,
            kPart_Unnamed30 = 1 << 30,
            kPart_FX01 = 1 << 31,
        };

        enum
        {
            kWeight_Light = 0,
            kWeight_Heavy,
            kWeight_None,
        };

        struct Data
        {
            std::uint32_t parts; // 00 - init'd to 0
            std::uint32_t weightClass; // 04 - init'd to 2 (none)
        };

        Data data; // 08

        std::uint32_t GetSlotMask() const { return data.parts; }
        void SetSlotMask(std::uint32_t mask) { data.parts = mask; }

        std::uint32_t AddSlotToMask(std::uint32_t slot)
        {
            data.parts |= slot;
            return data.parts;
        }

        std::uint32_t RemoveSlotFromMask(std::uint32_t slot)
        {
            data.parts &= ~slot;
            return data.parts;
        }
    };

    // 2E0
    class TESObjectARMO : public TESBoundObject
    {
    public:
        enum
        {
            kTypeID = RE::ENUM_FORM_ID::kARMO
        };

        TESFullName fullName; // 068
        TESRaceForm raceForm; // 078
        TESEnchantableForm enchantable; // 088
        BGSDestructibleObjectForm destructible; // 0A0
        BGSPickupPutdownSounds pickupPutdown; // 0B0
        TESBipedModelForm bipedModel; // 0C8
        BGSEquipType equipType; // 1D0
        BGSBipedObjectForm bipedObject; // 1E0
        BGSBlockBashData blockBash; // 1F0
        RE::BGSKeywordForm keywordForm; // 208
        RE::TESDescription description; // 228
        BGSInstanceNamingRulesForm namingRules; // 240

        // 58
        struct InstanceData : RE::TBO_InstanceData
        {
            std::uint64_t unk10; // 10
            std::uint64_t unk18; // 18
            std::uint64_t unk20; // 20
            RE::BGSKeywordForm* keywords; // 28
            RE::BSTArray<DamageTypes>* damageTypes; // 30
            std::uint64_t unk38; // 38
            float weight; // 40
            std::uint32_t pad44; // 44
            std::uint32_t value; // 48
            std::uint32_t health; // 4C
            std::uint32_t unk50; // 50
            std::uint16_t armorRating; // 54
            std::uint16_t unk56; // 56
        };

        InstanceData instanceData; // 250 - 2A8 ( 592 - 680)

        struct ArmorAddons
        {
            void* unk00; // 00
            TESObjectARMA* armorAddon; // 08
        };

        RE::BSTArray<ArmorAddons> addons; // 2A8
        std::uint64_t unk2C0; // 2C0
        BGSAttachParentArray parentArray; // 2C8
    };

    static_assert(sizeof(TESObjectARMO::InstanceData) == 0x58);
    static_assert(offsetof(TESObjectARMO, parentArray) == 0x2C8);
    static_assert(sizeof(TESObjectARMO) == 0x2E0);

    // 10
    class BSTextureSet : public RE::NiObject
    {
    public:
        virtual RE::BSFixedString GetTextureFilenameFS(std::uint32_t typeEnum);
        virtual const char* GetTextureFilename(std::uint32_t typeEnum);
        virtual void Unk_2A();
        virtual void GetTexture(std::uint32_t typeEnum, RE::NiPointer<RE::NiTexture>& texture, bool unk1);
        virtual void SetTextureFilename(std::uint32_t typeEnum, const char* path);
    };

    // 350
    class BGSTextureSet : public TESBoundObject
    {
    public:
        enum
        {
            kTypeID = RE::ENUM_FORM_ID::kTXST
        };

        BSTextureSet textureSet; // 68

        void* unk78; // 78
        RE::TESTexture texture[8]; // 80
        std::uint64_t unk100[(0x350 - 0x100) / 8]; // 100
    };

    static_assert(sizeof(BGSTextureSet) == 0x350);

    // 228
    class TESObjectARMA : public TESObject
    {
    public:
        enum
        {
            kTypeID = RE::ENUM_FORM_ID::kARMA
        };

        TESRaceForm raceForm; // 20
        BGSBipedObjectForm bipedObject; // 30
        std::uint64_t unk040; // 40
        std::uint64_t unk048; // 48
        RE::BGSModelMaterialSwap swap50[2]; // 50
        RE::BGSModelMaterialSwap swapD0[2]; // D0
        RE::BGSModelMaterialSwap swap150[2]; // 150
        BGSTextureSet* unk1D0[2]; // 1D0
        BGSListForm* unk1E0; // 1E0
        std::uint64_t unk1E8; // 1E8
        RE::BSTArray<RE::TESRace*> additionalRaces; // 1F0
        RE::BGSFootstepSet* footstepSet; // 208
        RE::BGSArtObject* art; // 210
        void* unk218; // 218
        void* unk220; // 220

        // Constructs a node name from the specified armor and actor
        bool GetNodeName(char* dest, RE::TESNPC* refr, TESObjectARMO* armor);
    };

    static_assert(offsetof(TESObjectARMA, unk220) == 0x220);
    static_assert(sizeof(TESObjectARMA) == 0x228);

    // 10
    class BSInputEventReceiver
    {
    public:
        virtual ~BSInputEventReceiver();

        std::uint32_t unk08; // 08
        std::uint32_t unk0C; // 0C
    };

    // 24
    class TESCameraState : public RE::BSIntrusiveRefCounted, public RE::BSInputEventUser
    {
    public:
        ~TESCameraState() override;

        virtual void Unk_09();
        virtual void Unk_0A();
        virtual void Unk_0B(void* arg);
        virtual void GetRotation(RE::NiQuaternion* out);
        virtual void GetPosition(RE::NiPoint3* out);
        virtual void Unk_0E();
        virtual void Unk_0F();
        virtual void Unk_10();

        void* unk18; // 18
        std::uint32_t stateID; // 20
        std::uint32_t pad24; // 24
    };

    static_assert(sizeof(TESCameraState) == 0x28);

    // 138
    class ThirdPersonState : public TESCameraState
    {
    public:
        ~ThirdPersonState() override;

        virtual void UpdateMode(bool weaponDrawn);
        virtual bool Unk_12();
        virtual void Unk_13();
        virtual void Unk_14();
        virtual void Unk_15();
    };

    // 38
    class TESCamera
    {
    public:
        virtual ~TESCamera();

        virtual void SetCameraNode(RE::NiNode* node);
        virtual void Unk_02(std::uint8_t unk1); // Sets 0x30
        virtual void Unk_03();

        float unk08; // 08
        float unk0C; // 0C
        float unk10; // 10
        float unk14; // 14
        float unk18; // 18
        std::uint32_t unk1C; // 1C
        RE::NiNode* cameraNode; // 20
        TESCameraState* cameraState; // 28
        std::uint8_t unk30; // 30
        std::uint8_t unk31; // 31
        std::uint16_t unk32; // 32
        std::uint32_t unk34; // 34

        // MEMBER_FN_PREFIX(TESCamera);
        // DEFINE_MEMBER_FN(SetCameraState, void, 0x0081EFF0, TESCameraState* cameraState);
    };

    class PlayerCamera : public TESCamera
    {
    public:
        ~PlayerCamera() override;

        enum
        {
            kCameraState_FirstPerson = 0,
            kCameraState_AutoVanity,
            kCameraState_VATS,
            kCameraState_Free,
            kCameraState_IronSights,
            kCameraState_Transition,
            kCameraState_TweenMenu,
            kCameraState_ThirdPerson1,
            kCameraState_ThirdPerson2,
            kCameraState_Furniture,
            kCameraState_Horse,
            kCameraState_Bleedout,
            kCameraState_Dialogue,
            kNumCameraStates
        };

        BSInputEventReceiver inputEventReceiver; // 38
        std::uint64_t idleInputSink; // 48
        std::uint64_t userEventEnabledSink; // 50
        std::uint64_t otherEventEnabledSink; // 58

        std::uint32_t unk60; // 60
        std::uint32_t unk64; // 64 - Handle
        std::uint32_t unk68; // 68
        std::uint32_t unk6C; // 6C

        std::uint64_t unk70[(0xE0 - 0x70) >> 3];

        TESCameraState* cameraStates[kNumCameraStates]; // E0
        std::uint64_t unk148; // 148
        std::uint64_t unk150; // 150 - hknpSphereShape
        std::uint64_t unk158; // 158 - hknpBSWorld
        std::uint32_t unk160; // 160
        std::uint32_t unk164; // 164 - Handle
        float fDefaultWorldFov; // 168 - fDefaultWorldFOV:Display
        float fDefault1stPersonFOV; // 16C - fDefault1stPersonFOV:Display
        std::uint64_t unk170; // 170
        std::uint64_t unk178; // 178
        float unk180; // 180
        float unk184; // 184
        float unk188; // 188
        float unk18C; // 18C
        float unk190; // 190
        float unk194; // 194
        float unk198; // 198
        float unk19C; // 19C
        std::uint32_t unk1A0; // 1A0
        std::uint8_t unk1A4; // 1A4
        std::uint8_t unk1A5; // 1A5
        std::uint8_t unk1A6; // 1A6
        std::uint8_t unk1A7; // 1A7
        std::uint8_t unk1A8; // 1A8
        std::uint8_t unk1A9; // 1A9

        std::int32_t GetCameraStateId(TESCameraState* state);
    };

    inline PlayerCamera::~PlayerCamera() = default;

    static_assert(offsetof(PlayerCamera, cameraStates) == 0xE0);
    static_assert(offsetof(PlayerCamera, unk148) == 0x148);
}
