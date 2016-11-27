// Copyright (c) 2012-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "bloom.h"

#include "base58.h"
#include "clientversion.h"
#include "core_io.h"
#include "key.h"
#include "merkleblock.h"
#include "random.h"
#include "serialize.h"
#include "streams.h"
#include "uint256.h"
#include "util.h"
#include "utilstrencodings.h"
#include "test/test_bitcoin.h"

#include <vector>

#include <boost/test/unit_test.hpp>
#include <boost/tuple/tuple.hpp>

using namespace std;

BOOST_FIXTURE_TEST_SUITE(bloom_tests, BasicTestingSetup)

BOOST_AUTO_TEST_CASE(bloom_create_insert_serialize)
{
    CBloomFilter filter(3, 0.01, 0, BLOOM_UPDATE_ALL);

    filter.insert(ParseHex("99108ad8ed9bb6274d3980bab5a85c048f0950c8"));
    BOOST_CHECK_MESSAGE( filter.contains(ParseHex("99108ad8ed9bb6274d3980bab5a85c048f0950c8")), "BloomFilter doesn't contain just-inserted object!");
    // One bit different in first byte
    BOOST_CHECK_MESSAGE(!filter.contains(ParseHex("19108ad8ed9bb6274d3980bab5a85c048f0950c8")), "BloomFilter contains something it shouldn't!");

    filter.insert(ParseHex("b5a2c786d9ef4658287ced5914b37a1b4aa32eee"));
    BOOST_CHECK_MESSAGE(filter.contains(ParseHex("b5a2c786d9ef4658287ced5914b37a1b4aa32eee")), "BloomFilter doesn't contain just-inserted object (2)!");

    filter.insert(ParseHex("b9300670b4c5366e95b2699e8b18bc75e5f729c5"));
    BOOST_CHECK_MESSAGE(filter.contains(ParseHex("b9300670b4c5366e95b2699e8b18bc75e5f729c5")), "BloomFilter doesn't contain just-inserted object (3)!");

    CDataStream stream(SER_NETWORK, PROTOCOL_VERSION);
    stream << filter;

    vector<unsigned char> vch = ParseHex("03614e9b050000000000000001");
    vector<char> expected(vch.size());

    for (unsigned int i = 0; i < vch.size(); i++)
        expected[i] = (char)vch[i];

    BOOST_CHECK_EQUAL_COLLECTIONS(stream.begin(), stream.end(), expected.begin(), expected.end());

    BOOST_CHECK_MESSAGE( filter.contains(ParseHex("99108ad8ed9bb6274d3980bab5a85c048f0950c8")), "BloomFilter doesn't contain just-inserted object!");
    filter.clear();
    BOOST_CHECK_MESSAGE( !filter.contains(ParseHex("99108ad8ed9bb6274d3980bab5a85c048f0950c8")), "BloomFilter should be empty!");
}

BOOST_AUTO_TEST_CASE(bloom_create_insert_serialize_with_tweak)
{
    // Same test as bloom_create_insert_serialize, but we add a nTweak of 100
    CBloomFilter filter(3, 0.01, 2147483649UL, BLOOM_UPDATE_ALL);

    filter.insert(ParseHex("99108ad8ed9bb6274d3980bab5a85c048f0950c8"));
    BOOST_CHECK_MESSAGE( filter.contains(ParseHex("99108ad8ed9bb6274d3980bab5a85c048f0950c8")), "BloomFilter doesn't contain just-inserted object!");
    // One bit different in first byte
    BOOST_CHECK_MESSAGE(!filter.contains(ParseHex("19108ad8ed9bb6274d3980bab5a85c048f0950c8")), "BloomFilter contains something it shouldn't!");

    filter.insert(ParseHex("b5a2c786d9ef4658287ced5914b37a1b4aa32eee"));
    BOOST_CHECK_MESSAGE(filter.contains(ParseHex("b5a2c786d9ef4658287ced5914b37a1b4aa32eee")), "BloomFilter doesn't contain just-inserted object (2)!");

    filter.insert(ParseHex("b9300670b4c5366e95b2699e8b18bc75e5f729c5"));
    BOOST_CHECK_MESSAGE(filter.contains(ParseHex("b9300670b4c5366e95b2699e8b18bc75e5f729c5")), "BloomFilter doesn't contain just-inserted object (3)!");

    CDataStream stream(SER_NETWORK, PROTOCOL_VERSION);
    stream << filter;

    vector<unsigned char> vch = ParseHex("03ce4299050000000100008001");
    vector<char> expected(vch.size());

    for (unsigned int i = 0; i < vch.size(); i++)
        expected[i] = (char)vch[i];

    BOOST_CHECK_EQUAL_COLLECTIONS(stream.begin(), stream.end(), expected.begin(), expected.end());
}

BOOST_AUTO_TEST_CASE(bloom_create_insert_key)
{
    string strSecret = string("5Kg1gnAjaLfKiwhhPpGS3QfRg2m6awQvaj98JCZBZQ5SuS2F15C");
    CBitcoinSecret vchSecret;
    BOOST_CHECK(vchSecret.SetString(strSecret));

    CKey key = vchSecret.GetKey();
    CPubKey pubkey = key.GetPubKey();
    vector<unsigned char> vchPubKey(pubkey.begin(), pubkey.end());

    CBloomFilter filter(2, 0.001, 0, BLOOM_UPDATE_ALL);
    filter.insert(vchPubKey);
    uint160 hash = pubkey.GetID();
    filter.insert(vector<unsigned char>(hash.begin(), hash.end()));

    CDataStream stream(SER_NETWORK, PROTOCOL_VERSION);
    stream << filter;

    vector<unsigned char> vch = ParseHex("038fc16b080000000000000001");
    vector<char> expected(vch.size());

    for (unsigned int i = 0; i < vch.size(); i++)
        expected[i] = (char)vch[i];

    BOOST_CHECK_EQUAL_COLLECTIONS(stream.begin(), stream.end(), expected.begin(), expected.end());
}

BOOST_AUTO_TEST_CASE(bloom_match)
{
    CMutableTransaction mtx;

    // Random real transaction (b4749f017444b051c44dfd2720e88f314ff94f3dd6d56d40ef65854fcd7fff6b)
    mtx.vin.resize(1);
    mtx.vin[0].prevout.hash = uint256S("0x90c122d70786e899529d71dbeba91ba216982fb6ba58f3bdaab65e73b7e9260b");
    mtx.vin[0].prevout.n = 0;
    mtx.vin[0].scriptSig = CScript() << ParseHex("30450220070aca44506c5cef3a16ed519d7c3c39f8aab192c4e1c90d065f37b8a4af6141022100a8e160b856c2d43d27d8fba71e5aef6405b8643ac4cb7cb3c462aced7f14711a01") << ParseHex("046d11fee51b0e60666d5049a9101a72741df480b96ee26488a4d3466b95c9a40ac5eeef87e10a5cd336c19a84565f80fa6c547957b7700ff4dfbdefe76036c339");
    mtx.vout.resize(2);
    mtx.vout[0].nValue = 0x113dff1b;
    mtx.vout[0].scriptPubKey = CScript() << OP_DUP << OP_HASH160 << ParseHex("04943fdd508053c75000106d3bc6e2754dbcff19") << OP_EQUALVERIFY << OP_CHECKSIG;
    mtx.vout[1].nValue = 0xde152f;
    mtx.vout[1].scriptPubKey = CScript() << OP_DUP << OP_HASH160 << ParseHex("a266436d2965547608b9e15d9032a7b9d64fa431") << OP_EQUALVERIFY << OP_CHECKSIG;
    CTransaction tx = mtx;
    uint256 tx_txid = tx.GetHash();

    // and one which spends it (e2769b09e784f32f62ef849763d4f45b98e07ba658647343b915ff832b110436)
    mtx.vin[0].prevout.hash = mtx.GetHash();
    mtx.vin[0].scriptSig = CScript() << ParseHex("3046022100da0dc6aecefe1e06efdf05773757deb168820930e3b0d03f46f5fcf150bf990c022100d25b5c87040076e4f253f8262e763e2dd51e7ff0be157727c4bc42807f17bd3901") << ParseHex("04e6c26ef67dc610d2cd192484789a6cf9aea9930b944b7e2db5342b9d9e5b9ff79aff9a2ee1978dd7fd01dfc522ee02283d3b06a9d03acf8096968d7dbb0f9178");
    mtx.vout[0].nValue = 0xe94a78b;
    mtx.vout[0].scriptPubKey = CScript() << OP_DUP << OP_HASH160 << ParseHex("badeecfdef0507247fc8f74241d73bc039972d7b") << OP_EQUALVERIFY << OP_CHECKSIG;
    mtx.vout[1].nValue = 0x2a89440;
    mtx.vout[1].scriptPubKey = CScript() << OP_DUP << OP_HASH160 << ParseHex("c10932483fec93ed51f5fe95e72559f2cc7043f9") << OP_EQUALVERIFY << OP_CHECKSIG;
    CTransaction spendingTx = mtx;

    CBloomFilter filter(10, 0.000001, 0, BLOOM_UPDATE_ALL);
    filter.insert(tx_txid);
    BOOST_CHECK_MESSAGE(filter.IsRelevantAndUpdate(tx), "Simple Bloom filter didn't match tx hash");

    filter = CBloomFilter(10, 0.000001, 0, BLOOM_UPDATE_ALL);
    // byte-reversed tx hash
    uint256 tx_revtxid = uint256S(std::string("0x") + tx_txid.ToString());
    filter.insert(tx_revtxid);
    BOOST_CHECK_MESSAGE(filter.IsRelevantAndUpdate(tx), "Simple Bloom filter didn't match manually serialized tx hash");

    filter = CBloomFilter(10, 0.000001, 0, BLOOM_UPDATE_ALL);
    filter.insert(ParseHex("30450220070aca44506c5cef3a16ed519d7c3c39f8aab192c4e1c90d065f37b8a4af6141022100a8e160b856c2d43d27d8fba71e5aef6405b8643ac4cb7cb3c462aced7f14711a01"));
    BOOST_CHECK_MESSAGE(filter.IsRelevantAndUpdate(tx), "Simple Bloom filter didn't match input signature");

    filter = CBloomFilter(10, 0.000001, 0, BLOOM_UPDATE_ALL);
    filter.insert(ParseHex("046d11fee51b0e60666d5049a9101a72741df480b96ee26488a4d3466b95c9a40ac5eeef87e10a5cd336c19a84565f80fa6c547957b7700ff4dfbdefe76036c339"));
    BOOST_CHECK_MESSAGE(filter.IsRelevantAndUpdate(tx), "Simple Bloom filter didn't match input pub key");

    filter = CBloomFilter(10, 0.000001, 0, BLOOM_UPDATE_ALL);
    filter.insert(ParseHex("04943fdd508053c75000106d3bc6e2754dbcff19"));
    BOOST_CHECK_MESSAGE(filter.IsRelevantAndUpdate(tx), "Simple Bloom filter didn't match output address");
    BOOST_CHECK_MESSAGE(filter.IsRelevantAndUpdate(spendingTx), "Simple Bloom filter didn't add output");

    filter = CBloomFilter(10, 0.000001, 0, BLOOM_UPDATE_ALL);
    filter.insert(ParseHex("a266436d2965547608b9e15d9032a7b9d64fa431"));
    BOOST_CHECK_MESSAGE(filter.IsRelevantAndUpdate(tx), "Simple Bloom filter didn't match output address");

    filter = CBloomFilter(10, 0.000001, 0, BLOOM_UPDATE_ALL);
    filter.insert(COutPoint(uint256S("0x90c122d70786e899529d71dbeba91ba216982fb6ba58f3bdaab65e73b7e9260b"), 0));
    BOOST_CHECK_MESSAGE(filter.IsRelevantAndUpdate(tx), "Simple Bloom filter didn't match COutPoint");

    filter = CBloomFilter(10, 0.000001, 0, BLOOM_UPDATE_ALL);
    COutPoint prevOutPoint(uint256S("0x90c122d70786e899529d71dbeba91ba216982fb6ba58f3bdaab65e73b7e9260b"), 0);
    {
        vector<unsigned char> data(32 + sizeof(unsigned int));
        memcpy(&data[0], prevOutPoint.hash.begin(), 32);
        memcpy(&data[32], &prevOutPoint.n, sizeof(unsigned int));
        filter.insert(data);
    }
    BOOST_CHECK_MESSAGE(filter.IsRelevantAndUpdate(tx), "Simple Bloom filter didn't match manually serialized COutPoint");

    filter = CBloomFilter(10, 0.000001, 0, BLOOM_UPDATE_ALL);
    filter.insert(uint256S("00000009e784f32f62ef849763d4f45b98e07ba658647343b915ff832b110436"));
    BOOST_CHECK_MESSAGE(!filter.IsRelevantAndUpdate(tx), "Simple Bloom filter matched random tx hash");

    filter = CBloomFilter(10, 0.000001, 0, BLOOM_UPDATE_ALL);
    filter.insert(ParseHex("0000006d2965547608b9e15d9032a7b9d64fa431"));
    BOOST_CHECK_MESSAGE(!filter.IsRelevantAndUpdate(tx), "Simple Bloom filter matched random address");

    filter = CBloomFilter(10, 0.000001, 0, BLOOM_UPDATE_ALL);
    filter.insert(COutPoint(uint256S("0x90c122d70786e899529d71dbeba91ba216982fb6ba58f3bdaab65e73b7e9260b"), 1));
    BOOST_CHECK_MESSAGE(!filter.IsRelevantAndUpdate(tx), "Simple Bloom filter matched COutPoint for an output we didn't care about");

    filter = CBloomFilter(10, 0.000001, 0, BLOOM_UPDATE_ALL);
    filter.insert(COutPoint(uint256S("0x000000d70786e899529d71dbeba91ba216982fb6ba58f3bdaab65e73b7e9260b"), 0));
    BOOST_CHECK_MESSAGE(!filter.IsRelevantAndUpdate(tx), "Simple Bloom filter matched COutPoint for an output we didn't care about");
}

