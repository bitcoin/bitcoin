// Copyright (c) 2019-2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <signet.h>

#include <consensus/merkle.h>
#include <consensus/params.h>
#include <hash.h>
#include <primitives/block.h>
#include <script/interpreter.h>
#include <script/standard.h>        // MANDATORY_SCRIPT_VERIFY_FLAGS
#include <util/system.h>

CScript g_signet_blockscript;

// Signet block solution checker
bool CheckBlockSolution(const CBlock& block, const Consensus::Params& consensusParams)
{
    std::vector<uint8_t> signet_data;
    if (!block.GetWitnessCommitmentSection(SIGNET_HEADER, signet_data)) {
        return error("CheckBlockSolution: Errors in block (block solution missing)");
    }
    if (!CheckBlockSolution(GetSignetHash(block), signet_data, consensusParams)) {
        return error("CheckBlockSolution: Errors in block (block solution invalid)");
    }
    return true;
}

bool CheckBlockSolution(const uint256& signet_hash, const std::vector<uint8_t>& signature, const Consensus::Params& params)
{
    SimpleSignatureChecker bsc(signet_hash);
    CScript solution = CScript(signature.begin(), signature.end());
    return VerifyScript(solution, g_signet_blockscript, nullptr, MANDATORY_SCRIPT_VERIFY_FLAGS, bsc);
}

uint256 BlockSignetMerkleRoot(const CBlock& block, bool* mutated = nullptr)
{
    std::vector<uint256> leaves;
    leaves.resize(block.vtx.size());
    {
        // find and delete signet signature
        CMutableTransaction mtx(*block.vtx.at(0));
        CBlock::SetWitnessCommitmentSection(mtx, SIGNET_HEADER, std::vector<uint8_t>{});
        leaves[0] = mtx.GetHash();
    }
    for (size_t s = 1; s < block.vtx.size(); s++) {
        leaves[s] = block.vtx[s]->GetHash();
    }
    return ComputeMerkleRoot(std::move(leaves), mutated);
}

uint256 GetSignetHash(const CBlock& block)
{
    if (block.vtx.size() == 0) return block.GetHash();
    return (CHashWriter(SER_DISK, PROTOCOL_VERSION) << block.nVersion << block.hashPrevBlock << BlockSignetMerkleRoot(block) << block.nTime << block.nBits).GetHash();
}
