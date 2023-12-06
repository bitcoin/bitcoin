// Copyright (c) 2012-2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <test/util/setup_common.h>
#include <boost/test/unit_test.hpp>

#include <primitives/compression.h>
#include <node/context.h>
#include <validation.h>
#include <script/sign.h>
#include <node/coin.h>

#include <stdint.h>

using node::NodeContext;
using node::FindCoins;

BOOST_FIXTURE_TEST_SUITE(compressed_transaction_tests, TestChain100Setup)

//Compressed Serialization
BOOST_AUTO_TEST_CASE(serialize_compressed_outpoint_1)
{
    std::vector<CCompressedOutPoint> outpoints;
    outpoints.emplace_back(CCompressedOutPoint(0, 0));
    outpoints.emplace_back(CCompressedOutPoint(0, 1));
    outpoints.emplace_back(CCompressedOutPoint(0, 4294967295));
    outpoints.emplace_back(CCompressedOutPoint(1, 0));
    outpoints.emplace_back(CCompressedOutPoint(1, 1));
    outpoints.emplace_back(CCompressedOutPoint(1, 4294967295));
    outpoints.emplace_back(CCompressedOutPoint(4294967295, 0));
    outpoints.emplace_back(CCompressedOutPoint(4294967295, 1));
    outpoints.emplace_back(CCompressedOutPoint(4294967295, 4294967295));

    for (auto const outpoint : outpoints) {
        DataStream stream;
        outpoint.Serialize(stream);
        CCompressedOutPoint dscoutpoint = CCompressedOutPoint(deserialize, stream);
        if (outpoint != dscoutpoint) std::cout << "original: " << outpoint.ToString() << "\ndeserialized: " << dscoutpoint.ToString() << std::endl;
        BOOST_CHECK(outpoint == dscoutpoint);
    }
}

//Uncompressed Serialization
BOOST_AUTO_TEST_CASE(serialize_compressed_outpoint_2)
{
    std::vector<CCompressedOutPoint> outpoints;
    uint256 txid;
    txid.SetHex("0x4853");
    outpoints.emplace_back(CCompressedOutPoint(Txid::FromUint256(txid), 0));
    txid.SetHex("0x00");
    outpoints.emplace_back(CCompressedOutPoint(Txid::FromUint256(txid), 0));
    txid.SetNull();
    outpoints.emplace_back(CCompressedOutPoint(Txid::FromUint256(txid), 0));
    txid.SetHex("0x4853");
    outpoints.emplace_back(CCompressedOutPoint(Txid::FromUint256(txid), 1));
    txid.SetHex("0x00");
    outpoints.emplace_back(CCompressedOutPoint(Txid::FromUint256(txid), 1));
    txid.SetNull();
    outpoints.emplace_back(CCompressedOutPoint(Txid::FromUint256(txid), 1));
    txid.SetHex("0x4853");
    outpoints.emplace_back(CCompressedOutPoint(Txid::FromUint256(txid), 4294967295));
    txid.SetHex("0x00");
    outpoints.emplace_back(CCompressedOutPoint(Txid::FromUint256(txid), 4294967295));
    txid.SetNull();
    outpoints.emplace_back(CCompressedOutPoint(Txid::FromUint256(txid), 4294967295));

    for (auto const outpoint : outpoints) {
        DataStream stream;
        outpoint.Serialize(stream);
        CCompressedOutPoint dscoutpoint = CCompressedOutPoint(deserialize, stream);
        if (outpoint != dscoutpoint) std::cout << "original: " << outpoint.ToString() << "\ndeserialized: " << dscoutpoint.ToString() << std::endl;
        BOOST_CHECK(outpoint == dscoutpoint);
    }
}

//No nLockTime, No Inputs
BOOST_AUTO_TEST_CASE(compressed_outpoint_1)
{
    CMutableTransaction mtx;

    std::vector<std::string> warning_strings;

    {
        LOCK(cs_main);
        std::tuple<uint32_t, std::vector<CCompressedInput>> cinputstup = m_node.chainman->ActiveChainstate().CompressOutPoints(CTransaction(mtx), true, warning_strings);
        BOOST_CHECK(std::get<0>(cinputstup) == 0);
    }
}

//nLockTime, No Inputs
BOOST_AUTO_TEST_CASE(compressing_outpoint_2)
{
    CMutableTransaction mtx;
    mtx.nLockTime = 0x01356363;

    std::vector<std::string> warning_strings;

    {
        LOCK(cs_main);
        std::tuple<uint32_t, std::vector<CCompressedInput>> cinputstup = m_node.chainman->ActiveChainstate().CompressOutPoints(CTransaction(mtx), true, warning_strings);
        BOOST_CHECK(std::get<0>(cinputstup) == mtx.nLockTime-1);
    }
}

