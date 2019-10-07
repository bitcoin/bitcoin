// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2017 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chain.h>
#include <crypto/shabal256.h>
#include <poc/poc.h>
#include <script/standard.h>
#include <util.h>

/**
 * CChain implementation
 */
void CChain::SetTip(CBlockIndex *pindex) {
    if (pindex == nullptr) {
        vChain.clear();
        return;
    }
    vChain.resize(pindex->nHeight + 1);
    while (pindex && vChain[pindex->nHeight] != pindex) {
        vChain[pindex->nHeight] = pindex;
        pindex = pindex->pprev;
    }
}

CBlockLocator CChain::GetLocator(const CBlockIndex *pindex, int lastCheckpointHeight) const {
    std::vector<uint256> vHave;
    vHave.reserve(32);

    int nStep = 1;
    if (!pindex)
        pindex = Tip();
    if (pindex && lastCheckpointHeight > pindex->nHeight + 1000) {
        // Always use checkpoint
        int nStep = 1000;
        if (Contains(pindex)) {
            // O(1)
            pindex = (*this)[std::max(1000 * (pindex->nHeight / 1000), 0)];
        } else {
            // O(log n)
            pindex = pindex->GetAncestor(std::max(1000 * (pindex->nHeight / 1000), 0));
        }
        LogPrint(BCLog::NET, "GetLocator(by checkpoints):\r\n");
        while (pindex) {
            vHave.push_back(pindex->GetBlockHash());
            LogPrint(BCLog::NET, "\t%6d: %s\r\n", pindex->nHeight, pindex->GetBlockHash().ToString());
            // Stop when we have added the genesis block.
            if (pindex->nHeight == 0)
                break;
            // Exponentially larger steps back, plus the genesis block.
            int nHeight = std::max(1000 * ((pindex->nHeight - nStep) / 1000), 0);
            if (Contains(pindex)) {
                // Use O(1) CChain index if possible.
                pindex = (*this)[nHeight];
            } else {
                // Otherwise, use O(log n) skiplist.
                pindex = pindex->GetAncestor(nHeight);
            }
            if (vHave.size() > 10)
                nStep *= 2;
        }
    } else {
        LogPrint(BCLog::NET, "GetLocator:\r\n");
        while (pindex) {
            vHave.push_back(pindex->GetBlockHash());
            LogPrint(BCLog::NET, "\t%6d: %s\r\n", pindex->nHeight, pindex->GetBlockHash().ToString());
            // Stop when we have added the genesis block.
            if (pindex->nHeight == 0)
                break;
            // Exponentially larger steps back, plus the genesis block.
            int nHeight = std::max(pindex->nHeight - nStep, 0);
            if (Contains(pindex)) {
                // Use O(1) CChain index if possible.
                pindex = (*this)[nHeight];
            } else {
                // Otherwise, use O(log n) skiplist.
                pindex = pindex->GetAncestor(nHeight);
            }
            if (vHave.size() > 10)
                nStep *= 2;
        }
    }

    return CBlockLocator(vHave);
}

const CBlockIndex *CChain::FindFork(const CBlockIndex *pindex) const {
    if (pindex == nullptr) {
        return nullptr;
    }
    if (pindex->nHeight > Height())
        pindex = pindex->GetAncestor(Height());
    while (pindex && !Contains(pindex))
        pindex = pindex->pprev;
    return pindex;
}

CBlockIndex* CChain::FindEarliestAtLeast(int64_t nTime) const
{
    std::vector<CBlockIndex*>::const_iterator lower = std::lower_bound(vChain.begin(), vChain.end(), nTime,
        [](CBlockIndex* pBlock, const int64_t& time) -> bool { return pBlock->GetBlockTimeMax() < time; });
    return (lower == vChain.end() ? nullptr : *lower);
}

/** Turn the lowest '1' bit in the binary representation of a number into a '0'. */
int static inline InvertLowestOne(int n) { return n & (n - 1); }

/** Compute what height to jump back to with the CBlockIndex::pskip pointer. */
int static inline GetSkipHeight(int height) {
    if (height < 2)
        return 0;

    // Determine which height to jump back to. Any number strictly lower than height is acceptable,
    // but the following expression seems to perform well in simulations (max 110 steps to go back
    // up to 2**18 blocks).
    return (height & 1) ? InvertLowestOne(InvertLowestOne(height - 1)) + 1 : InvertLowestOne(height);
}

