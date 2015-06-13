#include "omnicore/test/utils_tx.h"

#include "omnicore/omnicore.h"
#include "omnicore/script.h"

#include "primitives/transaction.h"

#include <limits>

#include <boost/test/unit_test.hpp>

using namespace mastercore;

BOOST_AUTO_TEST_SUITE(omnicore_marker_tests)

BOOST_AUTO_TEST_CASE(class_no_marker)
{
    {
        int nBlock = std::numeric_limits<int>().max();

        CMutableTransaction mutableTx;
        CTransaction tx(mutableTx);

        BOOST_CHECK_EQUAL(GetEncodingClass(tx, nBlock), NO_MARKER);
    }
    {
        int nBlock = 0;

        CMutableTransaction mutableTx;
        mutableTx.vout.push_back(OpReturn_Unrelated());
        mutableTx.vout.push_back(PayToPubKeyHash_Unrelated());
        mutableTx.vout.push_back(OpReturn_Tagged_C());
        mutableTx.vout.push_back(NonStandardOutput());
        mutableTx.vout.push_back(OpReturn_Tagged_B());
        mutableTx.vout.push_back(OpReturn_UnrelatedShort());
        mutableTx.vout.push_back(OpReturn_Empty());
        mutableTx.vout.push_back(PayToPubKey_Unrelated());
        mutableTx.vout.push_back(PayToScriptHash_Unrelated());
        mutableTx.vout.push_back(OpReturn_Tagged_A());

        CTransaction tx(mutableTx);
        BOOST_CHECK_EQUAL(GetEncodingClass(tx, nBlock), NO_MARKER);
    }
    {
        int nBlock = 0;

        CMutableTransaction mutableTx;
        mutableTx.vout.push_back(OpReturn_Unrelated());
        mutableTx.vout.push_back(PayToPubKeyHash_Unrelated());
        mutableTx.vout.push_back(OpReturn_Tagged_C());
        mutableTx.vout.push_back(NonStandardOutput());
        mutableTx.vout.push_back(OpReturn_Tagged_B());
        mutableTx.vout.push_back(OpReturn_UnrelatedShort());
        mutableTx.vout.push_back(OpReturn_Empty());
        mutableTx.vout.push_back(PayToPubKey_Unrelated());
        mutableTx.vout.push_back(PayToScriptHash_Unrelated());
        mutableTx.vout.push_back(OpReturn_Tagged_A());

        CTransaction tx(mutableTx);
        BOOST_CHECK_EQUAL(GetEncodingClass(tx, nBlock), NO_MARKER);
    }
    {
        int nBlock = std::numeric_limits<int>().max();

        CMutableTransaction mutableTx;
        mutableTx.vout.push_back(PayToPubKeyHash_Unrelated());
        mutableTx.vout.push_back(OpReturn_UnrelatedShort());
        mutableTx.vout.push_back(OpReturn_Unrelated());
        mutableTx.vout.push_back(PayToScriptHash_Unrelated());
        mutableTx.vout.push_back(PayToPubKey_Unrelated());
        mutableTx.vout.push_back(OpReturn_Empty());
        mutableTx.vout.push_back(NonStandardOutput());

        CTransaction tx(mutableTx);
        BOOST_CHECK_EQUAL(GetEncodingClass(tx, nBlock), NO_MARKER);
    }
    {
        int nBlock = std::numeric_limits<int>().max();

        CMutableTransaction mutableTx;
        mutableTx.vout.push_back(PayToPubKey_Unrelated());
        mutableTx.vout.push_back(PayToBareMultisig_3of5());
        mutableTx.vout.push_back(OpReturn_Unrelated());
        mutableTx.vout.push_back(NonStandardOutput());
        mutableTx.vout.push_back(OpReturn_UnrelatedShort());
        mutableTx.vout.push_back(PayToPubKeyHash_Unrelated());
        mutableTx.vout.push_back(PayToScriptHash_Unrelated());
        mutableTx.vout.push_back(OpReturn_Empty());
        mutableTx.vout.push_back(PayToBareMultisig_1of3());

        CTransaction tx(mutableTx);
        BOOST_CHECK_EQUAL(GetEncodingClass(tx, nBlock), NO_MARKER);
    }
}

