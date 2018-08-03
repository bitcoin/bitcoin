#include <string>
#include <vector>

#include <boost/test/unit_test.hpp>
#include <rapidcheck/boost_test.h>
#include <rapidcheck/gen/Arbitrary.h>
#include <rapidcheck/Gen.h>

#include "script/script.h"
#include "test/test_bitcoin.h"
#include "test/gen/script_gen.h"

BOOST_FIXTURE_TEST_SUITE(script_properties, BasicTestingSetup)

/** Check CScript serialization symmetry */
RC_BOOST_PROP(cscript_serialization_symmetry, (const CScript& script))
{
    CDataStream ss(SER_NETWORK, PROTOCOL_VERSION);
    ss << static_cast<const CScriptBase&>(script);
    std::vector<unsigned char> deserialized;
    ss >> deserialized;
    CScript script2 = CScript(deserialized.begin(), deserialized.end());
    RC_ASSERT(script == script2);
}

BOOST_AUTO_TEST_SUITE_END()