//No nLockTime, Unompressed Input
BOOST_AUTO_TEST_CASE(compressing_outpoint_3)
{
    uint256 txid;
    txid.SetHex("f664899d3fe7e8a6a4fddbac300cc8a995b282569a69ed361627d7bb92e990ac");
    CMutableTransaction mtx;
    mtx.vin.emplace_back(CTxIn(COutPoint(Txid::FromUint256(txid), 0)));

    std::vector<std::string> warning_strings;

    {
        LOCK(cs_main);
        std::tuple<uint32_t, std::vector<CCompressedInput>> cinputstup = m_node.chainman->ActiveChainstate().CompressOutPoints(CTransaction(mtx), true, warning_strings);
        BOOST_CHECK(std::get<0>(cinputstup) == 0);
    }
}

//No nLockTime, Compressed Input
BOOST_AUTO_TEST_CASE(compressing_outpoint_4)
{
    CScript scriptPubKey = CScript() << ToByteVector(coinbaseKey.GetPubKey()) << OP_CHECKSIG;
    CBlock block = CreateAndProcessBlock({}, scriptPubKey);
    { LOCK(cs_main); BOOST_CHECK(m_node.chainman->ActiveChain().Tip()->GetBlockHash() == block.GetHash()); }
    for (int i = 0; i < 100; i++) {
        CScript scriptPubKey = CScript() << ToByteVector(coinbaseKey.GetPubKey()) << OP_CHECKSIG;
        CBlock block = CreateAndProcessBlock({}, scriptPubKey);
        { LOCK(cs_main); BOOST_CHECK(m_node.chainman->ActiveChain().Tip()->GetBlockHash() == block.GetHash()); }
    }

    CMutableTransaction mtx;
    mtx.vin.emplace_back(CTxIn(COutPoint(block.vtx[0]->GetHash(), 0)));

    std::vector<std::string> warning_strings;

    {
        LOCK(cs_main);
        Coin coin;
        m_node.chainman->ActiveChainstate().CoinsTip().GetCoin(mtx.vin[0].prevout, coin);
        std::tuple<uint32_t, std::vector<CCompressedInput>> cinputstup = m_node.chainman->ActiveChainstate().CompressOutPoints(CTransaction(mtx), true, warning_strings);
        BOOST_CHECK(std::get<0>(cinputstup) == (uint32_t)coin.nHeight-1);
        BOOST_CHECK(std::get<1>(cinputstup)[0].outpoint().block_height() == 1);
        BOOST_CHECK(std::get<1>(cinputstup)[0].outpoint().block_index() == 0);
    }
}


//nLockTime, Compressed Input
BOOST_AUTO_TEST_CASE(compressing_outpoint_5)
{
    CScript scriptPubKey = CScript() << ToByteVector(coinbaseKey.GetPubKey()) << OP_CHECKSIG;
    CBlock block = CreateAndProcessBlock({}, scriptPubKey);
    { LOCK(cs_main); BOOST_CHECK(m_node.chainman->ActiveChain().Tip()->GetBlockHash() == block.GetHash()); }
    for (int i = 0; i < 100; i++) {
        CScript scriptPubKey = CScript() << ToByteVector(coinbaseKey.GetPubKey()) << OP_CHECKSIG;
        CBlock block = CreateAndProcessBlock({}, scriptPubKey);
        { LOCK(cs_main); BOOST_CHECK(m_node.chainman->ActiveChain().Tip()->GetBlockHash() == block.GetHash()); }
    }

    CMutableTransaction mtx;
    mtx.vin.emplace_back(CTxIn(COutPoint(block.vtx[0]->GetHash(), 0)));
    mtx.nLockTime = 99;
    {
        std::vector<std::string> warning_strings;

        {
            LOCK(cs_main);
            std::tuple<uint32_t, std::vector<CCompressedInput>> cinputstup = m_node.chainman->ActiveChainstate().CompressOutPoints(CTransaction(mtx), true, warning_strings);
            BOOST_CHECK(std::get<0>(cinputstup) == mtx.nLockTime-1);
            BOOST_CHECK(std::get<1>(cinputstup)[0].outpoint().block_height() == 3);
            BOOST_CHECK(std::get<1>(cinputstup)[0].outpoint().block_index() == 0);
        }
    }

    mtx.nLockTime = 200;

    {
        std::vector<std::string> warning_strings;

        {
            LOCK(cs_main);
            Coin coin;
            m_node.chainman->ActiveChainstate().CoinsTip().GetCoin(mtx.vin[0].prevout, coin);
            std::tuple<uint32_t, std::vector<CCompressedInput>> cinputstup = m_node.chainman->ActiveChainstate().CompressOutPoints(CTransaction(mtx), true, warning_strings);
            BOOST_CHECK(std::get<0>(cinputstup) == (uint32_t)coin.nHeight-1);
            BOOST_CHECK(std::get<1>(cinputstup)[0].outpoint().block_height() == 1);
            BOOST_CHECK(std::get<1>(cinputstup)[0].outpoint().block_index() == 0);
        }
    }
}

