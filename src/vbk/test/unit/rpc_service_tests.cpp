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
        altintegration::WriteStream stream;
        b.toVbkEncoding(stream);
        vbk_blocks_params.push_back(HexStr(stream.data()));
    }

    UniValue vtb_params(UniValue::VARR);
    for (const auto& vtb : vtbs) {
        altintegration::WriteStream stream;
        vtb.toVbkEncoding(stream);
        vtb_params.push_back(HexStr(stream.data()));
    }

    BOOST_CHECK_EQUAL(vbk_blocks.size(), vbk_blocks_params.size());
    BOOST_CHECK_EQUAL(vtbs.size(), vtb_params.size());

    UniValue atv_empty(UniValue::VARR);
    request.params.push_back(vbk_blocks_params);
    request.params.push_back(vtb_params);
    request.params.push_back(atv_empty);

    if (RPCIsInWarmup(nullptr)) SetRPCWarmupFinished();

    UniValue result;
    BOOST_CHECK_NO_THROW(result = tableRPC.execute(request));

    BOOST_CHECK_EQUAL(result["atvs"].size(), 0);
    BOOST_CHECK_EQUAL(result["vtbs"].size(), vtbs.size());
    BOOST_CHECK_EQUAL(result["vbkblocks"].size(), vbk_blocks.size());
}

BOOST_AUTO_TEST_SUITE_END()
