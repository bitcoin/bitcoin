#include "omnicore/createtx.h"

#include "base58.h"
#include "coins.h"
#include "core_io.h"
#include "main.h"
#include "primitives/transaction.h"
#include "script/script.h"
#include "script/standard.h"
#include "test/test_bitcoin.h"
#include "utilstrencodings.h"

#include <boost/test/unit_test.hpp>

#include <stdint.h>
#include <string>
#include <utility>
#include <vector>

// Is resetted after the last test
extern CFeeRate minRelayTxFee;
static CFeeRate minRelayTxFeeOriginal = CFeeRate(DEFAULT_MIN_RELAY_TX_FEE);

BOOST_FIXTURE_TEST_SUITE(omnicore_create_tx_tests, BasicTestingSetup)

BOOST_AUTO_TEST_CASE(txbuilder_empty)
{
    TxBuilder builder;
    CMutableTransaction tx = builder.build();

    BOOST_CHECK_EQUAL("01000000000000000000", EncodeHexTx(CTransaction(tx)));
}

BOOST_AUTO_TEST_CASE(txbuilder_from_existing)
{
    std::string rawTx("0100000001ea6f7b27245fb97eca56c942600b31102d42ef2cc04b3990e63fea9619e137110300000000"
        "ffffffff0141e40000000000001976a9140b15428b98e6a459cc2ffeed085153dc1bc8078188ac00000000");

    CTransaction txBasis;
    BOOST_CHECK(DecodeHexTx(txBasis, rawTx));

    CMutableTransaction tx = TxBuilder(txBasis).build();
    BOOST_CHECK_EQUAL(rawTx, EncodeHexTx(CTransaction(tx)));
}

BOOST_AUTO_TEST_CASE(txbuilder_add_outpoint)
{
    std::string rawTx("010000000270ef6bf12e5155116532c4696e59c354d39c639db7498ce86f1e479b4ac6fbdd0200000000"
        "ffffffff50259f6673c372006ffa6f52309bc3b68501e3dbdaadc910b81846a0202792b10000000000ffffffff00000000"
        "00");

    CMutableTransaction tx = TxBuilder()
        .addInput(COutPoint(uint256S("ddfbc64a9b471e6fe88c49b79d639cd354c3596e69c432651155512ef16bef70"), 2))
        .addInput(COutPoint(uint256S("b1922720a04618b810c9addadbe30185b6c39b30526ffa6f0072c373669f2550"), 0))
        .build();

    BOOST_CHECK_EQUAL(rawTx, EncodeHexTx(CTransaction(tx)));
}

BOOST_AUTO_TEST_CASE(txbuilder_add_input)
{
    std::string rawTx("010000000270ef6bf12e5155116532c4696e59c354d39c639db7498ce86f1e479b4ac6fbdd0200000000"
        "ffffffff50259f6673c372006ffa6f52309bc3b68501e3dbdaadc910b81846a0202792b10000000000ffffffff00000000"
        "00");

    CMutableTransaction tx = TxBuilder()
        .addInput(uint256S("ddfbc64a9b471e6fe88c49b79d639cd354c3596e69c432651155512ef16bef70"), 2)
        .addInput(uint256S("b1922720a04618b810c9addadbe30185b6c39b30526ffa6f0072c373669f2550"), 0)
        .build();

    BOOST_CHECK_EQUAL(rawTx, EncodeHexTx(CTransaction(tx)));
}