//Input missing or spent
BOOST_AUTO_TEST_CASE(compressing_outpoint_6)
{
    uint256 txid;
    txid.SetHex("f664899d3fe7e8a6a4fddbac300cc8a995b282569a69ed361627d7bb92e990ac");

    CMutableTransaction mtx;
    mtx.vin.emplace_back(CTxIn(COutPoint(Txid::FromUint256(txid), 0)));

    std::vector<std::string> warning_strings;

    {
        LOCK(cs_main);
        std::tuple<uint32_t, std::vector<CCompressedInput>> cinputs = m_node.chainman->ActiveChainstate().CompressOutPoints(CTransaction(mtx), true, warning_strings);
        BOOST_CHECK(warning_strings.size() == 1);
        BOOST_CHECK(warning_strings[0] == "UTXO(0): Input missing or spent");
        BOOST_CHECK(std::get<1>(cinputs).size() == 1);
        BOOST_CHECK(std::get<1>(cinputs)[0].scriptPubKey() == CScript());
        BOOST_CHECK(!std::get<1>(cinputs)[0].outpoint().IsCompressed());
    }
}

//Input Too Young
BOOST_AUTO_TEST_CASE(compressing_outpoint_7)
{
    CScript scriptPubKey = CScript() << ToByteVector(coinbaseKey.GetPubKey()) << OP_CHECKSIG;
    CBlock block = CreateAndProcessBlock({}, scriptPubKey);
    { LOCK(cs_main); BOOST_CHECK(m_node.chainman->ActiveChain().Tip()->GetBlockHash() == block.GetHash()); }

    CMutableTransaction mtx;
    mtx.vin.emplace_back(CTxIn(COutPoint(block.vtx[0]->GetHash(), 0)));

    std::vector<std::string> warning_strings;

    {
        LOCK(cs_main);
        std::tuple<uint32_t, std::vector<CCompressedInput>> cinputs = m_node.chainman->ActiveChainstate().CompressOutPoints(CTransaction(mtx), true, warning_strings);
        BOOST_CHECK(warning_strings.size() == 1);
        BOOST_CHECK(warning_strings[0] == "UTXO(0): Input less then 100 blocks old");
        BOOST_CHECK(std::get<1>(cinputs).size() == 1);
        BOOST_CHECK(std::get<1>(cinputs)[0].scriptPubKey() == scriptPubKey);
        BOOST_CHECK(!std::get<1>(cinputs)[0].outpoint().IsCompressed());
    }
}

//Input Valid
BOOST_AUTO_TEST_CASE(compressing_outpoint_8)
{
    CScript scriptPubKey = CScript() << ToByteVector(coinbaseKey.GetPubKey()) << OP_CHECKSIG;
    CBlock block = CreateAndProcessBlock({}, scriptPubKey);
    { LOCK(cs_main); BOOST_CHECK(m_node.chainman->ActiveChain().Tip()->GetBlockHash() == block.GetHash()); }
    for (int i = 0; i < 100; i++) {
        CScript scriptPubKey = CScript() << ToByteVector(coinbaseKey.GetPubKey()) << OP_CHECKSIG;
        CBlock block = CreateAndProcessBlock({}, scriptPubKey);
        { LOCK(cs_main); BOOST_CHECK(m_node.chainman->ActiveChain().Tip()->GetBlockHash() == block.GetHash()); }
    }

    CMutableTransaction mtx;
    COutPoint preout = COutPoint(block.vtx[0]->GetHash(), 0);
    mtx.vin.emplace_back(CTxIn(preout));

    std::vector<std::string> warning_strings;

    {
        LOCK(cs_main);
        std::tuple<uint32_t, std::vector<CCompressedInput>> cinputs = m_node.chainman->ActiveChainstate().CompressOutPoints(CTransaction(mtx), true, warning_strings);
        BOOST_CHECK(warning_strings.size() == 0);
        BOOST_CHECK(std::get<1>(cinputs).size() == 1);
        BOOST_CHECK(std::get<1>(cinputs)[0].scriptPubKey() == scriptPubKey);
        BOOST_CHECK(std::get<1>(cinputs)[0].outpoint().IsCompressed());
    }
}

//Input Valid But Uncompressed
BOOST_AUTO_TEST_CASE(compressing_outpoint_9)
{
    CScript scriptPubKey = CScript() << ToByteVector(coinbaseKey.GetPubKey()) << OP_CHECKSIG;
    CBlock block = CreateAndProcessBlock({}, scriptPubKey);
    { LOCK(cs_main); BOOST_CHECK(m_node.chainman->ActiveChain().Tip()->GetBlockHash() == block.GetHash()); }
    for (int i = 0; i < 100; i++) {
        CScript scriptPubKey = CScript() << ToByteVector(coinbaseKey.GetPubKey()) << OP_CHECKSIG;
        CBlock block = CreateAndProcessBlock({}, scriptPubKey);
        { LOCK(cs_main); BOOST_CHECK(m_node.chainman->ActiveChain().Tip()->GetBlockHash() == block.GetHash()); }
    }

    CMutableTransaction mtx;
    COutPoint preout = COutPoint(block.vtx[0]->GetHash(), 0);
    mtx.vin.emplace_back(CTxIn(preout));

    std::vector<std::string> warning_strings;

    {
        LOCK(cs_main);
        std::tuple<uint32_t, std::vector<CCompressedInput>> cinputs = m_node.chainman->ActiveChainstate().CompressOutPoints(CTransaction(mtx), false, warning_strings);
        BOOST_CHECK(warning_strings.size() == 0);
        BOOST_CHECK(std::get<1>(cinputs).size() == 1);
        BOOST_CHECK(std::get<1>(cinputs)[0].scriptPubKey() == scriptPubKey);
        BOOST_CHECK(!std::get<1>(cinputs)[0].outpoint().IsCompressed());
    }
}