BOOST_AUTO_TEST_CASE(merkle_block_1)
{
    // Random real block (0000000000013b8ab2cd513b0261a14096412195a72a0c4827d229dcc7e0f7af)
    // With 9 txes
    CBlock block;
    CMutableTransaction mtx;
    std::vector<unsigned char> vchScript;

    block.nVersion = 1;
    block.hashPrevBlock = uint256S("0x00000000000080b66c911bd5ba14a74260057311eaeb1982802f7010f1a9f090");
    block.nTime = 1293625051;
    block.nBits = 0x1b04864c;
    block.nNonce = 0x93bfb11a;
    block.vtx.resize(9);

    mtx.vin.resize(1);
    vchScript = ParseHex("044c86041b0146");
    mtx.vin[0].scriptSig = CScript(vchScript.begin(), vchScript.end());
    mtx.vout.resize(1);
    mtx.vout[0].nValue = 5000000000;
    vchScript = ParseHex("4104e18f7afbe4721580e81e8414fc8c24d7cfacf254bb5c7b949450c3e997c2dc1242487a8169507b631eb3771f2b425483fb13102c4eb5d858eef260fe70fbfae0ac");
    mtx.vout[0].scriptPubKey = CScript(vchScript.begin(), vchScript.end());
    block.vtx[0] = mtx;

    mtx.vin.resize(1);
    mtx.vin[0].prevout.hash = uint256S("0x36e8f98c5f5733f88ca00dfa05afd7af5dc34dda802790daba6aa1afcb8c6096");
    mtx.vin[0].prevout.n = 0;
    vchScript = ParseHex("493046022100dab24889213caf43ae6adc41cf1c9396c08240c199f5225acf45416330fd7dbd022100fe37900e0644bf574493a07fc5edba06dbc07c311b947520c2d514bc5725dcb401");
    mtx.vin[0].scriptSig = CScript(vchScript.begin(), vchScript.end());
    mtx.vout.resize(1);
    mtx.vout[0].nValue = 5000000000;
    vchScript = ParseHex("76a914f15d1921f52e4007b146dfa60f369ed2fc393ce288ac");
    mtx.vout[0].scriptPubKey = CScript(vchScript.begin(), vchScript.end());
    block.vtx[1] = mtx;

    mtx.vin.resize(1);
    mtx.vin[0].prevout.hash = uint256S("0x6db5acbf1994c2e3c4f78a6bde06c78ed9248be481eccfaf2b8c4588126c76fb");
    mtx.vin[0].prevout.n = 0;
    vchScript = ParseHex("493046022100f268ba165ce0ad2e6d93f089cfcd3785de5c963bb5ea6b8c1b23f1ce3e517b9f022100da7c0f21adc6c401887f2bfd1922f11d76159cbc597fbd756a23dcbb00f4d7290141042b4e8625a96127826915a5b109852636ad0da753c9e1d5606a50480cd0c40f1f8b8d898235e571fe9357d9ec842bc4bba1827daaf4de06d71844d0057707966a");
    mtx.vin[0].scriptSig = CScript(vchScript.begin(), vchScript.end());
    mtx.vout.resize(2);
    mtx.vout[0].nValue = 10000000;
    vchScript = ParseHex("76a9146963907531db72d0ed1a0cfb471ccb63923446f388ac");
    mtx.vout[0].scriptPubKey = CScript(vchScript.begin(), vchScript.end());
    mtx.vout[1].nValue = 1290000000;
    vchScript = ParseHex("76a914f0688ba1c0d1ce182c7af6741e02658c7d4dfcd388ac");
    mtx.vout[1].scriptPubKey = CScript(vchScript.begin(), vchScript.end());
    block.vtx[2] = mtx;

    mtx.vin.resize(2);
    mtx.vin[0].prevout.hash = uint256S("0xfff2525b8931402dd09222c50775608f75787bd2b87e56995a7bdd30f79702c4");
    mtx.vin[0].prevout.n = 1;
    vchScript = ParseHex("483045022100f7edfd4b0aac404e5bab4fd3889e0c6c41aa8d0e6fa122316f68eddd0a65013902205b09cc8b2d56e1cd1f7f2fafd60a129ed94504c4ac7bdc67b56fe67512658b3e014104732012cb962afa90d31b25d8fb0e32c94e513ab7a17805c14ca4c3423e18b4fb5d0e676841733cb83abaf975845c9f6f2a8097b7d04f4908b18368d6fc2d68ec");
    mtx.vin[0].scriptSig = CScript(vchScript.begin(), vchScript.end());
    mtx.vin[1].prevout.hash = uint256S("0xfbde5d03b027d2b9ba4cf5d4fecab9a99864df2637b25ea4cbcb1796ff6550ca");
    mtx.vin[1].prevout.n = 0;
    vchScript = ParseHex("473044022068010362a13c7f9919fa832b2dee4e788f61f6f5d344a7c2a0da6ae740605658022006d1af525b9a14a35c003b78b72bd59738cd676f845d1ff3fc25049e01003614014104732012cb962afa90d31b25d8fb0e32c94e513ab7a17805c14ca4c3423e18b4fb5d0e676841733cb83abaf975845c9f6f2a8097b7d04f4908b18368d6fc2d68ec");
    mtx.vin[1].scriptSig = CScript(vchScript.begin(), vchScript.end());
    mtx.vout.resize(1);
    mtx.vout[0].nValue = 8888000000;
    vchScript = ParseHex("410469ab4181eceb28985b9b4e895c13fa5e68d85761b7eee311db5addef76fa8621865134a221bd01f28ec9999ee3e021e60766e9d1f3458c115fb28650605f11c9ac");
    mtx.vout[0].scriptPubKey = CScript(vchScript.begin(), vchScript.end());
    block.vtx[3] = mtx;

    mtx.vin.resize(1);
    mtx.vin[0].prevout.hash = uint256S("0xfac2d53ed2f083c8dba090aaf88949c8e4d13306c52d5e6514c5918e752fafcd");
    mtx.vin[0].prevout.n = 1;
    vchScript = ParseHex("48304502207ab51be6f12a1962ba0aaaf24a20e0b69b27a94fac5adf45aa7d2d18ffd9236102210086ae728b370e5329eead9accd880d0cb070aea0c96255fae6c4f1ddcce1fd56e014104462e76fd4067b3a0aa42070082dcb0bf2f388b6495cf33d789904f07d0f55c40fbd4b82963c69b3dc31895d0c772c812b1d5fbcade15312ef1c0e8ebbb12dcd4");
    mtx.vin[0].scriptSig = CScript(vchScript.begin(), vchScript.end());
    mtx.vout.resize(2);
    mtx.vout[0].nValue = 5000000;
    vchScript = ParseHex("76a9142b6ba7c9d796b75eef7942fc9288edd37c32f5c388ac");
    mtx.vout[0].scriptPubKey = CScript(vchScript.begin(), vchScript.end());
    mtx.vout[1].nValue = 20000000;
    vchScript = ParseHex("76a9141befba0cdc1ad56529371864d9f6cb042faa06b588ac");
    mtx.vout[1].scriptPubKey = CScript(vchScript.begin(), vchScript.end());
    block.vtx[4] = mtx;

    mtx.vin.resize(1);
    mtx.vin[0].prevout.hash = uint256S("0xcb00f8a0573b18faa8c4f467b049f5d202bf1101d9ef2633bc611be70376a4b4");
    mtx.vin[0].prevout.n = 1;
    vchScript = ParseHex("4730440220177c37f9a505c3f1a1f0ce2da777c339bd8339ffa02c7cb41f0a5804f473c9230220585b25a2ee80eb59292e52b987dad92acb0c64eced92ed9ee105ad153cdb12d001410443bd44f683467e549dae7d20d1d79cbdb6df985c6e9c029c8d0c6cb46cc1a4d3cf7923c5021b27f7a0b562ada113bc85d5fda5a1b41e87fe6e8802817cf69996");
    mtx.vin[0].scriptSig = CScript(vchScript.begin(), vchScript.end());
    mtx.vout.resize(2);
    mtx.vout[0].nValue = 102000000;
    vchScript = ParseHex("76a9145505614859643ab7b547cd7f1f5e7e2a12322d3788ac");
    mtx.vout[0].scriptPubKey = CScript(vchScript.begin(), vchScript.end());
    mtx.vout[1].nValue = 1896000000;
    vchScript = ParseHex("76a914ea4720a7a52fc166c55ff2298e07baf70ae67e1b88ac");
    mtx.vout[1].scriptPubKey = CScript(vchScript.begin(), vchScript.end());
    block.vtx[5] = mtx;

    mtx.vin.resize(5);
    mtx.vin[0].prevout.hash = uint256S("0x9eb4ab14fb5480539ae42f02f9765170e04d203e4ab1ed60bb19d202d62cc686");
    mtx.vin[0].prevout.n = 1;
    vchScript = ParseHex("493046022100f2bc2aba2534becbdf062eb993853a42bbbc282083d0daf9b4b585bd401aa8c9022100b1d7fd7ee0b95600db8535bbf331b19eed8d961f7a8e54159c53675d5f69df8c014104462e76fd4067b3a0aa42070082dcb0bf2f388b6495cf33d789904f07d0f55c40fbd4b82963c69b3dc31895d0c772c812b1d5fbcade15312ef1c0e8ebbb12dcd4");
    mtx.vin[0].scriptSig = CScript(vchScript.begin(), vchScript.end());
    mtx.vin[1].prevout.hash = uint256S("0x79217b4483ae283a4baafa0633a9b097196fcf8b218ac29ddfc3dacc580ead03");
    mtx.vin[1].prevout.n = 1;
    vchScript = ParseHex("483045022100be12b2937179da88599e27bb31c3525097a07cdb52422d165b3ca2f2020ffcf702200971b51f853a53d644ebae9ec8f3512e442b1bcb6c315a5b491d119d10624c83014104462e76fd4067b3a0aa42070082dcb0bf2f388b6495cf33d789904f07d0f55c40fbd4b82963c69b3dc31895d0c772c812b1d5fbcade15312ef1c0e8ebbb12dcd4");
    mtx.vin[1].scriptSig = CScript(vchScript.begin(), vchScript.end());
    mtx.vin[2].prevout.hash = uint256S("0x7b109e08956a7171d253f54a14ba30005821c9623760925768c8bb29b6cacf2a");
    mtx.vin[2].prevout.n = 1;
    vchScript = ParseHex("483045022100fa579a840ac258871365dd48cd7552f96c8eea69bd00d84f05b283a0dab311e102207e3c0ee9234814cfbb1b659b83671618f45abc1326b9edcc77d552a4f2a805c0014104462e76fd4067b3a0aa42070082dcb0bf2f388b6495cf33d789904f07d0f55c40fbd4b82963c69b3dc31895d0c772c812b1d5fbcade15312ef1c0e8ebbb12dcd4");
    mtx.vin[2].scriptSig = CScript(vchScript.begin(), vchScript.end());
    mtx.vin[3].prevout.hash = uint256S("0xd2fc17c534865a3b114fd23c0adf7d73cbea618e58dc8d654a94c9bb2360dcdc");
    mtx.vin[3].prevout.n = 0;
    vchScript = ParseHex("4830450221008d6df731df5d32267954bd7d2dda2302b74c6c2a6aa5c0ca64ecbabc1af03c75022010e55c571d65da7701ae2da1956c442df81bbf076cdbac25133f99d98a9ed34c014104462e76fd4067b3a0aa42070082dcb0bf2f388b6495cf33d789904f07d0f55c40fbd4b82963c69b3dc31895d0c772c812b1d5fbcade15312ef1c0e8ebbb12dcd4");
    mtx.vin[3].scriptSig = CScript(vchScript.begin(), vchScript.end());
    mtx.vin[4].prevout.hash = uint256S("0x6ec73ab8069fd68f034afafc215bedd7f6ed1465dcd6df79f458e25ccd5755e1");
    mtx.vin[4].prevout.n = 1;
    vchScript = ParseHex("483045022023b3e0ab071eb11de2eb1cc3a67261b866f86bf6867d4558165f7c8c8aca2d86022100dc6e1f53a91de3efe8f63512850811f26284b62f850c70ca73ed5de8771fb451014104462e76fd4067b3a0aa42070082dcb0bf2f388b6495cf33d789904f07d0f55c40fbd4b82963c69b3dc31895d0c772c812b1d5fbcade15312ef1c0e8ebbb12dcd4");
    mtx.vin[4].scriptSig = CScript(vchScript.begin(), vchScript.end());
    mtx.vout.resize(1);
    mtx.vout[0].nValue = 5000000;
    vchScript = ParseHex("76a9142b6ba7c9d796b75eef7942fc9288edd37c32f5c388ac");
    mtx.vout[0].scriptPubKey = CScript(vchScript.begin(), vchScript.end());
    block.vtx[6] = mtx;

    mtx.vin.resize(1);
    mtx.vin[0].prevout.hash = uint256S("0x8ded96ae2a609555df2390faa2f001b04e6e0ba8a60c69f9b432c9637157d766");
    mtx.vin[0].prevout.n = 0;
    vchScript = ParseHex("4730440220262b42546302dfb654a229cefc86432b89628ff259dc87edd1154535b16a67e102207b4634c020a97c3e7bbd0d4d19da6aa2269ad9dded4026e896b213d73ca4b63f014104979b82d02226b3a4597523845754d44f13639e3bf2df5e82c6aab2bdc79687368b01b1ab8b19875ae3c90d661a3d0a33161dab29934edeb36aa01976be3baf8a");
    mtx.vin[0].scriptSig = CScript(vchScript.begin(), vchScript.end());
    mtx.vout.resize(2);
    mtx.vout[0].nValue = 5000000;
    vchScript = ParseHex("76a9144854e695a02af0aeacb823ccbc272134561e0a1688ac");
    mtx.vout[0].scriptPubKey = CScript(vchScript.begin(), vchScript.end());
    mtx.vout[1].nValue = 1000000;
    vchScript = ParseHex("76a914abee93376d6b37b5c2940655a6fcaf1c8e74237988ac");
    mtx.vout[1].scriptPubKey = CScript(vchScript.begin(), vchScript.end());
    block.vtx[7] = mtx;

    mtx.vin.resize(1);
    mtx.vin[0].prevout.hash = uint256S("0x71b3dbaca67e9f9189dad3617138c19725ab541ef0b49c05a94913e9f28e3f4e");
    mtx.vin[0].prevout.n = 1;
    vchScript = ParseHex("49304602210081f3183471a5ca22307c0800226f3ef9c353069e0773ac76bb580654d56aa523022100d4c56465bdc069060846f4fbf2f6b20520b2a80b08b168b31e66ddb9c694e240014104976c79848e18251612f8940875b2b08d06e6dc73b9840e8860c066b7e87432c477e9a59a453e71e6d76d5fe34058b800a098fc1740ce3012e8fc8a00c96af966");
    mtx.vin[0].scriptSig = CScript(vchScript.begin(), vchScript.end());
    mtx.vout.resize(2);
    mtx.vout[0].nValue = 15000000;
    vchScript = ParseHex("76a9144134e75a6fcb6042034aab5e18570cf1f844f54788ac");
    mtx.vout[0].scriptPubKey = CScript(vchScript.begin(), vchScript.end());
    mtx.vout[1].nValue = 5000000;
    vchScript = ParseHex("76a9142b6ba7c9d796b75eef7942fc9288edd37c32f5c388ac");
    mtx.vout[1].scriptPubKey = CScript(vchScript.begin(), vchScript.end());
    block.vtx[8] = mtx;

    block.hashMerkleRoot = block.BuildMerkleTree();

    CBloomFilter filter(10, 0.000001, 0, BLOOM_UPDATE_ALL);
    // Match the last transaction
    filter.insert(block.vtx[8].GetHash());

    CMerkleBlock merkleBlock(block, filter);
    BOOST_CHECK(merkleBlock.header.GetHash() == block.GetHash());

    BOOST_CHECK(merkleBlock.vMatchedTxn.size() == 1);
    pair<unsigned int, uint256> pair = merkleBlock.vMatchedTxn[0];

    BOOST_CHECK(merkleBlock.vMatchedTxn[0].second == block.vtx[8].GetHash());
    BOOST_CHECK(merkleBlock.vMatchedTxn[0].first == 8);

    vector<uint256> vMatched;
    vector<unsigned int> vIndex;
    BOOST_CHECK(merkleBlock.txn.ExtractMatches(vMatched, vIndex) == block.hashMerkleRoot);
    BOOST_CHECK(vMatched.size() == merkleBlock.vMatchedTxn.size());
    for (unsigned int i = 0; i < vMatched.size(); i++)
        BOOST_CHECK(vMatched[i] == merkleBlock.vMatchedTxn[i].second);

    // Also match the 8th transaction
    filter.insert(block.vtx[7].GetHash());
    merkleBlock = CMerkleBlock(block, filter);
    BOOST_CHECK(merkleBlock.header.GetHash() == block.GetHash());

    BOOST_CHECK(merkleBlock.vMatchedTxn.size() == 2);

    BOOST_CHECK(merkleBlock.vMatchedTxn[1] == pair);

    BOOST_CHECK(merkleBlock.vMatchedTxn[0].second == block.vtx[7].GetHash());
    BOOST_CHECK(merkleBlock.vMatchedTxn[0].first == 7);

    BOOST_CHECK(merkleBlock.txn.ExtractMatches(vMatched, vIndex) == block.hashMerkleRoot);
    BOOST_CHECK(vMatched.size() == merkleBlock.vMatchedTxn.size());
    for (unsigned int i = 0; i < vMatched.size(); i++)
        BOOST_CHECK(vMatched[i] == merkleBlock.vMatchedTxn[i].second);
}

