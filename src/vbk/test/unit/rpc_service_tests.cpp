#include <boost/test/unit_test.hpp>
#include <chainparams.h>
#include <consensus/merkle.h>
#include <rpc/request.h>
#include <rpc/server.h>
#include <test/util/setup_common.h>
#include <univalue.h>
#include <validation.h>
#include <wallet/wallet.h>

#include <vbk/service_locator.hpp>
#include <vbk/test/util/mock.hpp>

#include <string>
#include <vbk/init.hpp>
#include <vbk/merkle.hpp>

UniValue CallRPC(std::string args);

struct RpcServiceFixture : public TestChain100Setup {

    RpcServiceFixture(): TestChain100Setup()
    {
//        VeriBlock::InitConfig();
//        VeriBlock::InitPopService();
    }
};

BOOST_FIXTURE_TEST_SUITE(rpc_service_tests, RpcServiceFixture)

BOOST_AUTO_TEST_CASE(getpopdata_test)
{
//    int blockHeight = 10;
//    CBlockIndex* blockIndex = ChainActive()[blockHeight];
//    CBlock block;
//
//    BOOST_CHECK(ReadBlockFromDisk(block, blockIndex, Params().GetConsensus()));
//
//    CDataStream ssBlock(SER_NETWORK, PROTOCOL_VERSION);
//    ssBlock << blockIndex->GetBlockHeader();
//
//    uint256 txRoot = BlockMerkleRoot(block);
//    auto keystones = VeriBlock::getKeystoneHashesForTheNextBlock(blockIndex->pprev);
//    auto contextInfo = VeriBlock::ContextInfoContainer(blockIndex->nHeight, keystones, txRoot);
//    auto authedContext = contextInfo.getAuthenticated();
//
//    UniValue result;
//    BOOST_CHECK_NO_THROW(result = CallRPC("getpopdata " + std::to_string(blockHeight)));
//
//    BOOST_CHECK(find_value(result.get_obj(), "raw_contextinfocontainer").get_str() == HexStr(authedContext.begin(), authedContext.end()));
//    BOOST_CHECK(find_value(result.get_obj(), "block_header").get_str() == HexStr(ssBlock));
}

BOOST_AUTO_TEST_CASE(submitpop_test)
{
//    JSONRPCRequest request;
//    request.strMethod = "submitpop";
//    request.params = UniValue(UniValue::VARR);
//    request.fHelp = false;
//
//    std::vector<uint8_t> atv(100, 1);
//    std::vector<uint8_t> vtb(100, 2);
//
//    UniValue vtbs_params(UniValue::VARR);
//    vtbs_params.push_back(HexStr(vtb));
//
//    request.params.push_back(HexStr(atv));
//    request.params.push_back(vtbs_params);
//
//    if (RPCIsInWarmup(nullptr)) SetRPCWarmupFinished();
//
//    UniValue result;
//    BOOST_CHECK_NO_THROW(result = tableRPC.execute(request));
//
//    uint256 popTxHash;
//    popTxHash.SetHex(result.get_str());
//
//    BOOST_CHECK(mempool.exists(popTxHash));
}
BOOST_AUTO_TEST_SUITE_END()
