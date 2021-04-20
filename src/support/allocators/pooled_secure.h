// Copyright (c) 2014-2020 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_SUPPORT_ALLOCATORS_POOLED_SECURE_H
#define BITCOIN_SUPPORT_ALLOCATORS_POOLED_SECURE_H

#include <support/lockedpool.h>
#include <support/cleanse.h>

#include <string>
#include <vector>

#include <boost/pool/pool_alloc.hpp>

//
// Allocator that allocates memory in chunks from a pool, which in turn allocates larger chunks from secure memory
// Memory is cleaned when freed as well. This allocator is NOT thread safe
//
template <typename T>
struct pooled_secure_allocator : public std::allocator<T> {
    // MSVC8 default copy constructor is broken
    typedef std::allocator<T> base;
    typedef typename base::size_type size_type;
    typedef typename base::difference_type difference_type;
    typedef typename base::pointer pointer;
    typedef typename base::const_pointer const_pointer;
    typedef typename base::reference reference;
    typedef typename base::const_reference const_reference;
    typedef typename base::value_type value_type;
    pooled_secure_allocator(const size_type nrequested_size = 32,
                            const size_type nnext_size = 32,
                            const size_type nmax_size = 0) noexcept :
                            pool(nrequested_size, nnext_size, nmax_size){}
    ~pooled_secure_allocator() noexcept {}

    T* allocate(std::size_t n, const void* hint = 0)
    {
        size_t chunks = (n * sizeof(T) + pool.get_requested_size() - 1) / pool.get_requested_size();
        return static_cast<T*>(pool.ordered_malloc(chunks));
    }

    void deallocate(T* p, std::size_t n)
    {
        if (!p) {
            return;
        }

        size_t chunks = (n * sizeof(T) + pool.get_requested_size() - 1) / pool.get_requested_size();
        memory_cleanse(p, chunks * pool.get_requested_size());
        pool.ordered_free(p, chunks);
    }

public:
    struct internal_secure_allocator {
        typedef std::size_t size_type;
        typedef std::ptrdiff_t difference_type;

        static char* malloc(const size_type bytes)
        {
            return static_cast<char*>(LockedPoolManager::Instance().alloc(bytes));
        }

        static void free(char* const block)
        {
            LockedPoolManager::Instance().free(block);
        }
    };
private:
    boost::pool<internal_secure_allocator> pool;
};

#endif // BITCOIN_SUPPORT_ALLOCATORS_POOLED_SECURE_H
