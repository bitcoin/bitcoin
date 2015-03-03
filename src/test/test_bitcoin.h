#ifndef BITCOIN_TEST_TEST_BITCOIN_H
#define BITCOIN_TEST_TEST_BITCOIN_H

#include "txdb.h"

#include <boost/filesystem.hpp>
#include <boost/thread.hpp>

struct TestingSetup {
    CCoinsViewDB *pcoinsdbview;
    boost::filesystem::path pathTemp;
    boost::thread_group threadGroup;

    TestingSetup();
    ~TestingSetup();
};

#endif
