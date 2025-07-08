// ReSharper disable CppRedundantVoidArgumentList
#pragma once

// ReSharper disable CppClangTidyBugproneVirtualNearMiss
// ReSharper disable CppEnforceOverridingFunctionStyle

namespace F4SEVR
{
    class NiNode;
    class BSGeometry;

    // 30
    class NiMatrix43
    {
    public:
        union
        {
            float data[3][4];
            float arr[12];
        };

        NiMatrix43 Transpose() const
        {
            NiMatrix43 result;
            result.data[0][0] = data[0][0];
            result.data[0][1] = data[1][0];
            result.data[0][2] = data[2][0];
            result.data[0][3] = data[0][3];
            result.data[1][0] = data[0][1];
            result.data[1][1] = data[1][1];
            result.data[1][2] = data[2][1];
            result.data[1][3] = data[1][3];
            result.data[2][0] = data[0][2];
            result.data[2][1] = data[1][2];
            result.data[2][2] = data[2][2];
            result.data[2][3] = data[2][3];
            return result;
        }

        RE::NiPoint3 operator*(const RE::NiPoint3& pt) const
        {
            float x, y, z;
            //x = data[0][0] * pt.x + data[0][1] * pt.y + data[0][2] * pt.z;
            //y = data[1][0] * pt.x + data[1][1] * pt.y + data[1][2] * pt.z;
            //z = data[2][0] * pt.x + data[2][1] * pt.y + data[2][2] * pt.z;
            x = data[0][0] * pt.x + data[1][0] * pt.y + data[2][0] * pt.z;
            y = data[0][1] * pt.x + data[1][1] * pt.y + data[2][1] * pt.z;
            z = data[0][2] * pt.x + data[1][2] * pt.y + data[2][2] * pt.z;
            return RE::NiPoint3(x, y, z);
        }
    };

    // 40
    class NiTransform
    {
    public:
        NiMatrix43 rotate; // 00
        RE::NiPoint3 translate; // 30 (renamed from pos)
        float scale; // 3C

        RE::NiPoint3 operator*(const RE::NiPoint3& pt) const;
    };

    static_assert(sizeof(NiTransform) == 0x40);

    // 10
    class NiRefObject
    {
    public:
        NiRefObject() :
            m_uiRefCount(0) {}

        virtual ~NiRefObject() {}

        virtual void DeleteThis(void) { delete this; }; // calls virtual dtor

        void IncRef(void) {}

        void DecRef(void) {}

        bool Release(void);

        //	void	** _vtbl;		// 00
        volatile std::int32_t m_uiRefCount; // 08
    };

    // 10
    class NiObject : public NiRefObject
    {
    public:
        virtual RE::NiRTTI* GetRTTI(void) { return nullptr; }
        virtual NiNode* GetAsNiNode(void) { return nullptr; }
        virtual RE::NiSwitchNode* GetAsNiSwitchNode(void) { return nullptr; }
        virtual void* Unk_05() { return nullptr; }
        virtual void* Unk_06() { return nullptr; }
        virtual void* Unk_07() { return nullptr; }
        virtual BSGeometry* GetAsBSGeometry(void) { return nullptr; }
        virtual void* GetAsBStriStrips() { return nullptr; }
        virtual RE::BSTriShape* GetAsBSTriShape(void) { return nullptr; }
        virtual RE::BSDynamicTriShape* GetAsBSDynamicTriShape(void) { return nullptr; }
        virtual void* GetAsSegmentedTriShape() { return nullptr; }
        virtual RE::BSSubIndexTriShape* GetAsBSSubIndexTriShape(void) { return nullptr; }
        virtual void* GetAsNiGeometry() { return nullptr; }
        virtual void* GetAsNiTriBasedGeom() { return nullptr; }
        virtual void* GetAsNiTriShape() { return nullptr; }
        virtual void* GetAsParticlesGeom() { return nullptr; }
        virtual void* GetAsParticleSystem() { return nullptr; }
        virtual void* GetAsLinesGeom() { return nullptr; }
        virtual void* GetAsLight() { return nullptr; }
        virtual RE::bhkNiCollisionObject* GetAsBhkNiCollisionObject() { return nullptr; }
        virtual RE::bhkBlendCollisionObject* GetAsBhkBlendCollisionObject() { return nullptr; }
        virtual RE::bhkRigidBody* GetAsBhkRigidBody() { return nullptr; }
        virtual RE::bhkLimitedHingeConstraint* GetAsBhkLimitedHingeConstraint() { return nullptr; }
        virtual RE::bhkNPCollisionObject* GetAsbhkNPCollisionObject() { return nullptr; }
        virtual NiObject* CreateClone(RE::NiCloningProcess* unk1) { return nullptr; }
        virtual void LoadBinary(void* stream) {} // LoadBinary
        virtual void Unk_1C() {};
        virtual bool Unk_1D() { return false; }
        virtual void SaveBinary(void* stream) {} // SaveBinary
        virtual bool IsEqual(NiObject* object) { return false; } // IsEqual
        virtual bool Unk_20(void* unk1) { return false; }
        virtual void Unk_21() {};
        virtual bool Unk_22() { return false; }
        virtual RE::NiRTTI* GetStreamableRTTI() { return GetRTTI(); }
        virtual bool Unk_24() { return false; }
        virtual bool Unk_25() { return false; }
        virtual void Unk_26() {}
        virtual bool Unk_27() { return false; }

