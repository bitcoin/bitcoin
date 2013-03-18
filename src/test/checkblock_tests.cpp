//
// Unit tests for block.CheckBlock()
//
#include <algorithm>

#include <boost/assign/list_of.hpp> // for 'map_list_of()'
#include <boost/date_time/posix_time/posix_time_types.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/foreach.hpp>

#include "main.h"
#include "wallet.h"
#include "net.h"
#include "util.h"

BOOST_AUTO_TEST_SUITE(CheckBlock_tests)

bool
read_block(const std::string& filename, CBlock& block)
{
    namespace fs = boost::filesystem;
    fs::path testFile = fs::current_path() / "test" / "data" / filename;
#ifdef TEST_DATA_DIR
    if (!fs::exists(testFile))
    {
        testFile = fs::path(BOOST_PP_STRINGIZE(TEST_DATA_DIR)) / filename;
    }
#endif
    FILE* fp = fopen(testFile.string().c_str(), "rb");
    if (!fp) return false;

    fseek(fp, 8, SEEK_SET); // skip msgheader/size

    CAutoFile filein = CAutoFile(fp, SER_DISK, CLIENT_VERSION);
    if (!filein) return false;

    filein >> block;

    return true;
}

BOOST_AUTO_TEST_CASE(May15)
{
    // Putting a 1MB binary file in the git repository is not a great
    // idea, so this test is only run if you manually download
    // test/data/Mar12Fork.dat from
    // http://sourceforge.net/projects/bitcoin/files/Bitcoin/blockchain/Mar12Fork.dat/download
    unsigned int tMay15 = 1368576000;
    SetMockTime(tMay15); // Test as if it was right at May 15

    CBlock forkingBlock;
    if (read_block("Mar12Fork.dat", forkingBlock))
    {
        CValidationState state;
        forkingBlock.nTime = tMay15-1; // Invalidates PoW
        BOOST_CHECK(!forkingBlock.CheckBlock(state, false, false));

        // After May 15'th, big blocks are OK:
        forkingBlock.nTime = tMay15; // Invalidates PoW
        BOOST_CHECK(forkingBlock.CheckBlock(state, false, false));
    }

    SetMockTime(0);
}

BOOST_AUTO_TEST_SUITE_END()
