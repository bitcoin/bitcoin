// Copyright (c) 2017-2024 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <evo/simplifiedmns.h>

#include <clientversion.h>
#include <consensus/merkle.h>
#include <evo/netinfo.h>
#include <key_io.h>
#include <logging.h>
#include <pubkey.h>
#include <serialize.h>
#include <sync.h>
#include <univalue.h>
#include <util/time.h>
#include <util/underlying.h>
#include <version.h>

CSimplifiedMNListEntry::CSimplifiedMNListEntry(const uint256& proreg_tx_hash, const uint256& confirmed_hash,
                                               const std::shared_ptr<NetInfoInterface>& net_info,
                                               const CBLSLazyPublicKey& pubkey_operator, const CKeyID& keyid_voting,
                                               bool is_valid, uint16_t platform_http_port,
                                               const uint160& platform_node_id, const CScript& script_payout,
                                               const CScript& script_operator_payout, uint16_t version, MnType type) :
    proRegTxHash(proreg_tx_hash),
    confirmedHash(confirmed_hash),
    netInfo(net_info),
    pubKeyOperator(pubkey_operator),
    keyIDVoting(keyid_voting),
    isValid(is_valid),
    platformHTTPPort(platform_http_port),
    platformNodeID(platform_node_id),
    scriptPayout(script_payout),
    scriptOperatorPayout(script_operator_payout),
    nVersion(version),
    nType(type)
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

CSimplifiedMNList::CSimplifiedMNList(std::vector<std::unique_ptr<CSimplifiedMNListEntry>>&& smlEntries)
{
    mnList = std::move(smlEntries);

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

bool CalcCbTxMerkleRootMNList(uint256& merkleRootRet, gsl::not_null<std::shared_ptr<const CSimplifiedMNList>> sml,
                              BlockValidationState& state)
{
    try {
        static std::atomic<int64_t> nTimeMerkle = 0;

        int64_t nTime1 = GetTimeMicros();

        static Mutex cached_mutex;
        static gsl::not_null<std::shared_ptr<const CSimplifiedMNList>> cached_sml GUARDED_BY(cached_mutex){
            std::make_shared<const CSimplifiedMNList>()};
        static uint256 merkleRootCached GUARDED_BY(cached_mutex);
        static bool mutatedCached GUARDED_BY(cached_mutex){false};

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