        // MEMBER_FN_PREFIX(NiObject);
        // DEFINE_MEMBER_FN(Internal_IsEqual, bool, 0x01C13EA0, NiObject* object);
    };

    // 28
    class NiObjectNET : public NiObject
    {
    public:
        enum CopyType
        {
            COPY_NONE,
            COPY_EXACT,
            COPY_UNIQUE
        };

        RE::BSFixedString m_name; // 10
        void* unk10; // 18 - Controller?
        std::uint64_t m_extraData; // 20 - must be locked/unlocked when altering/acquiring

        bool AddExtraData(RE::NiExtraData* extraData);
        RE::NiExtraData* GetExtraData(const RE::BSFixedString& name);
        bool HasExtraData(const RE::BSFixedString& name) { return GetExtraData(name) != nullptr; }

        // MEMBER_FN_PREFIX(NiObjectNET);
        // DEFINE_MEMBER_FN(Internal_AddExtraData, bool, 0x01C16CD0, NiExtraData* extraData);
    };

    // 120
    class NiAVObject : public NiObjectNET
    {
    public:
        struct NiUpdateData
        {
            NiUpdateData() :
                unk00(nullptr), pCamera(nullptr), flags(0), unk14(0), unk18(0), unk20(0), unk28(0), unk30(0), unk38(0) {}

            void* unk00; // 00
            void* pCamera; // 08
            std::uint32_t flags; // 10
            std::uint32_t unk14; // 14
            std::uint64_t unk18; // 18
            std::uint64_t unk20; // 20
            std::uint64_t unk28; // 28
            std::uint64_t unk30; // 30
            std::uint64_t unk38; // 38
        };

        virtual void UpdateControllers();
        virtual void PerformOp();
        virtual void AttachProperty();
        virtual void SetMaterialNeedsUpdate(); // empty?
        virtual void SetDefaultMaterialNeedsUpdateFlag(); // empty?
        virtual void SetAppCulled(bool set);
        //	virtual NiAVObject * GetObjectByName(const BSFixedString * nodeName);
        virtual void unkwrongfunc0();
        virtual void SetSelectiveUpdateFlags(bool* unk1, bool unk2, bool* unk3);
        virtual void UpdateDownwardPass(NiUpdateData* ud, std::uint32_t flags);
        //	virtual void UpdateSelectedDownwardPass();
        virtual NiAVObject* GetObjectByName(const RE::BSFixedString* nodeName);
        virtual void UpdateRigidDownwardPass();
        virtual void UpdateWorldBound();
        virtual void unka0();
        virtual void unka8();
        virtual void unkb0();
        virtual void UpdateWorldData(NiUpdateData* ctx);
        virtual void UpdateTransformAndBounds();
        virtual void UpdateTransforms();
        virtual void Unk_37();
        virtual void Unk_38();

        NiNode* m_parent; // 28
        RE::NiTransform m_localTransform; // 30
        RE::NiTransform m_worldTransform; // 70
        RE::NiBound m_worldBound; // B0
        RE::NiTransform m_previousWorld; // C0
        RE::NiPointer<RE::NiCollisionObject> m_spCollisionObject; // 100

