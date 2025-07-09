#pragma once
#include "common/CommonUtils.h"

// ReSharper disable CppNonExplicitConvertingConstructor

namespace F4SEVR
{
#define CALL_MEMBER_FN(obj, fn) ((*(obj)).*(*((obj)->_##fn##_GetPtr())))

#define MEMBER_FN_PREFIX(className) typedef className _MEMBER_FN_BASE_TYPE

#define DEFINE_MEMBER_FN_LONG(className, functionName, retnType, address, ...) \
    typedef retnType (className::* _##functionName##_type)(__VA_ARGS__);       \
                                                                               \
    inline _##functionName##_type* _##functionName##_GetPtr(void)              \
    {                                                                          \
        static uintptr_t _address;                                             \
        _address = address + REL::Module::get().base();                        \
        return (_##functionName##_type*)&_address;                             \
    }

#define DEFINE_MEMBER_FN(functionName, retnType, address, ...) DEFINE_MEMBER_FN_LONG(_MEMBER_FN_BASE_TYPE, functionName, retnType, address, __VA_ARGS__)

#define DEFINE_STATIC_HEAP(staticAllocate, staticFree)                                                  \
    static void* operator new(std::size_t size) { return staticAllocate(size); }                        \
    static void* operator new(std::size_t size, const std::nothrow_t&) { return staticAllocate(size); } \
    static void* operator new(std::size_t size, void* ptr) { return ptr; }                              \
    static void operator delete(void* ptr) { staticFree(ptr); }                                         \
    static void operator delete(void* ptr, const std::nothrow_t&) { staticFree(ptr); }                  \
    static void operator delete(void*, void*) {}

#define FORCE_INLINE __forceinline

#define DEFINE_MEMBER_FN_0(fnName, retnType, addr)                             \
    FORCE_INLINE retnType fnName()                                             \
    {                                                                          \
        struct empty_struct                                                    \
        {};                                                                    \
        typedef retnType (empty_struct::* _##fnName##_type)();                 \
        const static uintptr_t address = addr + REL::Module::get().base();     \
        _##fnName##_type fn = *(_##fnName##_type*)&address;                    \
        return (reinterpret_cast<empty_struct*>(this)->*fn)();                 \
    }

#define CALL_MEMBER_FN(obj, fn) ((*(obj)).*(*((obj)->_##fn##_GetPtr())))

    typedef std::uint8_t UInt8; //!< An unsigned 8-bit integer value
    typedef std::uint16_t UInt16; //!< An unsigned 16-bit integer value
    typedef std::uint32_t UInt32; //!< An unsigned 32-bit integer value
    typedef std::uint64_t UInt64; //!< An unsigned 64-bit integer value
    typedef std::int8_t SInt8; //!< A signed 8-bit integer value
    typedef std::int16_t SInt16; //!< A signed 16-bit integer value
    typedef std::int32_t SInt32; //!< A signed 32-bit integer value
    typedef std::int64_t SInt64; //!< A signed 64-bit integer value

    class Heap
    {
    public:
        MEMBER_FN_PREFIX(Heap);
        DEFINE_MEMBER_FN(Allocate, void*, 0x01B91950, size_t size, size_t alignment, bool aligned);
        DEFINE_MEMBER_FN(Free, void, 0x01B91C60, void* buf, bool aligned);
    };

    // B53CEF7AA7FC153E48CDE9DBD36CD8242577E27F+11D
    inline Heap* getMainHeap()
    {
        static REL::Relocation<Heap*> g_mainHeap(REL::Offset(0x0392E400));
        return g_mainHeap.get();
    }

    inline void* Heap_Allocate(size_t size)
    {
        return CALL_MEMBER_FN(getMainHeap(), Allocate)(size, 0, false);
    }

    inline void Heap_Free(void* ptr)
    {
        CALL_MEMBER_FN(getMainHeap(), Free)(ptr, false);
    }

    typedef void (*_CalculateCRC32_64)(UInt32* out, UInt64 data, UInt32 previous);
    inline REL::Relocation<_CalculateCRC32_64> CalculateCRC32_64 = REL::Relocation<_CalculateCRC32_64>(REL::Offset(0x01B931B0));

    typedef void (*_CalculateCRC32_32)(UInt32* out, UInt32 data, UInt32 previous);
    inline REL::Relocation<_CalculateCRC32_32> CalculateCRC32_32 = REL::Relocation<_CalculateCRC32_32>(REL::Offset(0x01B93120));

    // 8
    template <class T>
    class NiPointer
    {
    public:
        T* m_pObject; // 00

        NiPointer(T* pObject = (T*)0)
        {
            m_pObject = pObject;
            if (m_pObject)
                m_pObject->IncRef();
        }

        ~NiPointer()
        {
            if (m_pObject)
                m_pObject->DecRef();
        }

        operator bool() const { return m_pObject != nullptr; }

        operator T*() const { return m_pObject; }

        T& operator*() const { return *m_pObject; }

        T* operator->() const { return m_pObject; }

        NiPointer<T>& operator=(const NiPointer& rhs)
        {
            if (m_pObject != rhs.m_pObject) {
                if (rhs)
                    rhs.m_pObject->IncRef();
                if (m_pObject)
                    m_pObject->DecRef();

                m_pObject = rhs.m_pObject;
            }

            return *this;
        }

        NiPointer<T>& operator=(T* rhs)
        {
            if (m_pObject != rhs) {
                if (rhs)
                    rhs->IncRef();
                if (m_pObject)
                    m_pObject->DecRef();

                m_pObject = rhs;
            }

            return *this;
        }

        bool operator==(T* pObject) const { return m_pObject == pObject; }

        bool operator!=(T* pObject) const { return m_pObject != pObject; }

        bool operator==(const NiPointer& ptr) const { return m_pObject == ptr.m_pObject; }

        bool operator!=(const NiPointer& ptr) const { return m_pObject != ptr.m_pObject; }
    };

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

                return reinterpret_cast<T*>(iter->data);
            }
        };

        struct Ref
        {
            Entry* data;

            MEMBER_FN_PREFIX(Ref);
            // D3703E13297FD78BE317E0223C90DAB9021465DD+6F
            DEFINE_MEMBER_FN(ctor, Ref*, 0x01BC1650, const char* buf);
            // 34CA732E6B3C7BCD20DEFC8B3711427E5285FF82+AA
            DEFINE_MEMBER_FN(ctor_w, Ref*, 0x01BC2470, const wchar_t* buf);
            // 489C5F60950D108691FCB6CB0026101275BE474A+79
            DEFINE_MEMBER_FN(Set, Ref*, 0x01BC1780, const char* buf);
            DEFINE_MEMBER_FN(Set_w, Ref*, 0x01BC3CD0, const wchar_t* buf);

            DEFINE_MEMBER_FN(Release, void, 0x01BC28E0);

            Ref()
            {
                CALL_MEMBER_FN(this, ctor)("");
            }

            Ref(const char* buf)
            {
                CALL_MEMBER_FN(this, ctor)(buf);
            }

            Ref(const wchar_t* buf)
            {
                CALL_MEMBER_FN(this, ctor_w)(buf);
            }

            void Release()
            {
                CALL_MEMBER_FN(this, Release)();
            }

            bool operator==(const char* lhs) const
            {
                Ref tmp(lhs);
                bool res = data == tmp.data;
                CALL_MEMBER_FN(&tmp, Release)();
                return res;
            }

            bool operator==(const Ref& lhs) const { return data == lhs.data; }
            bool operator<(const Ref& lhs) const { return data < lhs.data; }

            const char* c_str() const { return operator const char*(); }
            operator const char*() const { return data ? data->Get<char>() : nullptr; }

            const wchar_t* wc_str() { return operator const wchar_t*(); }
            operator const wchar_t*() { return data ? data->Get<wchar_t>() : nullptr; }
        };

        // 10
        struct Lock
        {
            UInt32 unk00; // 00 - set to 80000000 when locked
            UInt32 pad04; // 04
            UInt64 pad08; // 08
        };

        Entry* lut[0x10000]; // 00000
        Lock lock[0x80]; // 80000
        UInt8 isInit; // 80800
    };