//Input Uncompressed
BOOST_AUTO_TEST_CASE(decompressing_outpoint_1)
{
    CScript scriptPubKey = CScript() << ToByteVector(coinbaseKey.GetPubKey()) << OP_CHECKSIG;
    CBlock block = CreateAndProcessBlock({}, scriptPubKey);
    { LOCK(cs_main); BOOST_CHECK(m_node.chainman->ActiveChain().Tip()->GetBlockHash() == block.GetHash()); }
    CCompressedOutPoint coutpoint = CCompressedOutPoint(block.vtx[0]->GetHash(), 0);
    CCompressedTxIn ctxin = CCompressedTxIn(CTxIn(), coutpoint, CScript());
    std::vector<COutPoint> prevouts;

    {
        LOCK(cs_main);
        BOOST_CHECK(m_node.chainman->ActiveChainstate().DecompressOutPoints(0, {ctxin}, prevouts));
    }
}

//Input Compressed Unreadable block/index
BOOST_AUTO_TEST_CASE(decompressing_outpoint_2)
{
    CScript scriptPubKey = CScript() << ToByteVector(coinbaseKey.GetPubKey()) << OP_CHECKSIG;
    CBlock block = CreateAndProcessBlock({}, scriptPubKey);
    { LOCK(cs_main); BOOST_CHECK(m_node.chainman->ActiveChain().Tip()->GetBlockHash() == block.GetHash()); }
    CCompressedOutPoint coutpoint = CCompressedOutPoint(2, 0);
    CCompressedTxIn ctxin = CCompressedTxIn(CTxIn(), coutpoint, CScript());
    std::vector<COutPoint> prevouts;

    {
        LOCK(cs_main);
        BOOST_CHECK(m_node.chainman->ActiveChainstate().DecompressOutPoints(0, {ctxin}, prevouts));
    }
}

//Input Compressed
BOOST_AUTO_TEST_CASE(decompressing_outpoint_3)
{
    CScript scriptPubKey = CScript() << ToByteVector(coinbaseKey.GetPubKey()) << OP_CHECKSIG;
    CBlock block = CreateAndProcessBlock({}, scriptPubKey);
    { LOCK(cs_main); BOOST_CHECK(m_node.chainman->ActiveChain().Tip()->GetBlockHash() == block.GetHash()); }
    CCompressedOutPoint coutpoint = CCompressedOutPoint(1, 0);
    CCompressedTxIn ctxin = CCompressedTxIn(CTxIn(), coutpoint, CScript());
    std::vector<COutPoint> prevouts;

    {
        LOCK(cs_main);
        BOOST_CHECK(m_node.chainman->ActiveChainstate().DecompressOutPoints(0, {ctxin}, prevouts));
    }
}

//Signature: P2PKH invalid signature
BOOST_AUTO_TEST_CASE(roundtrip_txin_1)
{
    CScript scriptPubKey = GetScriptForDestination(PKHash(coinbaseKey.GetPubKey()));
    CBlock block = CreateAndProcessBlock({}, scriptPubKey);
    { LOCK(cs_main); BOOST_CHECK(m_node.chainman->ActiveChain().Tip()->GetBlockHash() == block.GetHash()); }
    CMutableTransaction mtx;
    CScript signature = CScript(0x385735ae);
    mtx.vin.emplace_back(CTxIn(COutPoint(block.vtx[0]->GetHash(), 0), signature));
    mtx.vout.emplace_back(CTxOut(CAmount(100), CScript(0x37332847afeb873)));

    CCompressedInput cinput = CCompressedInput(CCompressedOutPoint(0, 0), scriptPubKey);
    CCompressedTransaction ctx = CCompressedTransaction(CTransaction(mtx), 0, {cinput});

    BOOST_CHECK(!ctx.vin()[0].compressedSignature());
    BOOST_CHECK(ctx.vin()[0].warning() == "Signature could not be compressed");

    DataStream stream;
    ctx.Serialize(stream);
    CCompressedTransaction dsctx = CCompressedTransaction(deserialize, stream);
    BOOST_CHECK(ctx == dsctx);

    CMutableTransaction ucmtx = CMutableTransaction(ctx, {COutPoint(block.vtx.at(0)->GetHash(), 0)}, {CTxOut(block.vtx.at(0)->vout.at(0).nValue, scriptPubKey)});

    BOOST_CHECK(ucmtx.vin[0].scriptSig == signature);
    BOOST_CHECK(ucmtx.vin[0].scriptWitness.stack == mtx.vin[0].scriptWitness.stack);
}

