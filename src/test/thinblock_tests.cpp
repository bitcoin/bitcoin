// Copyright (c) 2016 Bitcoin Unlimited Developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "thinblock.h"
#include "bloom.h"
#include "chainparams.h"
#include "main.h"
#include "primitives/block.h"
#include "random.h"
#include "serialize.h"
#include "streams.h"
#include "txmempool.h"
#include "uint256.h"
#include "unlimited.h"
#include "util.h"
#include "utilstrencodings.h"
#include "version.h"

#include "test/test_bitcoin.h"

#include <boost/test/unit_test.hpp>
#include <sstream>
#include <string.h>

CBlock TestBlock()
{ // Thanks dagurval :)
    // Block taken from bloom_tests.cpp merkle_block_1
    // Random real block (0000000000013b8ab2cd513b0261a14096412195a72a0c4827d229dcc7e0f7af)
    // With 9 txes
    CDataStream stream(
        ParseHex(
            "0100000090f0a9f110702f808219ebea1173056042a714bad51b916cb6800000000000005275289558f51c9966699404ae2294730c"
            "3c9f9bda53523ce50e9b95e558da2fdb261b4d4c86041b1ab1bf930901000000010000000000000000000000000000000000000000"
            "000000000000000000000000ffffffff07044c86041b0146ffffffff0100f2052a01000000434104e18f7afbe4721580e81e8414fc"
            "8c24d7cfacf254bb5c7b949450c3e997c2dc1242487a8169507b631eb3771f2b425483fb13102c4eb5d858eef260fe70fbfae0ac00"
            "000000010000000196608ccbafa16abada902780da4dc35dafd7af05fa0da08cf833575f8cf9e836000000004a493046022100dab2"
            "4889213caf43ae6adc41cf1c9396c08240c199f5225acf45416330fd7dbd022100fe37900e0644bf574493a07fc5edba06dbc07c31"
            "1b947520c2d514bc5725dcb401ffffffff0100f2052a010000001976a914f15d1921f52e4007b146dfa60f369ed2fc393ce288ac00"
            "0000000100000001fb766c1288458c2bafcfec81e48b24d98ec706de6b8af7c4e3c29419bfacb56d000000008c493046022100f268"
            "ba165ce0ad2e6d93f089cfcd3785de5c963bb5ea6b8c1b23f1ce3e517b9f022100da7c0f21adc6c401887f2bfd1922f11d76159cbc"
            "597fbd756a23dcbb00f4d7290141042b4e8625a96127826915a5b109852636ad0da753c9e1d5606a50480cd0c40f1f8b8d898235e5"
            "71fe9357d9ec842bc4bba1827daaf4de06d71844d0057707966affffffff0280969800000000001976a9146963907531db72d0ed1a"
            "0cfb471ccb63923446f388ac80d6e34c000000001976a914f0688ba1c0d1ce182c7af6741e02658c7d4dfcd388ac00000000010000"
            "0002c40297f730dd7b5a99567eb8d27b78758f607507c52292d02d4031895b52f2ff010000008b483045022100f7edfd4b0aac404e"
            "5bab4fd3889e0c6c41aa8d0e6fa122316f68eddd0a65013902205b09cc8b2d56e1cd1f7f2fafd60a129ed94504c4ac7bdc67b56fe6"
            "7512658b3e014104732012cb962afa90d31b25d8fb0e32c94e513ab7a17805c14ca4c3423e18b4fb5d0e676841733cb83abaf97584"
            "5c9f6f2a8097b7d04f4908b18368d6fc2d68ecffffffffca5065ff9617cbcba45eb23726df6498a9b9cafed4f54cbab9d227b0035d"
            "defb000000008a473044022068010362a13c7f9919fa832b2dee4e788f61f6f5d344a7c2a0da6ae740605658022006d1af525b9a14"
            "a35c003b78b72bd59738cd676f845d1ff3fc25049e01003614014104732012cb962afa90d31b25d8fb0e32c94e513ab7a17805c14c"
            "a4c3423e18b4fb5d0e676841733cb83abaf975845c9f6f2a8097b7d04f4908b18368d6fc2d68ecffffffff01001ec4110200000043"
            "410469ab4181eceb28985b9b4e895c13fa5e68d85761b7eee311db5addef76fa8621865134a221bd01f28ec9999ee3e021e60766e9"
            "d1f3458c115fb28650605f11c9ac000000000100000001cdaf2f758e91c514655e2dc50633d1e4c84989f8aa90a0dbc883f0d23ed5"
            "c2fa010000008b48304502207ab51be6f12a1962ba0aaaf24a20e0b69b27a94fac5adf45aa7d2d18ffd9236102210086ae728b370e"
            "5329eead9accd880d0cb070aea0c96255fae6c4f1ddcce1fd56e014104462e76fd4067b3a0aa42070082dcb0bf2f388b6495cf33d7"
            "89904f07d0f55c40fbd4b82963c69b3dc31895d0c772c812b1d5fbcade15312ef1c0e8ebbb12dcd4ffffffff02404b4c0000000000"
            "1976a9142b6ba7c9d796b75eef7942fc9288edd37c32f5c388ac002d3101000000001976a9141befba0cdc1ad56529371864d9f6cb"
            "042faa06b588ac000000000100000001b4a47603e71b61bc3326efd90111bf02d2f549b067f4c4a8fa183b57a0f800cb010000008a"
            "4730440220177c37f9a505c3f1a1f0ce2da777c339bd8339ffa02c7cb41f0a5804f473c9230220585b25a2ee80eb59292e52b987da"
            "d92acb0c64eced92ed9ee105ad153cdb12d001410443bd44f683467e549dae7d20d1d79cbdb6df985c6e9c029c8d0c6cb46cc1a4d3"
            "cf7923c5021b27f7a0b562ada113bc85d5fda5a1b41e87fe6e8802817cf69996ffffffff0280651406000000001976a91455056148"
            "59643ab7b547cd7f1f5e7e2a12322d3788ac00aa0271000000001976a914ea4720a7a52fc166c55ff2298e07baf70ae67e1b88ac00"
            "000000010000000586c62cd602d219bb60edb14a3e204de0705176f9022fe49a538054fb14abb49e010000008c493046022100f2bc"
            "2aba2534becbdf062eb993853a42bbbc282083d0daf9b4b585bd401aa8c9022100b1d7fd7ee0b95600db8535bbf331b19eed8d961f"
            "7a8e54159c53675d5f69df8c014104462e76fd4067b3a0aa42070082dcb0bf2f388b6495cf33d789904f07d0f55c40fbd4b82963c6"
            "9b3dc31895d0c772c812b1d5fbcade15312ef1c0e8ebbb12dcd4ffffffff03ad0e58ccdac3df9dc28a218bcf6f1997b0a93306faaa"
            "4b3a28ae83447b2179010000008b483045022100be12b2937179da88599e27bb31c3525097a07cdb52422d165b3ca2f2020ffcf702"
            "200971b51f853a53d644ebae9ec8f3512e442b1bcb6c315a5b491d119d10624c83014104462e76fd4067b3a0aa42070082dcb0bf2f"
            "388b6495cf33d789904f07d0f55c40fbd4b82963c69b3dc31895d0c772c812b1d5fbcade15312ef1c0e8ebbb12dcd4ffffffff2acf"
            "cab629bbc8685792603762c921580030ba144af553d271716a95089e107b010000008b483045022100fa579a840ac258871365dd48"
            "cd7552f96c8eea69bd00d84f05b283a0dab311e102207e3c0ee9234814cfbb1b659b83671618f45abc1326b9edcc77d552a4f2a805"
            "c0014104462e76fd4067b3a0aa42070082dcb0bf2f388b6495cf33d789904f07d0f55c40fbd4b82963c69b3dc31895d0c772c812b1"
            "d5fbcade15312ef1c0e8ebbb12dcd4ffffffffdcdc6023bbc9944a658ddc588e61eacb737ddf0a3cd24f113b5a8634c517fcd20000"
            "00008b4830450221008d6df731df5d32267954bd7d2dda2302b74c6c2a6aa5c0ca64ecbabc1af03c75022010e55c571d65da7701ae"
            "2da1956c442df81bbf076cdbac25133f99d98a9ed34c014104462e76fd4067b3a0aa42070082dcb0bf2f388b6495cf33d789904f07"
            "d0f55c40fbd4b82963c69b3dc31895d0c772c812b1d5fbcade15312ef1c0e8ebbb12dcd4ffffffffe15557cd5ce258f479dfd6dc65"
            "14edf6d7ed5b21fcfa4a038fd69f06b83ac76e010000008b483045022023b3e0ab071eb11de2eb1cc3a67261b866f86bf6867d4558"
            "165f7c8c8aca2d86022100dc6e1f53a91de3efe8f63512850811f26284b62f850c70ca73ed5de8771fb451014104462e76fd4067b3"
            "a0aa42070082dcb0bf2f388b6495cf33d789904f07d0f55c40fbd4b82963c69b3dc31895d0c772c812b1d5fbcade15312ef1c0e8eb"
            "bb12dcd4ffffffff01404b4c00000000001976a9142b6ba7c9d796b75eef7942fc9288edd37c32f5c388ac00000000010000000166"
            "d7577163c932b4f9690ca6a80b6e4eb001f0a2fa9023df5595602aae96ed8d000000008a4730440220262b42546302dfb654a229ce"
            "fc86432b89628ff259dc87edd1154535b16a67e102207b4634c020a97c3e7bbd0d4d19da6aa2269ad9dded4026e896b213d73ca4b6"
            "3f014104979b82d02226b3a4597523845754d44f13639e3bf2df5e82c6aab2bdc79687368b01b1ab8b19875ae3c90d661a3d0a3316"
            "1dab29934edeb36aa01976be3baf8affffffff02404b4c00000000001976a9144854e695a02af0aeacb823ccbc272134561e0a1688"
            "ac40420f00000000001976a914abee93376d6b37b5c2940655a6fcaf1c8e74237988ac0000000001000000014e3f8ef2e91349a905"
            "9cb4f01e54ab2597c1387161d3da89919f7ea6acdbb371010000008c49304602210081f3183471a5ca22307c0800226f3ef9c35306"
            "9e0773ac76bb580654d56aa523022100d4c56465bdc069060846f4fbf2f6b20520b2a80b08b168b31e66ddb9c694e240014104976c"
            "79848e18251612f8940875b2b08d06e6dc73b9840e8860c066b7e87432c477e9a59a453e71e6d76d5fe34058b800a098fc1740ce30"
            "12e8fc8a00c96af966ffffffff02c0e1e400000000001976a9144134e75a6fcb6042034aab5e18570cf1f844f54788ac404b4c0000"
            "0000001976a9142b6ba7c9d796b75eef7942fc9288edd37c32f5c388ac00000000"),
        SER_NETWORK, PROTOCOL_VERSION);
    CBlock block;
    stream >> block;
    return block;
};

