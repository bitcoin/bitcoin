// Copyright (c) 2019-2020 Xenios SEZC
// https://www.veriblock.org
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <pow.h>
#include <util/strencodings.h>
#include <vbk/merkle.hpp>
#include "genesis.hpp"

namespace VeriBlock {

CBlock MineGenesisBlock(
    uint32_t nTime,
    const std::string& pszTimestamp,
    const std::string& initialPubkeyHex,
    uint32_t nBits,
    uint32_t nVersion,
    uint32_t nNonce, // starting nonce
    uint64_t genesisReward)
{
    CBlock genesis = CreateGenesisBlock(nTime, nNonce, nBits, nVersion, genesisReward, initialPubkeyHex, pszTimestamp);

    printf("started genesis block mining...\n");
    while (!CheckProofOfWork(genesis.GetHash(), genesis.nBits, Params().GetConsensus())) {
        ++genesis.nNonce;
        if (genesis.nNonce > 4294967294LL) {
            ++genesis.nTime;
            genesis.nNonce = 0;
            printf("nonce reset... nTime=%d\n", genesis.nTime);
        }
    }

    assert(CheckProofOfWork(genesis.GetHash(), genesis.nBits, Params().GetConsensus()));

    return genesis;
}

} // namespace VeriBlock
