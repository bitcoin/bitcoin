// Copyright (c) 2019-2020 Xenios SEZC
// https://www.veriblock.org
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_SRC_VBK_POP_COMMON_HPP
#define BITCOIN_SRC_VBK_POP_COMMON_HPP

#include <uint256.h>
#include <veriblock/pop.hpp>


class CBlockIndex;

namespace VeriBlock {

altintegration::PopContext& GetPop();

void StopPop();

void SetPopConfig(const altintegration::Config& config);

void SetPop(std::shared_ptr<altintegration::PayloadsStorage> payloads_provider, std::shared_ptr<altintegration::BlockReader> block_provider);

altintegration::BlockIndex<altintegration::AltBlock>* GetAltBlockIndex(const uint256& hash);
altintegration::BlockIndex<altintegration::AltBlock>* GetAltBlockIndex(const CBlockIndex* index);

bool isKeystone(const CBlockIndex& block);

std::string toPrettyString(const altintegration::PopContext& pop);

} // namespace VeriBlock

#endif //BITCOIN_SRC_VBK_POP_COMMON_HPP
