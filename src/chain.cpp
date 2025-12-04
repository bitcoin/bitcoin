// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chain.h>

#include <tinyformat.h>
#include <util/check.h>

#include <algorithm>
#include <vector>

std::string CBlockIndex::ToString() const
{
    return strprintf("CBlockIndex(pprev=%p, nHeight=%d, merkle=%s, hashBlock=%s)",
                     pprev, nHeight, hashMerkleRoot.ToString(), GetBlockHash().ToString());
}

void CChain::SetTip(CBlockIndex& block)
{
    CBlockIndex* old_tip = Tip();
    int old_height = old_tip ? old_tip->nHeight : -1;
    bool is_sequential = (block.nHeight == old_height + 1 && block.pprev == old_tip);

    auto& impl = m_impl.write();

    if (!is_sequential) {
        HandleReorg(impl, block);
        return;
    }

    if (impl.tail.size() + 1 >= MAX_TAIL_SIZE) {
        MergeTailIntoBase(impl, block);
        return;
    }

    AppendToTail(impl, block);
}

std::vector<uint256> LocatorEntries(const CBlockIndex* index)
{
    int step = 1;
    std::vector<uint256> have;
    if (index == nullptr) return have;

    have.reserve(32);
    while (index) {
        have.emplace_back(index->GetBlockHash());
        if (index->nHeight == 0) break;
        // Exponentially larger steps back, plus the genesis block.
        int height = std::max(index->nHeight - step, 0);
        // Use skiplist.
        index = index->GetAncestor(height);
        if (have.size() > 10) step *= 2;
    }
    return have;
}

CBlockLocator GetLocator(const CBlockIndex* index)
{
    return CBlockLocator{LocatorEntries(index)};
}

const CBlockIndex* CChain::FindFork(const CBlockIndex* index) const
{
    if (index == nullptr) {
        return nullptr;
    }
    const auto& impl = m_impl.read();
    const auto& base = impl.base.read();
    const auto& tail = impl.tail;

    int height = int(base.size() + tail.size()) - 1;

    if (index->nHeight > height)
        index = index->GetAncestor(height);

    while (index) {
        CBlockIndex* chain_block = nullptr;
        if (index->nHeight < (int)base.size()) {
            chain_block = base[index->nHeight];
        } else {
            size_t tail_idx = index->nHeight - base.size();
            if (tail_idx < tail.size()) {
                chain_block = tail[tail_idx];
            }
        }

        if (chain_block == index) return index;
        index = index->pprev;
    }

    return nullptr;
}

CBlockIndex* CChain::FindEarliestAtLeast(int64_t nTime, int height) const
{
    std::pair<int64_t, int> blockparams = std::make_pair(nTime, height);

    const auto& impl = m_impl.read();
    const auto& base = impl.base.read();
    const auto& tail = impl.tail;

    auto lower_base = std::lower_bound(base.begin(), base.end(), blockparams,
                                       [](CBlockIndex* block, const std::pair<int64_t, int>& blockparams) -> bool {
                                           return block->GetBlockTimeMax() < blockparams.first || block->nHeight < blockparams.second;
                                       });

    if (lower_base != base.end()) {
        return *lower_base;
    }

    auto lower_tail = std::lower_bound(tail.begin(), tail.end(), blockparams,
                                       [](CBlockIndex* block, const std::pair<int64_t, int>& blockparams) -> bool {
                                           return block->GetBlockTimeMax() < blockparams.first || block->nHeight < blockparams.second;
                                       });

    return (lower_tail == tail.end() ? nullptr : *lower_tail);
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

void CBlockIndex::BuildSkip()
{
    if (pprev)
        pskip = pprev->GetAncestor(GetSkipHeight(nHeight));
}

arith_uint256 GetBitsProof(uint32_t bits)
{
    arith_uint256 bnTarget;
    bool fNegative;
    bool fOverflow;
    bnTarget.SetCompact(bits, &fNegative, &fOverflow);
    if (fNegative || fOverflow || bnTarget == 0)
        return 0;
    // We need to compute 2**256 / (bnTarget+1), but we can't represent 2**256
    // as it's too large for an arith_uint256. However, as 2**256 is at least as large
    // as bnTarget+1, it is equal to ((2**256 - bnTarget - 1) / (bnTarget+1)) + 1,
    // or ~bnTarget / (bnTarget+1) + 1.
    return (~bnTarget / (bnTarget + 1)) + 1;
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
    r = r * arith_uint256(params.nPowTargetSpacing) / GetBlockProof(tip);
    if (r.bits() > 63) {
        return sign * std::numeric_limits<int64_t>::max();
    }
    return sign * int64_t(r.GetLow64());
}

/** Find the last common ancestor two blocks have.
 *  Both pa and pb must be non-nullptr. */
const CBlockIndex* LastCommonAncestor(const CBlockIndex* pa, const CBlockIndex* pb) {
    // First rewind to the last common height (the forking point cannot be past one of the two).
    if (pa->nHeight > pb->nHeight) {
        pa = pa->GetAncestor(pb->nHeight);
    } else if (pb->nHeight > pa->nHeight) {
        pb = pb->GetAncestor(pa->nHeight);
    }
    while (pa != pb) {
        // Jump back until pa and pb have a common "skip" ancestor.
        while (pa->pskip != pb->pskip) {
            // This logic relies on the property that equal-height blocks have equal-height skip
            // pointers.
            Assume(pa->nHeight == pb->nHeight);
            Assume(pa->pskip->nHeight == pb->pskip->nHeight);
            pa = pa->pskip;
            pb = pb->pskip;
        }
        // At this point, pa and pb are different, but have equal pskip. The forking point lies in
        // between pa/pb on the one end, and pa->pskip/pb->pskip on the other end.
        pa = pa->pprev;
        pb = pb->pprev;
    }
    return pa;
}