    typedef StringCache::Ref BSFixedString;

    // 18
    template <class T, int nGrow = 10, int nShrink = 10>
    class tArray
    {
    public:
        T* entries; // 00
        UInt32 capacity; // 08
        UInt32 pad0C; // 0C
        UInt32 count; // 10
        UInt32 pad14; // 14

        tArray() :
            entries(NULL), capacity(0), count(0), pad0C(0), pad14(0) {}

        T& operator[](UInt64 index) { return entries[index]; }

        void Clear()
        {
            Heap_Free(entries);
            entries = NULL;
            capacity = 0;
            count = 0;
        }

        bool Allocate(UInt32 numEntries)
        {
            entries = (T*)Heap_Allocate(sizeof(T) * numEntries);
            if (!entries)
                return false;

            for (UInt32 i = 0; i < numEntries; i++)
                new(&entries[i]) T;

            capacity = numEntries;
            count = numEntries;

            return true;
        }

        bool Resize(UInt32 numEntries)
        {
            if (numEntries == capacity)
                return false;

            if (!entries) {
                Allocate(numEntries);
                return true;
            }
            if (numEntries < capacity) {
                // Delete the truncated entries
                for (UInt32 i = numEntries; i < capacity; i++)
                    delete &entries[i];
            }

            T* newBlock = (T*)Heap_Allocate(sizeof(T) * numEntries); // Create a new block
            memmove_s(newBlock, sizeof(T) * numEntries, entries, sizeof(T) * numEntries); // Move the old memory to the new block
            if (numEntries > capacity) {
                // Fill in new remaining entries
                for (UInt32 i = capacity; i < numEntries; i++)
                    new(&entries[i]) T;
            }
            Heap_Free(entries); // Free the old block
            entries = newBlock; // Assign the new block
            capacity = numEntries; // Capacity is now the number of total entries in the block
            count = min(capacity, count); // Count stays the same, or is truncated to capacity
            return true;
        }

