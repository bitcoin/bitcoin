// Copyright (c) 2019-2020 Xenios SEZC
// https://www.veriblock.org
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_SRC_VBK_MERKLE_HPP
#define BITCOIN_SRC_VBK_MERKLE_HPP

#include <iostream>

#include <chain.h>
#include <chainparams.h>
#include <consensus/validation.h>
#include <primitives/transaction.h>

namespace VeriBlock {

int GetPopMerkleRootCommitmentIndex(const CBlock& block);

uint256 BlockPopDataMerkleRoot(const CBlock& block);

uint256 makeTopLevelRoot(int height, const KeystoneArray& keystones, const uint256& txRoot);

const CBlockIndex* getPreviousKeystone(const CBlockIndex& block);

KeystoneArray getKeystoneHashesForTheNextBlock(const CBlockIndex* pindexPrev);

uint256 TopLevelMerkleRoot(const CBlockIndex* prevIndex, const CBlock& block, bool* mutated = nullptr);

bool VerifyTopLevelMerkleRoot(const CBlock& block, const CBlockIndex* pprevIndex, BlockValidationState& state);

CTxOut addPopDataRootIntoCoinbaseCommitment(const CBlock& block);

bool isKeystone(const CBlockIndex& block);

} // namespace VeriBlock

#endif //BITCOIN_SRC_VBK_MERKLE_HPP
