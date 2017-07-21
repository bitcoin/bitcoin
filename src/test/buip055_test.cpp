// Copyright (c) 2015-2017 The Bitcoin Unlimited developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "data/tx_invalid.json.h"
#include "data/tx_valid.json.h"
#include "test/test_bitcoin.h"

#include "clientversion.h"
#include "consensus/validation.h"
#include "core_io.h"
#include "key.h"
#include "keystore.h"
#include "main.h" // For CheckTransaction
#include "policy/policy.h"
#include "script/script.h"
#include "script/script_error.h"
#include "utilstrencodings.h"
#include "buip055fork.h"

#include <map>
#include <string>

#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/assign/list_of.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/assign/list_of.hpp>

#include <univalue.h>

BOOST_FIXTURE_TEST_SUITE(buip055_tests, BasicTestingSetup)

BOOST_AUTO_TEST_CASE(buip055_op_return)
{

    // Check that a transaction with the invalid OP_RETURN is seen as invalid
    CMutableTransaction tx;
    tx.vout.resize(1);
    tx.vout[0].nValue = 0;
    tx.vout[0].scriptPubKey = CScript() << OP_RETURN << invalidOpReturn;
    BOOST_CHECK(IsTxOpReturnInvalid(tx) == true);

    CKey key;
    key.MakeNewKey(true);

    // Check that an arbitrary OP_RETURN is not invalid
    tx.vout[0].scriptPubKey = CScript() << OP_RETURN << ParseHex("010203040506070809");
    BOOST_CHECK(IsTxOpReturnInvalid(tx) == false);

    // Check that a normal tx (without OP_RETURN) is not invalid
    tx.vout[0].nValue = 50000;
    tx.vout[0].scriptPubKey = CScript() << OP_DUP << OP_HASH160 << ToByteVector(key.GetPubKey().GetID()) << OP_EQUALVERIFY << OP_CHECKSIG;
    BOOST_CHECK(IsTxOpReturnInvalid(tx) == false);

    // Check that a normal tx with the invalid OP_RETURN is invalid
    tx.vout.resize(2);
    tx.vout[1].scriptPubKey = CScript() << OP_RETURN << invalidOpReturn;
    tx.vout[1].nValue = 0;
    BOOST_CHECK(IsTxOpReturnInvalid(tx) == true);

    tx.vout[1].scriptPubKey = CScript() << OP_DUP << OP_HASH160 << ToByteVector(key.GetPubKey().GetID()) << OP_EQUALVERIFY << OP_RETURN << invalidOpReturn;
    tx.vout[1].nValue = 0;
    BOOST_CHECK(IsTxOpReturnInvalid(tx) == true);

    tx.vout[1].scriptPubKey = CScript() << OP_DUP << OP_HASH160 << ToByteVector(key.GetPubKey().GetID()) << OP_EQUALVERIFY << invalidOpReturn;
    tx.vout[1].nValue = 0;
    BOOST_CHECK(IsTxOpReturnInvalid(tx) == false);

    tx.vout[1].scriptPubKey = CScript() << OP_DUP << OP_HASH160 << ToByteVector(key.GetPubKey().GetID()) << OP_EQUALVERIFY << OP_RETURN << ParseHex("0") << invalidOpReturn;
    BOOST_CHECK(IsTxOpReturnInvalid(tx) == false);

}



BOOST_AUTO_TEST_SUITE_END()
