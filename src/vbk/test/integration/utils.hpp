// Copyright (c) 2019-2020 Xenios SEZC
// https://www.veriblock.org
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef VERIBLOCK_INTEGRATION_TEST_UTILS_H
#define VERIBLOCK_INTEGRATION_TEST_UTILS_H

#include <primitives/block.h>
#include <script/script.h>

#include <vector>

namespace VeriBlockTest {

void mineVeriBlockBlocks(VeriBlock::VeriBlockBlock& block);
void mineBitcoinBlock(VeriBlock::BitcoinBlock& block);
void setUpGenesisBlocks();

VeriBlock::AltPublication generateSignedAltPublication(const CBlock& endorsedBlock, const uint32_t& identifier, const CScript& payoutInfo);
} // namespace VeriBlockTest

#endif
