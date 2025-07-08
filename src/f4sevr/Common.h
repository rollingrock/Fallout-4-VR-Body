#pragma once

// ReSharper disable CppNonExplicitConvertingConstructor

namespace F4SEVR
{
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

                return static_cast<T*>(iter->data);
            }
        };
    };
}
