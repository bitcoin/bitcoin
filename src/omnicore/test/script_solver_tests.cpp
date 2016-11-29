#include "omnicore/script.h"

#include "script/script.h"
#include "test/test_bitcoin.h"
#include "util.h"

#include <boost/test/unit_test.hpp>

#include <vector>

/** To be set in the tests. */
extern unsigned nMaxDatacarrierBytes;

BOOST_FIXTURE_TEST_SUITE(omnicore_script_solver_tests, BasicTestingSetup)

/** Checks whether the custom solver is unaffected by user settings. */
static void CheckOutputType(const CScript& script, txnouttype outTypeExpected)
{
    // Explicit check via SafeSolver
    txnouttype outTypeExplicit;
    std::vector<std::vector<unsigned char> > vSolutions;
    BOOST_CHECK(SafeSolver(script, outTypeExplicit, vSolutions));
    BOOST_CHECK_EQUAL(outTypeExplicit, outTypeExpected);

    // Implicit check via GetOutputType
    txnouttype outTypeImplicit;
    BOOST_CHECK(GetOutputType(script, outTypeImplicit));
    BOOST_CHECK_EQUAL(outTypeImplicit, outTypeExpected);
}

BOOST_AUTO_TEST_CASE(solve_op_return_test)
{
    // Store initial data carrier size
    unsigned nMaxDatacarrierBytesOriginal = nMaxDatacarrierBytes;

    // Empty data script without payload
    CScript scriptNoData;
    scriptNoData << OP_RETURN;

    // Data script with standard sized 40 byte payload
    CScript scriptDefault;
    scriptDefault << OP_RETURN << std::vector<unsigned char>(40, 0x40);

    // Data script with large 80 byte payload
    CScript scriptLarge;
    scriptLarge << OP_RETURN << std::vector<unsigned char>(80, 0x80);

    // Data script with extremely large 2500 byte payload
    CScript scriptExtreme;
    scriptExtreme << OP_RETURN << std::vector<unsigned char>(2500, 0x07);

    // Default settings
    CheckOutputType(scriptNoData, TX_NULL_DATA);
    CheckOutputType(scriptDefault, TX_NULL_DATA);
    CheckOutputType(scriptLarge, TX_NULL_DATA);
    CheckOutputType(scriptExtreme, TX_NULL_DATA);

    // Tests with different data carrier size settings following ...
    nMaxDatacarrierBytes = 0;
    CheckOutputType(scriptNoData, TX_NULL_DATA);
    CheckOutputType(scriptDefault, TX_NULL_DATA);
    CheckOutputType(scriptLarge, TX_NULL_DATA);
    CheckOutputType(scriptExtreme, TX_NULL_DATA);

    nMaxDatacarrierBytes = 40;
    CheckOutputType(scriptNoData, TX_NULL_DATA);
    CheckOutputType(scriptDefault, TX_NULL_DATA);
    CheckOutputType(scriptLarge, TX_NULL_DATA);
    CheckOutputType(scriptExtreme, TX_NULL_DATA);

    nMaxDatacarrierBytes = 120;
    CheckOutputType(scriptNoData, TX_NULL_DATA);
    CheckOutputType(scriptDefault, TX_NULL_DATA);
    CheckOutputType(scriptLarge, TX_NULL_DATA);
    CheckOutputType(scriptExtreme, TX_NULL_DATA);

    nMaxDatacarrierBytes = 2500;
    CheckOutputType(scriptNoData, TX_NULL_DATA);
    CheckOutputType(scriptDefault, TX_NULL_DATA);
    CheckOutputType(scriptLarge, TX_NULL_DATA);
    CheckOutputType(scriptExtreme, TX_NULL_DATA);

    // Restore original data carrier size settings
    nMaxDatacarrierBytes = nMaxDatacarrierBytesOriginal;
}


BOOST_AUTO_TEST_SUITE_END()