        bool Push(const T& entry)
        {
            if (!entries || count + 1 > capacity) {
                if (!Grow(nGrow))
                    return false;
            }

            entries[count] = entry;
            count++;
            return true;
        };

        bool Insert(UInt32 index, const T& entry)
        {
            if (!entries)
                return false;

            UInt32 lastSize = count;
            if (count + 1 > capacity) // Not enough space, grow
            {
                if (!Grow(nGrow))
                    return false;
            }

            if (index != lastSize) // Not inserting onto the end, need to move everything down
            {
                UInt32 remaining = count - index;
                memmove_s(&entries[index + 1], sizeof(T) * remaining, &entries[index], sizeof(T) * remaining); // Move the rest up
            }

            entries[index] = entry;
            count++;
            return true;
        };

        bool Remove(UInt32 index)
        {
            if (!entries || index >= count)
                return false;

            // This might not be right for pointer types...
            (&entries[index])->~T();

            if (index + 1 < count) {
                UInt32 remaining = count - index;
                memmove_s(&entries[index], sizeof(T) * remaining, &entries[index + 1], sizeof(T) * remaining); // Move the rest up
            }
            count--;

            if (capacity > count + nShrink)
                Shrink();

            return true;
        }

        bool Shrink()
        {
            if (!entries || count == capacity)
                return false;

            try {
                UInt32 newSize = count;
                T* oldArray = entries;
                T* newArray = (T*)Heap_Allocate(sizeof(T) * newSize); // Allocate new block
                memmove_s(newArray, sizeof(T) * newSize, entries, sizeof(T) * newSize); // Move the old block
                entries = newArray;
                capacity = count;
                Heap_Free(oldArray); // Free the old block
                return true;
            } catch (...) {
                return false;
            }

            return false;
        }