BOOST_AUTO_TEST_CASE(merkle_block_2)
{
    // Random real block (000000005a4ded781e667e06ceefafb71410b511fe0d5adc3e5a27ecbec34ae6)
    // With 4 txes
    CBlock block;
    CMutableTransaction mtx;
    std::vector<unsigned char> vchScript;

    block.nVersion = 1;
    block.hashPrevBlock = uint256S("0x00000000689051c09ff2cd091cc4c22c10b965eb8db3ad5f032621cc36626175");
    block.nTime = 1231999700;
    block.nBits = 0x1d00ffff;
    block.nNonce = 0x41a4f000;
    block.vtx.resize(4);

    mtx.vin.resize(1);
    vchScript = ParseHex("04ffff001d029105");
    mtx.vin[0].scriptSig = CScript(vchScript.begin(), vchScript.end());
    mtx.vout.resize(1);
    mtx.vout[0].nValue = 5000000000;
    vchScript = ParseHex("41046d8709a041d34357697dfcb30a9d05900a6294078012bf3bb09c6f9b525f1d16d5503d7905db1ada9501446ea00728668fc5719aa80be2fdfc8a858a4dbdd4fbac");
    mtx.vout[0].scriptPubKey = CScript(vchScript.begin(), vchScript.end());
    block.vtx[0] = mtx;

    mtx.vin.resize(2);
    mtx.vin[0].prevout.hash = uint256S("0x35288d269cee1941eaebb2ea85e32b42cdb2b04284a56d8b14dcc3f5c65d6055");
    mtx.vin[0].prevout.n = 0;
    vchScript = ParseHex("483045022100aa46504baa86df8a33b1192b1b9367b4d729dc41e389f2c04f3e5c7f0559aae702205e82253a54bf5c4f65b7428551554b2045167d6d206dfe6a2e198127d3f7df1501");
    mtx.vin[0].scriptSig = CScript(vchScript.begin(), vchScript.end());
    mtx.vin[1].prevout.hash = uint256S("0x35288d269cee1941eaebb2ea85e32b42cdb2b04284a56d8b14dcc3f5c65d6055");
    mtx.vin[1].prevout.n = 1;
    vchScript = ParseHex("47304402202329484c35fa9d6bb32a55a70c0982f606ce0e3634b69006138683bcd12cbb6602200c28feb1e2555c3210f1dddb299738b4ff8bbe9667b68cb8764b5ac17b7adf0001");
    mtx.vin[1].scriptSig = CScript(vchScript.begin(), vchScript.end());
    mtx.vout.resize(2);
    mtx.vout[0].nValue = 100000000;
    vchScript = ParseHex("41046a0765b5865641ce08dd39690aade26dfbf5511430ca428a3089261361cef170e3929a68aee3d8d4848b0c5111b0a37b82b86ad559fd2a745b44d8e8d9dfdc0cac");
    mtx.vout[0].scriptPubKey = CScript(vchScript.begin(), vchScript.end());
    mtx.vout[1].nValue = 2400000000LL;
    vchScript = ParseHex("41044a656f065871a353f216ca26cef8dde2f03e8c16202d2e8ad769f02032cb86a5eb5e56842e92e19141d60a01928f8dd2c875a390f67c1f6c94cfc617c0ea45afac");
    mtx.vout[1].scriptPubKey = CScript(vchScript.begin(), vchScript.end());
    block.vtx[1] = mtx;

    mtx.vin.resize(2);
    mtx.vin[0].prevout.hash = block.vtx[1].GetHash();
    mtx.vin[0].prevout.n = 0;
    vchScript = ParseHex("47304402205d6058484157235b06028c30736c15613a28bdb768ee628094ca8b0030d4d6eb0220328789c9a2ec27ddaec0ad5ef58efded42e6ea17c2e1ce838f3d6913f5e95db601");
    mtx.vin[0].scriptSig = CScript(vchScript.begin(), vchScript.end());
    mtx.vin[1].prevout.hash = block.vtx[1].GetHash();
    mtx.vin[1].prevout.n = 1;
    vchScript = ParseHex("493046022100c45af050d3cea806cedd0ab22520c53ebe63b987b8954146cdca42487b84bdd6022100b9b027716a6b59e640da50a864d6dd8a0ef24c76ce62391fa3eabaf4d2886d2d01");
    mtx.vin[1].scriptSig = CScript(vchScript.begin(), vchScript.end());
    mtx.vout.resize(2);
    mtx.vout[0].nValue = 100000000;
    vchScript = ParseHex("41046a0765b5865641ce08dd39690aade26dfbf5511430ca428a3089261361cef170e3929a68aee3d8d4848b0c5111b0a37b82b86ad559fd2a745b44d8e8d9dfdc0cac");
    mtx.vout[0].scriptPubKey = CScript(vchScript.begin(), vchScript.end());
    mtx.vout[1].nValue = 2400000000LL;
    vchScript = ParseHex("41046a0765b5865641ce08dd39690aade26dfbf5511430ca428a3089261361cef170e3929a68aee3d8d4848b0c5111b0a37b82b86ad559fd2a745b44d8e8d9dfdc0cac");
    mtx.vout[1].scriptPubKey = CScript(vchScript.begin(), vchScript.end());
    block.vtx[2] = mtx;

    mtx.vin.resize(2);
    mtx.vin[0].prevout.hash = uint256S("0x6b0f8a73a56c04b519f1883e8aafda643ba61a30bd1439969df21bea5f4e27e2");
    mtx.vin[0].prevout.n = 0;
    vchScript = ParseHex("473044022016e7a727a061ea2254a6c358376aaa617ac537eb836c77d646ebda4c748aac8b0220192ce28bf9f2c06a6467e6531e27648d2b3e2e2bae85159c9242939840295ba501");
    mtx.vin[0].scriptSig = CScript(vchScript.begin(), vchScript.end());
    mtx.vin[1].prevout.hash = uint256S("0x6b0f8a73a56c04b519f1883e8aafda643ba61a30bd1439969df21bea5f4e27e2");
    mtx.vin[1].prevout.n = 1;
    vchScript = ParseHex("493046022100b7a1a755588d4190118936e15cd217d133b0e4a53c3c15924010d5648d8925c9022100aaef031874db2114f2d869ac2de4ae53908fbfea5b2b1862e181626bb9005c9f01");
    mtx.vin[1].scriptSig = CScript(vchScript.begin(), vchScript.end());
    mtx.vout.resize(2);
    mtx.vout[0].nValue = 100000000;
    vchScript = ParseHex("41044a656f065871a353f216ca26cef8dde2f03e8c16202d2e8ad769f02032cb86a5eb5e56842e92e19141d60a01928f8dd2c875a390f67c1f6c94cfc617c0ea45afac");
    mtx.vout[0].scriptPubKey = CScript(vchScript.begin(), vchScript.end());
    mtx.vout[1].nValue = 2400000000LL;
    vchScript = ParseHex("41046a0765b5865641ce08dd39690aade26dfbf5511430ca428a3089261361cef170e3929a68aee3d8d4848b0c5111b0a37b82b86ad559fd2a745b44d8e8d9dfdc0cac");
    mtx.vout[1].scriptPubKey = CScript(vchScript.begin(), vchScript.end());
    block.vtx[3] = mtx;

    block.hashMerkleRoot = block.BuildMerkleTree();

    CBloomFilter filter(10, 0.000001, 0, BLOOM_UPDATE_ALL);
    // Match the first transaction
    filter.insert(block.vtx[0].GetHash());

    CMerkleBlock merkleBlock(block, filter);
    BOOST_CHECK(merkleBlock.header.GetHash() == block.GetHash());

    BOOST_CHECK(merkleBlock.vMatchedTxn.size() == 1);
    pair<unsigned int, uint256> pair = merkleBlock.vMatchedTxn[0];

    BOOST_CHECK(merkleBlock.vMatchedTxn[0].second == block.vtx[0].GetHash());
    BOOST_CHECK(merkleBlock.vMatchedTxn[0].first == 0);

    vector<uint256> vMatched;
    vector<unsigned int> vIndex;
    BOOST_CHECK(merkleBlock.txn.ExtractMatches(vMatched, vIndex) == block.hashMerkleRoot);
    BOOST_CHECK(vMatched.size() == merkleBlock.vMatchedTxn.size());
    for (unsigned int i = 0; i < vMatched.size(); i++)
        BOOST_CHECK(vMatched[i] == merkleBlock.vMatchedTxn[i].second);

    // Match an output from the second transaction (the pubkey for address 1DZTzaBHUDM7T3QvUKBz4qXMRpkg8jsfB5)
    // This should match the third transaction because it spends the output matched
    // It also matches the fourth transaction, which spends to the pubkey again
    filter.insert(ParseHex("044a656f065871a353f216ca26cef8dde2f03e8c16202d2e8ad769f02032cb86a5eb5e56842e92e19141d60a01928f8dd2c875a390f67c1f6c94cfc617c0ea45af"));

    merkleBlock = CMerkleBlock(block, filter);
    BOOST_CHECK(merkleBlock.header.GetHash() == block.GetHash());

    BOOST_CHECK(merkleBlock.vMatchedTxn.size() == 4);

    BOOST_CHECK(pair == merkleBlock.vMatchedTxn[0]);

    BOOST_CHECK(merkleBlock.vMatchedTxn[1].second == block.vtx[1].GetHash());
    BOOST_CHECK(merkleBlock.vMatchedTxn[1].first == 1);

    BOOST_CHECK(merkleBlock.vMatchedTxn[2].second == block.vtx[2].GetHash());
    BOOST_CHECK(merkleBlock.vMatchedTxn[2].first == 2);

    BOOST_CHECK(merkleBlock.vMatchedTxn[3].second == block.vtx[3].GetHash());
    BOOST_CHECK(merkleBlock.vMatchedTxn[3].first == 3);

    BOOST_CHECK(merkleBlock.txn.ExtractMatches(vMatched, vIndex) == block.hashMerkleRoot);
    BOOST_CHECK(vMatched.size() == merkleBlock.vMatchedTxn.size());
    for (unsigned int i = 0; i < vMatched.size(); i++)
        BOOST_CHECK(vMatched[i] == merkleBlock.vMatchedTxn[i].second);
}