BOOST_AUTO_TEST_CASE(txbuilder_add_output)
{
    std::string rawTxBasis("0100000001bfdad3c09f11cdff041295dc83cf4df8a76a8478b73ab3bf6695e37f8068c20800000"
        "00000ffffffff0100e1f505000000001976a9140a45d5830d00b972fe00f92f919889875ec5437e88ac00000000");
    std::string rawTx("0100000001bfdad3c09f11cdff041295dc83cf4df8a76a8478b73ab3bf6695e37f8068c2080000000000"
        "ffffffff0200e1f505000000001976a9140a45d5830d00b972fe00f92f919889875ec5437e88ac008d1418000000001976"
        "a9140e609a27d6389989a0fa7ffaac1ae8ad3e92650e88ac00000000");
    std::vector<unsigned char> script = ParseHex("76a9140e609a27d6389989a0fa7ffaac1ae8ad3e92650e88ac");

    CTransaction txBasis;
    BOOST_CHECK(DecodeHexTx(txBasis, rawTxBasis));

    CMutableTransaction tx = TxBuilder(txBasis)
        .addOutput(CScript(script.begin(), script.end()), 404000000LL)
        .build();

    BOOST_CHECK_EQUAL(rawTx, EncodeHexTx(CTransaction(tx)));
}

BOOST_AUTO_TEST_CASE(txbuilder_add_outputs)
{
    std::string rawTx("0100000000031203000000000000695121028f925841f1a8090b6d98e3272eacda571f9dceec50c9ca0b"
        "24d1d7da57a3ab4821020abd5dadd6326adab819bc643ab7c6f30af81fcfe6ba2cb0ac697a8bba67288a2102ecb5ba7bcc"
        "e4579855c9e5f250a90747433f79c080cd6865c6163012dbe6434353ae22020000000000001976a914643ce12b15906330"
        "77b8620316f43a9362ef18e588acfc150f00000000001976a9142123d4b097b822b58c3798c1a01cebf0b0ff6edc88ac00"
        "000000");
    std::vector<unsigned char> scriptA = ParseHex("5121028f925841f1a8090b6d98e3272eacda571f9dceec50c9ca0b24"
        "d1d7da57a3ab4821020abd5dadd6326adab819bc643ab7c6f30af81fcfe6ba2cb0ac697a8bba67288a2102ecb5ba7bcce4"
        "579855c9e5f250a90747433f79c080cd6865c6163012dbe6434353ae");
    std::vector<unsigned char> scriptB = ParseHex("76a914643ce12b1590633077b8620316f43a9362ef18e588ac");
    std::vector<unsigned char> scriptC = ParseHex("76a9142123d4b097b822b58c3798c1a01cebf0b0ff6edc88ac");

    std::vector<std::pair<CScript, int64_t> > outputs;
    outputs.push_back(std::make_pair(CScript(scriptA.begin(), scriptA.end()), 786LL));
    outputs.push_back(std::make_pair(CScript(scriptB.begin(), scriptB.end()), 546LL));
    outputs.push_back(std::make_pair(CScript(scriptC.begin(), scriptC.end()), 988668LL));

    TxBuilder builder;
    builder.addOutputs(outputs);

    CMutableTransaction tx = builder.build();
    BOOST_CHECK_EQUAL(rawTx, EncodeHexTx(CTransaction(tx)));
}

