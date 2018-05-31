
#include "assets/assets.h"
#include <boost/test/unit_test.hpp>
#include <test/test_raven.h>

BOOST_FIXTURE_TEST_SUITE(cache_tests, BasicTestingSetup)


    const int NUM_OF_ASSETS1 = 100000;

BOOST_AUTO_TEST_CASE(cache_test)
{
    std::cout << "Testing cache test" << std::endl;
    CLRUCache<std::string, CNewAsset> cache(NUM_OF_ASSETS1);

    std::string assetName = "TEST";

    int counter = 0;
    while(cache.Size() != NUM_OF_ASSETS1)
    {
        CNewAsset asset(std::string(assetName + std::to_string(counter)), CAmount(1), 0, 0, 1, "43f81c6f2c0593bde5a85e09ae662816eca80797");

        cache.Put(asset.strName, asset);
        counter++;
    }

    BOOST_CHECK_MESSAGE(cache.Exists("TEST0"), "Didn't have TEST0");

    CNewAsset asset("THISWILLOVERWRITE", CAmount(1), 0, 0, 1, "43f81c6f2c0593bde5a85e09ae662816eca80797");

    cache.Put(asset.strName, asset);

    BOOST_CHECK_MESSAGE(cache.Exists("THISWILLOVERWRITE"), "New asset wasn't added to cache");
    BOOST_CHECK_MESSAGE(!cache.Exists("TEST0"), "Cache didn't remove the least recently used");
    BOOST_CHECK_MESSAGE(cache.Exists("TEST1"), "Cache didn't have TEST1");

}

BOOST_AUTO_TEST_SUITE_END()