BOOST_AUTO_TEST_CASE(merkle_block_2_with_update_none)
{
    // Random real block (000000005a4ded781e667e06ceefafb71410b511fe0d5adc3e5a27ecbec34ae6)
    // With 4 txes
    CBlock block;
    CMutableTransaction mtx;
    std::vector<unsigned char> vchScript;

    block.nVersion = 1;
    block.hashPrevBlock = uint256S("0x00000000689051c09ff2cd091cc4c22c10b965eb8db3ad5f032621cc36626175");
    block.nTime = 1231999700;
    block.nBits = 0x1d00ffff;
    block.nNonce = 0x41a4f000;
    block.vtx.resize(4);

    mtx.vin.resize(1);
    vchScript = ParseHex("04ffff001d029105");
    mtx.vin[0].scriptSig = CScript(vchScript.begin(), vchScript.end());
    mtx.vout.resize(1);
    mtx.vout[0].nValue = 5000000000;
    vchScript = ParseHex("41046d8709a041d34357697dfcb30a9d05900a6294078012bf3bb09c6f9b525f1d16d5503d7905db1ada9501446ea00728668fc5719aa80be2fdfc8a858a4dbdd4fbac");
    mtx.vout[0].scriptPubKey = CScript(vchScript.begin(), vchScript.end());
    block.vtx[0] = mtx;

    mtx.vin.resize(2);
    mtx.vin[0].prevout.hash = uint256S("0x35288d269cee1941eaebb2ea85e32b42cdb2b04284a56d8b14dcc3f5c65d6055");
    mtx.vin[0].prevout.n = 0;
    vchScript = ParseHex("483045022100aa46504baa86df8a33b1192b1b9367b4d729dc41e389f2c04f3e5c7f0559aae702205e82253a54bf5c4f65b7428551554b2045167d6d206dfe6a2e198127d3f7df1501");
    mtx.vin[0].scriptSig = CScript(vchScript.begin(), vchScript.end());
    mtx.vin[1].prevout.hash = uint256S("0x35288d269cee1941eaebb2ea85e32b42cdb2b04284a56d8b14dcc3f5c65d6055");
    mtx.vin[1].prevout.n = 1;
    vchScript = ParseHex("47304402202329484c35fa9d6bb32a55a70c0982f606ce0e3634b69006138683bcd12cbb6602200c28feb1e2555c3210f1dddb299738b4ff8bbe9667b68cb8764b5ac17b7adf0001");
    mtx.vin[1].scriptSig = CScript(vchScript.begin(), vchScript.end());
    mtx.vout.resize(2);
    mtx.vout[0].nValue = 100000000;
    vchScript = ParseHex("41046a0765b5865641ce08dd39690aade26dfbf5511430ca428a3089261361cef170e3929a68aee3d8d4848b0c5111b0a37b82b86ad559fd2a745b44d8e8d9dfdc0cac");
    mtx.vout[0].scriptPubKey = CScript(vchScript.begin(), vchScript.end());
    mtx.vout[1].nValue = 2400000000LL;
    vchScript = ParseHex("41044a656f065871a353f216ca26cef8dde2f03e8c16202d2e8ad769f02032cb86a5eb5e56842e92e19141d60a01928f8dd2c875a390f67c1f6c94cfc617c0ea45afac");
    mtx.vout[1].scriptPubKey = CScript(vchScript.begin(), vchScript.end());
    block.vtx[1] = mtx;

    mtx.vin.resize(2);
    mtx.vin[0].prevout.hash = block.vtx[1].GetHash();
    mtx.vin[0].prevout.n = 0;
    vchScript = ParseHex("47304402205d6058484157235b06028c30736c15613a28bdb768ee628094ca8b0030d4d6eb0220328789c9a2ec27ddaec0ad5ef58efded42e6ea17c2e1ce838f3d6913f5e95db601");
    mtx.vin[0].scriptSig = CScript(vchScript.begin(), vchScript.end());
    mtx.vin[1].prevout.hash = block.vtx[1].GetHash();
    mtx.vin[1].prevout.n = 1;
    vchScript = ParseHex("493046022100c45af050d3cea806cedd0ab22520c53ebe63b987b8954146cdca42487b84bdd6022100b9b027716a6b59e640da50a864d6dd8a0ef24c76ce62391fa3eabaf4d2886d2d01");
    mtx.vin[1].scriptSig = CScript(vchScript.begin(), vchScript.end());
    mtx.vout.resize(2);
    mtx.vout[0].nValue = 100000000;
    vchScript = ParseHex("41046a0765b5865641ce08dd39690aade26dfbf5511430ca428a3089261361cef170e3929a68aee3d8d4848b0c5111b0a37b82b86ad559fd2a745b44d8e8d9dfdc0cac");
    mtx.vout[0].scriptPubKey = CScript(vchScript.begin(), vchScript.end());
    mtx.vout[1].nValue = 2400000000LL;
    vchScript = ParseHex("41046a0765b5865641ce08dd39690aade26dfbf5511430ca428a3089261361cef170e3929a68aee3d8d4848b0c5111b0a37b82b86ad559fd2a745b44d8e8d9dfdc0cac");
    mtx.vout[1].scriptPubKey = CScript(vchScript.begin(), vchScript.end());
    block.vtx[2] = mtx;

    mtx.vin.resize(2);
    mtx.vin[0].prevout.hash = uint256S("0x6b0f8a73a56c04b519f1883e8aafda643ba61a30bd1439969df21bea5f4e27e2");
    mtx.vin[0].prevout.n = 0;
    vchScript = ParseHex("473044022016e7a727a061ea2254a6c358376aaa617ac537eb836c77d646ebda4c748aac8b0220192ce28bf9f2c06a6467e6531e27648d2b3e2e2bae85159c9242939840295ba501");
    mtx.vin[0].scriptSig = CScript(vchScript.begin(), vchScript.end());
    mtx.vin[1].prevout.hash = uint256S("0x6b0f8a73a56c04b519f1883e8aafda643ba61a30bd1439969df21bea5f4e27e2");
    mtx.vin[1].prevout.n = 1;
    vchScript = ParseHex("493046022100b7a1a755588d4190118936e15cd217d133b0e4a53c3c15924010d5648d8925c9022100aaef031874db2114f2d869ac2de4ae53908fbfea5b2b1862e181626bb9005c9f01");
    mtx.vin[1].scriptSig = CScript(vchScript.begin(), vchScript.end());
    mtx.vout.resize(2);
    mtx.vout[0].nValue = 100000000;
    vchScript = ParseHex("41044a656f065871a353f216ca26cef8dde2f03e8c16202d2e8ad769f02032cb86a5eb5e56842e92e19141d60a01928f8dd2c875a390f67c1f6c94cfc617c0ea45afac");
    mtx.vout[0].scriptPubKey = CScript(vchScript.begin(), vchScript.end());
    mtx.vout[1].nValue = 2400000000LL;
    vchScript = ParseHex("41046a0765b5865641ce08dd39690aade26dfbf5511430ca428a3089261361cef170e3929a68aee3d8d4848b0c5111b0a37b82b86ad559fd2a745b44d8e8d9dfdc0cac");
    mtx.vout[1].scriptPubKey = CScript(vchScript.begin(), vchScript.end());
    block.vtx[3] = mtx;

    block.hashMerkleRoot = block.BuildMerkleTree();

    CBloomFilter filter(10, 0.000001, 0, BLOOM_UPDATE_NONE);
    // Match the first transaction
    filter.insert(block.vtx[0].GetHash());

    CMerkleBlock merkleBlock(block, filter);
    BOOST_CHECK(merkleBlock.header.GetHash() == block.GetHash());

    BOOST_CHECK(merkleBlock.vMatchedTxn.size() == 1);
    pair<unsigned int, uint256> pair = merkleBlock.vMatchedTxn[0];

    BOOST_CHECK(merkleBlock.vMatchedTxn[0].second == block.vtx[0].GetHash());
    BOOST_CHECK(merkleBlock.vMatchedTxn[0].first == 0);

    vector<uint256> vMatched;
    vector<unsigned int> vIndex;
    BOOST_CHECK(merkleBlock.txn.ExtractMatches(vMatched, vIndex) == block.hashMerkleRoot);
    BOOST_CHECK(vMatched.size() == merkleBlock.vMatchedTxn.size());
    for (unsigned int i = 0; i < vMatched.size(); i++)
        BOOST_CHECK(vMatched[i] == merkleBlock.vMatchedTxn[i].second);

    // Match an output from the second transaction (the pubkey for address 1DZTzaBHUDM7T3QvUKBz4qXMRpkg8jsfB5)
    // This should not match the third transaction though it spends the output matched
    // It will match the fourth transaction, which has another pay-to-pubkey output to the same address
    filter.insert(ParseHex("044a656f065871a353f216ca26cef8dde2f03e8c16202d2e8ad769f02032cb86a5eb5e56842e92e19141d60a01928f8dd2c875a390f67c1f6c94cfc617c0ea45af"));

    merkleBlock = CMerkleBlock(block, filter);
    BOOST_CHECK(merkleBlock.header.GetHash() == block.GetHash());

    BOOST_CHECK(merkleBlock.vMatchedTxn.size() == 3);

    BOOST_CHECK(pair == merkleBlock.vMatchedTxn[0]);

    BOOST_CHECK(merkleBlock.vMatchedTxn[1].second == block.vtx[1].GetHash());
    BOOST_CHECK(merkleBlock.vMatchedTxn[1].first == 1);

    BOOST_CHECK(merkleBlock.vMatchedTxn[2].second == block.vtx[3].GetHash());
    BOOST_CHECK(merkleBlock.vMatchedTxn[2].first == 3);

    BOOST_CHECK(merkleBlock.txn.ExtractMatches(vMatched, vIndex) == block.hashMerkleRoot);
    BOOST_CHECK(vMatched.size() == merkleBlock.vMatchedTxn.size());
    for (unsigned int i = 0; i < vMatched.size(); i++)
        BOOST_CHECK(vMatched[i] == merkleBlock.vMatchedTxn[i].second);
}

