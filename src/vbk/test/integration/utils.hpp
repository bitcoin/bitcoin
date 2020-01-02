#ifndef VERIBLOCK_INTEGRATION_TEST_UTILS_H
#define VERIBLOCK_INTEGRATION_TEST_UTILS_H

#include <vbk/test/integration/grpc_integration_service.hpp>

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