        bool Grow(UInt32 numEntries)
        {
            if (!entries) {
                entries = (T*)Heap_Allocate(sizeof(T) * numEntries);
                count = 0;
                capacity = numEntries;
                return true;
            }

            try {
                UInt32 oldSize = capacity;
                UInt32 newSize = oldSize + numEntries;
                T* oldArray = entries;
                T* newArray = (T*)Heap_Allocate(sizeof(T) * newSize); // Allocate new block
                if (oldArray)
                    memmove_s(newArray, sizeof(T) * newSize, entries, sizeof(T) * capacity); // Move the old block
                entries = newArray;
                capacity = newSize;

                if (oldArray)
                    Heap_Free(oldArray); // Free the old block

                for (UInt32 i = oldSize; i < newSize; i++) // Allocate the rest of the free blocks
                    new(&entries[i]) T;

                return true;
            } catch (...) {
                return false;
            }

            return false;
        }

        bool GetNthItem(UInt64 index, T& pT) const
        {
            if (index < count) {
                pT = entries[index];
                return true;
            }
            return false;
        }

        SInt64 GetItemIndex(T& pFind) const
        {
            for (UInt64 n = 0; n < count; n++) {
                T& pT = entries[n];
                if (pT == pFind)
                    return n;
            }
            return -1;
        }

        DEFINE_STATIC_HEAP(Heap_Allocate, Heap_Free)
    };

    // 30
    template <typename Item, typename Key = Item>
    class tHashSet
    {
        class _Entry
        {
        public:
            Item item;
            _Entry* next;

            _Entry() :
                next(NULL) {}

            bool IsFree() const { return next == NULL; }
            void SetFree() { next = NULL; }

            void Dump(void)
            {
                item.Dump();
                _MESSAGE("\t\tnext: %016I64X", next);
            }
        };

        // When creating a new tHashSet, init sentinel pointer with address of this entry
        static _Entry sentinel;

        void* unk00; // 000
        UInt32 unk_000; // 008
        UInt32 m_size; // 00C
        UInt32 m_freeCount; // 010
        UInt32 m_freeOffset; // 014
        _Entry* m_eolPtr; // 018
        UInt64 unk_018; // 020
        _Entry* m_entries; // 028

        _Entry* GetEntry(UInt32 hash) const { return (_Entry*)(((uintptr_t)m_entries) + sizeof(_Entry) * (hash & (m_size - 1))); }

        _Entry* GetEntryAt(UInt32 index) const { return (_Entry*)(((uintptr_t)m_entries) + sizeof(_Entry) * index); }

        _Entry* NextFreeEntry(void)
        {
            _Entry* result = NULL;

            if (m_freeCount == 0)
                return NULL;

            do {
                m_freeOffset = (m_size - 1) & (m_freeOffset - 1);
                _Entry* entry = GetEntryAt(m_freeOffset);

                if (entry->IsFree())
                    result = entry;
            } while (!result);

            m_freeCount--;

            return result;
        }

        enum InsertResult
        {
            kInsert_Duplicate = -1,
            kInsert_OutOfSpace = 0,
            kInsert_Success = 1
        };

