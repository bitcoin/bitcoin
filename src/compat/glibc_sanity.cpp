// Copyright (c) 2009-2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#if defined(HAVE_CONFIG_H)
#include <config/bitcoin-config.h>
#endif

#include <cstddef>

extern "C" void* memcpy(void* a, const void* b, size_t c);
void* memcpy_int(void* a, const void* b, size_t c)
{
    return memcpy(a, b, c);
}

namespace
{
// trigger: Use the memcpy_int wrapper which calls our internal memcpy.
//   A direct call to memcpy may be optimized away by the compiler.
// test: Fill an array with a sequence of integers. memcpy to a new empty array.
//   Verify that the arrays are equal. Use an odd size to decrease the odds of
//   the call being optimized away.
template <unsigned int T>
bool sanity_test_memcpy()
{
    unsigned int memcpy_test[T];
    unsigned int memcpy_verify[T] = {};
    for (unsigned int i = 0; i != T; ++i)
        memcpy_test[i] = i;

    memcpy_int(memcpy_verify, memcpy_test, sizeof(memcpy_test));

    for (unsigned int i = 0; i != T; ++i) {
        if (memcpy_verify[i] != i)
            return false;
    }
    return true;
}
} // namespace

bool glibc_sanity_test()
{
    return sanity_test_memcpy<1025>();
}
