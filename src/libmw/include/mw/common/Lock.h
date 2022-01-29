#pragma once

// Copyright (c) 2018-2019 David Burkett
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.

#include <memory>
#include <shared_mutex>
#include <mutex>
#include <tuple>

template<class T>
class Reader
{
    template<class U>
    class InnerReader
    {
    public:
        InnerReader(std::shared_ptr<const U> pObject, std::shared_ptr<std::shared_timed_mutex> pMutex, const bool lock, const bool unlock)
            : m_pObject(pObject), m_pMutex(pMutex), m_unlock(unlock)
        {
            if (lock)
            {
                m_pMutex->lock_shared();
            }
        }

        ~InnerReader()
        {
            if (m_unlock)
            {
                m_pMutex->unlock_shared();
            }
        }

        std::shared_ptr<const U> m_pObject;
        std::shared_ptr<std::shared_timed_mutex> m_pMutex;
        bool m_unlock;
    };

public:
    static Reader Create(std::shared_ptr<T> pObject, std::shared_ptr<std::shared_timed_mutex> pMutex, const bool lock, const bool unlock)
    {
        return Reader(std::shared_ptr<InnerReader<T>>(new InnerReader<T>(pObject, pMutex, lock, unlock)));
    }

    Reader() = default;
    virtual ~Reader() = default;

    const T* operator->() const
    {
        return m_pReader->m_pObject.get();
    }

    const T& operator*() const
    {
        return *m_pReader->m_pObject;
    }

    std::shared_ptr<const T> GetShared() const
    {
        return m_pReader->m_pObject;
    }

    bool IsNull() const
    {
        return m_pReader->m_pObject == nullptr;
    }

private:
    Reader(std::shared_ptr<InnerReader<T>> pReader)
        : m_pReader(pReader)
    {

    }

    std::shared_ptr<InnerReader<T>> m_pReader;
};

class MutexUnlocker
{
public:
    MutexUnlocker(std::shared_ptr<std::shared_timed_mutex> pMutex)
        : m_pMutex(pMutex)
    {

    }

    ~MutexUnlocker()
    {
        m_pMutex->unlock();
    }

private:
    std::shared_ptr<std::shared_timed_mutex> m_pMutex;
};

template<class T>
class Writer : virtual public Reader<T>
{
    template<class U>
    class InnerWriter
    {
    public:
        InnerWriter(std::shared_ptr<U> pObject, std::shared_ptr<std::shared_timed_mutex> pMutex, const bool lock)
            : m_pObject(pObject), m_pMutex(pMutex)
        {
            if (lock)
            {
                m_pMutex->lock();
            }
        }

        virtual ~InnerWriter()
        {
            // Using MutexUnlocker in case exception is thrown.
            MutexUnlocker unlocker(m_pMutex);
        }

        std::shared_ptr<U> m_pObject;
        std::shared_ptr<std::shared_timed_mutex> m_pMutex;
    };

public:
    static Writer Create(std::shared_ptr<T> pObject, std::shared_ptr<std::shared_timed_mutex> pMutex, const bool lock)
    {
        return Writer(std::shared_ptr<InnerWriter<T>>(new InnerWriter<T>(pObject, pMutex, lock)));
    }

    Writer() = default;
    virtual ~Writer() = default;

    T* operator->()
    {
        return m_pWriter->m_pObject.get();
    }

    const T* operator->() const
    {
        return m_pWriter->m_pObject.get();
    }

    T& operator*()
    {
        return *m_pWriter->m_pObject;
    }

    const T& operator*() const
    {
        return *m_pWriter->m_pObject;
    }

    std::shared_ptr<T> GetShared()
    {
        return m_pWriter->m_pObject;
    }

    std::shared_ptr<const T> GetShared() const
    {
        return m_pWriter->m_pObject;
    }

    bool IsNull() const
    {
        return m_pWriter == nullptr;
    }

    void Clear()
    {
        m_pWriter = nullptr;
    }

private:
    Writer(std::shared_ptr<InnerWriter<T>> pWriter)
        : Reader<T>(Reader<T>::Create(pWriter->m_pObject, pWriter->m_pMutex, false, false)), m_pWriter(pWriter)
    {

    }

    std::shared_ptr<InnerWriter<T>> m_pWriter;
};

template<class T>
class Locked
{
    friend class MultiLocker;

public:
    Locked(std::shared_ptr<T> pObject)
        : m_pObject(pObject), m_pMutex(std::make_shared<std::shared_timed_mutex>())
    {

    }

    virtual ~Locked() = default;

    Reader<T> Read() const
    {
        return Reader<T>::Create(m_pObject, m_pMutex, true, true);
    }

    Reader<T> Read(std::adopt_lock_t) const
    {
        return Reader<T>::Create(m_pObject, m_pMutex, false, true);
    }

    Writer<T> Write()
    {
        return Writer<T>::Create(m_pObject, m_pMutex, true);
    }

    Writer<T> Write(std::adopt_lock_t)
    {
        return Writer<T>::Create(m_pObject, m_pMutex, false);
    }

private:
    std::shared_ptr<T> m_pObject;
    std::shared_ptr<std::shared_timed_mutex> m_pMutex;
};