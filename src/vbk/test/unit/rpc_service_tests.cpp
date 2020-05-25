// Copyright (c) 2019-2020 Xenios SEZC
// https://www.veriblock.org
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <boost/test/unit_test.hpp>
#include <fstream>
#include <thread>
#include <chrono>
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

#include "veriblock/entities/test_case_entity.hpp"

UniValue CallRPC(std::string args);

BOOST_FIXTURE_TEST_SUITE(rpc_service_tests, TestChain100Setup)

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

static void InvalidateTestBlock(CBlockIndex* pblock)
{
    BlockValidationState state;

    InvalidateBlock(state, Params(), pblock);
    ActivateBestChain(state, Params());
}

static void ReconsiderTestBlock(CBlockIndex* pblock)
{
    BlockValidationState state;

    {
        LOCK(cs_main);
        ResetBlockFailureFlags(pblock);
    }
    ActivateBestChain(state, Params(), std::shared_ptr<const CBlock>());
}

BOOST_AUTO_TEST_CASE(savepopstate_test)
{
    CScript cbKey = CScript() << ToByteVector(coinbaseKey.GetPubKey()) << OP_CHECKSIG;
    std::string file_name = "vbtc_state_test";

    auto* oldTip = ChainActive().Tip();
    InvalidateTestBlock(oldTip);
    BOOST_CHECK(oldTip->GetBlockHash() != ChainActive().Tip()->GetBlockHash());

    CBlock block;
    block = this->CreateAndProcessBlock({}, cbKey);
    BOOST_CHECK(block.GetHash() == ChainActive().Tip()->GetBlockHash());
    block = this->CreateAndProcessBlock({}, cbKey);
    BOOST_CHECK(block.GetHash() == ChainActive().Tip()->GetBlockHash());
    block = this->CreateAndProcessBlock({}, cbKey);
    BOOST_CHECK(block.GetHash() == ChainActive().Tip()->GetBlockHash());
    auto* newTip = ChainActive().Tip();

    ReconsiderTestBlock(oldTip);

    JSONRPCRequest request;
    request.strMethod = "savepopstate";
    request.fHelp = false;
    request.params = UniValue(UniValue::VARR);
    request.params.push_back(file_name);

    if (RPCIsInWarmup(nullptr)) {
        SetRPCWarmupFinished();
    }

    UniValue result;
    BOOST_CHECK_NO_THROW(result = tableRPC.execute(request));
    
    std::ifstream file(file_name, std::ios::binary | std::ios::ate);
    size_t fsize = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<uint8_t> bytes(fsize);
    file.read((char*)bytes.data(), bytes.size());

    altintegration::TestCase vbtc_state = altintegration::TestCase::fromRaw(bytes);

    BOOST_CHECK_EQUAL(vbtc_state.alt_tree.size(), 104);

    altintegration::AltBlock tip = VeriBlock::blockToAltBlock(*ChainActive().Tip());
    BOOST_CHECK(tip == vbtc_state.alt_tree.back().first);

    file.close();
}

BOOST_AUTO_TEST_SUITE_END()
