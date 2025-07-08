#pragma once

// ReSharper disable CppCStyleCast
// ReSharper disable CppClangTidyBugproneVirtualNearMiss
// ReSharper disable CppEnforceOverridingFunctionStyle
// ReSharper disable CppEnforceOverridingDestructorStyle

#define CALL_MEMBER_FN(obj, fn) ((*(obj)).*(*((obj)->_##fn##_GetPtr())))
#include "Common.h"

namespace F4SEVR
{
    // 08
    class IComplexType
    {
    public:
        virtual ~IComplexType();

        virtual std::uint32_t GetType() = 0;

        std::int32_t m_refCount; // 08
        std::uint32_t unk0C; // 0C
        RE::BSFixedString m_typeName; // 10
        IComplexType* m_parent; // 18

        void AddRef(void);
        void Release(void);
    };

    // 58
    class VMObjectTypeInfo : public IComplexType
    {
    public:
        virtual ~VMObjectTypeInfo();

        virtual std::uint32_t GetType(); // returns 1 (kType_Identifier)

        void* unk20; // 20
        std::uint64_t unk28; // 28
        std::uint64_t unk30; // 30
        std::uint64_t unk38; // 38
        struct MemberData
        {
            unsigned unk00 : 3; // This == 3 is usually always checked before accessing properties
            unsigned unk03 : 5;
            unsigned numMembers : 10; // Variables + Properties
            unsigned unk19 : 14;
        } memberData;

        struct PropertyData
        {
            unsigned numVariables : 10; // Sometimes this is 0 and member != numProperties
            unsigned numProperties : 10; // Excludes variables
            unsigned unk21 : 12;
        } propertyData;

        std::uint32_t numFunc; // 48
        std::uint32_t unk4C; // 4C

        struct PropertyElement
        {
            RE::BSFixedString propertyName; // 00

            union // Can be number or IComplexType or IComplexType | 1 (array)
            {
                std::uint64_t value;
                RE::BSScript::IComplexType* id;
            } type; // 08
            std::uint64_t unk10; // 10 - Not sure what this is, maybe a hash?
        };

        struct Properties
        {
            RE::BSFixedString unk00;
            RE::BSFixedString unk08;
            RE::BSFixedString unk10;
            RE::BSFixedString unk18;
            RE::BSFixedString unk20;
            RE::BSFixedString unk28;
            PropertyElement defs[0];
        };

        Properties* properties; // 50
    };

    static_assert(offsetof(VMObjectTypeInfo, memberData) == 0x40);
    static_assert(offsetof(VMObjectTypeInfo, propertyData) == 0x44);
    static_assert(offsetof(VMObjectTypeInfo, properties) == 0x50);

    // 30 - Sized based on number of properties
    class VMIdentifier
    {
    public:
        enum
        {
            kLockBit = 0x80000000,
            kFastSpinThreshold = 10000
        };

        std::int32_t m_refCount; // 00
        std::uint32_t unk04; // 04
        VMObjectTypeInfo* m_typeInfo; // 08
        void* unk10; // 10
        std::uint64_t unk18; // 18
        volatile std::uint64_t m_handle; // 20
        volatile std::int32_t m_lock; // 28
        std::uint32_t unk2C; // 2C
        VMValue properties[0]; // 30

        std::uint64_t GetHandle(void);

        std::int32_t Lock(void);
        void Unlock(std::int32_t oldLock);

        // lock and ref count?
        void IncrementLock(void);
        std::int32_t DecrementLock(void);

        void Destroy(void);

        // MEMBER_FN_PREFIX(VMIdentifier);
        // DEFINE_MEMBER_FN(Destroy_Internal, void, 0x026BA690);
    };

    // 70
    class VMStructTypeInfo : public RE::BSScript::IComplexType
    {
    public:
        virtual ~VMStructTypeInfo();

        virtual std::uint32_t GetType(); // returns 7 (kType_Struct)

        struct StructData
        {
            std::uint64_t unk00; // 00
            std::uint64_t unk08; // 08
            std::uint64_t m_type; // 10
            void* unk18; // 18
            std::uint64_t unk20; // 20
        };

        RE::BSTArray<StructData> m_data; // 20

        class MemberItem
        {
        public:
            RE::BSFixedString name; // 00
            std::uint32_t index; // 08

            bool operator==(const MemberItem& rhs) const { return name == rhs.name; }
            bool operator==(const RE::BSFixedString a_name) const { return name == a_name; }
            operator UInt64() const { return (std::uint64_t)name.data->Get<char>(); }

            static inline std::uint32_t GetHash(RE::BSFixedString* key)
            {
                std::uint32_t hash;
                CalculateCRC32_64(&hash, (std::uint64_t)key->data, 0);
                return hash;
            }

            void Dump(void)
            {
                // _MESSAGE("\t\tname: %s", name.data->Get<char>());
                // _MESSAGE("\t\tinstance: %08X", index);
            }
        };

        RE::BSTHashMap<MemberItem, RE::BSFixedString> m_members;
    };

    // 10
    class VMValue
    {
    public:
        VMValue()
        {
            type.value = kType_None;
            data.p = nullptr;
        }

