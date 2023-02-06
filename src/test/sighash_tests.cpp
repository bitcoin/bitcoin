// Copyright (c) 2013-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <common/system.h>
#include <consensus/tx_check.h>
#include <consensus/validation.h>
#include <hash.h>
#include <script/interpreter.h>
#include <script/script.h>
#include <serialize.h>
#include <streams.h>
#include <test/data/sighash.json.h>
#include <test/util/json.h>
#include <test/util/random.h>
#include <test/util/setup_common.h>
#include <util/strencodings.h>

#include <iostream>

#include <boost/test/unit_test.hpp>

#include <univalue.h>

// Old script.cpp SignatureHash function
uint256 static SignatureHashOld(CScript scriptCode, const CTransaction& txTo, unsigned int nIn, int nHashType)
{
    if (nIn >= txTo.vin.size())
    {
        return uint256::ONE;
    }
    CMutableTransaction txTmp(txTo);

    // In case concatenating two scripts ends up with two codeseparators,
    // or an extra one at the end, this prevents all those possible incompatibilities.
    FindAndDelete(scriptCode, CScript(OP_CODESEPARATOR));

    // Blank out other inputs' signatures
    for (unsigned int i = 0; i < txTmp.vin.size(); i++)
        txTmp.vin[i].scriptSig = CScript();
    txTmp.vin[nIn].scriptSig = scriptCode;

    // Blank out some of the outputs
    if ((nHashType & 0x1f) == SIGHASH_NONE)
    {
        // Wildcard payee
        txTmp.vout.clear();

        // Let the others update at will
        for (unsigned int i = 0; i < txTmp.vin.size(); i++)
            if (i != nIn)
                txTmp.vin[i].nSequence = 0;
    }
    else if ((nHashType & 0x1f) == SIGHASH_SINGLE)
    {
        // Only lock-in the txout payee at same index as txin
        unsigned int nOut = nIn;
        if (nOut >= txTmp.vout.size())
        {
            return uint256::ONE;
        }
        txTmp.vout.resize(nOut+1);
        for (unsigned int i = 0; i < nOut; i++)
            txTmp.vout[i].SetNull();

        // Let the others update at will
        for (unsigned int i = 0; i < txTmp.vin.size(); i++)
            if (i != nIn)
                txTmp.vin[i].nSequence = 0;
    }

    // Blank out other inputs completely, not recommended for open transactions
    if (nHashType & SIGHASH_ANYONECANPAY)
    {
        txTmp.vin[0] = txTmp.vin[nIn];
        txTmp.vin.resize(1);
    }

    // Serialize and hash
    HashWriter ss{};
    ss << TX_NO_WITNESS(txTmp) << nHashType;
    return ss.GetHash();
}

void static RandomScript(CScript &script) {
    static const opcodetype oplist[] = {OP_FALSE, OP_1, OP_2, OP_3, OP_CHECKSIG, OP_IF, OP_VERIF, OP_RETURN, OP_CODESEPARATOR};
    script = CScript();
    int ops = (InsecureRandRange(10));
    for (int i=0; i<ops; i++)
        script << oplist[InsecureRandRange(std::size(oplist))];
}

void static RandomTransaction(CMutableTransaction& tx, bool fSingle)
{
    tx.nVersion = int(InsecureRand32());
    tx.vin.clear();
    tx.vout.clear();
    tx.nLockTime = (InsecureRandBool()) ? InsecureRand32() : 0;
    int ins = (InsecureRandBits(2)) + 1;
    int outs = fSingle ? ins : (InsecureRandBits(2)) + 1;
    for (int in = 0; in < ins; in++) {
        tx.vin.emplace_back();
        CTxIn &txin = tx.vin.back();
        txin.prevout.hash = Txid::FromUint256(InsecureRand256());
        txin.prevout.n = InsecureRandBits(2);
        RandomScript(txin.scriptSig);
        txin.nSequence = (InsecureRandBool()) ? InsecureRand32() : std::numeric_limits<uint32_t>::max();
    }
    for (int out = 0; out < outs; out++) {
        tx.vout.emplace_back();
        CTxOut &txout = tx.vout.back();
        txout.nValue = InsecureRandMoneyAmount();
        RandomScript(txout.scriptPubKey);
    }
}

void static MutateInputs(CMutableTransaction &tx, const bool inPrevout, const bool inInputSequence) {

    // mutate previous input
    for (std::size_t in = 0; in < tx.vin.size(); in++) {
        CTxIn &txin = tx.vin[in];

        if (inPrevout) {
            txin.prevout.hash = Txid::FromUint256(InsecureRand256());
            txin.prevout.n = InsecureRandBits(2);
        }

        // mutate input sequences
        if (inInputSequence) {
            txin.nSequence = InsecureRand32();
        }
    }
}