//Signature: Custom
BOOST_AUTO_TEST_CASE(roundtrip_txin_2)
{
    CScript scriptPubKey = CScript(0x348234ae);
    CBlock block = CreateAndProcessBlock({}, scriptPubKey);
    { LOCK(cs_main); BOOST_CHECK(m_node.chainman->ActiveChain().Tip()->GetBlockHash() == block.GetHash()); }
    CMutableTransaction mtx;
    CScript signature = CScript(0x485774a2);
    mtx.vin.emplace_back(CTxIn(COutPoint(block.vtx[0]->GetHash(), 0), signature));
    mtx.vout.emplace_back(CTxOut(CAmount(100), CScript(0x37332847afeb873)));

    CCompressedInput cinput = CCompressedInput(CCompressedOutPoint(0, 0), scriptPubKey);
    CCompressedTransaction ctx = CCompressedTransaction(CTransaction(mtx), 0, {cinput});

    BOOST_CHECK(!ctx.vin()[0].compressedSignature());
    BOOST_CHECK(ctx.vin()[0].warning() == "");

    DataStream stream;
    ctx.Serialize(stream);
    CCompressedTransaction dsctx = CCompressedTransaction(deserialize, stream);
    BOOST_CHECK(ctx == dsctx);

    CMutableTransaction ucmtx = CMutableTransaction(ctx, {COutPoint(block.vtx.at(0)->GetHash(), 0)}, {CTxOut(block.vtx.at(0)->vout.at(0).nValue, scriptPubKey)});

    BOOST_CHECK(ucmtx.vin[0].scriptSig == signature);
    BOOST_CHECK(ucmtx.vin[0].scriptWitness.stack == mtx.vin[0].scriptWitness.stack);
}

//Signature: P2SH
BOOST_AUTO_TEST_CASE(roundtrip_txin_3)
{
    CScript scriptPubKey = GetScriptForDestination(ScriptHash(GetScriptForDestination(PKHash(coinbaseKey.GetPubKey()))));
    CBlock block = CreateAndProcessBlock({}, scriptPubKey);
    { LOCK(cs_main); BOOST_CHECK(m_node.chainman->ActiveChain().Tip()->GetBlockHash() == block.GetHash()); }
    CMutableTransaction mtx;
    mtx.vin.emplace_back(CTxIn(COutPoint(block.vtx[0]->GetHash(), 0)));
    mtx.vout.emplace_back(CTxOut(CAmount(100), CScript(0x37332847afeb873)));

    FillableSigningProvider keystore;
    keystore.AddKey(coinbaseKey);
    keystore.AddCScript(GetScriptForDestination(PKHash(coinbaseKey.GetPubKey())));

    std::map<COutPoint, Coin> coins;
    coins[mtx.vin[0].prevout];
    FindCoins(m_node, coins);

    std::map<int, bilingual_str> input_errors;
    ::SignTransaction(mtx, &keystore, coins, SIGHASH_ALL, input_errors);
    BOOST_CHECK(input_errors.size() == 0);

    CCompressedInput cinput = CCompressedInput(CCompressedOutPoint(0, 0), scriptPubKey);
    CCompressedTransaction ctx = CCompressedTransaction(CTransaction(mtx), 0, {cinput});

    BOOST_CHECK(!ctx.vin()[0].compressedSignature());
    BOOST_CHECK(ctx.vin()[0].warning() == "");

    DataStream stream;
    ctx.Serialize(stream);
    CCompressedTransaction dsctx = CCompressedTransaction(deserialize, stream);
    BOOST_CHECK(ctx == dsctx);

    CMutableTransaction ucmtx = CMutableTransaction(ctx, {COutPoint(block.vtx.at(0)->GetHash(), 0)}, {CTxOut(block.vtx.at(0)->vout.at(0).nValue, scriptPubKey)});

    BOOST_CHECK(ucmtx.vin[0].scriptSig == mtx.vin[0].scriptSig);
    BOOST_CHECK(ucmtx.vin[0].scriptWitness.stack == mtx.vin[0].scriptWitness.stack);
}

//Signature: P2PKH
BOOST_AUTO_TEST_CASE(roundtrip_txin_4)
{
    CScript scriptPubKey = GetScriptForDestination(PKHash(coinbaseKey.GetPubKey()));
    CBlock block = CreateAndProcessBlock({}, scriptPubKey);
    { LOCK(cs_main); BOOST_CHECK(m_node.chainman->ActiveChain().Tip()->GetBlockHash() == block.GetHash()); }
    CMutableTransaction mtx;
    mtx.vin.emplace_back(CTxIn(COutPoint(block.vtx[0]->GetHash(), 0)));
    mtx.vout.emplace_back(CTxOut(CAmount(100), CScript(0x37332847afeb873)));

    FillableSigningProvider keystore;
    keystore.AddKey(coinbaseKey);

    std::map<COutPoint, Coin> coins;
    coins[mtx.vin[0].prevout];
    FindCoins(m_node, coins);

    std::map<int, bilingual_str> input_errors;
    ::SignTransaction(mtx, &keystore, coins, SIGHASH_ALL, input_errors);
    BOOST_CHECK(input_errors.size() == 0);

    CCompressedInput cinput = CCompressedInput(CCompressedOutPoint(0, 0), scriptPubKey);
    CCompressedTransaction ctx = CCompressedTransaction(CTransaction(mtx), 0, {cinput});

    BOOST_CHECK(ctx.vin()[0].compressedSignature());
    BOOST_CHECK(ctx.vin()[0].signature().size() == 64);
    BOOST_CHECK(ctx.vin()[0].warning() == "");

    DataStream stream;
    ctx.Serialize(stream);
    CCompressedTransaction dsctx = CCompressedTransaction(deserialize, stream);
    BOOST_CHECK(ctx == dsctx);

    CMutableTransaction ucmtx = CMutableTransaction(ctx, {COutPoint(block.vtx.at(0)->GetHash(), 0)}, {CTxOut(block.vtx.at(0)->vout.at(0).nValue, scriptPubKey)});

    BOOST_CHECK(ucmtx.vin[0].scriptSig == mtx.vin[0].scriptSig);
    BOOST_CHECK(ucmtx.vin[0].scriptWitness.stack == mtx.vin[0].scriptWitness.stack);
}

