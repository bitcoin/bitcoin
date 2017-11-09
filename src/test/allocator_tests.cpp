// Copyright (c) 2012-2015 The Bitcoin Core developers
// Copyright (c) 2015-2017 The Bitcoin Unlimited developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "util.h"

#include "support/allocators/secure.h"
#include "test/test_bitcoin.h"

#include <boost/test/unit_test.hpp>

BOOST_FIXTURE_TEST_SUITE(allocator_tests, BasicTestingSetup)

// Dummy memory page locker for platform independent tests
static const void *last_lock_addr, *last_unlock_addr;
static size_t last_lock_len, last_unlock_len;
class TestLocker
{
public:
    bool Lock(const void *addr, size_t len)
    {
        last_lock_addr = addr;
        last_lock_len = len;
        return true;
    }
    bool Unlock(const void *addr, size_t len)
    {
        last_unlock_addr = addr;
        last_unlock_len = len;
        return true;
    }
};

BOOST_AUTO_TEST_CASE(test_LockedPageManagerBase)
{
    const size_t test_page_size = 4096;
    LockedPageManagerBase<TestLocker> lpm(test_page_size);
    size_t addr;
    last_lock_addr = last_unlock_addr = 0;
    last_lock_len = last_unlock_len = 0;

    /* Try large number of small objects */
    addr = 0;
    for (int i = 0; i < 1000; ++i)
    {
        lpm.LockRange(reinterpret_cast<void *>(addr), 33);
        addr += 33;
    }
    /* Try small number of page-sized objects, straddling two pages */
    addr = test_page_size * 100 + 53;
    for (int i = 0; i < 100; ++i)
    {
        lpm.LockRange(reinterpret_cast<void *>(addr), test_page_size);
        addr += test_page_size;
    }
    /* Try small number of page-sized objects aligned to exactly one page */
    addr = test_page_size * 300;
    for (int i = 0; i < 100; ++i)
    {
        lpm.LockRange(reinterpret_cast<void *>(addr), test_page_size);
        addr += test_page_size;
    }
    /* one very large object, straddling pages */
    lpm.LockRange(reinterpret_cast<void *>(test_page_size * 600 + 1), test_page_size * 500);
    BOOST_CHECK(last_lock_addr == reinterpret_cast<void *>(test_page_size * (600 + 500)));
    /* one very large object, page aligned */
    lpm.LockRange(reinterpret_cast<void *>(test_page_size * 1200), test_page_size * 500 - 1);
    BOOST_CHECK(last_lock_addr == reinterpret_cast<void *>(test_page_size * (1200 + 500 - 1)));

    BOOST_CHECK(lpm.GetLockedPageCount() == ((1000 * 33 + test_page_size - 1) / test_page_size + // small objects
                                                101 + 100 + // page-sized objects
                                                501 + 500)); // large objects
    BOOST_CHECK((last_lock_len & (test_page_size - 1)) == 0); // always lock entire pages
    BOOST_CHECK(last_unlock_len == 0); // nothing unlocked yet

    /* And unlock again */
    addr = 0;
    for (int i = 0; i < 1000; ++i)
    {
        lpm.UnlockRange(reinterpret_cast<void *>(addr), 33);
        addr += 33;
    }
    addr = test_page_size * 100 + 53;
    for (int i = 0; i < 100; ++i)
    {
        lpm.UnlockRange(reinterpret_cast<void *>(addr), test_page_size);
        addr += test_page_size;
    }
    addr = test_page_size * 300;
    for (int i = 0; i < 100; ++i)
    {
        lpm.UnlockRange(reinterpret_cast<void *>(addr), test_page_size);
        addr += test_page_size;
    }
    lpm.UnlockRange(reinterpret_cast<void *>(test_page_size * 600 + 1), test_page_size * 500);
    lpm.UnlockRange(reinterpret_cast<void *>(test_page_size * 1200), test_page_size * 500 - 1);

    /* Check that everything is released */
    BOOST_CHECK(lpm.GetLockedPageCount() == 0);

    /* A few and unlocks of size zero (should have no effect) */
    addr = 0;
    for (int i = 0; i < 1000; ++i)
    {
        lpm.LockRange(reinterpret_cast<void *>(addr), 0);
        addr += 1;
    }
    BOOST_CHECK(lpm.GetLockedPageCount() == 0);
    addr = 0;
    for (int i = 0; i < 1000; ++i)
    {
        lpm.UnlockRange(reinterpret_cast<void *>(addr), 0);
        addr += 1;
    }
    BOOST_CHECK(lpm.GetLockedPageCount() == 0);
    BOOST_CHECK((last_unlock_len & (test_page_size - 1)) == 0); // always unlock entire pages
}

BOOST_AUTO_TEST_SUITE_END()