        enum : std::uint64_t
        {
            kFlagAlwaysDraw = (1 << 11),
            kFlagIsMeshLOD = (1 << 12),
            kFlagFixedBound = (1 << 13),
            kFlagTopFadeNode = (1 << 14),
            kFlagIgnoreFade = (1 << 15),
            kFlagNoAnimSyncX = (1 << 16),
            kFlagNoAnimSyncY = (1 << 17),
            kFlagNoAnimSyncZ = (1 << 18),
            kFlagNoAnimSyncS = (1 << 19),
            kFlagNoDismember = (1 << 20),
            kFlagNoDismemberValidity = (1 << 21),
            kFlagRenderUse = (1 << 22),
            kFlagMaterialsApplied = (1 << 23),
            kFlagHighDetail = (1 << 24),
            kFlagForceUpdate = (1 << 25),
            kFlagPreProcessedNode = (1 << 26),
            kFlagScenegraphChange = (1 << 29),
            kFlagInInstanceGroup = (1LL << 35),
            kFlagLODFadingOut = (1LL << 36),
            kFlagFadedIn = (1LL << 37),
            kFlagForcedFadeOut = (1LL << 38),
            kFlagNotVisible = (1LL << 39),
            kFlagShadowCaster = (1LL << 40),
            kFlagNeedsRendererData = (1LL << 41),
            kFlagAccumulated = (1LL << 42),
            kFlagAlreadyTraversed = (1LL << 43),
            kFlagPickOff = (1LL << 44),
            kFlagUpdateWorldData = (1LL << 45),
            kFlagHasPropController = (1LL << 46),
            kFlagHasLockedChildAccess = (1LL << 47),
            kFlagHasMovingSound = (1LL << 49),
        };

        std::uint64_t flags; // 108
        void* unk110; // 110
        float unk118; // 118
        std::uint32_t unk11C; // 11C

        // MEMBER_FN_PREFIX(NiAVObject);
        // DEFINE_MEMBER_FN(GetAVObjectByName, NiAVObject*, 0x01D137A0, BSFixedString* name, bool unk1, bool unk2);
        // DEFINE_MEMBER_FN(SetScenegraphChange, void, 0x01C23E10);

        // Return true in the functor to halt traversal
        template <typename T>
        bool Visit(T& functor)
        {
            // if (functor(this))
            //     return true;
            //
            // RE::NiPointer<NiNode> node(GetAsNiNode());
            // if (node) {
            //     for (std::uint32_t i = 0; i < node->m_children.m_emptyRunStart; i++) {
            //         RE::NiPointer<NiAVObject> object(node->m_children.m_data[i]);
            //         if (object) {
            //             if (object->Visit(functor))
            //                 return true;
            //         }
            //     }
            // }

            return false;
        }
    };

    static_assert(sizeof(NiAVObject) == 0x120);

    // 140
    class NiNode : public NiAVObject
    {
    public:
        virtual void Unk_39(void);
        virtual void AttachChild(NiAVObject* obj, bool firstAvail);
        virtual void InsertChildAt(std::uint32_t index, NiAVObject* obj);
        virtual void DetachChild(NiAVObject* obj, NiPointer<NiAVObject>& out);
        virtual void RemoveChild(NiAVObject* obj);
        virtual void DetachChildAt(std::uint32_t i, NiPointer<NiAVObject>& out);
        virtual void RemoveChildAt(std::uint32_t i);
        virtual void SetAt(std::uint32_t index, NiAVObject* obj, NiPointer<NiAVObject>& replaced);
        virtual void SetAt(std::uint32_t index, NiAVObject* obj);
        virtual void Unk_42(void);

        std::uint32_t pad120[0x10]; // 120 - offset so that m_children lines up
        RE::BSTArray<NiAVObject*> m_children; // 160
        float unk138;
        float unk13C;

        static NiNode* Create(std::uint16_t children = 0);

        // MEMBER_FN_PREFIX(NiNode);
        // DEFINE_MEMBER_FN(ctor, NiNode*, 0x01C17D30, UInt16 children);
    };

    static_assert(sizeof(NiNode) == 0x180);

    class BSGeometryData
    {
    public:
        std::uint64_t vertexDesc;

        struct VertexData
        {
            REX::W32::ID3D11Buffer* d3d11Buffer; // 00 - const CLayeredObjectWithCLS<class CBuffer>::CContainedObject::`vftable'{for `CPrivateDataImpl<struct ID3D11Buffer>'}
            std::uint8_t* vertexBlock; // 08
            std::uint64_t unk10; // 10
            std::uint64_t unk18; // 18
            std::uint64_t unk20; // 20
            std::uint64_t unk28; // 28
            std::uint64_t unk30; // 30
            volatile std::int32_t refCount; // 38
        };

