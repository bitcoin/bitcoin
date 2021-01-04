// Copyright (c) 2019-2020 Xenios SEZC
// https://www.veriblock.org
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <boost/test/unit_test.hpp>
#include <chainparams.h>
#include <chrono>
#include <consensus/merkle.h>
#include <fstream>
#include <rpc/request.h>
#include <rpc/server.h>
#include <test/util/setup_common.h>
#include <thread>
#include <univalue.h>
#include <validation.h>
#include <wallet/wallet.h>
#include <string>
#include <vbk/merkle.hpp>

#include <vbk/test/util/e2e_fixture.hpp>
#include <utility>

UniValue CallRPC(std::string args);

BOOST_AUTO_TEST_SUITE(rpc_service_tests)

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

BOOST_FIXTURE_TEST_CASE(submitpop_test, E2eFixture)
{
    auto makeRequest = [](std::string req, const std::string& arg){
      JSONRPCRequest request;
      request.strMethod = std::move(req);
      request.params = UniValue(UniValue::VARR);
      request.params.push_back(arg);
      request.fHelp = false;

      if (RPCIsInWarmup(nullptr)) SetRPCWarmupFinished();

      UniValue result;
      BOOST_CHECK_NO_THROW(result = tableRPC.execute(request));

      return result;
    };

    JSONRPCRequest request;
    request.strMethod = "submitpop";
    request.params = UniValue(UniValue::VARR);
    request.fHelp = false;

    uint32_t generateVtbs = 20;
    std::vector<VTB> vtbs;
    vtbs.reserve(generateVtbs);
    std::generate_n(std::back_inserter(vtbs), generateVtbs, [&]() {
        return endorseVbkTip();
    });

    BOOST_CHECK_EQUAL(vtbs.size(), generateVtbs);

    std::vector<altintegration::VbkBlock> vbk_blocks;
    for (const auto& vtb : vtbs) {
        vbk_blocks.push_back(vtb.containingBlock);
    }

    BOOST_CHECK(!vbk_blocks.empty());

    UniValue vbk_blocks_params(UniValue::VARR);
    for (const auto& b : vbk_blocks) {
        auto res = makeRequest("submitpopvbk", altintegration::SerializeToHex(b));
    }

    UniValue vtb_params(UniValue::VARR);
    for (const auto& vtb : vtbs) {
        auto res = makeRequest("submitpopvtb", altintegration::SerializeToHex(vtb));
    }
}

BOOST_AUTO_TEST_SUITE_END()