BOOST_AUTO_TEST_CASE(class_class_a)
{
    {
        int nBlock = 0;

        CMutableTransaction mutableTx;
        mutableTx.vout.push_back(PayToPubKeyHash_Exodus());

        CTransaction tx(mutableTx);
        BOOST_CHECK_EQUAL(GetEncodingClass(tx, nBlock), OMNI_CLASS_A);
    }
    {
        int nBlock = P2SH_BLOCK;

        CMutableTransaction mutableTx;
        mutableTx.vout.push_back(PayToPubKey_Unrelated());
        mutableTx.vout.push_back(PayToPubKeyHash_Exodus());
        mutableTx.vout.push_back(PayToScriptHash_Unrelated());

        CTransaction tx(mutableTx);
        BOOST_CHECK_EQUAL(GetEncodingClass(tx, nBlock), OMNI_CLASS_A);
    }
    {
        int nBlock = 0;

        CMutableTransaction mutableTx;
        mutableTx.vout.push_back(OpReturn_UnrelatedShort());
        mutableTx.vout.push_back(PayToPubKeyHash_ExodusCrowdsale(nBlock));
        mutableTx.vout.push_back(PayToPubKeyHash_Exodus());
        mutableTx.vout.push_back(OpReturn_Empty());
        mutableTx.vout.push_back(PayToPubKey_Unrelated());
        mutableTx.vout.push_back(OpReturn_Tagged_C());
        mutableTx.vout.push_back(PayToPubKeyHash_Unrelated());
        mutableTx.vout.push_back(NonStandardOutput());
        mutableTx.vout.push_back(OpReturn_Tagged_B());
        mutableTx.vout.push_back(PayToScriptHash_Unrelated());
        mutableTx.vout.push_back(OpReturn_Tagged_A());
        mutableTx.vout.push_back(OpReturn_Unrelated());

        CTransaction tx(mutableTx);
        BOOST_CHECK_EQUAL(GetEncodingClass(tx, nBlock), OMNI_CLASS_A);
    }
    {
        int nBlock = std::numeric_limits<int>().max();

        CMutableTransaction mutableTx;
        mutableTx.vout.push_back(NonStandardOutput());
        mutableTx.vout.push_back(OpReturn_Empty());
        mutableTx.vout.push_back(PayToPubKeyHash_Exodus());
        mutableTx.vout.push_back(PayToPubKey_Unrelated());
        mutableTx.vout.push_back(OpReturn_Unrelated());
        mutableTx.vout.push_back(OpReturn_UnrelatedShort());
        mutableTx.vout.push_back(PayToPubKeyHash_ExodusCrowdsale(nBlock));
        mutableTx.vout.push_back(PayToPubKeyHash_Unrelated());
        mutableTx.vout.push_back(PayToScriptHash_Unrelated());

        CTransaction tx(mutableTx);
        BOOST_CHECK_EQUAL(GetEncodingClass(tx, nBlock), OMNI_CLASS_A);
    }
}

BOOST_AUTO_TEST_CASE(class_class_b)
{
    {
        int nBlock = 0;

        CMutableTransaction mutableTx;
        mutableTx.vout.push_back(PayToPubKeyHash_Exodus());
        mutableTx.vout.push_back(PayToBareMultisig_1of3());

        CTransaction tx(mutableTx);
        BOOST_CHECK_EQUAL(GetEncodingClass(tx, nBlock), OMNI_CLASS_B);
    }
    {
        int nBlock = OP_RETURN_BLOCK;

        CMutableTransaction mutableTx;
        mutableTx.vout.push_back(PayToPubKey_Unrelated());
        mutableTx.vout.push_back(PayToBareMultisig_3of5());
        mutableTx.vout.push_back(PayToBareMultisig_3of5());
        mutableTx.vout.push_back(PayToPubKeyHash_Exodus());
        mutableTx.vout.push_back(PayToPubKey_Unrelated());

        CTransaction tx(mutableTx);
        BOOST_CHECK_EQUAL(GetEncodingClass(tx, nBlock), OMNI_CLASS_B);
    }
    {
        int nBlock = 0;

        CMutableTransaction mutableTx;
        mutableTx.vout.push_back(OpReturn_Empty());
        mutableTx.vout.push_back(PayToPubKey_Unrelated());
        mutableTx.vout.push_back(PayToBareMultisig_1of3());
        mutableTx.vout.push_back(OpReturn_UnrelatedShort());
        mutableTx.vout.push_back(OpReturn_Tagged_A());
        mutableTx.vout.push_back(OpReturn_Tagged_C());
        mutableTx.vout.push_back(PayToPubKeyHash_Unrelated());
        mutableTx.vout.push_back(PayToBareMultisig_3of5());
        mutableTx.vout.push_back(OpReturn_Unrelated());
        mutableTx.vout.push_back(NonStandardOutput());
        mutableTx.vout.push_back(OpReturn_Tagged_B());
        mutableTx.vout.push_back(PayToScriptHash_Unrelated());
        mutableTx.vout.push_back(PayToPubKeyHash_Exodus());

        CTransaction tx(mutableTx);
        BOOST_CHECK_EQUAL(GetEncodingClass(tx, nBlock), OMNI_CLASS_B);
    }
    {
        int nBlock = std::numeric_limits<int>().max();

        CMutableTransaction mutableTx;
        mutableTx.vout.push_back(PayToPubKeyHash_ExodusCrowdsale(nBlock));
        mutableTx.vout.push_back(OpReturn_UnrelatedShort());
        mutableTx.vout.push_back(PayToPubKeyHash_Exodus());
        mutableTx.vout.push_back(OpReturn_Empty());
        mutableTx.vout.push_back(PayToPubKey_Unrelated());
        mutableTx.vout.push_back(PayToPubKeyHash_Unrelated());
        mutableTx.vout.push_back(NonStandardOutput());
        mutableTx.vout.push_back(PayToBareMultisig_1of3());
        mutableTx.vout.push_back(PayToScriptHash_Unrelated());
        mutableTx.vout.push_back(OpReturn_Unrelated());

        CTransaction tx(mutableTx);
        BOOST_CHECK_EQUAL(GetEncodingClass(tx, nBlock), OMNI_CLASS_B);
    }
}

