#include <script/script.h>
#include <script/solver.h>
#include <test/util/str.h>

#include <boost/test/unit_test.hpp>

#include <string>

BOOST_AUTO_TEST_SUITE(p2pk_detect)

BOOST_AUTO_TEST_CASE(p2pk_testvectors_valid)
{
    // Generate valid vectors from OP_0 to OP_PUSHBYTES_75
    for (int op = OP_0; op < OP_PUSHDATA1; ++op) {
        CScript p2pk;
        if(op != OP_0) {
            std::vector<unsigned char> bytes(op, 0x00);
            p2pk << bytes << OP_CHECKSIG;
            BOOST_CHECK(p2pk.IsPayToPublicKey() == true);
        } else {
            p2pk << op << OP_CHECKSIG;
            BOOST_CHECK(p2pk.IsPayToPublicKey() == false);
        }
    }
}

BOOST_AUTO_TEST_CASE(p2pk_dual_ops_invalid)
{
    // Test double OP_CHECKSIG scripts
    for (int op = OP_0; op < OP_PUSHDATA1; ++op) {
        CScript p2pk;
        unsigned char n_op = OP_PUSHDATA1 - 4 - op;
        std::vector<unsigned char> bytes(op,0x00);
        std::vector<unsigned char> n_bytes(n_op,0xFF);
        p2pk << bytes << OP_CHECKSIG << n_bytes << OP_CHECKSIG;
        BOOST_CHECK(p2pk.IsPayToPublicKey() == false);
    }
}

BOOST_AUTO_TEST_SUITE_END()