BOOST_AUTO_TEST_CASE(txbuilder_add_change)
{
    std::string rawTx("0100000002605842f019601de54248b9fb4a32b498a7762c6513213b39041fbd89890e3a010200000000"
        "ffffffff878b454ca384a37d72762477406003397ed90b1f8d5ad4061af29ee541162c260000000000ffffffff0280d1f0"
        "08000000001976a91442784829d93f6de9b2c43b8f7fe9cd4bfbe9e2e888ac5622b701000000001976a9141287169f63d2"
        "09d2fa640b8ec12f6188404ba51388ac00000000");
    std::vector<unsigned char> scriptA = ParseHex("76a9146093da1c808a3a93c7c729ef9b09e9a29bcfc9ed88ac");
    std::vector<unsigned char> scriptB = ParseHex("21036f617ea0d03059cfaf9959b35717906a131a69b1438cc0eaa026"
        "5ddb5faf95dcac");

    std::vector<PrevTxsEntry> prevTxs;
    prevTxs.push_back(PrevTxsEntry(
        uint256S("013a0e8989bd1f04393b2113652c76a798b4324afbb94842e51d6019f0425860"),
        2,
        99967336LL,
        CScript(scriptA.begin(), scriptA.end())));
    prevTxs.push_back(PrevTxsEntry(
        uint256S("262c1641e59ef21a06d45a8d1f0bd97e39036040772476727da384a34c458b87"),
        0,
        78825000LL,
        CScript(scriptB.begin(), scriptB.end())));
    
    CBitcoinAddress addrA("174TgzbFFWiKg1VWt8Z55EVP7rJ54jQSar");
    CBitcoinAddress addrB("12gxzZL9g6tWsX6ut8srcgcUTQ4c9wWuGS");

    CCoinsView viewDummy;
    CCoinsViewCache viewTemp(&viewDummy);
    InputsToView(prevTxs, viewTemp);

    CMutableTransaction tx = TxBuilder()
        .addInput(prevTxs[0].outPoint)
        .addOutput(GetScriptForDestination(addrA.Get()), 150000000LL)
        .addInput(prevTxs[1].outPoint)
        .build();

    BOOST_CHECK(viewTemp.HaveInputs(CTransaction(tx)));

    tx = TxBuilder(tx)
        .addChange(addrB.Get(), viewTemp, 13242LL)
        .build();

    BOOST_CHECK_EQUAL(rawTx, EncodeHexTx(CTransaction(tx)));
}

BOOST_AUTO_TEST_CASE(txbuilder_add_change_position)
{
    std::string rawTxBasis("0100000001e83e80ce7b1c618bb21acc4ffdf1f420998a2dea7f6f974edae7e5f3dd28be4401000"
        "00000ffffffff028058840c000000001976a914390ba459d1746d49221e031e5038b78e0d99e7b688ac80778e060000000"
        "01976a914c656177b31a1cf5f9f319ce67c6de16bb56922b788ac00000000");
    std::string rawTx("0100000001e83e80ce7b1c618bb21acc4ffdf1f420998a2dea7f6f974edae7e5f3dd28be440100000000"
        "ffffffff038058840c000000001976a914390ba459d1746d49221e031e5038b78e0d99e7b688acb0d1b90a000000001976"
        "a91486e4e00cacf83a6ff87d75caf551c5f2d4574a9d88ac80778e06000000001976a914c656177b31a1cf5f9f319ce67c"
        "6de16bb56922b788ac00000000");
    std::vector<unsigned char> script = ParseHex("76a91453d3a1e3aa03063d660d769acbe2be84821a0b1388ac");

    std::vector<PrevTxsEntry> prevTxs;
    prevTxs.push_back(PrevTxsEntry(
        uint256S("44be28ddf3e5e7da4e976f7fea2d8a9920f4f1fd4fcc1ab28b611c7bce803ee8"),
        1,
        500000000LL,
        CScript(script.begin(), script.end())));

    CBitcoinAddress addr("1DJFjEV9U7TgyDZVT1tcCGJDhDeRYSQGuD");

    CTransaction txBasis;
    BOOST_CHECK(DecodeHexTx(txBasis, rawTxBasis));

    CCoinsView viewDummy;
    CCoinsViewCache viewTemp(&viewDummy);
    InputsToView(prevTxs, viewTemp);
    BOOST_CHECK(viewTemp.HaveInputs(CTransaction(txBasis)));

    CMutableTransaction tx = TxBuilder(txBasis)
        .addChange(addr.Get(), viewTemp, 50000LL, 1)
        .build();

    BOOST_CHECK_EQUAL(rawTx, EncodeHexTx(CTransaction(tx)));
}

BOOST_AUTO_TEST_CASE(omnitxbuilder_empty)
{
    OmniTxBuilder builder;
    CMutableTransaction tx = builder.build();

    BOOST_CHECK_EQUAL("01000000000000000000", EncodeHexTx(CTransaction(tx)));
}