static std::vector<CTxOut> MutateSpentOutputs(const size_t inNumOutputs, const bool inInputScripts, const bool inValue) {
    FastRandomContext context(InsecureRand256());

    // default to nValue of 0 and witness script with address of all zeros
    std::vector<CTxOut> spent_outputs(inNumOutputs, CTxOut(0, CScript() << OP_1 << std::vector<unsigned char>(WITNESS_V1_TAPROOT_SIZE, 0)));

    for (std::size_t in = 0; in < spent_outputs.size(); in++) {

        if (inInputScripts) {
            spent_outputs[in].scriptPubKey = CScript() << OP_1 << context.randbytes(WITNESS_V1_TAPROOT_SIZE);
        }

        if (inValue) {
            // random, but not default nValue of 0
            spent_outputs[in].nValue = InsecureRandRange(99999999)+1;
        }
    }
    return spent_outputs;
}

BOOST_FIXTURE_TEST_SUITE(sighash_tests, BasicTestingSetup)

BOOST_AUTO_TEST_CASE(sighash_test)
{
    #if defined(PRINT_SIGHASH_JSON)
    std::cout << "[\n";
    std::cout << "\t[\"raw_transaction, script, input_index, hashType, signature_hash (result)\"],\n";
    int nRandomTests = 500;
    #else
    int nRandomTests = 50000;
    #endif
    for (int i=0; i<nRandomTests; i++) {
        int nHashType{int(InsecureRand32())};
        CMutableTransaction txTo;
        RandomTransaction(txTo, (nHashType & 0x1f) == SIGHASH_SINGLE);
        CScript scriptCode;
        RandomScript(scriptCode);
        int nIn = InsecureRandRange(txTo.vin.size());

        uint256 sh, sho;
        sho = SignatureHashOld(scriptCode, CTransaction(txTo), nIn, nHashType);
        sh = SignatureHash(scriptCode, txTo, nIn, nHashType, 0, SigVersion::BASE);
        #if defined(PRINT_SIGHASH_JSON)
        DataStream ss;
        ss << TX_WITH_WITNESS(txTo);

        std::cout << "\t[\"" ;
        std::cout << HexStr(ss) << "\", \"";
        std::cout << HexStr(scriptCode) << "\", ";
        std::cout << nIn << ", ";
        std::cout << nHashType << ", \"";
        std::cout << sho.GetHex() << "\"]";
        if (i+1 != nRandomTests) {
          std::cout << ",";
        }
        std::cout << "\n";
        #endif
        BOOST_CHECK(sh == sho);
    }
    #if defined(PRINT_SIGHASH_JSON)
    std::cout << "]\n";
    #endif
}

// Goal: check that SignatureHash generates correct hash
BOOST_AUTO_TEST_CASE(sighash_from_data)
{
    UniValue tests = read_json(json_tests::sighash);

    for (unsigned int idx = 0; idx < tests.size(); idx++) {
        const UniValue& test = tests[idx];
        std::string strTest = test.write();
        if (test.size() < 1) // Allow for extra stuff (useful for comments)
        {
            BOOST_ERROR("Bad test: " << strTest);
            continue;
        }
        if (test.size() == 1) continue; // comment

        std::string raw_tx, raw_script, sigHashHex;
        int nIn, nHashType;
        uint256 sh;
        CTransactionRef tx;
        CScript scriptCode = CScript();

        try {
          // deserialize test data
          raw_tx = test[0].get_str();
          raw_script = test[1].get_str();
          nIn = test[2].getInt<int>();
          nHashType = test[3].getInt<int>();
          sigHashHex = test[4].get_str();

          DataStream stream(ParseHex(raw_tx));
          stream >> TX_WITH_WITNESS(tx);

          TxValidationState state;
          BOOST_CHECK_MESSAGE(CheckTransaction(*tx, state), strTest);
          BOOST_CHECK(state.IsValid());

          std::vector<unsigned char> raw = ParseHex(raw_script);
          scriptCode.insert(scriptCode.end(), raw.begin(), raw.end());
        } catch (...) {
          BOOST_ERROR("Bad test, couldn't deserialize data: " << strTest);
          continue;
        }

        sh = SignatureHash(scriptCode, *tx, nIn, nHashType, 0, SigVersion::BASE);
        BOOST_CHECK_MESSAGE(sh.GetHex() == sigHashHex, strTest);
    }
}