//Signature: P2WPKH
BOOST_AUTO_TEST_CASE(roundtrip_txin_5)
{
    CScript scriptPubKey = GetScriptForDestination(WitnessV0KeyHash(coinbaseKey.GetPubKey()));
    CBlock block = CreateAndProcessBlock({}, scriptPubKey);
    { LOCK(cs_main); BOOST_CHECK(m_node.chainman->ActiveChain().Tip()->GetBlockHash() == block.GetHash()); }
    CMutableTransaction mtx;
    mtx.vin.emplace_back(CTxIn(COutPoint(block.vtx[0]->GetHash(), 0)));
    mtx.vout.emplace_back(CTxOut(CAmount(100), CScript(0x37332847afeb873)));

    FillableSigningProvider keystore;
    keystore.AddKey(coinbaseKey);

    std::map<COutPoint, Coin> coins;
    coins[mtx.vin[0].prevout];
    FindCoins(m_node, coins);

    std::map<int, bilingual_str> input_errors;
    ::SignTransaction(mtx, &keystore, coins, SIGHASH_ALL, input_errors);
    BOOST_CHECK(input_errors.size() == 0);

    CCompressedInput cinput = CCompressedInput(CCompressedOutPoint(0, 0), scriptPubKey);
    CCompressedTransaction ctx = CCompressedTransaction(CTransaction(mtx), 0, {cinput});

    BOOST_CHECK(ctx.vin()[0].compressedSignature());
    BOOST_CHECK(ctx.vin()[0].signature().size() == 64);
    BOOST_CHECK(ctx.vin()[0].warning() == "");

    DataStream stream;
    ctx.Serialize(stream);
    CCompressedTransaction dsctx = CCompressedTransaction(deserialize, stream);
    BOOST_CHECK(ctx == dsctx);

    CMutableTransaction ucmtx = CMutableTransaction(ctx, {COutPoint(block.vtx.at(0)->GetHash(), 0)}, {CTxOut(block.vtx.at(0)->vout.at(0).nValue, scriptPubKey)});

    BOOST_CHECK(ucmtx.vin[0].scriptSig == mtx.vin[0].scriptSig);
    BOOST_CHECK(ucmtx.vin[0].scriptWitness.stack == mtx.vin[0].scriptWitness.stack);
}

//Signature: P2SH-P2WPKH
BOOST_AUTO_TEST_CASE(roundtrip_txin_6)
{
    CScript scriptPubKey = GetScriptForDestination(ScriptHash(GetScriptForDestination(WitnessV0KeyHash(coinbaseKey.GetPubKey()))));
    CBlock block = CreateAndProcessBlock({}, scriptPubKey);
    { LOCK(cs_main); BOOST_CHECK(m_node.chainman->ActiveChain().Tip()->GetBlockHash() == block.GetHash()); }
    CMutableTransaction mtx;
    mtx.vin.emplace_back(CTxIn(COutPoint(block.vtx[0]->GetHash(), 0)));
    mtx.vout.emplace_back(CTxOut(CAmount(100), CScript(0x37332847afeb873)));

    FillableSigningProvider keystore;
    keystore.AddKey(coinbaseKey);

    std::map<COutPoint, Coin> coins;
    coins[mtx.vin[0].prevout];
    FindCoins(m_node, coins);

    std::map<int, bilingual_str> input_errors;
    ::SignTransaction(mtx, &keystore, coins, SIGHASH_ALL, input_errors);
    BOOST_CHECK(input_errors.size() == 0);

    CCompressedInput cinput = CCompressedInput(CCompressedOutPoint(0, 0), scriptPubKey);
    CCompressedTransaction ctx = CCompressedTransaction(CTransaction(mtx), 0, {cinput});

    BOOST_CHECK(ctx.vin()[0].compressedSignature());
    BOOST_CHECK(ctx.vin()[0].signature().size() == 64);
    BOOST_CHECK(ctx.vin()[0].pubkeyHash().size() == 20);
    BOOST_CHECK(ctx.vin()[0].warning() == "");

    DataStream stream;
    ctx.Serialize(stream);
    CCompressedTransaction dsctx = CCompressedTransaction(deserialize, stream);
    BOOST_CHECK(ctx == dsctx);

    CMutableTransaction ucmtx = CMutableTransaction(ctx, {COutPoint(block.vtx.at(0)->GetHash(), 0)}, {CTxOut(block.vtx.at(0)->vout.at(0).nValue, scriptPubKey)});

    BOOST_CHECK(ucmtx.vin[0].scriptSig == mtx.vin[0].scriptSig);
    BOOST_CHECK(ucmtx.vin[0].scriptWitness.stack == mtx.vin[0].scriptWitness.stack);
}