        InsertResult Insert(Item* item)
        {
            if (!m_entries)
                return kInsert_OutOfSpace;

            Key k = (Key)*item;
            _Entry* targetEntry = GetEntry(Item::GetHash(&k));

            // Case 1: Target entry is free
            if (targetEntry->IsFree()) {
                targetEntry->item = *item;
                targetEntry->next = m_eolPtr;
                --m_freeCount;

                return kInsert_Success;
            }

            // -- Target entry is already in use

            // Case 2: Item already included
            _Entry* p = targetEntry;
            do {
                if (p->item == *item)
                    return kInsert_Duplicate;
                p = p->next;
            } while (p != m_eolPtr);

            // -- Either hash collision or bucket overlap

            _Entry* freeEntry = NextFreeEntry();
            // No more space?
            if (!freeEntry)
                return kInsert_OutOfSpace;

            // Original position of the entry that currently uses the target position
            k = (Key)targetEntry->item;
            p = GetEntry(Item::GetHash(&k));

            // Case 3a: Hash collision - insert new entry between target entry and successor
            if (targetEntry == p) {
                freeEntry->item = *item;
                freeEntry->next = targetEntry->next;
                targetEntry->next = freeEntry;

                return kInsert_Success;
            }
            // Case 3b: Bucket overlap
            else {
                while (p->next != targetEntry)
                    p = p->next;

                freeEntry->item = targetEntry->item;
                freeEntry->next = targetEntry->next;

                p->next = freeEntry;
                targetEntry->item = *item;
                targetEntry->next = m_eolPtr;

                return kInsert_Success;
            }
        }

        bool CopyEntry(_Entry* sourceEntry)
        {
            if (!m_entries)
                return false;

            Key k = (Key)sourceEntry->item;
            _Entry* targetEntry = GetEntry(Item::GetHash(&k));

            // Case 1: Target location is unused
            if (!targetEntry->next) {
                targetEntry->item = sourceEntry->item;
                targetEntry->next = m_eolPtr;
                --m_freeCount;

                return true;
            }

            // Target location is in use. Either hash collision or bucket overlap.

            _Entry* freeEntry = NextFreeEntry();
            k = (Key)targetEntry->item;
            _Entry* p = GetEntry(Item::GetHash(&k));

            // Case 2a: Hash collision - insert new entry between target entry and successor
            if (targetEntry == p) {
                freeEntry->item = sourceEntry->item;
                freeEntry->next = targetEntry->next;
                targetEntry->next = freeEntry;

                return true;
            }

            // Case 2b: Bucket overlap - forward until hash collision is found, then insert
            while (p->next != targetEntry)
                p = p->next;

            // Source entry takes position of target entry - not completely understood yet
            freeEntry->item = targetEntry->item;
            freeEntry->next = targetEntry->next;
            p->next = freeEntry;
            targetEntry->item = sourceEntry->item;
            targetEntry->next = m_eolPtr;

            return true;
        }

        void Grow(void)
        {
            UInt32 oldSize = m_size;
            UInt32 newSize = oldSize ? 2 * oldSize : 8;

            _Entry* oldEntries = m_entries;
            _Entry* newEntries = (_Entry*)Heap_Allocate(newSize * sizeof(_Entry));
            ASSERT(newEntries);

            m_entries = newEntries;
            m_size = m_freeCount = m_freeOffset = newSize;

            // Initialize new table data (clear next pointers)
            _Entry* p = newEntries;
            for (UInt32 i = 0; i < newSize; i++, p++)
                p->SetFree();

            // Copy old entries, free old table data
            if (oldEntries) {
                _Entry* p = oldEntries;
                for (UInt32 i = 0; i < oldSize; i++, p++)
                    if (p->next)
                        CopyEntry(p);
                Heap_Free(oldEntries);
            }
        }

    public:
        tHashSet() :
            m_size(0), m_freeCount(0), m_freeOffset(0), m_entries(NULL), m_eolPtr(&sentinel) {}

        ~tHashSet()
        {
            if (m_entries)
                Heap_Free(m_entries);
        }

        UInt32 Size() const { return m_size; }
        UInt32 FreeCount() const { return m_freeCount; }
        UInt32 FillCount() const { return m_size - m_freeCount; }

