// Copyright (c) 2017-2024 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <evo/simplifiedmns.h>

#include <consensus/merkle.h>
#include <evo/deterministicmns.h>
#include <evo/netinfo.h>
#include <key_io.h>
#include <logging.h>
#include <pubkey.h>
#include <serialize.h>
#include <univalue.h>
#include <util/underlying.h>
#include <version.h>

CSimplifiedMNListEntry::CSimplifiedMNListEntry(const CDeterministicMN& dmn) :
    proRegTxHash(dmn.proTxHash),
    confirmedHash(dmn.pdmnState->confirmedHash),
    netInfo(dmn.pdmnState->netInfo),
    pubKeyOperator(dmn.pdmnState->pubKeyOperator),
    keyIDVoting(dmn.pdmnState->keyIDVoting),
    isValid(!dmn.pdmnState->IsBanned()),
    platformHTTPPort(dmn.pdmnState->platformHTTPPort),
    platformNodeID(dmn.pdmnState->platformNodeID),
    scriptPayout(dmn.pdmnState->scriptPayout),
    scriptOperatorPayout(dmn.pdmnState->scriptOperatorPayout),
    nVersion(dmn.pdmnState->nVersion),
    nType(dmn.nType)
{
}

uint256 CSimplifiedMNListEntry::CalcHash() const
{
    CHashWriter hw(SER_GETHASH, CLIENT_VERSION);
    hw << *this;
    return hw.GetHash();
}

std::string CSimplifiedMNListEntry::ToString() const
{
    CTxDestination dest;
    std::string payoutAddress = "unknown";
    std::string operatorPayoutAddress = "none";
    if (ExtractDestination(scriptPayout, dest)) {
        payoutAddress = EncodeDestination(dest);
    }
    if (ExtractDestination(scriptOperatorPayout, dest)) {
        operatorPayoutAddress = EncodeDestination(dest);
    }

    return strprintf("CSimplifiedMNListEntry(nVersion=%d, nType=%d, proRegTxHash=%s, confirmedHash=%s, "
                     "pubKeyOperator=%s, votingAddress=%s, isValid=%d, payoutAddress=%s, operatorPayoutAddress=%s, "
                     "platformHTTPPort=%d, platformNodeID=%s)\n"
                     "  %s",
                     nVersion, ToUnderlying(nType), proRegTxHash.ToString(), confirmedHash.ToString(),
                     pubKeyOperator.ToString(), EncodeDestination(PKHash(keyIDVoting)), isValid, payoutAddress,
                     operatorPayoutAddress, platformHTTPPort, platformNodeID.ToString(), netInfo->ToString());
}

CSimplifiedMNList::CSimplifiedMNList(const std::vector<CSimplifiedMNListEntry>& smlEntries)
{
    mnList.reserve(smlEntries.size());
    for (const auto& entry : smlEntries) {
        mnList.emplace_back(std::make_unique<CSimplifiedMNListEntry>(entry));
    }

    std::sort(mnList.begin(), mnList.end(), [&](const std::unique_ptr<CSimplifiedMNListEntry>& a, const std::unique_ptr<CSimplifiedMNListEntry>& b) {
        return a->proRegTxHash.Compare(b->proRegTxHash) < 0;
    });
}

CSimplifiedMNList::CSimplifiedMNList(const CDeterministicMNList& dmnList)
{
    mnList.reserve(dmnList.GetAllMNsCount());
    dmnList.ForEachMN(false, [this](auto& dmn) {
        mnList.emplace_back(std::make_unique<CSimplifiedMNListEntry>(dmn));
    });

    std::sort(mnList.begin(), mnList.end(), [&](const std::unique_ptr<CSimplifiedMNListEntry>& a, const std::unique_ptr<CSimplifiedMNListEntry>& b) {
        return a->proRegTxHash.Compare(b->proRegTxHash) < 0;
    });
}

uint256 CSimplifiedMNList::CalcMerkleRoot(bool* pmutated) const
{
    std::vector<uint256> leaves;
    leaves.reserve(mnList.size());
    for (const auto& e : mnList) {
        leaves.emplace_back(e->CalcHash());
    }
    return ComputeMerkleRoot(leaves, pmutated);
}

bool CSimplifiedMNList::operator==(const CSimplifiedMNList& rhs) const
{
    return mnList.size() == rhs.mnList.size() &&
            std::equal(mnList.begin(), mnList.end(), rhs.mnList.begin(),
                [](const std::unique_ptr<CSimplifiedMNListEntry>& left, const std::unique_ptr<CSimplifiedMNListEntry>& right)
                {
                    return *left == *right;
                }
            );
}

bool CalcCbTxMerkleRootMNList(uint256& merkleRootRet, const CDeterministicMNList& mn_list, BlockValidationState& state)
{
    try {
        static std::atomic<int64_t> nTimeMerkle = 0;

        int64_t nTime1 = GetTimeMicros();

        static Mutex cached_mutex;
        static std::shared_ptr<const CSimplifiedMNList> cached_sml GUARDED_BY(cached_mutex){
            std::make_shared<const CSimplifiedMNList>()};
        static uint256 merkleRootCached GUARDED_BY(cached_mutex);
        static bool mutatedCached GUARDED_BY(cached_mutex){false};

        std::shared_ptr<const CSimplifiedMNList> sml{mn_list.GetSML()};
        LOCK(cached_mutex);
        if (sml == cached_sml || *sml == *cached_sml) {
            merkleRootRet = merkleRootCached;
            if (mutatedCached) {
                return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "mutated-cached-calc-cb-mnmerkleroot");
            }
            return true;
        }

        bool mutated = false;
        merkleRootRet = sml->CalcMerkleRoot(&mutated);

        int64_t nTime2 = GetTimeMicros();
        nTimeMerkle += nTime2 - nTime1;
        LogPrint(BCLog::BENCHMARK, "            - CalcMerkleRoot: %.2fms [%.2fs]\n", 0.001 * (nTime2 - nTime1),
                 nTimeMerkle * 0.000001);

        cached_sml = sml;
        merkleRootCached = merkleRootRet;
        mutatedCached = mutated;

        if (mutated) {
            return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "mutated-calc-cb-mnmerkleroot");
        }

        return true;
    } catch (const std::exception& e) {
        LogPrintf("%s -- failed: %s\n", __func__, e.what());
        return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "failed-calc-cb-mnmerkleroot");
    }
}
