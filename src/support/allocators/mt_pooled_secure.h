// Copyright (c) 2014-2018 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_SUPPORT_ALLOCATORS_MT_POOLED_SECURE_H
#define BITCOIN_SUPPORT_ALLOCATORS_MT_POOLED_SECURE_H

#include "pooled_secure.h"

#include <thread>
#include <mutex>

//
// Manages a pool of pools to balance allocation between those when multiple threads are involved
// This allocator is fully thread safe
//
template <typename T>
struct mt_pooled_secure_allocator : public std::allocator<T> {
    // MSVC8 default copy constructor is broken
    typedef std::allocator<T> base;
    typedef typename base::size_type size_type;
    typedef typename base::difference_type difference_type;
    typedef typename base::pointer pointer;
    typedef typename base::const_pointer const_pointer;
    typedef typename base::reference reference;
    typedef typename base::const_reference const_reference;
    typedef typename base::value_type value_type;
    mt_pooled_secure_allocator(size_type nrequested_size = 32,
                               size_type nnext_size = 32,
                               size_type nmax_size = 0) throw()
    {
        // we add enough bytes to the requested size so that we can store the bucket as well
        nrequested_size += sizeof(size_t);

        size_t pools_count = std::thread::hardware_concurrency();
        pools.resize(pools_count);
        for (size_t i = 0; i < pools_count; i++) {
            pools[i] = std::make_unique<internal_pool>(nrequested_size, nnext_size, nmax_size);
        }
    }
    ~mt_pooled_secure_allocator() throw() {}

    T* allocate(std::size_t n, const void* hint = 0)
    {
        size_t bucket = get_bucket();
        std::lock_guard<std::mutex> lock(pools[bucket]->mutex);
        uint8_t* ptr = pools[bucket]->allocate(n * sizeof(T) + sizeof(size_t));
        *(size_t*)ptr = bucket;
        return static_cast<T*>(ptr + sizeof(size_t));
    }

    void deallocate(T* p, std::size_t n)
    {
        if (!p) {
            return;
        }
        uint8_t* ptr = (uint8_t*)p - sizeof(size_t);
        size_t bucket = *(size_t*)ptr;
        std::lock_guard<std::mutex> lock(pools[bucket]->mutex);
        pools[bucket]->deallocate(ptr, n * sizeof(T));
    }

private:
    size_t get_bucket()
    {
        auto tid = std::this_thread::get_id();
        size_t x = std::hash<std::thread::id>{}(std::this_thread::get_id());
        return x % pools.size();
    }

    struct internal_pool : pooled_secure_allocator<uint8_t> {
        internal_pool(size_type nrequested_size,
                      size_type nnext_size,
                      size_type nmax_size) :
                      pooled_secure_allocator(nrequested_size, nnext_size, nmax_size)
        {
        }
        std::mutex mutex;
    };

private:
    std::vector<std::unique_ptr<internal_pool>> pools;
};

#endif // BITCOIN_SUPPORT_ALLOCATORS_MT_POOLED_SECURE_H