//Signature: P2TR
BOOST_AUTO_TEST_CASE(roundtrip_txin_7)
{
    CScript scriptPubKey = GetScriptForDestination(WitnessV1Taproot(XOnlyPubKey(coinbaseKey.GetPubKey())));
    CBlock block = CreateAndProcessBlock({}, scriptPubKey);
    { LOCK(cs_main); BOOST_CHECK(m_node.chainman->ActiveChain().Tip()->GetBlockHash() == block.GetHash()); }
    CMutableTransaction mtx;
    mtx.vin.emplace_back(CTxIn(COutPoint(block.vtx[0]->GetHash(), 0)));
    mtx.vout.emplace_back(CTxOut(CAmount(100), CScript(0x37332847afeb873)));

    FillableSigningProvider keystore;
    keystore.AddKey(coinbaseKey);

    std::map<COutPoint, Coin> coins;
    coins[mtx.vin[0].prevout];
    FindCoins(m_node, coins);

    std::map<int, bilingual_str> input_errors;
    ::SignTransaction(mtx, &keystore, coins, SIGHASH_ALL, input_errors);
    BOOST_CHECK(input_errors.size() == 0);

    CCompressedInput cinput = CCompressedInput(CCompressedOutPoint(0, 0), scriptPubKey);
    CCompressedTransaction ctx = CCompressedTransaction(CTransaction(mtx), 0, {cinput});

    BOOST_CHECK(ctx.vin()[0].compressedSignature());
    BOOST_CHECK(ctx.vin()[0].signature().size() == 64);
    BOOST_CHECK(ctx.vin()[0].warning() == "");

    DataStream stream;
    ctx.Serialize(stream);
    CCompressedTransaction dsctx = CCompressedTransaction(deserialize, stream);
    BOOST_CHECK(ctx == dsctx);

    CMutableTransaction ucmtx = CMutableTransaction(ctx, {COutPoint(block.vtx.at(0)->GetHash(), 0)}, {CTxOut(block.vtx.at(0)->vout.at(0).nValue, scriptPubKey)});

    BOOST_CHECK(ucmtx.vin[0].scriptSig == mtx.vin[0].scriptSig);
    BOOST_CHECK(ucmtx.vin[0].scriptWitness.stack == mtx.vin[0].scriptWitness.stack);
}

//ScriptPubKey: Custom
BOOST_AUTO_TEST_CASE(roundtrip_txout_1)
{
    CScript scriptPubKey = CScript(0x8474721afeb);
    CTxOut txout = CTxOut(CAmount(0), scriptPubKey);
    CCompressedTxOut ctxout = CCompressedTxOut(txout);
    BOOST_CHECK(ctxout.scriptType() == TxoutType::NONSTANDARD);

    DataStream stream;
    ctxout.Serialize(stream);
    CCompressedTxOut dsctxout = CCompressedTxOut(deserialize, stream, ctxout.GetSerializedScriptType());
    BOOST_CHECK(ctxout == dsctxout);

    CTxOut uctxout = CTxOut(ctxout);
    BOOST_CHECK(uctxout.scriptPubKey == scriptPubKey);
}

//ScriptPubKey: P2PK
BOOST_AUTO_TEST_CASE(roundtrip_txout_2)
{
    CScript scriptPubKey = CScript() << ToByteVector(coinbaseKey.GetPubKey()) << OP_CHECKSIG;
    CTxOut txout = CTxOut(CAmount(0), scriptPubKey);
    CCompressedTxOut ctxout = CCompressedTxOut(txout);
    BOOST_CHECK(ctxout.scriptType() == TxoutType::PUBKEY);

    DataStream stream;
    ctxout.Serialize(stream);
    CCompressedTxOut dsctxout = CCompressedTxOut(deserialize, stream, ctxout.GetSerializedScriptType());
    BOOST_CHECK(ctxout == dsctxout);

    CTxOut uctxout = CTxOut(ctxout);
    BOOST_CHECK(uctxout.scriptPubKey == scriptPubKey);
}

//ScriptPubKey: P2PKH
BOOST_AUTO_TEST_CASE(roundtrip_txout_3)
{
    CScript scriptPubKey = GetScriptForDestination(PKHash(coinbaseKey.GetPubKey()));
    CTxOut txout = CTxOut(CAmount(0), scriptPubKey);
    CCompressedTxOut ctxout = CCompressedTxOut(txout);
    BOOST_CHECK(ctxout.scriptType() == TxoutType::PUBKEYHASH);

    CTxOut uctxout = CTxOut(ctxout);
    BOOST_CHECK(uctxout.scriptPubKey == scriptPubKey);
}

