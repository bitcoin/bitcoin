// Copyright (c) 2019-2020 Xenios SEZC
// https://www.veriblock.org
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_SRC_VBK_GENESIS_COMMON_HPP
#define BITCOIN_SRC_VBK_GENESIS_COMMON_HPP

#include <primitives/block.h>
#include <script/script.h>

namespace VeriBlock {

CScript ScriptWithPrefix(uint32_t nBits);

CBlock CreateGenesisBlock(
    std::string pszTimestamp,
    const CScript& genesisOutputScript,
    uint32_t nTime,
    uint32_t nNonce,
    uint32_t nBits,
    int32_t nVersion,
    const CAmount& genesisReward);

CBlock CreateGenesisBlock(
    uint32_t nTime,
    uint32_t nNonce,
    uint32_t nBits,
    int32_t nVersion,
    const CAmount& genesisReward,
    const std::string& initialPubkeyHex,
    const std::string& pszTimestamp);

} // namespace VeriBlock

#endif //BITCOIN_SRC_VBK_GENESIS_COMMON_HPP
