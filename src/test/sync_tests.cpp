#include <boost/test/unit_test.hpp>

#include "sync.h"
#include "util.h"

BOOST_AUTO_TEST_SUITE(sync_tests)

CCriticalSection cs;
int nIsInCS = 0;
bool fIsInSharedCS = false;
bool fIsInUpgradeCS = false;
bool fThreadOneDone = false;
bool fThreadTwoDone = false;

void ThreadOne(void* parg)
{
    {
        LOCK(cs);
        nIsInCS++;
        Sleep(100);
        nIsInCS--;
    }
    Sleep(50);
    BOOST_CHECK_MESSAGE(nIsInCS != 0, "LOCK doesnt unlock.");
    BOOST_CHECK_MESSAGE(nIsInCS != 1, "Multiple LOCKs in a thread deadlocks.");
    BOOST_CHECK_MESSAGE(nIsInCS == 2, "See previous error, or int++/-- is broken.");
    {
        LOCK(cs);
    }
    BOOST_CHECK_MESSAGE(nIsInCS == 0, "Recursive lock unlocks fully after first unlock.");
    // Finished first test set (200ms in Sleeps)
    Sleep(50);
    {
        SHARED_LOCK(cs);
        BOOST_CHECK_MESSAGE(fIsInSharedCS, "Cant get a shared lock.");
        {
            LOCK(cs);
            BOOST_CHECK_MESSAGE(!fIsInSharedCS, "Lock upgrade from shared doesn't wait.");
            nIsInCS++;
            Sleep(100);
            nIsInCS--;
        }
    }
    Sleep(25);
    {
        SHARED_LOCK(cs);
        BOOST_CHECK_MESSAGE(nIsInCS == 0, "SHARED_LOCK doesn't wait for LOCK.");
    }
    // Finished second test set (300ms in Sleeps)
    Sleep(25);
    {
        UPGRADE_LOCK(cs);
        fIsInUpgradeCS = true;
        Sleep(75);
        {
            LOCK(cs);
            BOOST_CHECK_MESSAGE(!fIsInSharedCS, "Lock upgrade from upgrade doesn't wait.");
            nIsInCS++;
            Sleep(50);
            nIsInCS--;
        }
        Sleep(50);
        fIsInUpgradeCS = false;
    }
    // Finished third test set (225ms in Sleeps)
    fThreadOneDone = true;
}

void ThreadTwo(void* parg)
{
    Sleep(50);
    {
        LOCK(cs);
        BOOST_CHECK_MESSAGE(nIsInCS == 0, "LOCK doesn't lock.");
        nIsInCS++;
        {
            LOCK(cs);
            nIsInCS++;
            Sleep(50);
            nIsInCS--;
        }
        Sleep(50);
        nIsInCS--;
    }
    // Finished first test set (200ms in Sleeps)
    Sleep(25);
    {
        SHARED_LOCK(cs);
        fIsInSharedCS = true;
        Sleep(75);
        fIsInSharedCS = false;
    }
    Sleep(25);
    {
        LOCK(cs);
        BOOST_CHECK_MESSAGE(nIsInCS == 0, "LOCK doesn't lock after SHARED_LOCK.");
        nIsInCS++;
        Sleep(100);
        nIsInCS--;
    }
    // Finished second test set (300ms in Sleeps)
    Sleep(50);
    {
        SHARED_LOCK(cs);
        BOOST_CHECK_MESSAGE(fIsInUpgradeCS, "SHARED_LOCK waits for UPGRADE_LOCK.");
        fIsInSharedCS = true;
        Sleep(75);
        fIsInSharedCS = false;
    }
    {
        UPGRADE_LOCK(cs);
        BOOST_CHECK_MESSAGE(nIsInCS == 0, "UPGRADE_LOCK doesn't wait for LOCK.");
        BOOST_CHECK_MESSAGE(!fIsInUpgradeCS, "UPGRADE_LOCK is non-exclusive.");
    }
    // Finished third test set (225ms in Sleeps)
    fThreadTwoDone = true;
}

BOOST_AUTO_TEST_CASE(lock_tests)
{
    BOOST_CHECK_MESSAGE(CreateThread(ThreadOne, NULL), "CreateThread Failed!");
    BOOST_CHECK_MESSAGE(CreateThread(ThreadTwo, NULL), "CreateThread Failed!");
    Sleep(750);
    BOOST_CHECK_MESSAGE(fThreadOneDone, "CreateThread doesn't create thread or LOCK/SHARED_LOCK deadlocks.");
    BOOST_CHECK_MESSAGE(fThreadTwoDone, "CreateThread doesn't create thread or LOCK/SHARED_LOCK deadlocks.");
}

BOOST_AUTO_TEST_SUITE_END()