BOOST_AUTO_TEST_CASE(merkle_block_3_and_serialize)
{
    // Random real block (000000000000dab0130bbcc991d3d7ae6b81aa6f50a798888dfe62337458dc45)
    // With one tx
    CBlock block;
    CMutableTransaction mtx;
    std::vector<unsigned char> vchScript;

    block.nVersion = 1;
    block.hashPrevBlock = uint256S("0x000000000000b0b8b4e8105d62300d63c8ec1a1df0af1c2cdbd943b156a8cd79");
    block.nTime = 1293625703;
    block.nBits = 0x1b04864c;
    block.nNonce = 0x635da48f;
    block.vtx.resize(1);

    mtx.vin.resize(1);
    vchScript = ParseHex("044c86041b020a02");
    mtx.vin[0].scriptSig = CScript(vchScript.begin(), vchScript.end());
    mtx.vout.resize(1);
    mtx.vout[0].nValue = 5000000000;
    vchScript = ParseHex("4104ecd3229b0571c3be876feaac0442a9f13c5a572742927af1dc623353ecf8c202225f64868137a18cdd85cbbb4c74fbccfd4f49639cf1bdc94a5672bb15ad5d4cac");
    mtx.vout[0].scriptPubKey = CScript(vchScript.begin(), vchScript.end());
    block.vtx[0] = mtx;

    block.hashMerkleRoot = block.BuildMerkleTree();

    CBloomFilter filter(10, 0.000001, 0, BLOOM_UPDATE_ALL);
    // Match the only transaction
    filter.insert(block.vtx[0].GetHash());

    CMerkleBlock merkleBlock(block, filter);
    BOOST_CHECK(merkleBlock.header.GetHash() == block.GetHash());

    BOOST_CHECK(merkleBlock.vMatchedTxn.size() == 1);

    BOOST_CHECK(merkleBlock.vMatchedTxn[0].second == block.vtx[0].GetHash());
    BOOST_CHECK(merkleBlock.vMatchedTxn[0].first == 0);

    vector<uint256> vMatched;
    vector<unsigned int> vIndex;
    BOOST_CHECK(merkleBlock.txn.ExtractMatches(vMatched, vIndex) == block.hashMerkleRoot);
    BOOST_CHECK(vMatched.size() == merkleBlock.vMatchedTxn.size());
    for (unsigned int i = 0; i < vMatched.size(); i++)
        BOOST_CHECK(vMatched[i] == merkleBlock.vMatchedTxn[i].second);

    CDataStream merkleStream(SER_NETWORK, PROTOCOL_VERSION);
    merkleStream << merkleBlock;

    vector<unsigned char> vch = ParseHex("0100000079cda856b143d9db2c1caff01d1aecc8630d30625d10e8b4b8b0000000000000b50cc069d6a3e33e3ff84a5c41d9d3febe7c770fdcc96b2c3ff60abe184f196367291b4d4c86041b8fa45d630100000001b50cc069d6a3e33e3ff84a5c41d9d3febe7c770fdcc96b2c3ff60abe184f19630101");
    vector<char> expected(vch.size());

    for (unsigned int i = 0; i < vch.size(); i++)
        expected[i] = (char)vch[i];

    BOOST_CHECK_EQUAL_COLLECTIONS(expected.begin(), expected.end(), merkleStream.begin(), merkleStream.end());
}

BOOST_AUTO_TEST_CASE(merkle_block_4)
{
    // Random real block (000000000000b731f2eef9e8c63173adfb07e41bd53eb0ef0a6b720d6cb6dea4)
    // With 7 txes
    CBlock block;
    CMutableTransaction mtx;
    std::vector<unsigned char> vchScript;

    block.nVersion = 1;
    block.hashPrevBlock = uint256S("0x0000000000016780c81d42b7eff86974c36f5ae026e8662a4393a7f39c86bb82");
    block.nTime = 1293629558;
    block.nBits = 0x1b04864c;
    block.nNonce = 0x29854b55;
    block.vtx.resize(7);

    mtx.vin.resize(1);
    vchScript = ParseHex("044c86041b0136");
    mtx.vin[0].scriptSig = CScript(vchScript.begin(), vchScript.end());
    mtx.vout.resize(1);
    mtx.vout[0].nValue = 5000000000;
    vchScript = ParseHex("4104eaafc2314def4ca98ac970241bcab022b9c1e1f4ea423a20f134c876f2c01ec0f0dd5b2e86e7168cefe0d81113c3807420ce13ad1357231a2252247d97a46a91ac");
    mtx.vout[0].scriptPubKey = CScript(vchScript.begin(), vchScript.end());
    block.vtx[0] = mtx;

    mtx.vin.resize(1);
    mtx.vin[0].prevout.hash = uint256S("0x437248e07f73b031bba6aacde3e9f3f70b12559298084f42d12798a2a620adbc");
    mtx.vin[0].prevout.n = 0;
    vchScript = ParseHex("4830450220356e834b046cadc0f8ebb5a8a017b02de59c86305403dad52cd77b55af062ea10221009253cd6c119d4729b77c978e1e2aa19f5ea6e0e52b3f16e32fa608cd5bab753901");
    mtx.vin[0].scriptSig = CScript(vchScript.begin(), vchScript.end());
    mtx.vout.resize(2);
    mtx.vout[0].nValue = 4500000000;
    vchScript = ParseHex("76a9142b4b8072ecbba129b6453c63e129e643207249ca88ac");
    mtx.vout[0].scriptPubKey = CScript(vchScript.begin(), vchScript.end());
    mtx.vout[1].nValue = 500000000;
    vchScript = ParseHex("76a9141b8dd13b994bcfc787b32aeadf58ccb3615cbd5488ac");
    mtx.vout[1].scriptPubKey = CScript(vchScript.begin(), vchScript.end());
    block.vtx[1] = mtx;

    mtx.vin.resize(3);
    mtx.vin[0].prevout.hash = uint256S("0x0bcb16af267dee77ed8761662d31ee9d9a1bf1e4d268a9e7127407ebb3f9acfd");
    mtx.vin[0].prevout.n = 0;
    vchScript = ParseHex("493046022100ea1608e70911ca0de5af51ba57ad23b9a51db8d28f82c53563c56a05c20f5a87022100a8bdc8b4a8acc8634c6b420410150775eb7f2474f5615f7fccd65af30f310fbf01410465fdf49e29b06b9a1582287b6279014f834edc317695d125ef623c1cc3aaece245bd69fcad7508666e9c74a49dc9056d5fc14338ef38118dc4afae5fe2c585ca");
    mtx.vin[0].scriptSig = CScript(vchScript.begin(), vchScript.end());
    mtx.vin[1].prevout.hash = uint256S("0x83afb597f39d42753e3a97b897b471f02d0be7963ef8c4f350cb4e6313199e30");
    mtx.vin[1].prevout.n = 0;
    vchScript = ParseHex("48304502202bdb79c596a9ffc24e96f4386199aba386e9bc7b6071516e2b51dda942b3a1ed022100c53a857e76b724fc14d45311eac5019650d415c3abb5428f3aae16d8e69bec2301");
    mtx.vin[1].scriptSig = CScript(vchScript.begin(), vchScript.end());
    mtx.vin[2].prevout.hash = uint256S("0xc1e6b1bc0c0e1b2bce13df72d3b6b54d837d8f428ac1edc98050699134e38920");
    mtx.vin[2].prevout.n = 0;
    vchScript = ParseHex("483045022100d4ce67c5896ee251c810ac1ff9ceccd328b497c8f553ab6e08431e7d40bad6b5022033119c0c2b7d792d31f1187779c7bd95aefd93d90a715586d73801d9b47471c601");
    mtx.vin[2].scriptSig = CScript(vchScript.begin(), vchScript.end());
    mtx.vout.resize(1);
    mtx.vout[0].nValue = 14500000000;
    vchScript = ParseHex("76a914c7b55141d097ea5df7a0ed330cf794376e53ec8d88ac");
    mtx.vout[0].scriptPubKey = CScript(vchScript.begin(), vchScript.end());
    block.vtx[2] = mtx;

    mtx.vin.resize(4);
    mtx.vin[0].prevout.hash = uint256S("0x4734748106a0724b8f1308dde8cd97d3c1f01b1eeeec92e7a36940aa14e2f05b");
    mtx.vin[0].prevout.n = 0;
    vchScript = ParseHex("48304502200c45de8c4f3e2c1821f2fc878cba97b1e6f8807d94930713aa1c86a67b9bf1e40221008581abfef2e30f957815fc89978423746b2086375ca8ecf359c85c2a5b7c88ad01410462bb73f76ca0994fcb8b4271e6fb7561f5c0f9ca0cf6485261c4a0dc894f4ab844c6cdfb97cd0b60ffb5018ffd6238f4d87270efb1d3ae37079b794a92d7ec95");
    mtx.vin[0].scriptSig = CScript(vchScript.begin(), vchScript.end());
    mtx.vin[1].prevout.hash = uint256S("0x2ca79a5fda3da844c3cebb809c599be248f2e0883d25d259fc408d95d7f769d6");
    mtx.vin[1].prevout.n = 0;
    vchScript = ParseHex("473044022078124c8beeaa825f9e0b30bff96e564dd859432f2d0cb3b72d3d5d93d38d7e930220691d233b6c0f995be5acb03d70a7f7a65b6bc9bdd426260f38a1346669507a3601410462bb73f76ca0994fcb8b4271e6fb7561f5c0f9ca0cf6485261c4a0dc894f4ab844c6cdfb97cd0b60ffb5018ffd6238f4d87270efb1d3ae37079b794a92d7ec95");
    mtx.vin[1].scriptSig = CScript(vchScript.begin(), vchScript.end());
    mtx.vin[2].prevout.hash = uint256S("0xa4daaf4d6b63536fa4e046292337a5b72b37fd51f06c16689a22f5930daf78f8");
    mtx.vin[2].prevout.n = 0;
    vchScript = ParseHex("493046022100c717d1714551663f69c3c5759bdbb3a0fcd3fab023abc0e522fe6440de35d8290221008d9cbe25bffc44af2b18e81c58eb37293fd7fe1c2e7b46fc37ee8c96c50ab1e201410462bb73f76ca0994fcb8b4271e6fb7561f5c0f9ca0cf6485261c4a0dc894f4ab844c6cdfb97cd0b60ffb5018ffd6238f4d87270efb1d3ae37079b794a92d7ec95");
    mtx.vin[2].scriptSig = CScript(vchScript.begin(), vchScript.end());
    mtx.vin[3].prevout.hash = uint256S("0xe2a1d7bf8c9b995f423f97887cd0bc6309e6d9d20faa94f8f2d79c8568b6f227");
    mtx.vin[3].prevout.n = 0;
    vchScript = ParseHex("493046022100e00847147cbf517bcc2f502f3ddc6d284358d102ed20d47a8aa788a62f0db780022100d17b2d6fa84dcaf1c95d88d7e7c30385aecf415588d749afd3ec81f6022cecd701410462bb73f76ca0994fcb8b4271e6fb7561f5c0f9ca0cf6485261c4a0dc894f4ab844c6cdfb97cd0b60ffb5018ffd6238f4d87270efb1d3ae37079b794a92d7ec95");
    mtx.vin[3].scriptSig = CScript(vchScript.begin(), vchScript.end());
    mtx.vout.resize(1);
    mtx.vout[0].nValue = 20000000000;
    vchScript = ParseHex("76a914b6efd80d99179f4f4ff6f4dd0a007d018c385d2188ac");
    mtx.vout[0].scriptPubKey = CScript(vchScript.begin(), vchScript.end());
    block.vtx[3] = mtx;

    mtx.vin.resize(1);
    mtx.vin[0].prevout.hash = uint256S("0xc4d3eb542503e05643cd16f68d750ba5e55c54108e253a37f98ecef1b2374583");
    mtx.vin[0].prevout.n = 0;
    vchScript = ParseHex("483045022100e68f422dd7c34fdce11eeb4509ddae38201773dd62f284e8aa9d96f85099d0b002202243bd399ff96b649a0fad05fa759d6a882f0af8c90cf7632c2840c29070aec20141045e58067e815c2f464c6a2a15f987758374203895710c2d452442e28496ff38ba8f5fd901dc20e29e88477167fe4fc299bf818fd0d9e1632d467b2a3d9503b1aa");
    mtx.vin[0].scriptSig = CScript(vchScript.begin(), vchScript.end());
    mtx.vout.resize(2);
    mtx.vout[0].nValue = 13806000000;
    vchScript = ParseHex("76a914f34c3e10eb387efe872acb614c89e78bfca7815d88ac");
    mtx.vout[0].scriptPubKey = CScript(vchScript.begin(), vchScript.end());
    mtx.vout[1].nValue = 5000000;
    vchScript = ParseHex("76a914a84e272933aaf87e1715d7786c51dfaeb5b65a6f88ac");
    mtx.vout[1].scriptPubKey = CScript(vchScript.begin(), vchScript.end());
    block.vtx[4] = mtx;

    mtx.vin.resize(1);
    mtx.vin[0].prevout.hash = uint256S("0xe73ae02709850d51c538a8fd89013ee299d906d9f317fe7d30eff6e6c881ac43");
    mtx.vin[0].prevout.n = 0;
    vchScript = ParseHex("4930460221009c87c344760a64cb8ae6685a3eec2c1ac1bed5b88c87de51acd0e124f266c16602210082d07c037359c3a257b5c63ebd90f5a5edf97b2ac1c434b08ca998839f346dd40141040ba7e521fa7946d12edbb1d1e95a15c34bd4398195e86433c92b431cd315f455fe30032ede69cad9d1e1ed6c3c4ec0dbfced53438c625462afb792dcb098544b");
    mtx.vin[0].scriptSig = CScript(vchScript.begin(), vchScript.end());
    mtx.vout.resize(2);
    mtx.vout[0].nValue = 1000000;
    vchScript = ParseHex("76a9144676d1b820d63ec272f1900d59d43bc6463d96f888ac");
    mtx.vout[0].scriptPubKey = CScript(vchScript.begin(), vchScript.end());
    mtx.vout[1].nValue = 1000000;
    vchScript = ParseHex("76a914648d04341d00d7968b3405c034adc38d4d8fb9bd88ac");
    mtx.vout[1].scriptPubKey = CScript(vchScript.begin(), vchScript.end());
    block.vtx[5] = mtx;

    mtx.vin.resize(2);
    mtx.vin[0].prevout.hash = uint256S("0xf1e305f72f450a7d01af712e7c03fe0cc467059c00d2a8f4555cea017591cc48");
    mtx.vin[0].prevout.n = 0;
    vchScript = ParseHex("483045022100bf5fdc86dc5f08a5d5c8e43a8c9d5b1ed8c65562e280007b52b133021acd9acc02205e325d613e555f772802bf413d36ba807892ed1a690a77811d3033b3de226e0a01410429fa713b124484cb2bd7b5557b2c0b9df7b2b1fee61825eadc5ae6c37a9920d38bfccdc7dc3cb0c47d7b173dbc9db8d37db0a33ae487982c59c6f8606e9d1791");
    mtx.vin[0].scriptSig = CScript(vchScript.begin(), vchScript.end());
    mtx.vin[1].prevout.hash = uint256S("0x68d0685759c3d4f3f90a4f0e48d1b77641f06bb1f0b83a8841e8d71d5570ed41");
    mtx.vin[1].prevout.n = 0;
    vchScript = ParseHex("4830450221008513ad65187b903aed1102d1d0c47688127658c51106753fed0151ce9c16b80902201432b9ebcb87bd04ceb2de66035fbbaf4bf8b00d1cfe41f1a1f7338f9ad79d210141049d4cf80125bf50be1709f718c07ad15d0fc612b7da1f5570dddc35f2a352f0f27c978b06820edca9ef982c35fda2d255afba340068c5035552368bc7200c1488");
    mtx.vin[1].scriptSig = CScript(vchScript.begin(), vchScript.end());
    mtx.vout.resize(1);
    mtx.vout[0].nValue = 4000000;
    vchScript = ParseHex("76a9148edb68822f1ad580b043c7b3df2e400f8699eb4888ac");
    mtx.vout[0].scriptPubKey = CScript(vchScript.begin(), vchScript.end());
    block.vtx[6] = mtx;

    block.hashMerkleRoot = block.BuildMerkleTree();

    CBloomFilter filter(10, 0.000001, 0, BLOOM_UPDATE_ALL);
    // Match the last transaction
    filter.insert(block.vtx[6].GetHash());

    CMerkleBlock merkleBlock(block, filter);
    BOOST_CHECK(merkleBlock.header.GetHash() == block.GetHash());

    BOOST_CHECK(merkleBlock.vMatchedTxn.size() == 1);
    pair<unsigned int, uint256> pair = merkleBlock.vMatchedTxn[0];

    BOOST_CHECK(merkleBlock.vMatchedTxn[0].second == block.vtx[6].GetHash());
    BOOST_CHECK(merkleBlock.vMatchedTxn[0].first == 6);

    vector<uint256> vMatched;
    vector<unsigned int> vIndex;
    BOOST_CHECK(merkleBlock.txn.ExtractMatches(vMatched, vIndex) == block.hashMerkleRoot);
    BOOST_CHECK(vMatched.size() == merkleBlock.vMatchedTxn.size());
    for (unsigned int i = 0; i < vMatched.size(); i++)
        BOOST_CHECK(vMatched[i] == merkleBlock.vMatchedTxn[i].second);

    // Also match the 4th transaction
    filter.insert(block.vtx[3].GetHash());
    merkleBlock = CMerkleBlock(block, filter);
    BOOST_CHECK(merkleBlock.header.GetHash() == block.GetHash());

    BOOST_CHECK(merkleBlock.vMatchedTxn.size() == 2);

    BOOST_CHECK(merkleBlock.vMatchedTxn[0].second == block.vtx[3].GetHash());
    BOOST_CHECK(merkleBlock.vMatchedTxn[0].first == 3);

    BOOST_CHECK(merkleBlock.vMatchedTxn[1] == pair);

    BOOST_CHECK(merkleBlock.txn.ExtractMatches(vMatched, vIndex) == block.hashMerkleRoot);
    BOOST_CHECK(vMatched.size() == merkleBlock.vMatchedTxn.size());
    for (unsigned int i = 0; i < vMatched.size(); i++)
        BOOST_CHECK(vMatched[i] == merkleBlock.vMatchedTxn[i].second);
}

