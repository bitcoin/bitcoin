// Copyright (c) 2017-2017 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "chainparams.h"
#include "keystore.h"
#include "pow.h"
#include "primitives/block.h"

#include <boost/test/unit_test.hpp>

void SingleBlockSignerTest(const Consensus::Params& params, unsigned int flags, CBlockHeader& block)
{
    ResetProof(params, &block);
}

BOOST_AUTO_TEST_CASE(BlockSignBasicTests)
{
    ECC_Start();

    const std::unique_ptr<CChainParams> testParams = CreateChainParams(CBaseChainParams::CUSTOM);
    const Consensus::Params& conParams = testParams->GetConsensus();
    unsigned int flags = SCRIPT_VERIFY_NONE;
    CBlockHeader header;
    CBlock block;

    SingleBlockSignerTest(conParams, flags, header);
    SingleBlockSignerTest(conParams, flags, block);

    ECC_Stop();
}
