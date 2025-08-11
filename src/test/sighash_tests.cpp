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

struct SigHashTest : BasicTestingSetup {
void RandomScript(CScript &script) {
    static const opcodetype oplist[] = {OP_FALSE, OP_1, OP_2, OP_3, OP_CHECKSIG, OP_IF, OP_VERIF, OP_RETURN, OP_CODESEPARATOR};
    script = CScript();
    int ops = (m_rng.randrange(10));
    for (int i=0; i<ops; i++)
        script << oplist[m_rng.randrange(std::size(oplist))];
}

void RandomTransaction(CMutableTransaction& tx, bool fSingle)
{
    tx.version = m_rng.rand32();
    tx.vin.clear();
    tx.vout.clear();
    tx.nLockTime = (m_rng.randbool()) ? m_rng.rand32() : 0;
    int ins = (m_rng.randbits(2)) + 1;
    int outs = fSingle ? ins : (m_rng.randbits(2)) + 1;
    for (int in = 0; in < ins; in++) {
        tx.vin.emplace_back();
        CTxIn &txin = tx.vin.back();
        txin.prevout.hash = Txid::FromUint256(m_rng.rand256());
        txin.prevout.n = m_rng.randbits(2);
        RandomScript(txin.scriptSig);
        txin.nSequence = (m_rng.randbool()) ? m_rng.rand32() : std::numeric_limits<uint32_t>::max();
    }
    for (int out = 0; out < outs; out++) {
        tx.vout.emplace_back();
        CTxOut &txout = tx.vout.back();
        txout.nValue = RandMoney(m_rng);
        RandomScript(txout.scriptPubKey);
    }
}
}; // struct SigHashTest

BOOST_FIXTURE_TEST_SUITE(sighash_tests, SigHashTest)

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
        int nHashType{int(m_rng.rand32())};
        CMutableTransaction txTo;
        RandomTransaction(txTo, (nHashType & 0x1f) == SIGHASH_SINGLE);
        CScript scriptCode;
        RandomScript(scriptCode);
        int nIn = m_rng.randrange(txTo.vin.size());

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