BOOST_AUTO_TEST_CASE(merkle_block_4_test_p2pubkey_only)
{
    // Random real block (000000000000b731f2eef9e8c63173adfb07e41bd53eb0ef0a6b720d6cb6dea4)
    // With 7 txes
    CBlock block;
    CMutableTransaction mtx;
    std::vector<unsigned char> vchScript;

    block.nVersion = 1;
    block.hashPrevBlock = uint256S("0x0000000000016780c81d42b7eff86974c36f5ae026e8662a4393a7f39c86bb82");
    block.nTime = 1293629558;
    block.nBits = 0x1b04864c;
    block.nNonce = 0x29854b55;
    block.vtx.resize(7);

    mtx.vin.resize(1);
    vchScript = ParseHex("044c86041b0136");
    mtx.vin[0].scriptSig = CScript(vchScript.begin(), vchScript.end());
    mtx.vout.resize(1);
    mtx.vout[0].nValue = 5000000000;
    vchScript = ParseHex("4104eaafc2314def4ca98ac970241bcab022b9c1e1f4ea423a20f134c876f2c01ec0f0dd5b2e86e7168cefe0d81113c3807420ce13ad1357231a2252247d97a46a91ac");
    mtx.vout[0].scriptPubKey = CScript(vchScript.begin(), vchScript.end());
    block.vtx[0] = mtx;

    mtx.vin.resize(1);
    mtx.vin[0].prevout.hash = uint256S("0x437248e07f73b031bba6aacde3e9f3f70b12559298084f42d12798a2a620adbc");
    mtx.vin[0].prevout.n = 0;
    vchScript = ParseHex("4830450220356e834b046cadc0f8ebb5a8a017b02de59c86305403dad52cd77b55af062ea10221009253cd6c119d4729b77c978e1e2aa19f5ea6e0e52b3f16e32fa608cd5bab753901");
    mtx.vin[0].scriptSig = CScript(vchScript.begin(), vchScript.end());
    mtx.vout.resize(2);
    mtx.vout[0].nValue = 4500000000;
    vchScript = ParseHex("76a9142b4b8072ecbba129b6453c63e129e643207249ca88ac");
    mtx.vout[0].scriptPubKey = CScript(vchScript.begin(), vchScript.end());
    mtx.vout[1].nValue = 500000000;
    vchScript = ParseHex("76a9141b8dd13b994bcfc787b32aeadf58ccb3615cbd5488ac");
    mtx.vout[1].scriptPubKey = CScript(vchScript.begin(), vchScript.end());
    block.vtx[1] = mtx;

    mtx.vin.resize(3);
    mtx.vin[0].prevout.hash = uint256S("0x0bcb16af267dee77ed8761662d31ee9d9a1bf1e4d268a9e7127407ebb3f9acfd");
    mtx.vin[0].prevout.n = 0;
    vchScript = ParseHex("493046022100ea1608e70911ca0de5af51ba57ad23b9a51db8d28f82c53563c56a05c20f5a87022100a8bdc8b4a8acc8634c6b420410150775eb7f2474f5615f7fccd65af30f310fbf01410465fdf49e29b06b9a1582287b6279014f834edc317695d125ef623c1cc3aaece245bd69fcad7508666e9c74a49dc9056d5fc14338ef38118dc4afae5fe2c585ca");
    mtx.vin[0].scriptSig = CScript(vchScript.begin(), vchScript.end());
    mtx.vin[1].prevout.hash = uint256S("0x83afb597f39d42753e3a97b897b471f02d0be7963ef8c4f350cb4e6313199e30");
    mtx.vin[1].prevout.n = 0;
    vchScript = ParseHex("48304502202bdb79c596a9ffc24e96f4386199aba386e9bc7b6071516e2b51dda942b3a1ed022100c53a857e76b724fc14d45311eac5019650d415c3abb5428f3aae16d8e69bec2301");
    mtx.vin[1].scriptSig = CScript(vchScript.begin(), vchScript.end());
    mtx.vin[2].prevout.hash = uint256S("0xc1e6b1bc0c0e1b2bce13df72d3b6b54d837d8f428ac1edc98050699134e38920");
    mtx.vin[2].prevout.n = 0;
    vchScript = ParseHex("483045022100d4ce67c5896ee251c810ac1ff9ceccd328b497c8f553ab6e08431e7d40bad6b5022033119c0c2b7d792d31f1187779c7bd95aefd93d90a715586d73801d9b47471c601");
    mtx.vin[2].scriptSig = CScript(vchScript.begin(), vchScript.end());
    mtx.vout.resize(1);
    mtx.vout[0].nValue = 14500000000;
    vchScript = ParseHex("76a914c7b55141d097ea5df7a0ed330cf794376e53ec8d88ac");
    mtx.vout[0].scriptPubKey = CScript(vchScript.begin(), vchScript.end());
    block.vtx[2] = mtx;

    mtx.vin.resize(4);
    mtx.vin[0].prevout.hash = uint256S("0x4734748106a0724b8f1308dde8cd97d3c1f01b1eeeec92e7a36940aa14e2f05b");
    mtx.vin[0].prevout.n = 0;
    vchScript = ParseHex("48304502200c45de8c4f3e2c1821f2fc878cba97b1e6f8807d94930713aa1c86a67b9bf1e40221008581abfef2e30f957815fc89978423746b2086375ca8ecf359c85c2a5b7c88ad01410462bb73f76ca0994fcb8b4271e6fb7561f5c0f9ca0cf6485261c4a0dc894f4ab844c6cdfb97cd0b60ffb5018ffd6238f4d87270efb1d3ae37079b794a92d7ec95");
    mtx.vin[0].scriptSig = CScript(vchScript.begin(), vchScript.end());
    mtx.vin[1].prevout.hash = uint256S("0x2ca79a5fda3da844c3cebb809c599be248f2e0883d25d259fc408d95d7f769d6");
    mtx.vin[1].prevout.n = 0;
    vchScript = ParseHex("473044022078124c8beeaa825f9e0b30bff96e564dd859432f2d0cb3b72d3d5d93d38d7e930220691d233b6c0f995be5acb03d70a7f7a65b6bc9bdd426260f38a1346669507a3601410462bb73f76ca0994fcb8b4271e6fb7561f5c0f9ca0cf6485261c4a0dc894f4ab844c6cdfb97cd0b60ffb5018ffd6238f4d87270efb1d3ae37079b794a92d7ec95");
    mtx.vin[1].scriptSig = CScript(vchScript.begin(), vchScript.end());
    mtx.vin[2].prevout.hash = uint256S("0xa4daaf4d6b63536fa4e046292337a5b72b37fd51f06c16689a22f5930daf78f8");
    mtx.vin[2].prevout.n = 0;
    vchScript = ParseHex("493046022100c717d1714551663f69c3c5759bdbb3a0fcd3fab023abc0e522fe6440de35d8290221008d9cbe25bffc44af2b18e81c58eb37293fd7fe1c2e7b46fc37ee8c96c50ab1e201410462bb73f76ca0994fcb8b4271e6fb7561f5c0f9ca0cf6485261c4a0dc894f4ab844c6cdfb97cd0b60ffb5018ffd6238f4d87270efb1d3ae37079b794a92d7ec95");
    mtx.vin[2].scriptSig = CScript(vchScript.begin(), vchScript.end());
    mtx.vin[3].prevout.hash = uint256S("0xe2a1d7bf8c9b995f423f97887cd0bc6309e6d9d20faa94f8f2d79c8568b6f227");
    mtx.vin[3].prevout.n = 0;
    vchScript = ParseHex("493046022100e00847147cbf517bcc2f502f3ddc6d284358d102ed20d47a8aa788a62f0db780022100d17b2d6fa84dcaf1c95d88d7e7c30385aecf415588d749afd3ec81f6022cecd701410462bb73f76ca0994fcb8b4271e6fb7561f5c0f9ca0cf6485261c4a0dc894f4ab844c6cdfb97cd0b60ffb5018ffd6238f4d87270efb1d3ae37079b794a92d7ec95");
    mtx.vin[3].scriptSig = CScript(vchScript.begin(), vchScript.end());
    mtx.vout.resize(1);
    mtx.vout[0].nValue = 20000000000;
    vchScript = ParseHex("76a914b6efd80d99179f4f4ff6f4dd0a007d018c385d2188ac");
    mtx.vout[0].scriptPubKey = CScript(vchScript.begin(), vchScript.end());
    block.vtx[3] = mtx;

    mtx.vin.resize(1);
    mtx.vin[0].prevout.hash = uint256S("0xc4d3eb542503e05643cd16f68d750ba5e55c54108e253a37f98ecef1b2374583");
    mtx.vin[0].prevout.n = 0;
    vchScript = ParseHex("483045022100e68f422dd7c34fdce11eeb4509ddae38201773dd62f284e8aa9d96f85099d0b002202243bd399ff96b649a0fad05fa759d6a882f0af8c90cf7632c2840c29070aec20141045e58067e815c2f464c6a2a15f987758374203895710c2d452442e28496ff38ba8f5fd901dc20e29e88477167fe4fc299bf818fd0d9e1632d467b2a3d9503b1aa");
    mtx.vin[0].scriptSig = CScript(vchScript.begin(), vchScript.end());
    mtx.vout.resize(2);
    mtx.vout[0].nValue = 13806000000;
    vchScript = ParseHex("76a914f34c3e10eb387efe872acb614c89e78bfca7815d88ac");
    mtx.vout[0].scriptPubKey = CScript(vchScript.begin(), vchScript.end());
    mtx.vout[1].nValue = 5000000;
    vchScript = ParseHex("76a914a84e272933aaf87e1715d7786c51dfaeb5b65a6f88ac");
    mtx.vout[1].scriptPubKey = CScript(vchScript.begin(), vchScript.end());
    block.vtx[4] = mtx;

    mtx.vin.resize(1);
    mtx.vin[0].prevout.hash = uint256S("0xe73ae02709850d51c538a8fd89013ee299d906d9f317fe7d30eff6e6c881ac43");
    mtx.vin[0].prevout.n = 0;
    vchScript = ParseHex("4930460221009c87c344760a64cb8ae6685a3eec2c1ac1bed5b88c87de51acd0e124f266c16602210082d07c037359c3a257b5c63ebd90f5a5edf97b2ac1c434b08ca998839f346dd40141040ba7e521fa7946d12edbb1d1e95a15c34bd4398195e86433c92b431cd315f455fe30032ede69cad9d1e1ed6c3c4ec0dbfced53438c625462afb792dcb098544b");
    mtx.vin[0].scriptSig = CScript(vchScript.begin(), vchScript.end());
    mtx.vout.resize(2);
    mtx.vout[0].nValue = 1000000;
    vchScript = ParseHex("76a9144676d1b820d63ec272f1900d59d43bc6463d96f888ac");
    mtx.vout[0].scriptPubKey = CScript(vchScript.begin(), vchScript.end());
    mtx.vout[1].nValue = 1000000;
    vchScript = ParseHex("76a914648d04341d00d7968b3405c034adc38d4d8fb9bd88ac");
    mtx.vout[1].scriptPubKey = CScript(vchScript.begin(), vchScript.end());
    block.vtx[5] = mtx;

    mtx.vin.resize(2);
    mtx.vin[0].prevout.hash = uint256S("0xf1e305f72f450a7d01af712e7c03fe0cc467059c00d2a8f4555cea017591cc48");
    mtx.vin[0].prevout.n = 0;
    vchScript = ParseHex("483045022100bf5fdc86dc5f08a5d5c8e43a8c9d5b1ed8c65562e280007b52b133021acd9acc02205e325d613e555f772802bf413d36ba807892ed1a690a77811d3033b3de226e0a01410429fa713b124484cb2bd7b5557b2c0b9df7b2b1fee61825eadc5ae6c37a9920d38bfccdc7dc3cb0c47d7b173dbc9db8d37db0a33ae487982c59c6f8606e9d1791");
    mtx.vin[0].scriptSig = CScript(vchScript.begin(), vchScript.end());
    mtx.vin[1].prevout.hash = uint256S("0x68d0685759c3d4f3f90a4f0e48d1b77641f06bb1f0b83a8841e8d71d5570ed41");
    mtx.vin[1].prevout.n = 0;
    vchScript = ParseHex("4830450221008513ad65187b903aed1102d1d0c47688127658c51106753fed0151ce9c16b80902201432b9ebcb87bd04ceb2de66035fbbaf4bf8b00d1cfe41f1a1f7338f9ad79d210141049d4cf80125bf50be1709f718c07ad15d0fc612b7da1f5570dddc35f2a352f0f27c978b06820edca9ef982c35fda2d255afba340068c5035552368bc7200c1488");
    mtx.vin[1].scriptSig = CScript(vchScript.begin(), vchScript.end());
    mtx.vout.resize(1);
    mtx.vout[0].nValue = 4000000;
    vchScript = ParseHex("76a9148edb68822f1ad580b043c7b3df2e400f8699eb4888ac");
    mtx.vout[0].scriptPubKey = CScript(vchScript.begin(), vchScript.end());
    block.vtx[6] = mtx;

    block.hashMerkleRoot = block.BuildMerkleTree();

    CBloomFilter filter(10, 0.000001, 0, BLOOM_UPDATE_P2PUBKEY_ONLY);
    // Match the generation pubkey
    filter.insert(ParseHex("04eaafc2314def4ca98ac970241bcab022b9c1e1f4ea423a20f134c876f2c01ec0f0dd5b2e86e7168cefe0d81113c3807420ce13ad1357231a2252247d97a46a91"));
    // ...and the output address of the 4th transaction
    filter.insert(ParseHex("b6efd80d99179f4f4ff6f4dd0a007d018c385d21"));

    CMerkleBlock merkleBlock(block, filter);
    BOOST_CHECK(merkleBlock.header.GetHash() == block.GetHash());

    // We should match the generation outpoint
    BOOST_CHECK(filter.contains(COutPoint(block.vtx[0].GetHash(), 0)));
    // ... but not the 4th transaction's output (its not pay-2-pubkey)
    BOOST_CHECK(!filter.contains(COutPoint(block.vtx[3].GetHash(), 0)));
}