// Goal: check that SignatureHashOld and SignatureHash ignore sighash flags SIGHASH_ANYPREVOUT and SIGHASH_ANYPREVOUTANYSCRIPT
BOOST_AUTO_TEST_CASE(sighash_anyprevout_legacy)
{
    #if defined(PRINT_SIGHASH_JSON)
    std::cout << "[\n";
    std::cout << "\t[\"raw_transaction, script, input_index, hashType, signature_hash (result)\"],\n";
    int nRandomTests = 500;
    #else
    int nRandomTests = 50000;
    #endif

    for (int i=0; i<nRandomTests; i++) {

        // random sighashs, with and without ANYPREVOUT*
        int nHashType = static_cast<int32_t>(InsecureRand32()) & ~SIGHASH_INPUT_MASK;
        int nHashTypeApo = nHashType | SIGHASH_ANYPREVOUT;
        int nHashTypeApoas = nHashType | SIGHASH_ANYPREVOUTANYSCRIPT;

        CMutableTransaction tx;
        RandomTransaction(tx, (nHashType & SIGHASH_OUTPUT_MASK) == SIGHASH_SINGLE);
        CScript scriptCode, mut_scriptCode;
        RandomScript(scriptCode);
        RandomScript(mut_scriptCode);

        // remove code separators from old scripts because they are not serialized
        CScript old_scriptCode(scriptCode), old_mut_scriptCode(mut_scriptCode);
        FindAndDelete(old_scriptCode, CScript(OP_CODESEPARATOR));
        FindAndDelete(old_mut_scriptCode, CScript(OP_CODESEPARATOR));

        int nIn = InsecureRandRange(tx.vin.size());

        CMutableTransaction tx_mut_prevout(tx);
        MutateInputs(tx_mut_prevout, true, false);

        CMutableTransaction tx_mut_sequence(tx);
        MutateInputs(tx_mut_sequence, false, true);

        // test OLD signature hash ignores SIGHASH_ANYPREVOUT and SIGHASH_ANYPREVOUTANYSCRIPT
        uint256 sho = SignatureHashOld(old_scriptCode, CTransaction(tx), nIn, nHashType);
        uint256 sho_apo = SignatureHashOld(old_scriptCode, CTransaction(tx), nIn, nHashTypeApo);
        uint256 sho_apoas = SignatureHashOld(old_scriptCode, CTransaction(tx), nIn, nHashTypeApoas);

        // mutate the previous output transaction
        uint256 sho_mut_prevout = SignatureHashOld(old_scriptCode, CTransaction(tx_mut_prevout), nIn, nHashType);
        uint256 sho_mut_prevout_apo = SignatureHashOld(old_scriptCode, CTransaction(tx_mut_prevout), nIn, nHashTypeApo);
        uint256 sho_mut_prevout_apoas = SignatureHashOld(old_scriptCode, CTransaction(tx_mut_prevout), nIn, nHashTypeApoas);

        // mutate the previous input script
        uint256 sho_mut_script = SignatureHashOld(old_mut_scriptCode, CTransaction(tx), nIn, nHashType);
        uint256 sho_mut_script_apo = SignatureHashOld(old_mut_scriptCode, CTransaction(tx), nIn, nHashTypeApo);
        uint256 sho_mut_script_apoas = SignatureHashOld(old_mut_scriptCode, CTransaction(tx), nIn, nHashTypeApoas);

        // mutate the input sequence
        uint256 sho_mut_sequence = SignatureHashOld(old_scriptCode, CTransaction(tx_mut_sequence), nIn, nHashType);
        uint256 sho_mut_sequence_apo = SignatureHashOld(old_scriptCode, CTransaction(tx_mut_sequence), nIn, nHashTypeApo);
        uint256 sho_mut_sequence_apoas = SignatureHashOld(old_scriptCode, CTransaction(tx_mut_sequence), nIn, nHashTypeApoas);

        // test BASE signature hash ignores SIGHASH_ANYPREVOUT and SIGHASH_ANYPREVOUTANYSCRIPT
        uint256 shb = SignatureHash(old_scriptCode, tx, nIn, nHashType, 0, SigVersion::BASE);
        uint256 shb_apo = SignatureHash(old_scriptCode, tx, nIn, nHashTypeApo, 0, SigVersion::BASE);
        uint256 shb_apoas = SignatureHash(old_scriptCode, tx, nIn, nHashTypeApoas, 0, SigVersion::BASE);

        // mutate the previous output transaction
        uint256 shb_mut_prevout = SignatureHash(old_scriptCode, tx_mut_prevout, nIn, nHashType, 0, SigVersion::BASE);
        uint256 shb_mut_prevout_apo = SignatureHash(old_scriptCode, tx_mut_prevout, nIn, nHashTypeApo, 0, SigVersion::BASE);
        uint256 shb_mut_prevout_apoas = SignatureHash(old_scriptCode, tx_mut_prevout, nIn, nHashTypeApoas, 0, SigVersion::BASE);

        // mutate the previous input script
        uint256 shb_mut_script = SignatureHash(old_mut_scriptCode, tx, nIn, nHashType, 0, SigVersion::BASE);
        uint256 shb_mut_script_apo = SignatureHash(old_mut_scriptCode, tx, nIn, nHashTypeApo, 0, SigVersion::BASE);
        uint256 shb_mut_script_apoas = SignatureHash(old_mut_scriptCode, tx, nIn, nHashTypeApoas, 0, SigVersion::BASE);

        // mutate the input sequence
        uint256 shb_mut_sequence = SignatureHash(old_scriptCode, tx_mut_sequence, nIn, nHashType, 0, SigVersion::BASE);
        uint256 shb_mut_sequence_apo = SignatureHash(old_scriptCode, tx_mut_sequence, nIn, nHashTypeApo, 0, SigVersion::BASE);
        uint256 shb_mut_sequence_apoas = SignatureHash(old_scriptCode, tx_mut_sequence, nIn, nHashTypeApoas, 0, SigVersion::BASE);

        // test v0 signature hash ignores SIGHASH_ANYPREVOUT and SIGHASH_ANYPREVOUTANYSCRIPT
        uint256 shv0 = SignatureHash(scriptCode, tx, nIn, nHashType, 0, SigVersion::WITNESS_V0);
        uint256 shv0_apo = SignatureHash(scriptCode, tx, nIn, nHashTypeApo, 0, SigVersion::WITNESS_V0);
        uint256 shv0_apoas = SignatureHash(scriptCode, tx, nIn, nHashTypeApoas, 0, SigVersion::WITNESS_V0);

        // mutate the previous output transaction
        uint256 shv0_mut_prevout = SignatureHash(scriptCode, tx_mut_prevout, nIn, nHashType, 0, SigVersion::WITNESS_V0);
        uint256 shv0_mut_prevout_apo = SignatureHash(scriptCode, tx_mut_prevout, nIn, nHashTypeApo, 0, SigVersion::WITNESS_V0);
        uint256 shv0_mut_prevout_apoas = SignatureHash(scriptCode, tx_mut_prevout, nIn, nHashTypeApoas, 0, SigVersion::WITNESS_V0);

        // mutate the previous input script
        uint256 shv0_mut_script = SignatureHash(mut_scriptCode, tx, nIn, nHashType, 0, SigVersion::WITNESS_V0);
        uint256 shv0_mut_script_apo = SignatureHash(mut_scriptCode, tx, nIn, nHashTypeApo, 0, SigVersion::WITNESS_V0);
        uint256 shv0_mut_script_apoas = SignatureHash(mut_scriptCode, tx, nIn, nHashTypeApoas, 0, SigVersion::WITNESS_V0);

        // mutate the input sequence
        uint256 shv0_mut_sequence = SignatureHash(scriptCode, tx_mut_sequence, nIn, nHashType, 0, SigVersion::BASE);
        uint256 shv0_mut_sequence_apo = SignatureHash(scriptCode, tx_mut_sequence, nIn, nHashTypeApo, 0, SigVersion::BASE);
        uint256 shv0_mut_sequence_apoas = SignatureHash(scriptCode, tx_mut_sequence, nIn, nHashTypeApoas, 0, SigVersion::BASE);

        #if defined(PRINT_SIGHASH_JSON)
        CDataStream ss(SER_NETWORK, PROTOCOL_VERSION);
        ss << tx;

        std::cout << "\t[\"" ;
        std::cout << HexStr(ss.begin(), ss.end()) << "\", \"";
        std::cout << HexStr(scriptCode) << "\", ";
        std::cout << nIn << ", ";
        std::cout << nHashType << ", \"";
        std::cout << sho.GetHex() << "\"]";
        if (i+1 != nRandomTests) {
          std::cout << ",";
        }
        std::cout << "\n";
        #endif

        // check that legacy, base and v0/SEGWIT transactions ignore SIGHASH_ANYPREVOUT and SIGHASH_ANYPREVOUTANYSCRIPT

        // check maleated previous input transaction creates different digests
        BOOST_CHECK(sho != sho_mut_prevout);
        BOOST_CHECK(sho_apo != sho_mut_prevout_apo);
        BOOST_CHECK(sho_apoas != sho_mut_prevout_apoas);
        BOOST_CHECK(shb != shb_mut_prevout);
        BOOST_CHECK(shb_apo != shb_mut_prevout_apo);
        BOOST_CHECK(shb_apoas != shb_mut_prevout_apoas);
        BOOST_CHECK(shv0 != shv0_mut_prevout);
        BOOST_CHECK(shv0_apo != shv0_mut_prevout_apo);
        BOOST_CHECK(shv0_apoas != shv0_mut_prevout_apoas);

        // check maleated input script creates different digests
        if (old_scriptCode != old_mut_scriptCode) {
            BOOST_CHECK(sho != sho_mut_script);
            BOOST_CHECK(sho_apo != sho_mut_script_apo);
            BOOST_CHECK(sho_apoas != sho_mut_script_apoas);
            BOOST_CHECK(shb != shb_mut_script);
            BOOST_CHECK(shb_apo != shb_mut_script_apo);
            BOOST_CHECK(shb_apoas != shb_mut_script_apoas);
            BOOST_CHECK(shv0 != shv0_mut_script);
            BOOST_CHECK(shv0_apo != shv0_mut_script_apo);
            BOOST_CHECK(shv0_apoas != shv0_mut_script_apoas);
        }

        // check maleated sequence creates different digests
        BOOST_CHECK(sho != sho_mut_sequence);
        BOOST_CHECK(sho_apo != sho_mut_sequence_apo);
        BOOST_CHECK(sho_apoas != sho_mut_sequence_apoas);
        BOOST_CHECK(shb != shb_mut_sequence);
        BOOST_CHECK(shb_apo != shb_mut_sequence_apo);
        BOOST_CHECK(shb_apoas != shb_mut_sequence_apoas);
        BOOST_CHECK(shv0 != shv0_mut_sequence);
        BOOST_CHECK(shv0_apo != shv0_mut_sequence_apo);
        BOOST_CHECK(shv0_apoas != shv0_mut_sequence_apoas);
    }

    #if defined(PRINT_SIGHASH_JSON)
    std::cout << "]\n";
    #endif
}