        ~VMValue()
        {
            CALL_MEMBER_FN(this, Destroy)();
        }

        VMValue(const VMValue& other);
        VMValue& operator=(const VMValue& other);

        enum
        {
            kType_None = 0,
            kType_Identifier = 1, // Number not actually used by VMValue
            kType_String = 2,
            kType_Int = 3,
            kType_Float = 4,
            kType_Bool = 5,
            kType_Variable = 6, // Points to a VMValue
            kType_Struct = 7, // Number not actually used by VMValue

            kType_ArrayOffset = 10,

            kType_IdentifierArray = 11,
            kType_StringArray = 12,
            kType_IntArray = 13,
            kType_FloatArray = 14,
            kType_BoolArray = 15,
            kType_VariableArray = 16,
            kType_StructArray = 17, // Number not actually used by VMValue
            kType_ArrayEnd = 18,

            kType_IntegralStart = kType_StringArray,
            kType_IntegralEnd = kType_VariableArray,
        };

        struct ArrayData
        {
            std::int32_t m_refCount; // 00
            std::uint32_t unk04; // 04
            std::uint64_t m_type; // 08 - base types 1-6
            std::uint64_t unk10; // 10
            RE::BSTArray<VMValue> arr; // 18

            // MEMBER_FN_PREFIX(ArrayData);
            // DEFINE_MEMBER_FN(Destroy, void, 0x026E6740);
        };

        struct StructData
        {
            std::int32_t m_refCount; // 00
            std::uint32_t unk04; // 04
            std::uint64_t unk08; // 08
            VMStructTypeInfo* m_type; // 10
            std::uint8_t unk18; // 18 - set to 1 if unk19 is 1 (3EFCF27952D674A8FA959AABC29A0FE3E726FA91)
            std::uint8_t unk19; // 19 - set to 1 when type+0x68 == 3
            std::uint16_t unk1A; // 1A
            std::uint32_t unk1C; // 1C
            char m_value[0]; // 20

            VMValue* GetStruct() { return (VMValue*)&m_value[0]; }
        };

        union // Can be number or IComplexType or IComplexType | 1 (array)
        {
            std::uint64_t value;
            RE::BSScript::IComplexType* id;
        } type;

        union
        {
            std::int32_t i;
            std::uint32_t u;
            float f;
            bool b;
            void* p;
            StructData* strct;
            ArrayData* arr;
            VMValue* var;
            VMIdentifier* id;
            StringCache::Entry* str;
            RE::BSFixedString* GetStr(void) { return (RE::BSFixedString*)(&str); }
            RE::BSFixedString* GetStr(void) const { return (RE::BSFixedString*)(&str); }
        } data;

        void SetNone(void);
        void SetInt(std::int32_t i);
        void SetFloat(float f);
        void SetBool(bool b);
        void SetString(const char* str);

        void SetVariable(VMValue* value);
        void SetComplexType(RE::BSScript::IComplexType* typeInfo);
        void SetIdentifier(VMIdentifier** identifier);

        bool IsIntegralType() const;
        bool IsIntegralArrayType() const;
        bool IsComplexArrayType() const;
        bool IsArrayType() const;
        bool IsComplexType() const;
        bool IsIdentifier();

        RE::BSScript::IComplexType* GetComplexType();
        RE::BSScript::IComplexType* GetComplexType() const;

        std::uint8_t GetTypeEnum() const;

        // MEMBER_FN_PREFIX(VMValue);
        // DEFINE_MEMBER_FN(Set, void, 0x026BF6A0, const VMValue* src);
        // DEFINE_MEMBER_FN(Destroy, void, 0x026BF050);
        // DEFINE_STATIC_HEAP(Heap_Allocate, Heap_Free)
    };

    class VMVariable
    {
    public:
        VMVariable() :
            m_var(nullptr) {}

        ~VMVariable() {}

        template <typename T>
        void Set(T* src, bool bReference = true)
        {
            PackValue(&m_value, src, (*g_gameVM)->m_virtualMachine);
            if (m_var && bReference)
                PackValue(m_var, src, (*g_gameVM)->m_virtualMachine);
        }

        // Fails on invalid type unpack
        template <typename T>
        bool Get(T* dst)
        {
            if (Is<T>()) {
                UnpackValue(dst, &m_value);
                return true;
            }

            return false;
        }

        // Skips type-check
        template <typename T>
        T As()
        {
            T tmp;
            UnpackValue(&tmp, &m_value);
            return tmp;
        }

        template <typename T>
        bool Is()
        {
            return m_value.type.value == GetTypeID<T>((*g_gameVM)->m_virtualMachine);
        }

        void PackVariable(VMValue* dst) const
        {
            VMValue* newValue = new VMValue(m_value);
            dst->SetVariable(newValue);
        }

        void UnpackVariable(VMValue* value)
        {
            m_var = value->data.var;
            m_value = *m_var;
        }

        bool IsNone() const { return m_value.GetTypeEnum() == 0; }

        // Provides direct access to the VM data, advanced use only
        VMValue& GetValue() { return m_value; }

    protected:
        VMValue* m_var; // Original reference
        VMValue m_value; // Copied data
    };
}