BOOST_AUTO_TEST_CASE(merkle_block_4_test_update_none)
{
    // Random real block (000000000000b731f2eef9e8c63173adfb07e41bd53eb0ef0a6b720d6cb6dea4)
    // With 7 txes
    CBlock block;
    CMutableTransaction mtx;
    std::vector<unsigned char> vchScript;

    block.nVersion = 1;
    block.hashPrevBlock = uint256S("0x0000000000016780c81d42b7eff86974c36f5ae026e8662a4393a7f39c86bb82");
    block.nTime = 1293629558;
    block.nBits = 0x1b04864c;
    block.nNonce = 0x29854b55;
    block.vtx.resize(7);

    mtx.vin.resize(1);
    vchScript = ParseHex("044c86041b0136");
    mtx.vin[0].scriptSig = CScript(vchScript.begin(), vchScript.end());
    mtx.vout.resize(1);
    mtx.vout[0].nValue = 5000000000;
    vchScript = ParseHex("4104eaafc2314def4ca98ac970241bcab022b9c1e1f4ea423a20f134c876f2c01ec0f0dd5b2e86e7168cefe0d81113c3807420ce13ad1357231a2252247d97a46a91ac");
    mtx.vout[0].scriptPubKey = CScript(vchScript.begin(), vchScript.end());
    block.vtx[0] = mtx;

    mtx.vin.resize(1);
    mtx.vin[0].prevout.hash = uint256S("0x437248e07f73b031bba6aacde3e9f3f70b12559298084f42d12798a2a620adbc");
    mtx.vin[0].prevout.n = 0;
    vchScript = ParseHex("4830450220356e834b046cadc0f8ebb5a8a017b02de59c86305403dad52cd77b55af062ea10221009253cd6c119d4729b77c978e1e2aa19f5ea6e0e52b3f16e32fa608cd5bab753901");
    mtx.vin[0].scriptSig = CScript(vchScript.begin(), vchScript.end());
    mtx.vout.resize(2);
    mtx.vout[0].nValue = 4500000000;
    vchScript = ParseHex("76a9142b4b8072ecbba129b6453c63e129e643207249ca88ac");
    mtx.vout[0].scriptPubKey = CScript(vchScript.begin(), vchScript.end());
    mtx.vout[1].nValue = 500000000;
    vchScript = ParseHex("76a9141b8dd13b994bcfc787b32aeadf58ccb3615cbd5488ac");
    mtx.vout[1].scriptPubKey = CScript(vchScript.begin(), vchScript.end());
    block.vtx[1] = mtx;

    mtx.vin.resize(3);
    mtx.vin[0].prevout.hash = uint256S("0x0bcb16af267dee77ed8761662d31ee9d9a1bf1e4d268a9e7127407ebb3f9acfd");
    mtx.vin[0].prevout.n = 0;
    vchScript = ParseHex("493046022100ea1608e70911ca0de5af51ba57ad23b9a51db8d28f82c53563c56a05c20f5a87022100a8bdc8b4a8acc8634c6b420410150775eb7f2474f5615f7fccd65af30f310fbf01410465fdf49e29b06b9a1582287b6279014f834edc317695d125ef623c1cc3aaece245bd69fcad7508666e9c74a49dc9056d5fc14338ef38118dc4afae5fe2c585ca");
    mtx.vin[0].scriptSig = CScript(vchScript.begin(), vchScript.end());
    mtx.vin[1].prevout.hash = uint256S("0x83afb597f39d42753e3a97b897b471f02d0be7963ef8c4f350cb4e6313199e30");
    mtx.vin[1].prevout.n = 0;
    vchScript = ParseHex("48304502202bdb79c596a9ffc24e96f4386199aba386e9bc7b6071516e2b51dda942b3a1ed022100c53a857e76b724fc14d45311eac5019650d415c3abb5428f3aae16d8e69bec2301");
    mtx.vin[1].scriptSig = CScript(vchScript.begin(), vchScript.end());
    mtx.vin[2].prevout.hash = uint256S("0xc1e6b1bc0c0e1b2bce13df72d3b6b54d837d8f428ac1edc98050699134e38920");
    mtx.vin[2].prevout.n = 0;
    vchScript = ParseHex("483045022100d4ce67c5896ee251c810ac1ff9ceccd328b497c8f553ab6e08431e7d40bad6b5022033119c0c2b7d792d31f1187779c7bd95aefd93d90a715586d73801d9b47471c601");
    mtx.vin[2].scriptSig = CScript(vchScript.begin(), vchScript.end());
    mtx.vout.resize(1);
    mtx.vout[0].nValue = 14500000000;
    vchScript = ParseHex("76a914c7b55141d097ea5df7a0ed330cf794376e53ec8d88ac");
    mtx.vout[0].scriptPubKey = CScript(vchScript.begin(), vchScript.end());
    block.vtx[2] = mtx;

    mtx.vin.resize(4);
    mtx.vin[0].prevout.hash = uint256S("0x4734748106a0724b8f1308dde8cd97d3c1f01b1eeeec92e7a36940aa14e2f05b");
    mtx.vin[0].prevout.n = 0;
    vchScript = ParseHex("48304502200c45de8c4f3e2c1821f2fc878cba97b1e6f8807d94930713aa1c86a67b9bf1e40221008581abfef2e30f957815fc89978423746b2086375ca8ecf359c85c2a5b7c88ad01410462bb73f76ca0994fcb8b4271e6fb7561f5c0f9ca0cf6485261c4a0dc894f4ab844c6cdfb97cd0b60ffb5018ffd6238f4d87270efb1d3ae37079b794a92d7ec95");
    mtx.vin[0].scriptSig = CScript(vchScript.begin(), vchScript.end());
    mtx.vin[1].prevout.hash = uint256S("0x2ca79a5fda3da844c3cebb809c599be248f2e0883d25d259fc408d95d7f769d6");
    mtx.vin[1].prevout.n = 0;
    vchScript = ParseHex("473044022078124c8beeaa825f9e0b30bff96e564dd859432f2d0cb3b72d3d5d93d38d7e930220691d233b6c0f995be5acb03d70a7f7a65b6bc9bdd426260f38a1346669507a3601410462bb73f76ca0994fcb8b4271e6fb7561f5c0f9ca0cf6485261c4a0dc894f4ab844c6cdfb97cd0b60ffb5018ffd6238f4d87270efb1d3ae37079b794a92d7ec95");
    mtx.vin[1].scriptSig = CScript(vchScript.begin(), vchScript.end());
    mtx.vin[2].prevout.hash = uint256S("0xa4daaf4d6b63536fa4e046292337a5b72b37fd51f06c16689a22f5930daf78f8");
    mtx.vin[2].prevout.n = 0;
    vchScript = ParseHex("493046022100c717d1714551663f69c3c5759bdbb3a0fcd3fab023abc0e522fe6440de35d8290221008d9cbe25bffc44af2b18e81c58eb37293fd7fe1c2e7b46fc37ee8c96c50ab1e201410462bb73f76ca0994fcb8b4271e6fb7561f5c0f9ca0cf6485261c4a0dc894f4ab844c6cdfb97cd0b60ffb5018ffd6238f4d87270efb1d3ae37079b794a92d7ec95");
    mtx.vin[2].scriptSig = CScript(vchScript.begin(), vchScript.end());
    mtx.vin[3].prevout.hash = uint256S("0xe2a1d7bf8c9b995f423f97887cd0bc6309e6d9d20faa94f8f2d79c8568b6f227");
    mtx.vin[3].prevout.n = 0;
    vchScript = ParseHex("493046022100e00847147cbf517bcc2f502f3ddc6d284358d102ed20d47a8aa788a62f0db780022100d17b2d6fa84dcaf1c95d88d7e7c30385aecf415588d749afd3ec81f6022cecd701410462bb73f76ca0994fcb8b4271e6fb7561f5c0f9ca0cf6485261c4a0dc894f4ab844c6cdfb97cd0b60ffb5018ffd6238f4d87270efb1d3ae37079b794a92d7ec95");
    mtx.vin[3].scriptSig = CScript(vchScript.begin(), vchScript.end());
    mtx.vout.resize(1);
    mtx.vout[0].nValue = 20000000000;
    vchScript = ParseHex("76a914b6efd80d99179f4f4ff6f4dd0a007d018c385d2188ac");
    mtx.vout[0].scriptPubKey = CScript(vchScript.begin(), vchScript.end());
    block.vtx[3] = mtx;

    mtx.vin.resize(1);
    mtx.vin[0].prevout.hash = uint256S("0xc4d3eb542503e05643cd16f68d750ba5e55c54108e253a37f98ecef1b2374583");
    mtx.vin[0].prevout.n = 0;
    vchScript = ParseHex("483045022100e68f422dd7c34fdce11eeb4509ddae38201773dd62f284e8aa9d96f85099d0b002202243bd399ff96b649a0fad05fa759d6a882f0af8c90cf7632c2840c29070aec20141045e58067e815c2f464c6a2a15f987758374203895710c2d452442e28496ff38ba8f5fd901dc20e29e88477167fe4fc299bf818fd0d9e1632d467b2a3d9503b1aa");
    mtx.vin[0].scriptSig = CScript(vchScript.begin(), vchScript.end());
    mtx.vout.resize(2);
    mtx.vout[0].nValue = 13806000000;
    vchScript = ParseHex("76a914f34c3e10eb387efe872acb614c89e78bfca7815d88ac");
    mtx.vout[0].scriptPubKey = CScript(vchScript.begin(), vchScript.end());
    mtx.vout[1].nValue = 5000000;
    vchScript = ParseHex("76a914a84e272933aaf87e1715d7786c51dfaeb5b65a6f88ac");
    mtx.vout[1].scriptPubKey = CScript(vchScript.begin(), vchScript.end());
    block.vtx[4] = mtx;

    mtx.vin.resize(1);
    mtx.vin[0].prevout.hash = uint256S("0xe73ae02709850d51c538a8fd89013ee299d906d9f317fe7d30eff6e6c881ac43");
    mtx.vin[0].prevout.n = 0;
    vchScript = ParseHex("4930460221009c87c344760a64cb8ae6685a3eec2c1ac1bed5b88c87de51acd0e124f266c16602210082d07c037359c3a257b5c63ebd90f5a5edf97b2ac1c434b08ca998839f346dd40141040ba7e521fa7946d12edbb1d1e95a15c34bd4398195e86433c92b431cd315f455fe30032ede69cad9d1e1ed6c3c4ec0dbfced53438c625462afb792dcb098544b");
    mtx.vin[0].scriptSig = CScript(vchScript.begin(), vchScript.end());
    mtx.vout.resize(2);
    mtx.vout[0].nValue = 1000000;
    vchScript = ParseHex("76a9144676d1b820d63ec272f1900d59d43bc6463d96f888ac");
    mtx.vout[0].scriptPubKey = CScript(vchScript.begin(), vchScript.end());
    mtx.vout[1].nValue = 1000000;
    vchScript = ParseHex("76a914648d04341d00d7968b3405c034adc38d4d8fb9bd88ac");
    mtx.vout[1].scriptPubKey = CScript(vchScript.begin(), vchScript.end());
    block.vtx[5] = mtx;

    mtx.vin.resize(2);
    mtx.vin[0].prevout.hash = uint256S("0xf1e305f72f450a7d01af712e7c03fe0cc467059c00d2a8f4555cea017591cc48");
    mtx.vin[0].prevout.n = 0;
    vchScript = ParseHex("483045022100bf5fdc86dc5f08a5d5c8e43a8c9d5b1ed8c65562e280007b52b133021acd9acc02205e325d613e555f772802bf413d36ba807892ed1a690a77811d3033b3de226e0a01410429fa713b124484cb2bd7b5557b2c0b9df7b2b1fee61825eadc5ae6c37a9920d38bfccdc7dc3cb0c47d7b173dbc9db8d37db0a33ae487982c59c6f8606e9d1791");
    mtx.vin[0].scriptSig = CScript(vchScript.begin(), vchScript.end());
    mtx.vin[1].prevout.hash = uint256S("0x68d0685759c3d4f3f90a4f0e48d1b77641f06bb1f0b83a8841e8d71d5570ed41");
    mtx.vin[1].prevout.n = 0;
    vchScript = ParseHex("4830450221008513ad65187b903aed1102d1d0c47688127658c51106753fed0151ce9c16b80902201432b9ebcb87bd04ceb2de66035fbbaf4bf8b00d1cfe41f1a1f7338f9ad79d210141049d4cf80125bf50be1709f718c07ad15d0fc612b7da1f5570dddc35f2a352f0f27c978b06820edca9ef982c35fda2d255afba340068c5035552368bc7200c1488");
    mtx.vin[1].scriptSig = CScript(vchScript.begin(), vchScript.end());
    mtx.vout.resize(1);
    mtx.vout[0].nValue = 4000000;
    vchScript = ParseHex("76a9148edb68822f1ad580b043c7b3df2e400f8699eb4888ac");
    mtx.vout[0].scriptPubKey = CScript(vchScript.begin(), vchScript.end());
    block.vtx[6] = mtx;

    block.hashMerkleRoot = block.BuildMerkleTree();

    CBloomFilter filter(10, 0.000001, 0, BLOOM_UPDATE_NONE);
    // Match the generation pubkey
    filter.insert(ParseHex("04eaafc2314def4ca98ac970241bcab022b9c1e1f4ea423a20f134c876f2c01ec0f0dd5b2e86e7168cefe0d81113c3807420ce13ad1357231a2252247d97a46a91"));
    // ...and the output address of the 4th transaction
    filter.insert(ParseHex("b6efd80d99179f4f4ff6f4dd0a007d018c385d21"));

    CMerkleBlock merkleBlock(block, filter);
    BOOST_CHECK(merkleBlock.header.GetHash() == block.GetHash());

    // We shouldn't match any outpoints (UPDATE_NONE)
    BOOST_CHECK(!filter.contains(COutPoint(block.vtx[0].GetHash(), 0)));
    BOOST_CHECK(!filter.contains(COutPoint(block.vtx[3].GetHash(), 0)));
}