// Goal: check that SignatureHashSchnorr generates the proper hash when inputs are maleated with SIGHASH_ANYPREVOUT and SIGHASH_ANYPREVOUTANYSCRIPT
BOOST_AUTO_TEST_CASE(sighash_anyprevout_taproot)
{
    #if defined(PRINT_SIGHASH_JSON)
    std::cout << "[\n";
    std::cout << "\t[\"raw_transaction, script, input_index, hashType, signature_hash (result)\"],\n";
    int nRandomTests = 500;
    #else
    int nRandomTests = 50000;
    #endif

    for (int i=0; i<nRandomTests; i++) {

        // Test random sighash, excluding sighash input flags
        int nHashType = static_cast<int32_t>(InsecureRand32()) & ~SIGHASH_INPUT_MASK;

        CMutableTransaction tx;
        RandomTransaction(tx, (nHashType & SIGHASH_OUTPUT_MASK) == SIGHASH_SINGLE);
        std::for_each(tx.vin.begin(), tx.vin.end(), [](CTxIn& vin){ vin.scriptWitness.stack.push_back({OP_TRUE}); });
        PrecomputedTransactionData txdata(tx);
        txdata.Init(tx, MutateSpentOutputs(tx.vin.size(), false, false));

        // test random transaction input
        int nIn = InsecureRandRange(tx.vin.size());

        // mutate the previous output transaction
        CMutableTransaction tx_mut_prevout(tx);
        MutateInputs(tx_mut_prevout, true, false);
        PrecomputedTransactionData txdata_mut_prevout(tx_mut_prevout);
        txdata_mut_prevout.Init(tx_mut_prevout, MutateSpentOutputs(tx_mut_prevout.vin.size(), false, false));

        // mutate the previous input script
        CMutableTransaction tx_mut_script(tx);
        PrecomputedTransactionData txdata_mut_script(tx_mut_script);
        txdata_mut_script.Init(tx_mut_script, MutateSpentOutputs(tx_mut_script.vin.size(), true, false));

        // mutate the input sequence
        CMutableTransaction tx_mut_sequence(tx);
        MutateInputs(tx_mut_sequence, false, true);
        PrecomputedTransactionData txdata_mut_sequence(tx_mut_sequence);
        txdata_mut_sequence.Init(tx_mut_sequence, MutateSpentOutputs(tx_mut_sequence.vin.size(), false, false));

        // mutate only the output values of the spent output
        CMutableTransaction tx_mut_spent_value(tx);
        PrecomputedTransactionData txdata_mut_spent_value(tx_mut_spent_value);
        txdata_mut_spent_value.Init(tx_mut_spent_value, MutateSpentOutputs(tx_mut_spent_value.vin.size(), false, true));

        // only check signature version of v1/TAPSCRIPT (0x3) because EvalChecksig will not allow execution of v1/TAPROOT (0x2) transactions
        SigVersion sigversion = SigVersion::TAPSCRIPT;

        ScriptExecutionData execdata;
        execdata.m_annex_init = true;
        execdata.m_annex_present = InsecureRandBool();
        execdata.m_annex_hash = Txid::FromUint256(InsecureRand256());
        execdata.m_tapleaf_hash_init = true;
        execdata.m_tapleaf_hash = Txid::FromUint256(InsecureRand256());
        execdata.m_codeseparator_pos_init = true;
        execdata.m_codeseparator_pos = InsecureRand32();

        int nHashType_outmask = nHashType & SIGHASH_OUTPUT_MASK;
        int nHashType_inoutmask = nHashType & (SIGHASH_INPUT_MASK | SIGHASH_OUTPUT_MASK);

        // any sighash that sets undefined input or output flags should fail
        if (nHashType_inoutmask != nHashType_outmask && nHashType_inoutmask != nHashType) {
            uint256 tmp;
            BOOST_CHECK(false == SignatureHashSchnorr(tmp, execdata, tx, nIn, nHashType, sigversion, KeyVersion::TAPROOT, txdata, MissingDataBehavior::ASSERT_FAIL));
            BOOST_CHECK(false == SignatureHashSchnorr(tmp, execdata, tx, nIn, nHashType, sigversion, KeyVersion::ANYPREVOUT, txdata, MissingDataBehavior::ASSERT_FAIL));
        }

        // check transactions using the TAPROOT key version reject ANYPREVOUT
        {
            // should fail all sighash input flags if no output flag is set
            uint256 tmp;
            if (nHashType_outmask == 0x0) {
                BOOST_CHECK(false == SignatureHashSchnorr(tmp, execdata, tx, nIn, nHashType_outmask | SIGHASH_ANYONECANPAY, sigversion, KeyVersion::TAPROOT, txdata, MissingDataBehavior::ASSERT_FAIL));
                BOOST_CHECK(false == SignatureHashSchnorr(tmp, execdata, tx, nIn, nHashType_outmask | SIGHASH_ANYPREVOUT, sigversion, KeyVersion::TAPROOT, txdata, MissingDataBehavior::ASSERT_FAIL));
                BOOST_CHECK(false == SignatureHashSchnorr(tmp, execdata, tx, nIn, nHashType_outmask | SIGHASH_ANYPREVOUTANYSCRIPT, sigversion, KeyVersion::TAPROOT, txdata, MissingDataBehavior::ASSERT_FAIL));
            }
            // otherwise all other output sighash flags should fail when any input sighash flags are set except SIGHASH_ANYONECANPAY
            else {
                BOOST_CHECK(SignatureHashSchnorr(tmp, execdata, tx, nIn, nHashType_outmask | SIGHASH_ANYONECANPAY, sigversion, KeyVersion::TAPROOT, txdata, MissingDataBehavior::ASSERT_FAIL));
                BOOST_CHECK(false == SignatureHashSchnorr(tmp, execdata, tx, nIn, nHashType_outmask | SIGHASH_ANYPREVOUT, sigversion, KeyVersion::TAPROOT, txdata, MissingDataBehavior::ASSERT_FAIL));
                BOOST_CHECK(false == SignatureHashSchnorr(tmp, execdata, tx, nIn, nHashType_outmask | SIGHASH_ANYPREVOUTANYSCRIPT, sigversion, KeyVersion::TAPROOT, txdata, MissingDataBehavior::ASSERT_FAIL));
            }
        }

        // check transactions using the ANYPREVOUT key version
        {
            // compute base sighash without any input sighash flags
            uint256 shts;
            BOOST_CHECK(SignatureHashSchnorr(shts, execdata, tx, nIn, nHashType_outmask, sigversion, KeyVersion::ANYPREVOUT, txdata, MissingDataBehavior::ASSERT_FAIL));

            // should fail when no output sighash is set, if any input sighash is set
            if (nHashType_outmask == 0x0) {
                uint256 tmp;
                BOOST_CHECK(false == SignatureHashSchnorr(tmp, execdata, tx, nIn, nHashType_outmask | SIGHASH_ANYONECANPAY, sigversion, KeyVersion::ANYPREVOUT, txdata, MissingDataBehavior::ASSERT_FAIL));
                BOOST_CHECK(false == SignatureHashSchnorr(tmp, execdata, tx, nIn, nHashType_outmask | SIGHASH_ANYPREVOUT, sigversion, KeyVersion::ANYPREVOUT, txdata, MissingDataBehavior::ASSERT_FAIL));
                BOOST_CHECK(false == SignatureHashSchnorr(tmp, execdata, tx, nIn, nHashType_outmask | SIGHASH_ANYPREVOUTANYSCRIPT, sigversion, KeyVersion::ANYPREVOUT, txdata, MissingDataBehavior::ASSERT_FAIL));
            }

            // otherwise any output sighash flags should succeed with any input sighash flags
            else {
                uint256 shts_acp, shts_apo, shts_apoas;
                BOOST_CHECK(SignatureHashSchnorr(shts_acp, execdata, tx, nIn, nHashType_outmask | SIGHASH_ANYONECANPAY, sigversion, KeyVersion::ANYPREVOUT, txdata, MissingDataBehavior::ASSERT_FAIL));
                BOOST_CHECK(SignatureHashSchnorr(shts_apo, execdata, tx, nIn, nHashType_outmask | SIGHASH_ANYPREVOUT, sigversion, KeyVersion::ANYPREVOUT, txdata, MissingDataBehavior::ASSERT_FAIL));
                BOOST_CHECK(SignatureHashSchnorr(shts_apoas, execdata, tx, nIn, nHashType_outmask | SIGHASH_ANYPREVOUTANYSCRIPT, sigversion, KeyVersion::ANYPREVOUT, txdata, MissingDataBehavior::ASSERT_FAIL));

                uint256 shts_mut_prevout_acp, shts_mut_prevout_apo, shts_mut_prevout_apoas;
                BOOST_CHECK(SignatureHashSchnorr(shts_mut_prevout_acp, execdata, tx_mut_prevout, nIn, nHashType_outmask | SIGHASH_ANYONECANPAY, sigversion, KeyVersion::ANYPREVOUT, txdata_mut_prevout, MissingDataBehavior::ASSERT_FAIL));
                BOOST_CHECK(SignatureHashSchnorr(shts_mut_prevout_apo, execdata, tx_mut_prevout, nIn, nHashType_outmask | SIGHASH_ANYPREVOUT, sigversion, KeyVersion::ANYPREVOUT, txdata_mut_prevout, MissingDataBehavior::ASSERT_FAIL));
                BOOST_CHECK(SignatureHashSchnorr(shts_mut_prevout_apoas, execdata, tx_mut_prevout, nIn, nHashType_outmask | SIGHASH_ANYPREVOUTANYSCRIPT, sigversion, KeyVersion::ANYPREVOUT, txdata_mut_prevout, MissingDataBehavior::ASSERT_FAIL));

                uint256 shts_mut_script_acp, shts_mut_script_apo, shts_mut_script_apoas;
                BOOST_CHECK(SignatureHashSchnorr(shts_mut_script_acp, execdata, tx_mut_script, nIn, nHashType_outmask | SIGHASH_ANYONECANPAY, sigversion, KeyVersion::ANYPREVOUT, txdata_mut_script, MissingDataBehavior::ASSERT_FAIL));
                BOOST_CHECK(SignatureHashSchnorr(shts_mut_script_apo, execdata, tx_mut_script, nIn, nHashType_outmask | SIGHASH_ANYPREVOUT, sigversion, KeyVersion::ANYPREVOUT, txdata_mut_script, MissingDataBehavior::ASSERT_FAIL));
                BOOST_CHECK(SignatureHashSchnorr(shts_mut_script_apoas, execdata, tx_mut_script, nIn, nHashType_outmask | SIGHASH_ANYPREVOUTANYSCRIPT, sigversion, KeyVersion::ANYPREVOUT, txdata_mut_script, MissingDataBehavior::ASSERT_FAIL));

                uint256 shts_mut_sequence_acp, shts_mut_sequence_apo, shts_mut_sequence_apoas;
                BOOST_CHECK(SignatureHashSchnorr(shts_mut_sequence_acp, execdata, tx_mut_sequence, nIn, nHashType_outmask | SIGHASH_ANYONECANPAY, sigversion, KeyVersion::ANYPREVOUT, txdata_mut_sequence, MissingDataBehavior::ASSERT_FAIL));
                BOOST_CHECK(SignatureHashSchnorr(shts_mut_sequence_apo, execdata, tx_mut_sequence, nIn, nHashType_outmask | SIGHASH_ANYPREVOUT, sigversion, KeyVersion::ANYPREVOUT, txdata_mut_sequence, MissingDataBehavior::ASSERT_FAIL));
                BOOST_CHECK(SignatureHashSchnorr(shts_mut_sequence_apoas, execdata, tx_mut_sequence, nIn, nHashType_outmask | SIGHASH_ANYPREVOUTANYSCRIPT, sigversion, KeyVersion::ANYPREVOUT, txdata_mut_sequence, MissingDataBehavior::ASSERT_FAIL));

                uint256 shts_mut_spent_value_acp, shts_mut_spent_value_apo, shts_mut_spent_value_apoas;
                BOOST_CHECK(SignatureHashSchnorr(shts_mut_spent_value_acp, execdata, tx_mut_spent_value, nIn, nHashType_outmask | SIGHASH_ANYONECANPAY, sigversion, KeyVersion::ANYPREVOUT, txdata_mut_spent_value, MissingDataBehavior::ASSERT_FAIL));
                BOOST_CHECK(SignatureHashSchnorr(shts_mut_spent_value_apo, execdata, tx_mut_spent_value, nIn, nHashType_outmask | SIGHASH_ANYPREVOUT, sigversion, KeyVersion::ANYPREVOUT, txdata_mut_spent_value, MissingDataBehavior::ASSERT_FAIL));
                BOOST_CHECK(SignatureHashSchnorr(shts_mut_spent_value_apoas, execdata, tx_mut_spent_value, nIn, nHashType_outmask | SIGHASH_ANYPREVOUTANYSCRIPT, sigversion, KeyVersion::ANYPREVOUT, txdata_mut_spent_value, MissingDataBehavior::ASSERT_FAIL));

                // both transactions must be signed with the SIGHASH_ANYPREVOUT input flag to create identical sighashes if previous input or output information changes
                BOOST_CHECK(shts != shts_mut_prevout_apo);
                BOOST_CHECK(shts != shts_mut_script_apo);
                BOOST_CHECK(shts != shts_mut_sequence_apo);
                BOOST_CHECK(shts != shts_mut_spent_value_apo);
                BOOST_CHECK(shts_acp != shts_mut_prevout_apo);
                BOOST_CHECK(shts_acp != shts_mut_script_apo);
                BOOST_CHECK(shts_acp != shts_mut_sequence_apo);
                BOOST_CHECK(shts_acp != shts_mut_spent_value_apo);

                // only when prevout hashes differ does SIGHASH_ANYPREVOUT create identical sighashes
                BOOST_CHECK(shts_apo == shts_mut_prevout_apo);
                BOOST_CHECK(shts_apo != shts_mut_script_apo);
                if (tx.vin[nIn].nSequence != tx_mut_sequence.vin[nIn].nSequence) {
                    BOOST_CHECK(shts_apo != shts_mut_sequence_apo);
                }
                BOOST_CHECK(shts_apo != shts_mut_spent_value_apo);
                BOOST_CHECK(shts_apo != shts_mut_sequence_apo);

                // both transactions must be signed with the SIGHASH_ANYPREVOUTANYSCRIPT input flag to create identical sighashes
                BOOST_CHECK(shts != shts_mut_prevout_apoas);
                BOOST_CHECK(shts != shts_mut_script_apoas);
                BOOST_CHECK(shts != shts_mut_sequence_apoas);
                BOOST_CHECK(shts != shts_mut_spent_value_apoas);
                BOOST_CHECK(shts_acp != shts_mut_prevout_apoas);
                BOOST_CHECK(shts_acp != shts_mut_script_apoas);
                BOOST_CHECK(shts_acp != shts_mut_sequence_apoas);
                BOOST_CHECK(shts_acp != shts_mut_spent_value_apoas);

                // only when the prevout hashes or input scripts are different does SIGHASH_ANYPREVOUTANYSCRIPT create identical sighashes
                BOOST_CHECK(shts_apoas == shts_mut_prevout_apoas);
                BOOST_CHECK(shts_apoas == shts_mut_script_apoas);
                if (tx.vin[nIn].nSequence != tx_mut_sequence.vin[nIn].nSequence) {
                    BOOST_CHECK(shts_apoas != shts_mut_sequence_apoas);
                }
                BOOST_CHECK(shts_apoas == shts_mut_spent_value_apoas);
                BOOST_CHECK(shts_apoas != shts_mut_sequence_apoas);
            }
        }

        #if defined(PRINT_SIGHASH_JSON)
        CDataStream ss(SER_NETWORK, PROTOCOL_VERSION);
        ss << tx;

        std::cout << "\t[\"" ;
        std::cout << HexStr(ss.begin(), ss.end()) << "\", \"";
        std::cout << HexStr(scriptCode) << "\", ";
        std::cout << nIn << ", ";
        std::cout << nHashType << ", \"";
        std::cout << sho.GetHex() << "\"]";
        if (i+1 != nRandomTests) {
          std::cout << ",";
        }
        std::cout << "\n";
        #endif
    }
    #if defined(PRINT_SIGHASH_JSON)
    std::cout << "]\n";
    #endif
}

BOOST_AUTO_TEST_SUITE_END()
