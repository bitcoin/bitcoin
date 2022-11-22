//
// This file is a part of minialloc, released under the MIT
// License (see COPYING.MIT)
//
// Copyright (c)        2022 The Dash Core developers
//               2009 - 2019 The Bitcoin Core developers
//               2009 - 2010 Satoshi Nakamoto
//

#include <loadstor.h>
#include <minialloc/export.h>
#include <support/lockedpool.h>
#include <support/cleanse.h>

#define POINTER_OFFSET 8              /* uint64_t */
#define GARBAGE_VALUE  0xd0           /* libsodium garbage value */

extern "C" {
__attribute__((malloc)) void*
msecure_malloc(const size_t size)
{
    void*       base_ptr        = nullptr; /* base pointer */
    void*       user_ptr        = nullptr; /* returned pointer */
    uint8_t*    transform_ptr   = nullptr; /* intermediate representation used to manipulate
                                              base pointer to store underlying datatype size */

    const uint64_t len = size;
    if (size + sizeof(len) < size) {
        return nullptr;
    }

    base_ptr = LockedPoolManager::Instance().alloc(size + sizeof(len));
    if (base_ptr == nullptr) {
        return nullptr;
    }

    transform_ptr = static_cast<uint8_t*>(base_ptr);
    store_le(len, transform_ptr);

    user_ptr = transform_ptr + POINTER_OFFSET;

    std::memset(user_ptr, (int)GARBAGE_VALUE, size);

    return user_ptr;
}

void
msecure_free(void *ptr)
{
    uint8_t*    base_ptr        = nullptr; /* base pointer */
    void*       user_ptr        = ptr;     /* returned pointer */

    if (user_ptr == nullptr) {
        return;
    }

    base_ptr = static_cast<uint8_t*>(ptr) - POINTER_OFFSET;

    const uint64_t len = load_le<uint64_t>(base_ptr, 0);
    memory_cleanse(base_ptr, len + sizeof(len));

    LockedPoolManager::Instance().free(base_ptr);
}
}