BOOST_AUTO_TEST_CASE(sighash_caching)
{
    // Get a script, transaction and parameters as inputs to the sighash function.
    CScript scriptcode;
    RandomScript(scriptcode);
    CScript diff_scriptcode{scriptcode};
    diff_scriptcode << OP_1;
    CMutableTransaction tx;
    RandomTransaction(tx, /*fSingle=*/false);
    const auto in_index{static_cast<uint32_t>(m_rng.randrange(tx.vin.size()))};
    const auto amount{m_rng.rand<CAmount>()};

    // Exercise the sighash function under both legacy and segwit v0.
    for (const auto sigversion: {SigVersion::BASE, SigVersion::WITNESS_V0}) {
        // For each, run it against all the 6 standard hash types and a few additional random ones.
        std::vector<int32_t> hash_types{{SIGHASH_ALL, SIGHASH_SINGLE, SIGHASH_NONE, SIGHASH_ALL | SIGHASH_ANYONECANPAY,
                                          SIGHASH_SINGLE | SIGHASH_ANYONECANPAY, SIGHASH_NONE | SIGHASH_ANYONECANPAY,
                                          SIGHASH_ANYONECANPAY, 0, std::numeric_limits<int32_t>::max()}};
        for (int i{0}; i < 10; ++i) {
            hash_types.push_back(i % 2 == 0 ? m_rng.rand<int8_t>() : m_rng.rand<int32_t>());
        }

        // Reuse the same cache across script types. This must not cause any issue as the cached value for one hash type must never
        // be confused for another (instantiating the cache within the loop instead would prevent testing this).
        SigHashCache cache;
        for (const auto hash_type: hash_types) {
            const bool expect_one{sigversion == SigVersion::BASE && ((hash_type & 0x1f) == SIGHASH_SINGLE) && in_index >= tx.vout.size()};

            // The result of computing the sighash should be the same with or without cache.
            const auto sighash_with_cache{SignatureHash(scriptcode, tx, in_index, hash_type, amount, sigversion, nullptr, &cache)};
            const auto sighash_no_cache{SignatureHash(scriptcode, tx, in_index, hash_type, amount, sigversion, nullptr, nullptr)};
            BOOST_CHECK_EQUAL(sighash_with_cache, sighash_no_cache);

            // Calling the cached version again should return the same value again.
            BOOST_CHECK_EQUAL(sighash_with_cache, SignatureHash(scriptcode, tx, in_index, hash_type, amount, sigversion, nullptr, &cache));

            // While here we might as well also check that the result for legacy is the same as for the old SignatureHash() function.
            if (sigversion == SigVersion::BASE) {
                BOOST_CHECK_EQUAL(sighash_with_cache, SignatureHashOld(scriptcode, CTransaction(tx), in_index, hash_type));
            }

            // Calling with a different scriptcode (for instance in case a CODESEP is encountered) will not return the cache value but
            // overwrite it. The sighash will always be different except in case of legacy SIGHASH_SINGLE bug.
            const auto sighash_with_cache2{SignatureHash(diff_scriptcode, tx, in_index, hash_type, amount, sigversion, nullptr, &cache)};
            const auto sighash_no_cache2{SignatureHash(diff_scriptcode, tx, in_index, hash_type, amount, sigversion, nullptr, nullptr)};
            BOOST_CHECK_EQUAL(sighash_with_cache2, sighash_no_cache2);
            if (!expect_one) {
                BOOST_CHECK_NE(sighash_with_cache, sighash_with_cache2);
            } else {
                BOOST_CHECK_EQUAL(sighash_with_cache, sighash_with_cache2);
                BOOST_CHECK_EQUAL(sighash_with_cache, uint256::ONE);
            }

            // Calling the cached version again should return the same value again.
            BOOST_CHECK_EQUAL(sighash_with_cache2, SignatureHash(diff_scriptcode, tx, in_index, hash_type, amount, sigversion, nullptr, &cache));

            // And if we store a different value for this scriptcode and hash type it will return that instead.
            {
                HashWriter h{};
                h << 42;
                cache.Store(hash_type, scriptcode, h);
                const auto stored_hash{h.GetHash()};
                BOOST_CHECK(cache.Load(hash_type, scriptcode, h));
                const auto loaded_hash{h.GetHash()};
                BOOST_CHECK_EQUAL(stored_hash, loaded_hash);
            }

            // And using this mutated cache with the sighash function will return the new value (except in the legacy SIGHASH_SINGLE bug
            // case in which it'll return 1).
            if (!expect_one) {
                BOOST_CHECK_NE(SignatureHash(scriptcode, tx, in_index, hash_type, amount, sigversion, nullptr, &cache), sighash_with_cache);
                HashWriter h{};
                BOOST_CHECK(cache.Load(hash_type, scriptcode, h));
                h << hash_type;
                const auto new_hash{h.GetHash()};
                BOOST_CHECK_EQUAL(SignatureHash(scriptcode, tx, in_index, hash_type, amount, sigversion, nullptr, &cache), new_hash);
            } else {
                BOOST_CHECK_EQUAL(SignatureHash(scriptcode, tx, in_index, hash_type, amount, sigversion, nullptr, &cache), uint256::ONE);
            }

            // Wipe the cache and restore the correct cached value for this scriptcode and hash_type before starting the next iteration.
            HashWriter dummy{};
            cache.Store(hash_type, diff_scriptcode, dummy);
            (void)SignatureHash(scriptcode, tx, in_index, hash_type, amount, sigversion, nullptr, &cache);
            BOOST_CHECK(cache.Load(hash_type, scriptcode, dummy) || expect_one);
        }
    }
}

BOOST_AUTO_TEST_SUITE_END()