BOOST_AUTO_TEST_CASE(omnitxbuilder_from_existing)
{
    std::string rawTx("01000000017f63a90b9f89c1ad0616825d2565050e53ef044721f58822148193da511e76f8010000006b"
        "483045022100c135ed7eb933d97e59ea758e394f93a83ddc5861277da3ccfd28bd77d02d55c70220568e4255aa69fdb860"
        "7c54aeed434d6bb5ee6b5f2f0aa9886526e6b99af427320121037e60c8486de3b67c931b6ef9b07e814e58d027ff0134a9"
        "da45b5e3ae97ebc6e7ffffffff02d058ea05000000001976a914616cfeaf60ed1a4831dfa238f6e8c676e660aa9588ac00"
        "00000000000000166a146f6d6e6900000003000000260000001cede08a8000000000");

    CTransaction txBasis;
    BOOST_CHECK(DecodeHexTx(txBasis, rawTx));

    CMutableTransaction tx = OmniTxBuilder(txBasis).build();
    BOOST_CHECK_EQUAL(rawTx, EncodeHexTx(CTransaction(tx)));
}

BOOST_AUTO_TEST_CASE(omnitxbuilder_op_return)
{
    minRelayTxFee = CFeeRate(1000);

    std::string rawTx("01000000021dc7f242305900960a80cadd2a5d06d2cbbc4bbdd029db37c56a975487b8d4b20100000000"
        "fffffffff1c05e491be9b9c73b918e96b0774d0db4632b41ace5bfbc2fcb0a58561b02bc0200000000ffffffff03000000"
        "0000000000166a146f6d6e690000000000000001000000009502f9006449f605000000001976a914c359d7d2e140127dd2"
        "adbeb1b3e9fa644e7dbd8e88acaa0a0000000000001976a9141243d1aba8f18d9bae91dac065549f95c403d7cc88ac0000"
        "0000");
    std::vector<unsigned char> scriptA = ParseHex("76a914c359d7d2e140127dd2adbeb1b3e9fa644e7dbd8e88ac");
    std::vector<unsigned char> scriptB = ParseHex("76a914c359d7d2e140127dd2adbeb1b3e9fa644e7dbd8e88ac");
    std::vector<unsigned char> payload = ParseHex("0000000000000001000000009502f900");

    std::vector<PrevTxsEntry> prevTxs;
    prevTxs.push_back(PrevTxsEntry(
        uint256S("b2d4b88754976ac537db29d0bd4bbccbd2065d2addca800a9600593042f2c71d"),
        1,
        50000LL,
        CScript(scriptA.begin(), scriptA.end())));
    prevTxs.push_back(PrevTxsEntry(
        uint256S("bc021b56580acb2fbcbfe5ac412b63b40d4d77b0968e913bc7b9e91b495ec0f1"),
        2,
        99989454LL,
        CScript(scriptB.begin(), scriptB.end())));

    CCoinsView viewDummy;
    CCoinsViewCache viewTemp(&viewDummy);
    InputsToView(prevTxs, viewTemp);

    CMutableTransaction tx = OmniTxBuilder()
        .addInputs(prevTxs)
        .build();

    BOOST_CHECK(viewTemp.HaveInputs(CTransaction(tx)));

    tx = OmniTxBuilder(tx)
        .addOpReturn(payload)
        .addReference("12faQbtHsD7ECFweehhHoRYxiRjyB1d1uy", 2730LL)
        .addChange("1JovPp7XB8Tjc6E2fVPigxXtfmrFLBoMhK", viewTemp, 10000LL, 1)
        .build();

    BOOST_CHECK_EQUAL(rawTx, EncodeHexTx(CTransaction(tx)));

    minRelayTxFee = minRelayTxFeeOriginal;
}

// TODO: test addMultisig -- currently an issue due to the non-deterministic ECDSA point manipulation


BOOST_AUTO_TEST_SUITE_END()