static std::vector<unsigned char> RandomData()
{
    uint256 r = GetRandHash();
    return std::vector<unsigned char>(r.begin(), r.end());
}

BOOST_AUTO_TEST_CASE(rolling_bloom)
{
    // last-100-entry, 1% false positive:
    CRollingBloomFilter rb1(100, 0.01);

    // Overfill:
    static const int DATASIZE=399;
    std::vector<unsigned char> data[DATASIZE];
    for (int i = 0; i < DATASIZE; i++) {
        data[i] = RandomData();
        rb1.insert(data[i]);
    }
    // Last 100 guaranteed to be remembered:
    for (int i = 299; i < DATASIZE; i++) {
        BOOST_CHECK(rb1.contains(data[i]));
    }

    // false positive rate is 1%, so we should get about 100 hits if
    // testing 10,000 random keys. We get worst-case false positive
    // behavior when the filter is as full as possible, which is
    // when we've inserted one minus an integer multiple of nElement*2.
    unsigned int nHits = 0;
    for (int i = 0; i < 10000; i++) {
        if (rb1.contains(RandomData()))
            ++nHits;
    }
    // Run test_bitcoin with --log_level=message to see BOOST_TEST_MESSAGEs:
    BOOST_TEST_MESSAGE("RollingBloomFilter got " << nHits << " false positives (~100 expected)");

    // Insanely unlikely to get a fp count outside this range:
    BOOST_CHECK(nHits > 25);
    BOOST_CHECK(nHits < 175);

    BOOST_CHECK(rb1.contains(data[DATASIZE-1]));
    rb1.reset();
    BOOST_CHECK(!rb1.contains(data[DATASIZE-1]));

    // Now roll through data, make sure last 100 entries
    // are always remembered:
    for (int i = 0; i < DATASIZE; i++) {
        if (i >= 100)
            BOOST_CHECK(rb1.contains(data[i-100]));
        rb1.insert(data[i]);
        BOOST_CHECK(rb1.contains(data[i]));
    }

    // Insert 999 more random entries:
    for (int i = 0; i < 999; i++) {
        std::vector<unsigned char> d = RandomData();
        rb1.insert(d);
        BOOST_CHECK(rb1.contains(d));
    }
    // Sanity check to make sure the filter isn't just filling up:
    nHits = 0;
    for (int i = 0; i < DATASIZE; i++) {
        if (rb1.contains(data[i]))
            ++nHits;
    }
    // Expect about 5 false positives, more than 100 means
    // something is definitely broken.
    BOOST_TEST_MESSAGE("RollingBloomFilter got " << nHits << " false positives (~5 expected)");
    BOOST_CHECK(nHits < 100);

    // last-1000-entry, 0.01% false positive:
    CRollingBloomFilter rb2(1000, 0.001);
    for (int i = 0; i < DATASIZE; i++) {
        rb2.insert(data[i]);
    }
    // ... room for all of them:
    for (int i = 0; i < DATASIZE; i++) {
        BOOST_CHECK(rb2.contains(data[i]));
    }
}

BOOST_AUTO_TEST_SUITE_END()