BOOST_AUTO_TEST_CASE(class_class_c)
{
    {
        int nBlock = OP_RETURN_BLOCK;

        CMutableTransaction mutableTx;
        mutableTx.vout.push_back(OpReturn_Tagged_A());

        CTransaction tx(mutableTx);
        BOOST_CHECK_EQUAL(GetEncodingClass(tx, nBlock), OMNI_CLASS_C);
    }
    {
        int nBlock = std::numeric_limits<int>().max();

        CMutableTransaction mutableTx;
        mutableTx.vout.push_back(OpReturn_Tagged_B());
        mutableTx.vout.push_back(PayToScriptHash_Unrelated());
        mutableTx.vout.push_back(PayToScriptHash_Unrelated());

        CTransaction tx(mutableTx);
        BOOST_CHECK_EQUAL(GetEncodingClass(tx, nBlock), OMNI_CLASS_C);
    }
    {
        int nBlock = std::numeric_limits<int>().max();

        CMutableTransaction mutableTx;
        mutableTx.vout.push_back(NonStandardOutput());
        mutableTx.vout.push_back(PayToScriptHash_Unrelated());
        mutableTx.vout.push_back(OpReturn_Tagged_A());
        mutableTx.vout.push_back(PayToPubKeyHash_Unrelated());
        mutableTx.vout.push_back(OpReturn_Empty());
        mutableTx.vout.push_back(OpReturn_Unrelated());
        mutableTx.vout.push_back(PayToPubKey_Unrelated());
        mutableTx.vout.push_back(OpReturn_UnrelatedShort());

        CTransaction tx(mutableTx);
        BOOST_CHECK_EQUAL(GetEncodingClass(tx, nBlock), OMNI_CLASS_C);
    }
    {
        int nBlock = OP_RETURN_BLOCK;

        CMutableTransaction mutableTx;
        mutableTx.vout.push_back(PayToScriptHash_Unrelated());
        mutableTx.vout.push_back(OpReturn_Unrelated());
        mutableTx.vout.push_back(OpReturn_UnrelatedShort());
        mutableTx.vout.push_back(OpReturn_Empty());
        mutableTx.vout.push_back(PayToPubKeyHash_Unrelated());
        mutableTx.vout.push_back(PayToBareMultisig_1of3());
        mutableTx.vout.push_back(NonStandardOutput());
        mutableTx.vout.push_back(PayToPubKey_Unrelated());
        mutableTx.vout.push_back(OpReturn_Tagged_B());
        mutableTx.vout.push_back(PayToBareMultisig_3of5());

        CTransaction tx(mutableTx);
        BOOST_CHECK_EQUAL(GetEncodingClass(tx, nBlock), OMNI_CLASS_C);
    }
    {
        int nBlock = std::numeric_limits<int>().max();

        CMutableTransaction mutableTx;
        mutableTx.vout.push_back(OpReturn_UnrelatedShort());
        mutableTx.vout.push_back(PayToPubKey_Unrelated());
        mutableTx.vout.push_back(OpReturn_Unrelated());
        mutableTx.vout.push_back(OpReturn_Empty());
        mutableTx.vout.push_back(PayToPubKeyHash_Unrelated());
        mutableTx.vout.push_back(PayToScriptHash_Unrelated());
        mutableTx.vout.push_back(NonStandardOutput());
        mutableTx.vout.push_back(PayToPubKeyHash_ExodusCrowdsale(nBlock));
        mutableTx.vout.push_back(OpReturn_Tagged_C());

        CTransaction tx(mutableTx);
        BOOST_CHECK_EQUAL(GetEncodingClass(tx, nBlock), OMNI_CLASS_C);
    }
    {
        int nBlock = OP_RETURN_BLOCK;

        CMutableTransaction mutableTx;
        mutableTx.vout.push_back(PayToPubKey_Unrelated());
        mutableTx.vout.push_back(PayToBareMultisig_1of3());
        mutableTx.vout.push_back(PayToPubKeyHash_Unrelated());
        mutableTx.vout.push_back(PayToScriptHash_Unrelated());
        mutableTx.vout.push_back(NonStandardOutput());
        mutableTx.vout.push_back(OpReturn_Tagged_A());
        mutableTx.vout.push_back(PayToPubKeyHash_Exodus());
        mutableTx.vout.push_back(PayToPubKeyHash_ExodusCrowdsale(nBlock));

        CTransaction tx(mutableTx);
        BOOST_CHECK_EQUAL(GetEncodingClass(tx, nBlock), OMNI_CLASS_C);
    }
}


BOOST_AUTO_TEST_SUITE_END()