//ScriptPubKey: P2SH
BOOST_AUTO_TEST_CASE(roundtrip_txout_4)
{
    CScript scriptPubKey = GetScriptForDestination(ScriptHash(GetScriptForDestination(PKHash(coinbaseKey.GetPubKey()))));
    CTxOut txout = CTxOut(CAmount(0), scriptPubKey);
    CCompressedTxOut ctxout = CCompressedTxOut(txout);
    BOOST_CHECK(ctxout.scriptType() == TxoutType::SCRIPTHASH);

    CTxOut uctxout = CTxOut(ctxout);
    BOOST_CHECK(uctxout.scriptPubKey == scriptPubKey);
}

//ScriptPubKey: P2WPKH
BOOST_AUTO_TEST_CASE(roundtrip_txout_5)
{
    CScript scriptPubKey = GetScriptForDestination(WitnessV0KeyHash(coinbaseKey.GetPubKey()));
    CTxOut txout = CTxOut(CAmount(0), scriptPubKey);
    CCompressedTxOut ctxout = CCompressedTxOut(txout);
    BOOST_CHECK(ctxout.scriptType() == TxoutType::WITNESS_V0_KEYHASH);

    CTxOut uctxout = CTxOut(ctxout);
    BOOST_CHECK(uctxout.scriptPubKey == scriptPubKey);
}

//ScriptPubKey: P2WSH
BOOST_AUTO_TEST_CASE(roundtrip_txout_6)
{
    CScript scriptPubKey = GetScriptForDestination(WitnessV0ScriptHash(GetScriptForDestination(WitnessV0KeyHash(coinbaseKey.GetPubKey()))));
    CTxOut txout = CTxOut(CAmount(0), scriptPubKey);
    CCompressedTxOut ctxout = CCompressedTxOut(txout);
    BOOST_CHECK(ctxout.scriptType() == TxoutType::WITNESS_V0_SCRIPTHASH);

    CTxOut uctxout = CTxOut(ctxout);
    BOOST_CHECK(uctxout.scriptPubKey == scriptPubKey);
}

//ScriptPubKey: P2TR
BOOST_AUTO_TEST_CASE(roundtrip_txout_7)
{
    CScript scriptPubKey = GetScriptForDestination(WitnessV1Taproot(XOnlyPubKey(coinbaseKey.GetPubKey())));
    CTxOut txout = CTxOut(CAmount(0), scriptPubKey);
    CCompressedTxOut ctxout = CCompressedTxOut(txout);
    BOOST_CHECK(ctxout.scriptType() == TxoutType::WITNESS_V1_TAPROOT);

    CTxOut uctxout = CTxOut(ctxout);
    BOOST_CHECK(uctxout.scriptPubKey == scriptPubKey);
}

BOOST_AUTO_TEST_CASE(roundtrip_locktime_1)
{
    CMutableTransaction mtx;
    mtx.nLockTime = 100;
    CCompressedTransaction ctx = CCompressedTransaction(CTransaction(mtx), 50, {});
    BOOST_CHECK(ctx.nLockTime() == 50);

    CMutableTransaction ucmtx = CMutableTransaction(ctx, {}, {});

    BOOST_CHECK(mtx.nLockTime == ucmtx.nLockTime);
}

BOOST_AUTO_TEST_CASE(roundtrip_transaction_1)
{
    CScript scriptPubKey = GetScriptForDestination(WitnessV1Taproot(XOnlyPubKey(coinbaseKey.GetPubKey())));
    CBlock block = CreateAndProcessBlock({}, scriptPubKey);
    { LOCK(cs_main); BOOST_CHECK(m_node.chainman->ActiveChain().Tip()->GetBlockHash() == block.GetHash()); }
    CMutableTransaction mtx;
    mtx.nVersion = 0xabcd37;
    mtx.nLockTime = 0xfbc387;
    mtx.vin.emplace_back(CTxIn(COutPoint(block.vtx[0]->GetHash(), 0)));
    mtx.vout.emplace_back(CTxOut(CAmount(100), CScript(0x37332847afeb873)));

    FillableSigningProvider keystore;
    keystore.AddKey(coinbaseKey);

    std::map<COutPoint, Coin> coins;
    coins[mtx.vin[0].prevout];
    FindCoins(m_node, coins);

    std::map<int, bilingual_str> input_errors;
    ::SignTransaction(mtx, &keystore, coins, SIGHASH_ALL, input_errors);
    BOOST_CHECK(input_errors.size() == 0);

    CCompressedInput cinput = CCompressedInput(CCompressedOutPoint(0, 0), scriptPubKey);
    CCompressedTransaction ctx = CCompressedTransaction(CTransaction(mtx), 0, {cinput});

    DataStream stream;
    ctx.Serialize(stream);
    CCompressedTransaction dsctx = CCompressedTransaction(deserialize, stream);
    BOOST_CHECK(ctx == dsctx);

    CMutableTransaction ucmtx = CMutableTransaction(ctx, {COutPoint(block.vtx.at(0)->GetHash(), 0)}, {CTxOut(block.vtx.at(0)->vout.at(0).nValue, scriptPubKey)});

    BOOST_CHECK(CTransaction(mtx) == CTransaction(ucmtx));
}

BOOST_AUTO_TEST_SUITE_END()