CService ipaddress2(uint32_t i, uint32_t port)
{
    struct in_addr s;
    s.s_addr = i;
    return CService(CNetAddr(s), port);
}


BOOST_FIXTURE_TEST_SUITE(thinblock_tests, TestingSetup)

BOOST_AUTO_TEST_CASE(thinblock_test)
{
    CBloomFilter filter;
    std::vector<uint256> vOrphanHashes;
    CAddress addr1(ipaddress2(0xa0b0c001, 10000));
    CNode dummyNode1(INVALID_SOCKET, addr1, "", true);

    // Create 10 random hashes to seed the orphanhash vector.  This way we will create a bloom filter
    // with a size of 10 elements.
    std::string hash = "3fba505b48865fccda4e248cecc39d5dfbc6b8ef7b4adc9cd27242c1193c714";
    for (int i = 0; i < 10; i++)
    {
        std::stringstream ss;
        ss << i;
        hash.append(ss.str());
        uint256 random_hash = uint256S(hash);
        vOrphanHashes.push_back(random_hash);
    }
    BuildSeededBloomFilter(filter, vOrphanHashes, TestBlock().GetHash(), &dummyNode1, true);

    /* empty filter */
    CBlock block = TestBlock();
    CThinBlock thinblock(block, filter);
    CXThinBlock xthinblock(block, &filter);
    BOOST_CHECK_EQUAL(9, thinblock.vMissingTx.size());
    BOOST_CHECK_EQUAL(9, xthinblock.vMissingTx.size());

    /* insert txid not in block */
    const uint256 random_hash = uint256S("3fba505b48865fccda4e248cecc39d5dfbc6b8ef7b4adc9cd27242c1193c7133");
    filter.insert(random_hash);
    CThinBlock thinblock1(block, filter);
    CXThinBlock xthinblock1(block, &filter);
    BOOST_CHECK_EQUAL(9, thinblock1.vMissingTx.size());
    BOOST_CHECK_EQUAL(9, xthinblock1.vMissingTx.size());

    /* insert txid in block */
    const uint256 hash_in_block = block.vtx[1].GetHash();
    filter.insert(hash_in_block);
    CThinBlock thinblock2(block, filter);
    CXThinBlock xthinblock2(block, &filter);
    BOOST_CHECK_EQUAL(8, thinblock2.vMissingTx.size());
    BOOST_CHECK_EQUAL(8, xthinblock2.vMissingTx.size());

    /*collision test*/
    BOOST_CHECK(!xthinblock2.collision);
    block.vtx.push_back(block.vtx[1]); // duplicate tx
    filter.clear();
    CXThinBlock xthinblock3(block, &filter);
    BOOST_CHECK(xthinblock3.collision);


    //  Add tests using a non-deterministic bloom filter which may
    //  or may not yeild a false positive.
    CBloomFilter filter1;
    BuildSeededBloomFilter(filter1, vOrphanHashes, TestBlock().GetHash(), &dummyNode1, false);

    /* empty filter */
    CBlock block1 = TestBlock();
    CThinBlock thinblock4(block1, filter1);
    CXThinBlock xthinblock4(block1, &filter1);
    BOOST_CHECK(thinblock4.vMissingTx.size() >= 8 && thinblock4.vMissingTx.size() <= 9);
    BOOST_CHECK(xthinblock4.vMissingTx.size() >= 8 && xthinblock4.vMissingTx.size() <= 9);

    /* insert txid not in block */
    const uint256 random_hash1 = uint256S("3fba505b48865fccda4e248cecc39d5dfbc6b8ef7b4adc9cd27242c1193c7132");
    filter1.insert(random_hash1);
    CThinBlock thinblock5(block1, filter1);
    CXThinBlock xthinblock5(block1, &filter1);
    BOOST_CHECK(thinblock5.vMissingTx.size() >= 8 && thinblock5.vMissingTx.size() <= 9);
    BOOST_CHECK(xthinblock5.vMissingTx.size() >= 8 && xthinblock5.vMissingTx.size() <= 9);

    /* insert txid in block */
    const uint256 hash_in_block1 = block.vtx[1].GetHash();
    filter1.insert(hash_in_block1);
    CThinBlock thinblock6(block1, filter1);
    CXThinBlock xthinblock6(block1, &filter1);
    BOOST_CHECK(thinblock6.vMissingTx.size() >= 7 && thinblock6.vMissingTx.size() <= 8);
    BOOST_CHECK(xthinblock6.vMissingTx.size() >= 7 && xthinblock6.vMissingTx.size() <= 8);

    /*collision test*/
    BOOST_CHECK(!xthinblock6.collision);
    block.vtx.push_back(block1.vtx[1]); // duplicate tx
    filter1.clear();
    CXThinBlock xthinblock7(block, &filter1);
    BOOST_CHECK(xthinblock7.collision);
}

BOOST_AUTO_TEST_SUITE_END()