const CBlockIndex* CBlockIndex::GetAncestor(int height) const
{
    if (height > nHeight || height < 0) {
        return nullptr;
    }

    const CBlockIndex* pindexWalk = this;
    int heightWalk = nHeight;
    while (heightWalk > height) {
        int heightSkip = GetSkipHeight(heightWalk);
        int heightSkipPrev = GetSkipHeight(heightWalk - 1);
        if (pindexWalk->pskip != nullptr &&
            (heightSkip == height ||
             (heightSkip > height && !(heightSkipPrev < heightSkip - 2 &&
                                       heightSkipPrev >= height)))) {
            // Only follow pskip if pprev->pskip isn't better than pskip->pprev.
            pindexWalk = pindexWalk->pskip;
            heightWalk = heightSkip;
        } else {
            assert(pindexWalk->pprev);
            pindexWalk = pindexWalk->pprev;
            heightWalk--;
        }
    }
    return pindexWalk;
}

CBlockIndex* CBlockIndex::GetAncestor(int height)
{
    return const_cast<CBlockIndex*>(static_cast<const CBlockIndex*>(this)->GetAncestor(height));
}

void CBlockIndex::Update(const Consensus::Params& params)
{
    // Genearation signature
    static uint256 dummyGenerationSignature;
    generationSignature = pprev ? &pprev->nextGenerationSignature : &dummyGenerationSignature;
    if (nHeight + 1 <= params.BHDIP001PreMiningEndHeight) {
        //! Pre-Mining not exist generation signature
        nextGenerationSignature.SetNull();
    } else if (nHeight + 1 <= params.BHDIP007Height) {
        //! hashMerkleRoot + nPlotterId. Unsafe
        // Legacy consensus use little endian
        uint64_t plotterId = htole64(nPlotterId);
        CShabal256()
            .Write(hashMerkleRoot.begin(), hashMerkleRoot.size())
            .Write((const unsigned char*)&plotterId, 8)
            .Finalize(nextGenerationSignature.begin());
    } else {
        //! generationSignature + nPlotterId
        assert(generationSignature != nullptr && !generationSignature->IsNull());
        uint64_t plotterId = htobe64(nPlotterId);
        CShabal256()
            .Write(generationSignature->begin(), generationSignature->size())
            .Write((const unsigned char*)&plotterId, 8)
            .Finalize(nextGenerationSignature.begin());
    }

    // Generator
    if (!vchPubKey.empty())
        generatorAccountID = ExtractAccountID(CPubKey(vchPubKey));
}

void CBlockIndex::BuildSkip()
{
    if (pprev)
        pskip = pprev->GetAncestor(GetSkipHeight(nHeight));
}

arith_uint256 GetBlockProof(const CBlockHeader& header, const Consensus::Params& params)
{
    //! Same nBaseTarget select biggest hash
    return (poc::TWO64 / header.nBaseTarget) * 100 + (header.vchSignature.empty() ? 0 : header.vchSignature.back()) % 100;
}

arith_uint256 GetBlockProof(const CBlockIndex& block, const Consensus::Params& params)
{
    //! Same nBaseTarget select biggest hash
    return (poc::TWO64 / block.nBaseTarget) * 100 + (block.vchSignature.empty() ? 0 : block.vchSignature.back()) % 100;
}

int64_t GetBlockProofEquivalentTime(const CBlockIndex& to, const CBlockIndex& from, const CBlockIndex& tip, const Consensus::Params& params)
{
    arith_uint256 r;
    int sign = 1;
    if (to.nChainWork > from.nChainWork) {
        r = to.nChainWork - from.nChainWork;
    } else {
        r = from.nChainWork - to.nChainWork;
        sign = -1;
    }
    r = r * arith_uint256(Consensus::GetTargetSpacing(tip.nHeight, params)) / GetBlockProof(tip, params);
    if (r.bits() > 63) {
        return sign * std::numeric_limits<int64_t>::max();
    }
    return sign * r.GetLow64();
}

/** Find the last common ancestor two blocks have.
 *  Both pa and pb must be non-nullptr. */
const CBlockIndex* LastCommonAncestor(const CBlockIndex* pa, const CBlockIndex* pb) {
    if (pa->nHeight > pb->nHeight) {
        pa = pa->GetAncestor(pb->nHeight);
    } else if (pb->nHeight > pa->nHeight) {
        pb = pb->GetAncestor(pa->nHeight);
    }

    while (pa != pb && pa && pb) {
        pa = pa->pprev;
        pb = pb->pprev;
    }

    // Eventually all chain branches meet at the genesis block.
    assert(pa == pb);
    return pa;
}