        struct TriangleData
        {
            REX::W32::ID3D11Buffer* d3d11Buffer; // 00 - Same buffer as VertexData
            std::uint16_t* triangles; // 08
            std::uint64_t unk10; // 10
            std::uint64_t unk18; // 18
            std::uint64_t unk20; // 20
            std::uint64_t unk28; // 28
            std::uint64_t unk30; // 30
            volatile std::int32_t refCount; // 38
        };

        VertexData* vertexData; // 08
        TriangleData* triangleData; // 10
        volatile std::int32_t refCount; // 18
    };

    // 1C0
    class BSFadeNode : public NiNode
    {
    public:
        virtual ~BSFadeNode();

        struct FlattenedGeometryData
        {
            RE::NiBound kBound; // 00
            RE::NiPointer<BSGeometry> spGeometry; // 10
            std::uint32_t uiFlags; // 18
        };

        RE::BSShaderPropertyLightData kLightData; // 140
        RE::BSTArray<FlattenedGeometryData> kGeomArray; // 168
        RE::BSTArray<RE::NiBound> MergedGeomBoundsA; // 180
        float fNearDistSqr; // 198
        float fFarDistSqr; // 19C
        float fCurrentFade; // 1A0
        float fCurrentDecalFade; // 1A4
        float fBoundRadius; // 1A8
        float fTimeSinceUpdate; // 1AC
        std::int32_t iFrameCounter; // 1B0
        float fPreviousMaxA; // 1B4
        float fCurrentShaderLODLevel; // 1B8
        float fPreviousShaderLODLevel; // 1BC
    };

    // 160
    class BSGeometry : public NiAVObject
    {
    public:
        virtual void Unk_39();
        virtual void Unk_3A();
        virtual void Unk_3B();
        virtual void Unk_3C();
        virtual void Unk_3D();
        virtual void Unk_3E();
        virtual void Unk_3F();
        virtual void Unk_40();

        RE::NiBound kModelBound; // 120
        RE::NiPointer<RE::NiProperty> effectState; // 130
        RE::NiPointer<RE::NiProperty> shaderProperty; // 138
        RE::NiPointer<RE::BSSkin::Instance> skinInstance; // 140

        union VertexDesc
        {
            struct
            {
                std::uint8_t szVertexData : 4;
                std::uint8_t szVertex : 4; // 0 when not dynamic
                std::uint8_t oTexCoord0 : 4;
                std::uint8_t oTexCoord1 : 4;
                std::uint8_t oNormal : 4;
                std::uint8_t oTangent : 4;
                std::uint8_t oColor : 4;
                std::uint8_t oSkinningData : 4;
                std::uint8_t oLandscapeData : 4;
                std::uint8_t oEyeData : 4;
                std::uint16_t vertexFlags : 16;
                std::uint8_t unused : 8;
            };

            std::uint64_t vertexDesc;
        };

        enum : std::uint64_t
        {
            kFlag_Unk1 = (1ULL << 40),
            kFlag_Unk2 = (1ULL << 41),
            kFlag_Unk3 = (1ULL << 42),
            kFlag_Unk4 = (1ULL << 43),
            kFlag_Vertex = (1ULL << 44),
            kFlag_UVs = (1ULL << 45),
            kFlag_Unk5 = (1ULL << 46),
            kFlag_Normals = (1ULL << 47),
            kFlag_Tangents = (1ULL << 48),
            kFlag_VertexColors = (1ULL << 49),
            kFlag_Skinned = (1ULL << 50),
            kFlag_Unk6 = (1ULL << 51),
            kFlag_MaleEyes = (1ULL << 52),
            kFlag_Unk7 = (1ULL << 53),
            kFlag_FullPrecision = (1ULL << 54),
            kFlag_Unk8 = (1ULL << 55),
        };

        BSGeometryData* geometryData; // 148
        std::uint64_t vertexDesc; // 150

        std::uint16_t GetVertexSize() const { return (vertexDesc << 2) & 0x3C; } // 0x3C might be a compiler optimization, (vertexDesc & 0xF) << 2 makes more sense

        std::int8_t ucType; // 158
        bool Registered; // 159
        std::uint16_t pad15A; // 15A
        std::uint32_t unk15C; // 15C

        // MEMBER_FN_PREFIX(BSGeometry);
        // 523E6E56493B00C91D9A86659158A735D8A58371+B
        // DEFINE_MEMBER_FN(UpdateShaderProperty, UInt32, 0x02804860);
    };

    static_assert(sizeof(BSGeometry) == 0x160);
}