        Item* Find(Key* key) const
        {
            if (!m_entries)
                return NULL;

            _Entry* entry = GetEntry(Item::GetHash(key));
            if (!entry->next)
                return NULL;

            while (!(entry->item == *key)) {
                entry = entry->next;
                if (entry == m_eolPtr)
                    return NULL;
            }

            return &entry->item;
        }

        bool Add(Item* item)
        {
            InsertResult result;

            for (result = Insert(item); result == kInsert_OutOfSpace; result = Insert(item))
                Grow();

            return result == kInsert_Success;
        }

        bool Remove(Key* key)
        {
            if (!m_entries)
                return false;

            _Entry* entry = GetEntry(Item::GetHash(key));
            if (!entry->next)
                return NULL;

            _Entry* prevEntry = NULL;
            while (!(entry->item == *key)) {
                prevEntry = entry;
                entry = entry->next;
                if (entry == m_eolPtr)
                    return false;
            }

            // Remove tail?
            _Entry* nextEntry = entry->next;
            if (nextEntry == m_eolPtr) {
                if (prevEntry)
                    prevEntry->next = m_eolPtr;
                entry->next = NULL;
            } else {
                entry->item = nextEntry->item;
                entry->next = nextEntry->next;
                nextEntry->next = NULL;
            }

            ++m_freeCount;
            return true;
        }

        void Clear(void)
        {
            if (m_entries) {
                _Entry* p = m_entries;
                for (UInt32 i = 0; i < m_size; i++, p++)
                    p->next = NULL;
            } else {
                m_size = 0;
            }
            m_freeCount = m_freeOffset = m_size;
        }

        template <typename T>
        void ForEach(T& functor)
        {
            if (!m_entries)
                return;

            if (m_size == m_freeCount) // The whole thing is free
                return;

            _Entry* cur = m_entries;
            _Entry* end = GetEntryAt(m_size); // one index beyond the entries data to check if we reached that point

            if (cur == end)
                return;

            if (cur->IsFree()) {
                // Forward to first non-free entry
                do
                    cur++;
                while (cur != end && cur->IsFree());
            }

            do {
                if (!functor(&cur->item))
                    return;

                // Forward to next non-free entry
                do
                    cur++;
                while (cur != end && cur->IsFree());
            } while (cur != end);
        }

        void Dump(void)
        {
            // _MESSAGE("tHashSet:");
            // _MESSAGE("> size: %d", Size());
            // _MESSAGE("> free: %d", FreeCount());
            // _MESSAGE("> filled: %d", FillCount());
            if (m_entries) {
                _Entry* p = m_entries;
                for (UInt32 i = 0; i < m_size; i++, p++) {
                    _MESSAGE("* %d %s:", i, p->IsFree() ? "(free)" : "");
                    if (!p->IsFree())
                        p->Dump();
                }
            }
        }

        DEFINE_STATIC_HEAP(Heap_Allocate, Heap_Free)
    };

    template <typename Key, typename Item>
    typename tHashSet<Key, Item>::_Entry tHashSet<Key, Item>::sentinel = tHashSet<Key, Item>::_Entry();

    // 08
    class BSReadWriteLock
    {
        enum
        {
            kFastSpinThreshold = 10000,
            kLockWrite = 0x80000000,
            kLockCountMask = 0xFFFFFFF
        };

        volatile SInt32 threadID; // 00
        volatile SInt32 lockValue; // 04

    public:
        BSReadWriteLock() :
            threadID(0), lockValue(0) {}

        //void LockForRead();
        //void LockForWrite();

        DEFINE_MEMBER_FN_0(LockForRead, void, 0x01B932B0);
        DEFINE_MEMBER_FN_0(LockForWrite, void, 0x01B93330);

        void LockForReadAndWrite();

        bool TryLockForWrite();
        bool TryLockForRead();

        void Unlock();
    };

    static_assert(sizeof(BSReadWriteLock) == 0x8);
}
