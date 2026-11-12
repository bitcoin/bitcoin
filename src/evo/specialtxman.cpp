// Copyright (c) 2018-2025 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <evo/specialtxman.h>

#include <chainlock/chainlock.h>
#include <chainlock/clsig.h>
#include <evo/assetlocktx.h>
#include <evo/cbtx.h>
#include <evo/creditpool.h>
#include <evo/deterministicmns.h>
#include <evo/evodb.h>
#include <evo/mnhftx.h>
#include <evo/netinfo.h>
#include <evo/sharedcollateral.h>
#include <evo/simplifiedmns.h>
#include <llmq/blockprocessor.h>
#include <llmq/commitment.h>
#include <llmq/quorumsman.h>
#include <llmq/utils.h>
#include <messagesigner.h>

#include <chainparams.h>
#include <coins.h>
#include <consensus/amount.h>
#include <consensus/validation.h>
#include <deploymentstatus.h>
#include <hash.h>
#include <primitives/block.h>
#include <util/system.h>

static bool AddNetInfoEntries(const std::shared_ptr<NetInfoInterface>& net_info, NetInfoPurpose purpose,
                              const NetInfoList& entries, BlockValidationState& state)
{
    for (const auto& entry : entries) {
        if (const auto ret{net_info->AddEntry(purpose, entry.ToStringAddrPort())}; ret != NetInfoStatus::Success) {
            return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "bad-protx-netinfo-version");
        }
    }
    return true;
}

// Raising a masternode's state version out of the legacy BLS scheme re-encodes its operator key and
// moves it to a new scheme-dependent unique-property slot; the collision guards key off this.
static bool IsSchemeMigration(int old_version, int new_version)
{
    return old_version == ProTxVersion::LegacyBLS && new_version > ProTxVersion::LegacyBLS;
}

static bool SetStateVersion(CDeterministicMNState& state_mn, uint16_t nVersion, MnType nType,
                            BlockValidationState& state)
{
    const bool needs_extended = nVersion >= ProTxVersion::ExtAddr;
    // Before the early-return: ProUpServ pre-sets nVersion, so migrate scriptPayout into the ExtAddr
    // payout list here or it would be skipped.
    if (needs_extended && state_mn.payouts.empty() && !state_mn.scriptPayout.empty()) {
        state_mn.payouts = LegacyPayoutAsList(state_mn.scriptPayout);
        state_mn.scriptPayout.clear();
    }

    // Keep the operator key's BLS encoding a deterministic function of nVersion, matching the SML and
    // on-disk serialization, so the stored key and the (scheme-dependent) unique-property index use
    // the same scheme on every node whether the list was built online or reloaded from a snapshot.
    // Set() rather than SetLegacy(): the latter only flips the flag and leaves the cached
    // serialization in the old encoding, so a reloaded node would decode a different key. This runs
    // before the early return because callers pre-set nVersion, so the version may already match here
    // while the key still needs re-encoding.
    if (state_mn.pubKeyOperator != CBLSLazyPublicKey()) {
        const CBLSPublicKey& pubkey{state_mn.pubKeyOperator.Get()};
        state_mn.pubKeyOperator.Set(pubkey, nVersion == ProTxVersion::LegacyBLS);
    }

    if (state_mn.nVersion == nVersion && state_mn.netInfo->CanStorePlatform() == needs_extended) {
        return true;
    }

    auto converted_netinfo{NetInfoInterface::MakeNetInfo(nVersion)};
    if (needs_extended) {
        if (!AddNetInfoEntries(converted_netinfo, NetInfoPurpose::CORE_P2P,
                               state_mn.netInfo->GetEntries(NetInfoPurpose::CORE_P2P), state)) {
            return false;
        }
        if (state_mn.netInfo->CanStorePlatform()) {
            if (!AddNetInfoEntries(converted_netinfo, NetInfoPurpose::PLATFORM_P2P,
                                   state_mn.netInfo->GetEntries(NetInfoPurpose::PLATFORM_P2P), state) ||
                !AddNetInfoEntries(converted_netinfo, NetInfoPurpose::PLATFORM_HTTPS,
                                   state_mn.netInfo->GetEntries(NetInfoPurpose::PLATFORM_HTTPS), state)) {
                return false;
            }
        } else if (nType == MnType::Evo && !state_mn.netInfo->IsEmpty()) {
            // The scalar Platform ports are not carried over: an EvoNode is raised to ExtAddr by a
            // ProUpServTx that spells out its Platform entries
            return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "bad-protx-netinfo-version");
        }
        state_mn.platformP2PPort = 0;
        state_mn.platformHTTPPort = 0;
    } else if (!AddNetInfoEntries(converted_netinfo, NetInfoPurpose::CORE_P2P,
                                  state_mn.netInfo->GetEntries(NetInfoPurpose::CORE_P2P), state)) {
        return false;
    }

    state_mn.nVersion = nVersion;
    state_mn.netInfo = std::move(converted_netinfo);
    return true;
}

// Shared by BuildNewListFromBlock and CheckProUpServTx so that a service update whose state-version
// migration fails is rejected on entry to the mempool rather than aborting block assembly.
static bool ApplyProUpServTx(CDeterministicMNState& state_mn, const CProUpServTx& proTx, bool is_v24_active,
                             BlockValidationState& state)
{
    const uint16_t current_version{static_cast<uint16_t>(state_mn.nVersion)};
    const uint16_t target_version{is_v24_active ? std::max<uint16_t>(current_version, proTx.nVersion) : current_version};
    if (is_v24_active) {
        // Extended addresses support in v24 means that the version can be updated
        state_mn.nVersion = proTx.nVersion;
    }
    state_mn.netInfo = proTx.netInfo;
    state_mn.scriptOperatorPayout = proTx.scriptOperatorPayout;
    if (proTx.nType == MnType::Evo) {
        state_mn.platformNodeID = proTx.platformNodeID;
        if (proTx.nVersion < ProTxVersion::ExtAddr) {
            state_mn.platformP2PPort = proTx.platformP2PPort;
            state_mn.platformHTTPPort = proTx.platformHTTPPort;
        } else {
            // From ExtAddr onwards the Platform ports are stored in netInfo. Clear the
            // legacy scalar fields (which a legacy registration may have left set) so the
            // in-memory state matches its serialized form, which omits them for ExtAddr
            // (see CDeterministicMNState serialization). Otherwise a stale value would
            // survive in diff-reconstructed lists but vanish through a snapshot round-trip.
            state_mn.platformP2PPort = 0;
            state_mn.platformHTTPPort = 0;
        }
    }
    return !is_v24_active || SetStateVersion(state_mn, target_version, proTx.nType, state);
}

bool CheckCbTxBestChainlock(const CCbTx& cbTx, const CBlockIndex* pindex, const Consensus::Params& consensus_params,
                            const CChain& chain, const llmq::CQuorumManager& qman,
                            const chainlock::Chainlocks& chainlocks, BlockValidationState& state)
{
    if (cbTx.nVersion < CCbTx::Version::CLSIG_AND_BALANCE) {
        return true;
    }

    static Mutex cached_mutex;
    static const CBlockIndex* cached_pindex GUARDED_BY(cached_mutex){nullptr};
    static std::optional<std::pair<CBLSSignature, uint32_t>> cached_chainlock GUARDED_BY(cached_mutex){std::nullopt};

    auto best_clsig = chainlocks.GetBestChainLock();
    if (best_clsig.getHeight() == pindex->nHeight - 1 && cbTx.bestCLHeightDiff == 0 &&
        cbTx.bestCLSignature == best_clsig.getSig()) {
        // matches our best clsig which still hold values for the previous block
        LOCK(cached_mutex);
        cached_chainlock = std::make_pair(cbTx.bestCLSignature, cbTx.bestCLHeightDiff);
        cached_pindex = pindex;
        return true;
    }

    std::optional<std::pair<CBLSSignature, uint32_t>> prevBlockCoinbaseChainlock{std::nullopt};
    if (LOCK(cached_mutex); cached_pindex == pindex->pprev) {
        prevBlockCoinbaseChainlock = cached_chainlock;
    }
    if (!prevBlockCoinbaseChainlock.has_value()) {
        prevBlockCoinbaseChainlock = GetNonNullCoinbaseChainlock(pindex->pprev);
    }
    // If std::optional prevBlockCoinbaseChainlock is empty, then up to the previous block, coinbase Chainlock is null.
    if (prevBlockCoinbaseChainlock.has_value()) {
        // Previous block Coinbase has a non-null Chainlock: current block's Chainlock must be non-null and at least as new as the previous one
        if (!cbTx.bestCLSignature.IsValid()) {
            // IsNull() doesn't exist for CBLSSignature: we assume that a non valid BLS sig is null
            return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "bad-cbtx-null-clsig");
        }
        if (cbTx.bestCLHeightDiff > prevBlockCoinbaseChainlock.value().second + 1) {
            return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "bad-cbtx-older-clsig");
        }
    }

    // IsNull() doesn't exist for CBLSSignature: we assume that a valid BLS sig is non-null
    if (cbTx.bestCLSignature.IsValid()) {
        // Reject out-of-range bestCLHeightDiff that would yield a pre-genesis ancestor height.
        if (cbTx.bestCLHeightDiff >= static_cast<uint32_t>(pindex->nHeight)) {
            return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "bad-cbtx-cldiff");
        }
        int curBlockCoinbaseCLHeight = pindex->nHeight - static_cast<int>(cbTx.bestCLHeightDiff) - 1;
        if (best_clsig.getHeight() == curBlockCoinbaseCLHeight && best_clsig.getSig() == cbTx.bestCLSignature) {
            // matches our best (but outdated) clsig, no need to verify it again
            LOCK(cached_mutex);
            cached_chainlock = std::make_pair(cbTx.bestCLSignature, cbTx.bestCLHeightDiff);
            cached_pindex = pindex;
            return true;
        }
        const CBlockIndex* pAncestor = pindex->GetAncestor(curBlockCoinbaseCLHeight);
        if (pAncestor == nullptr) {
            // Defense-in-depth: the range check above keeps curBlockCoinbaseCLHeight in
            // [0, pindex->nHeight - 1], so GetAncestor() should never return nullptr here.
            return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "bad-cbtx-cldiff-ancestor");
        }
        uint256 curBlockCoinbaseCLBlockHash = pAncestor->GetBlockHash();
        chainlock::ChainLockSig clsig{curBlockCoinbaseCLHeight, curBlockCoinbaseCLBlockHash, cbTx.bestCLSignature};
        llmq::VerifyRecSigStatus ret = chainlock::VerifyChainLock(consensus_params, chain, qman, clsig);
        if (ret != llmq::VerifyRecSigStatus::Valid) {
            return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "bad-cbtx-invalid-clsig");
        }
        LOCK(cached_mutex);
        cached_chainlock = std::make_pair(cbTx.bestCLSignature, cbTx.bestCLHeightDiff);
        cached_pindex = pindex;
    } else if (cbTx.bestCLHeightDiff != 0) {
        // Null bestCLSignature is allowed only with bestCLHeightDiff = 0
        return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "bad-cbtx-cldiff");
    }

    return true;
}

bool CSpecialTxProcessor::CheckSpecialTxInner(const CChain* chain, const CTransaction& tx,
                                              const CBlockIndex* pindexPrev, SpecialTxRules rules,
                                              const CCoinsViewCache& view, const std::optional<CRangesSet>& indexes,
                                              bool check_sigs, TxValidationState& state)
{
    AssertLockHeld(::cs_main);

    if (!tx.HasExtraPayloadField())
        return true;

    if (!DeploymentActiveAfter(pindexPrev, m_consensus_params, Consensus::DEPLOYMENT_DIP0003)) {
        return state.Invalid(TxValidationResult::TX_BAD_SPECIAL, "bad-tx-type-dip3-inactive");
    }

    try {
        switch (tx.nType) {
        case TRANSACTION_PROVIDER_REGISTER:
            return CheckProRegTx(tx, pindexPrev, m_dmnman, view, m_consensus_params, rules, state, check_sigs);
        case TRANSACTION_PROVIDER_UPDATE_SERVICE:
            return CheckProUpServTx(tx, pindexPrev, m_dmnman, m_consensus_params, rules, state, check_sigs);
        case TRANSACTION_PROVIDER_UPDATE_REGISTRAR:
            return CheckProUpRegTx(tx, pindexPrev, m_dmnman, view, m_consensus_params, rules, state, check_sigs);
        case TRANSACTION_PROVIDER_UPDATE_REVOKE:
            return CheckProUpRevTx(tx, pindexPrev, m_dmnman, m_consensus_params, rules.v24, state, check_sigs);
        case TRANSACTION_PROVIDER_DISSOLVE:
            return CheckProDisTx(tx, pindexPrev, m_dmnman, m_consensus_params, rules.v24, state, check_sigs);
        case TRANSACTION_PROVIDER_UPDATE_SHARE:
            return CheckProUpShareTx(tx, pindexPrev, m_dmnman, m_consensus_params, rules.v24, state, check_sigs);
        case TRANSACTION_PROVIDER_UPDATE_SHARED_REGISTRAR:
            return CheckProUpSharedRegTx(tx, pindexPrev, m_dmnman, m_consensus_params, rules.v24, state, check_sigs);
        case TRANSACTION_COINBASE: {
            if (!tx.IsCoinBase()) {
                return state.Invalid(TxValidationResult::TX_CONSENSUS, "bad-cbtx-invalid");
            }
            if (const auto opt_cbTx = GetTxPayload<CCbTx>(tx)) {
                return CheckCbTx(*opt_cbTx, pindexPrev, rules.v24, state);
            } else {
                return state.Invalid(TxValidationResult::TX_CONSENSUS, "bad-cbtx-payload");
            }
        }
        case TRANSACTION_QUORUM_COMMITMENT:
            return llmq::CheckLLMQCommitment({m_dmnman, m_qsnapman, m_chainman, pindexPrev}, tx, state);
        case TRANSACTION_MNHF_SIGNAL:
            return chain ? CheckMNHFTx(m_blockman, m_qman, *chain, tx, pindexPrev, state) :
                           CheckMNHFTx(m_blockman, m_qman, tx, pindexPrev, state);
        case TRANSACTION_ASSET_LOCK:
            return CheckAssetLockTx(tx, state, rules.v24);
        case TRANSACTION_ASSET_UNLOCK:
            return chain ? CheckAssetUnlockTx(m_blockman, m_qman, *chain, tx, pindexPrev, indexes, rules.v24, state)
                         : CheckAssetUnlockTx(m_blockman, m_qman, tx, pindexPrev, indexes, rules.v24, state);
        }
    } catch (const std::exception& e) {
        LogPrintf("%s -- failed: %s\n", __func__, e.what());
        return state.Invalid(TxValidationResult::TX_CONSENSUS, "failed-check-special-tx");
    }

    return state.Invalid(TxValidationResult::TX_BAD_SPECIAL, "bad-tx-type-check");
}

bool CSpecialTxProcessor::CheckSpecialTx(const CTransaction& tx, const CBlockIndex* pindexPrev, SpecialTxRules rules,
                                         const CCoinsViewCache& view, bool check_sigs, TxValidationState& state)
{
    AssertLockHeld(::cs_main);
    return CheckSpecialTxInner(nullptr, tx, pindexPrev, rules, view, std::nullopt, check_sigs, state);
}

static void HandleQuorumCommitment(const llmq::CFinalCommitment& qc, const std::vector<CDeterministicMNCPtr>& members,
                                   bool debugLogs, CDeterministicMNList& mnList)
{
    for (size_t i = 0; i < members.size(); i++) {
        if (!mnList.HasMN(members[i]->proTxHash)) {
            continue;
        }
        if (!qc.validMembers[i]) {
            // punish MN for failed DKG participation
            // The idea is to immediately ban a MN when it fails 2 DKG sessions with only a few blocks in-between
            // If there were enough blocks between failures, the MN has a chance to recover as he reduces his penalty by 1 for every block
            // If it however fails 3 times in the timespan of a single payment cycle, it should definitely get banned
            mnList.PoSePunish(members[i]->proTxHash, mnList.CalcPenalty(66), debugLogs);
        }
    }
}

bool CSpecialTxProcessor::BuildNewListFromBlock(const CBlock& block, gsl::not_null<const CBlockIndex*> pindexPrev,
                                                bool is_v24_active, const CCoinsViewCache& view, bool debugLogs,
                                                BlockValidationState& state, CDeterministicMNList& mnListRet)
{
    AssertLockHeld(cs_main);
    CDeterministicMNList oldList = m_dmnman.GetListForBlock(pindexPrev);
    return RebuildListFromBlock(block, pindexPrev, is_v24_active, oldList, view, debugLogs, state, mnListRet);
}

CDeterministicMNList CSpecialTxProcessor::BeginListForBlock(gsl::not_null<const CBlockIndex*> pindexPrev,
                                                            const CDeterministicMNList& prevList) const
{
    // Verify that prevList either represents an empty/initial state (default-constructed),
    // or it matches the previous block's hash.
    // cppcheck-suppress assertWithSideEffect
    assert(prevList == CDeterministicMNList() || prevList.GetBlockHash() == pindexPrev->GetBlockHash());

    int nHeight = pindexPrev->nHeight + 1;

    CDeterministicMNList newList = prevList;
    newList.SetBlockHash(uint256()); // we can't know the final block hash, so better not return a (invalid) block hash
    newList.SetHeight(nHeight);

    // we iterate the prevList here and update the newList
    // this is only valid as long these have not diverged at this point, which is the case as long as we don't add
    // code above this loop that modifies newList
    prevList.ForEachMN(/*onlyValid=*/false, [&pindexPrev, &newList, this](const auto& dmn) {
        if (!dmn.pdmnState->confirmedHash.IsNull()) {
            // already confirmed
            return;
        }
        // this works on the previous block, so confirmation will happen one block after nMasternodeMinimumConfirmations
        // has been reached, but the block hash will then point to the block at nMasternodeMinimumConfirmations
        int nConfirmations = pindexPrev->nHeight - dmn.pdmnState->nRegisteredHeight;
        if (nConfirmations >= this->m_consensus_params.nMasternodeMinimumConfirmations) {
            auto newState = std::make_shared<CDeterministicMNState>(*dmn.pdmnState);
            newState->UpdateConfirmedHash(dmn.proTxHash, pindexPrev->GetBlockHash());
            newList.UpdateMN(dmn.proTxHash, newState);
        }
    });

    newList.DecreaseScores();

    return newList;
}

bool CSpecialTxProcessor::ApplyTxToList(const CTransaction& tx, gsl::not_null<const CBlockIndex*> pindexPrev,
                                        bool is_v24_active, const CCoinsViewCache& view, bool debugLogs,
                                        CDeterministicMNList& newList, BlockValidationState& state) const
{
    const int nHeight = pindexPrev->nHeight + 1;

    if (!tx.IsSpecialTxVersion()) {
        // only interested in special TXs
        return true;
    }

    if (tx.nType == TRANSACTION_PROVIDER_REGISTER) {
        const auto opt_proTx = GetTxPayload<CProRegTx>(tx);
        if (!opt_proTx) {
            return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "bad-protx-payload");
        }
        auto& proTx = *opt_proTx;

        auto dmn = std::make_shared<CDeterministicMN>(newList.GetTotalRegisteredCount(), proTx.nType);
        dmn->proTxHash = tx.GetHash();

        // collateralOutpoint is either pointing to an external collateral or to the ProRegTx itself
        if (proTx.collateralOutpoint.hash.IsNull()) {
            dmn->collateralOutpoint = COutPoint(tx.GetHash(), proTx.collateralOutpoint.n);
        } else {
            dmn->collateralOutpoint = proTx.collateralOutpoint;
        }

        // Complain about spent collaterals only when we process the tip.
        // This is safe because blocks below the tip were verified
        // when they were connected initially.
        if (!view.GetBestBlock().IsNull()) {
            Coin coin;
            CAmount expectedCollateral = GetMnType(proTx.nType).collat_amount;
            if (!proTx.collateralOutpoint.hash.IsNull() && (!view.GetCoin(dmn->collateralOutpoint, coin) ||
                                                            coin.IsSpent() || coin.out.nValue != expectedCollateral)) {
                // should actually never get to this point as CheckProRegTx should have handled this case.
                // We do this additional check nevertheless to be 100% sure
                return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "bad-protx-collateral");
            }
        }

        auto replacedDmn = newList.GetMNByCollateral(dmn->collateralOutpoint);
        if (replacedDmn != nullptr) {
            // This might only happen with a ProRegTx that refers an external collateral
            // In that case the new ProRegTx will replace the old one. This means the old one is removed
            // and the new one is added like a completely fresh one, which is also at the bottom of the payment list
            newList.RemoveMN(replacedDmn->proTxHash);
            if (debugLogs) {
                LogPrintf("%s -- MN %s removed from list because collateral was used for " /* Continued */
                          "a new ProRegTx. collateralOutpoint=%s, nHeight=%d, mapCurMNs.allMNsCount=%d\n",
                          __func__, replacedDmn->proTxHash.ToString(), dmn->collateralOutpoint.ToStringShort(),
                          nHeight, newList.GetCounts().total());
            }
        }

        for (const auto& entry : proTx.netInfo->GetEntries()) {
            if (const auto service_opt{entry.GetAddrPort()}) {
                if (newList.HasUniqueProperty(*service_opt)) {
                    return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "bad-protx-dup-netinfo-entry");
                }
            } else if (const auto domain_opt{entry.GetDomainPort()}) {
                if (newList.HasUniqueProperty(*domain_opt)) {
                    return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "bad-protx-dup-netinfo-entry");
                }
            } else {
                return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "bad-protx-netinfo-entry");
            }
        }
        if (proTx.IsShared()) {
            for (const auto& share : proTx.shares) {
                if (newList.HasUniqueProperty(share.keyIDOwner)) {
                    return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "bad-protx-dup-key");
                }
            }
            if (newList.HasUniqueProperty(proTx.pubKeyOperator)) {
                return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "bad-protx-dup-key");
            }
        } else if (newList.HasUniqueProperty(proTx.keyIDOwner) || newList.HasUniqueProperty(proTx.pubKeyOperator)) {
            return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "bad-protx-dup-key");
        }

        dmn->nOperatorReward = proTx.nOperatorReward;

        auto dmnState = std::make_shared<CDeterministicMNState>(proTx);
        dmnState->nRegisteredHeight = nHeight;
        if (proTx.netInfo->IsEmpty()) {
            // start in banned pdmnState as we need to wait for a ProUpServTx
            dmnState->BanIfNotBanned(nHeight);
        }
        dmn->pdmnState = dmnState;

        // CheckProRegTx ran against pindexPrev, so transactions in this same block are invisible
        // to each other and two of them could claim one operator key under different encodings.
        // Re-probe the list as rebuilt so far. AddMN() reports a duplicate by throwing, which
        // would escape block assembly, so reject cleanly here instead.
        if (is_v24_active &&
            newList.HasOperatorKeyUnderAnyScheme(dmn->pdmnState->pubKeyOperator.Get(), /*self=*/uint256())) {
            return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "bad-protx-dup-key");
        }

        newList.AddMN(dmn);

        if (debugLogs) {
            LogPrintf("%s -- MN %s added at height %d: %s\n", __func__, tx.GetHash().ToString(), nHeight,
                      proTx.ToString());
        }
    } else if (tx.nType == TRANSACTION_PROVIDER_UPDATE_SERVICE) {
        const auto opt_proTx = GetTxPayload<CProUpServTx>(tx);
        if (!opt_proTx) {
            return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "bad-protx-payload");
        }
        if (!IsValidMnType(opt_proTx->nType)) {
            return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "bad-protx-type");
        }

        for (const auto& entry : opt_proTx->netInfo->GetEntries()) {
            if (const auto service_opt{entry.GetAddrPort()}) {
                if (newList.HasUniqueProperty(*service_opt) &&
                    newList.GetUniquePropertyMN(*service_opt)->proTxHash != opt_proTx->proTxHash) {
                    return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "bad-protx-dup-netinfo-entry");
                }
            } else if (const auto domain_opt{entry.GetDomainPort()}) {
                if (newList.HasUniqueProperty(*domain_opt) &&
                    newList.GetUniquePropertyMN(*domain_opt)->proTxHash != opt_proTx->proTxHash) {
                    return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "bad-protx-dup-netinfo-entry");
                }
            } else {
                return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "bad-protx-netinfo-entry");
            }
        }

        auto dmn = newList.GetMN(opt_proTx->proTxHash);
        if (!dmn) {
            return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "bad-protx-hash");
        }
        if (opt_proTx->nType != dmn->nType) {
            return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "bad-protx-type-mismatch");
        }

        auto newState = std::make_shared<CDeterministicMNState>(*dmn->pdmnState);
        const uint16_t current_version{static_cast<uint16_t>(newState->nVersion)};
        const uint16_t target_version{is_v24_active ? std::max<uint16_t>(current_version, opt_proTx->nVersion)
                                                    : current_version};
        if (!ApplyProUpServTx(*newState, *opt_proTx, is_v24_active, state)) {
            return false;
        }
        if (newState->IsBanned()) {
            // only revive when all keys are set (a shared masternode has a null keyIDOwner;
            // its share owner keys are immutable and always set)
            if (newState->pubKeyOperator != CBLSLazyPublicKey() && !newState->keyIDVoting.IsNull() &&
                (newState->IsShared() || !newState->keyIDOwner.IsNull())) {
                newState->Revive(nHeight);
                if (debugLogs) {
                    LogPrintf("%s -- MN %s revived at height %d\n", __func__, opt_proTx->proTxHash.ToString(), nHeight);
                }
            }
        }

        // Migrating a legacy masternode to the basic scheme re-encodes its stored key
        // (SetStateVersion), moving it to the basic-scheme slot. Per-transaction checks ran
        // against pindexPrev, so re-check against the list as rebuilt so far: if another
        // masternode holds this key under either encoding, the re-key in UpdateMN() would throw
        // out of block assembly.
        if (is_v24_active && IsSchemeMigration(current_version, target_version) &&
            newList.HasOperatorKeyUnderAnyScheme(dmn->pdmnState->pubKeyOperator.Get(),
                                                 /*self=*/opt_proTx->proTxHash)) {
            return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "bad-protx-dup-key");
        }

        newList.UpdateMN(opt_proTx->proTxHash, newState);
        if (debugLogs) {
            LogPrintf("%s -- MN %s updated at height %d: %s\n", __func__, opt_proTx->proTxHash.ToString(), nHeight,
                      opt_proTx->ToString());
        }
    } else if (tx.nType == TRANSACTION_PROVIDER_UPDATE_REGISTRAR) {
        const auto opt_proTx = GetTxPayload<CProUpRegTx>(tx);
        if (!opt_proTx) {
            return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "bad-protx-payload");
        }

        auto dmn = newList.GetMN(opt_proTx->proTxHash);
        if (!dmn) {
            return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "bad-protx-hash");
        }
        if (dmn->pdmnState->IsShared()) {
            // shared masternodes are updated via ProUpShareTx / ProUpSharedRegTx exclusively
            return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "bad-protx-shared-mn");
        }
        auto newState = std::make_shared<CDeterministicMNState>(*dmn->pdmnState);
        const uint16_t old_version{static_cast<uint16_t>(newState->nVersion)};
        const bool operator_changed{newState->pubKeyOperator != opt_proTx->pubKeyOperator};
        const uint16_t target_version{is_v24_active ? std::max<uint16_t>(old_version, opt_proTx->nVersion)
                                                    : (operator_changed ? opt_proTx->nVersion : old_version)};

        // Per-transaction checks ran against pindexPrev, so an earlier transaction in this same
        // block is invisible to them. Re-evaluate against the list as rebuilt so far: this update
        // moves the operator key to a new unique-property slot if it rotates the key or crosses
        // the legacy->basic boundary (which re-encodes the key), and if that slot is held by
        // another masternode the re-key in UpdateMN() would throw out of block assembly. Reject
        // cleanly. Scoped to those two cases so a pre-existing cross-scheme pair's non-migrating
        // routine update is not blocked.
        {
            const bool migrating{IsSchemeMigration(old_version, target_version)};
            if (is_v24_active && (operator_changed || migrating) &&
                newList.HasOperatorKeyUnderAnyScheme(opt_proTx->pubKeyOperator.Get(),
                                                     /*self=*/opt_proTx->proTxHash)) {
                return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "bad-protx-dup-key");
            }
        }

        if (operator_changed) {
            // reset all operator related fields and put MN into PoSe-banned state in case the operator key changes
            newState->ResetOperatorFields();
            newState->BanIfNotBanned(nHeight);
            newState->pubKeyOperator = opt_proTx->pubKeyOperator;
        }
        newState->keyIDVoting = opt_proTx->keyIDVoting;
        // SetStateVersion() re-encodes the operator key to target_version's scheme, so the stored
        // key stays consistent with its version whether it was carried in this payload (possibly
        // under a different version's encoding) or migrated in place.
        if (!SetStateVersion(*newState, target_version, dmn->nType, state)) {
            return false;
        }
        if (target_version >= ProTxVersion::ExtAddr) {
            newState->payouts = opt_proTx->nVersion >= ProTxVersion::ExtAddr
                ? opt_proTx->payouts
                : LegacyPayoutAsList(opt_proTx->scriptPayout);
            newState->scriptPayout.clear();
        } else {
            newState->scriptPayout = opt_proTx->scriptPayout;
            newState->payouts.clear();
        }

        newList.UpdateMN(opt_proTx->proTxHash, newState);

        if (debugLogs) {
            LogPrintf("%s -- MN %s updated at height %d: %s\n", __func__, opt_proTx->proTxHash.ToString(), nHeight,
                      opt_proTx->ToString());
        }
    } else if (tx.nType == TRANSACTION_PROVIDER_UPDATE_REVOKE) {
        const auto opt_proTx = GetTxPayload<CProUpRevTx>(tx);
        if (!opt_proTx) {
            return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "bad-protx-payload");
        }

        auto dmn = newList.GetMN(opt_proTx->proTxHash);
        if (!dmn) {
            return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "bad-protx-hash");
        }
        auto newState = std::make_shared<CDeterministicMNState>(*dmn->pdmnState);
        const uint16_t old_version{static_cast<uint16_t>(newState->nVersion)};
        // ResetOperatorFields() drops nVersion back to LegacyBLS. Post-v24 the state version must be
        // preserved (never silently downgraded), so pick the target the same max-based way the registrar
        // path does and restore it below. Pre-v24 keep the historical reset-to-legacy behaviour.
        uint16_t target_version{ProTxVersion::LegacyBLS};
        if (is_v24_active) {
            target_version = std::max<uint16_t>(old_version, opt_proTx->nVersion);
        }
        newState->ResetOperatorFields();
        if (target_version >= ProTxVersion::BasicBLS && !SetStateVersion(*newState, target_version, dmn->nType, state)) {
            return false;
        }
        newState->BanIfNotBanned(nHeight);
        newState->nRevocationReason = opt_proTx->nReason;

        newList.UpdateMN(opt_proTx->proTxHash, newState);

        if (debugLogs) {
            LogPrintf("%s -- MN %s revoked operator key at height %d: %s\n", __func__,
                      opt_proTx->proTxHash.ToString(), nHeight, opt_proTx->ToString());
        }
    } else if (tx.nType == TRANSACTION_PROVIDER_UPDATE_SHARE) {
        const auto opt_proTx = GetTxPayload<CProUpShareTx>(tx);
        if (!opt_proTx) {
            return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "bad-protx-payload");
        }

        auto dmn = newList.GetMN(opt_proTx->proTxHash);
        if (!dmn) {
            return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "bad-protx-hash");
        }
        if (!dmn->pdmnState->IsShared() || opt_proTx->shareIndex >= dmn->pdmnState->shares.size()) {
            return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "bad-proupshare-index");
        }

        auto newState = std::make_shared<CDeterministicMNState>(*dmn->pdmnState);
        newState->shares[opt_proTx->shareIndex].scriptReward = opt_proTx->scriptReward;

        // CheckProUpShareTx ran against pindexPrev, so a shared registrar update earlier in
        // this block is invisible to it and could have moved the voting key onto this share's
        // scripts. Recheck the separation rule against the state as rebuilt so far.
        if (!IsShareListVotingKeySafe(newState->shares, newState->keyIDVoting)) {
            return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "bad-proupshare-payee-reuse");
        }

        newList.UpdateMN(opt_proTx->proTxHash, newState);

        if (debugLogs) {
            LogPrintf("%s -- MN %s updated share at height %d: %s\n", __func__, opt_proTx->proTxHash.ToString(),
                      nHeight, opt_proTx->ToString());
        }
    } else if (tx.nType == TRANSACTION_PROVIDER_UPDATE_SHARED_REGISTRAR) {
        const auto opt_proTx = GetTxPayload<CProUpSharedRegTx>(tx);
        if (!opt_proTx) {
            return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "bad-protx-payload");
        }

        auto dmn = newList.GetMN(opt_proTx->proTxHash);
        if (!dmn) {
            return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "bad-protx-hash");
        }
        if (!dmn->pdmnState->IsShared()) {
            return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "bad-proupsharedreg-not-shared");
        }

        auto newState = std::make_shared<CDeterministicMNState>(*dmn->pdmnState);
        const uint16_t old_version{static_cast<uint16_t>(newState->nVersion)};
        const bool operator_changed{newState->pubKeyOperator != opt_proTx->pubKeyOperator};
        if (operator_changed) {
            // Operator-key change semantics match ProUpRegTx: reset all operator related
            // fields and put the MN into PoSe-banned state until a new ProUpServTx arrives
            newState->ResetOperatorFields();
            newState->BanIfNotBanned(nHeight);
            newState->pubKeyOperator = opt_proTx->pubKeyOperator;
        }
        newState->keyIDVoting = opt_proTx->keyIDVoting;
        // Shared masternodes are always ExtAddr or later; restore the state version that
        // ResetOperatorFields() cleared
        if (!SetStateVersion(*newState, old_version, dmn->nType, state)) {
            return false;
        }
        if (operator_changed) {
            newState->pubKeyOperator.SetLegacy(false);
        }

        // CheckProUpSharedRegTx ran against pindexPrev, so transactions earlier in this same
        // block are invisible to it. Re-evaluate against the list as rebuilt so far: another
        // update may have taken the operator key (under either encoding; the re-key in
        // UpdateMN() would throw out of block assembly), and a share reward update may have
        // moved a share script onto the new voting key.
        if (operator_changed && newList.HasOperatorKeyUnderAnyScheme(opt_proTx->pubKeyOperator.Get(),
                                                                     /*self=*/opt_proTx->proTxHash)) {
            return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "bad-protx-dup-key");
        }
        if (!IsShareListVotingKeySafe(newState->shares, newState->keyIDVoting)) {
            return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "bad-proupsharedreg-payee-reuse");
        }

        newList.UpdateMN(opt_proTx->proTxHash, newState);

        if (debugLogs) {
            LogPrintf("%s -- MN %s updated shared registrar at height %d: %s\n", __func__,
                      opt_proTx->proTxHash.ToString(), nHeight, opt_proTx->ToString());
        }
    } else if (tx.nType == TRANSACTION_QUORUM_COMMITMENT) {
        const auto opt_qc = GetTxPayload<llmq::CFinalCommitmentTxPayload>(tx);
        if (!opt_qc) {
            return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "bad-qc-payload");
        }
        if (!opt_qc->commitment.IsNull()) {
            const auto& llmq_params_opt = Params().GetLLMQ(opt_qc->commitment.llmqType);
            if (!llmq_params_opt.has_value()) {
                return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "bad-qc-commitment-type");
            }
            int qcnHeight = int(opt_qc->nHeight);
            int quorumHeight = qcnHeight - (qcnHeight % llmq_params_opt->dkgInterval) +
                               int(opt_qc->commitment.quorumIndex);
            auto pQuorumBaseBlockIndex = pindexPrev->GetAncestor(quorumHeight);
            if (!pQuorumBaseBlockIndex || pQuorumBaseBlockIndex->GetBlockHash() != opt_qc->commitment.quorumHash) {
                // we should actually never get into this case as validation should have caught it...but let's be sure
                return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "bad-qc-quorum-hash");
            }

            // The commitment has already been validated at this point, so it's safe to use members of it

            const auto members = llmq::utils::GetAllQuorumMembers(opt_qc->commitment.llmqType,
                                                                  {m_dmnman, m_qsnapman, m_chainman,
                                                                   pQuorumBaseBlockIndex});
            HandleQuorumCommitment(opt_qc->commitment, members, debugLogs, newList);
        }
    }

    return true;
}

bool CSpecialTxProcessor::FinishListForBlock(const CBlock& block, gsl::not_null<const CBlockIndex*> pindexPrev,
                                             const CDeterministicMNList& prevList, bool debugLogs,
                                             CDeterministicMNList newList, BlockValidationState& state,
                                             CDeterministicMNList& mnListRet) const
{
    const int nHeight = pindexPrev->nHeight + 1;
    auto payee = prevList.GetMNPayee(pindexPrev);
    const bool isMNRewardReallocation{DeploymentActiveAfter(pindexPrev, m_consensus_params, Consensus::DEPLOYMENT_MN_RR)};

    // we skip the coinbase
    for (int i = 1; i < static_cast<int>(block.vtx.size()); i++) {
        const CTransaction& tx = *block.vtx[i];

        // check if any existing MN collateral is spent by this transaction
        for (const auto& in : tx.vin) {
            auto dmn = newList.GetMNByCollateral(in.prevout);
            if (dmn && dmn->collateralOutpoint == in.prevout) {
                if (dmn->pdmnState->IsShared() && tx.nType != TRANSACTION_PROVIDER_DISSOLVE) {
                    // Shared collateral may only be spent by a ProDisTx. Any other spender is
                    // invalid (the ProDisTx that spends it was already validated in the loop above).
                    return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "bad-shared-collateral-spend");
                }
                // A shared masternode's dissolution takes effect here, in the same collateral-spend
                // phase as every other masternode removal, so its timing does not depend on where
                // the ProDisTx sits among the block's other provider transactions.
                newList.RemoveMN(dmn->proTxHash);

                if (debugLogs) {
                    LogPrintf("%s -- MN %s removed from list because collateral was spent. " /* Continued */
                              "collateralOutpoint=%s, nHeight=%d, mapCurMNs.allMNsCount=%d\n",
                              __func__, dmn->proTxHash.ToString(), dmn->collateralOutpoint.ToStringShort(), nHeight,
                              newList.GetCounts().total());
                }
            }
        }
    }

    // The payee for the current block was determined by the previous block's list, but it might have disappeared in the
    // current block. We still pay that MN one last time, however.
    if (auto dmn = payee ? newList.GetMN(payee->proTxHash) : nullptr) {
        auto newState = std::make_shared<CDeterministicMNState>(*dmn->pdmnState);
        newState->nLastPaidHeight = nHeight;
        // Starting from v19 and until MNRewardReallocation, EvoNodes will be paid 4 blocks in a row
        // No need to check if v19 is active, since EvoNode ProRegTxes are allowed only after v19 activation
        // Note: If the payee wasn't found in the current block that's fine
        if (dmn->nType == MnType::Evo && !isMNRewardReallocation) {
            ++newState->nConsecutivePayments;
            if (debugLogs) {
                LogPrint(BCLog::MNPAYMENTS, "%s -- MN %s is an EvoNode, bumping nConsecutivePayments to %d\n", __func__,
                         dmn->proTxHash.ToString(), newState->nConsecutivePayments);
            }
        }
        newList.UpdateMN(payee->proTxHash, newState);
    }

    // reset nConsecutivePayments on non-paid EvoNodes
    auto newList2 = newList;
    newList2.ForEachMN(/*onlyValid=*/false, [&](const auto& dmn) {
        if (dmn.nType != MnType::Evo) return;
        if (payee != nullptr && dmn.proTxHash == payee->proTxHash && !isMNRewardReallocation) return;
        if (dmn.pdmnState->nConsecutivePayments == 0) return;
        if (debugLogs) {
            LogPrint(BCLog::MNPAYMENTS, "%s -- MN %s, reset nConsecutivePayments %d->0\n", __func__,
                     dmn.proTxHash.ToString(), dmn.pdmnState->nConsecutivePayments);
        }
        auto newState = std::make_shared<CDeterministicMNState>(*dmn.pdmnState);
        newState->nConsecutivePayments = 0;
        newList.UpdateMN(dmn.proTxHash, newState);
    });

    mnListRet = newList;

    return true;
}

bool CSpecialTxProcessor::RebuildListFromBlock(const CBlock& block, gsl::not_null<const CBlockIndex*> pindexPrev,
                                               bool is_v24_active, const CDeterministicMNList& prevList,
                                               const CCoinsViewCache& view, bool debugLogs, BlockValidationState& state,
                                               CDeterministicMNList& mnListRet)
{
    CDeterministicMNList newList = BeginListForBlock(pindexPrev, prevList);

    // we skip the coinbase
    for (int i = 1; i < static_cast<int>(block.vtx.size()); i++) {
        if (!ApplyTxToList(*block.vtx[i], pindexPrev, is_v24_active, view, debugLogs, newList, state)) {
            return false;
        }
    }

    return FinishListForBlock(block, pindexPrev, prevList, debugLogs, newList, state, mnListRet);
}

CMNListBuilder::CMNListBuilder(const CSpecialTxProcessor& processor, const CDeterministicMNList& prevList,
                               gsl::not_null<const CBlockIndex*> pindexPrev, bool is_v24_active,
                               const CCoinsViewCache& view, bool debugLogs) :
    m_processor{&processor},
    m_prev_list{prevList},
    m_pindex_prev{pindexPrev},
    m_is_v24_active{is_v24_active},
    m_view{&view},
    m_debug_logs{debugLogs},
    m_list{processor.BeginListForBlock(pindexPrev, prevList)}
{
}

CMNListBuilder::CMNListBuilder(const CSpecialTxProcessor& processor, gsl::not_null<const CBlockIndex*> pindexPrev,
                               bool is_v24_active, const CCoinsViewCache& view, bool debugLogs) :
    CMNListBuilder(processor, processor.m_dmnman.GetListForBlock(pindexPrev), pindexPrev, is_v24_active, view,
                   debugLogs)
{
    AssertLockHeld(::cs_main);
}

bool CMNListBuilder::ApplyTx(const CTransaction& tx, BlockValidationState& state)
{
    return m_processor->ApplyTxToList(tx, m_pindex_prev, m_is_v24_active, *m_view, m_debug_logs, m_list, state);
}

bool CMNListBuilder::Finish(const CBlock& block, BlockValidationState& state, CDeterministicMNList& mnListRet) const
{
    return m_processor->FinishListForBlock(block, m_pindex_prev, m_prev_list, m_debug_logs, m_list, state, mnListRet);
}

void CSpecialTxProcessor::UpdatedMNListTip(gsl::not_null<const CBlockIndex*> pindex) { m_dmnman.UpdatedBlockTip(pindex); }

bool CSpecialTxProcessor::ProcessSpecialTxsInBlock(Chainstate& chainstate, const CChain& chain, const CBlock& block,
                                                   const CBlockIndex* pindex, SpecialTxRules rules,
                                                   const CCoinsViewCache& view, CAmount blockSubsidy, bool fJustCheck,
                                                   bool fCheckCbTxMerkleRoots, BlockValidationState& state,
                                                   MNListUpdates& updatesRet)
{
    AssertLockHeld(::cs_main);

    try {
        static int64_t nTimeLoop = 0;
        static int64_t nTimeQuorum = 0;
        static int64_t nTimeDMN = 0;
        static int64_t nTimeMnehf = 0;
        static int64_t nTimePayload = 0;
        static int64_t nTimeCreditPool = 0;

        int64_t nTime1 = GetTimeMicros();

        std::optional<CCbTx> opt_cbTx{std::nullopt};
        if (fCheckCbTxMerkleRoots && block.vtx.size() > 0 && block.vtx[0]->nType == TRANSACTION_COINBASE) {
            const auto& tx = block.vtx[0];
            if (!tx->IsCoinBase()) {
                return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "bad-cbtx-invalid");
            }
            if (opt_cbTx = GetTxPayload<CCbTx>(*tx); opt_cbTx) {
                TxValidationState tx_state;
                if (!CheckCbTx(*opt_cbTx, pindex->pprev, rules.v24, tx_state)) {
                    assert(tx_state.GetResult() == TxValidationResult::TX_CONSENSUS ||
                           tx_state.GetResult() == TxValidationResult::TX_BAD_SPECIAL);
                    return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, tx_state.GetRejectReason(),
                                         strprintf("Special Transaction check failed (tx hash %s) %s",
                                                   tx->GetHash().ToString(), tx_state.GetDebugMessage()));
                }
            } else {
                return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "bad-cbtx-payload");
            }
        }
        if (fCheckCbTxMerkleRoots) {
            // To ensure that opt_cbTx is not missing when it's supposed to be
            if (DeploymentActiveAt(*pindex, m_consensus_params, Consensus::DEPLOYMENT_DIP0003) && !opt_cbTx.has_value()) {
                return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "bad-cbtx-version");
            }
        }

        int64_t nTime2 = GetTimeMicros();
        nTimePayload += nTime2 - nTime1;
        LogPrint(BCLog::BENCHMARK, "      - GetTxPayload: %.2fms [%.2fs]\n", 0.001 * (nTime2 - nTime1),
                 nTimePayload * 0.000001);

        CRangesSet indexes;
        if (DeploymentActiveAt(*pindex, m_consensus_params, Consensus::DEPLOYMENT_V20)) {
            CCreditPool creditPool{m_cpoolman.GetCreditPool(pindex->pprev)};
            LogPrint(BCLog::CREDITPOOL, "CSpecialTxProcessor::%s -- CCreditPool is %s\n", __func__, creditPool.ToString());
            indexes = std::move(creditPool.indexes);
        }

        for (size_t i = 0; i < block.vtx.size(); ++i) {
            // The shared-collateral creation rule applies to every transaction, including the
            // coinbase and non-special transactions, which CheckSpecialTxInner never sees
            if (rules.v24) {
                if (TxValidationState tx_state; !CheckSharedCollateralTemplateOutputs(*block.vtx[i], tx_state)) {
                    return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, tx_state.GetRejectReason(),
                                         strprintf("Shared collateral check failed (tx hash %s) %s",
                                                   block.vtx[i]->GetHash().ToString(), tx_state.GetDebugMessage()));
                }
            }

            // we validated CCbTx above, starts from the 2nd transaction
            if (i == 0 && block.vtx[i]->nType == TRANSACTION_COINBASE) continue;

            const auto ptr_tx = block.vtx[i];
            TxValidationState tx_state;
            // At this moment CheckSpecialTx() may fail by 2 possible ways:
            // consensus failures and "TX_BAD_SPECIAL"
            if (!CheckSpecialTxInner(&chain, *ptr_tx, pindex->pprev, rules, view, indexes, fCheckCbTxMerkleRoots, tx_state)) {
                assert(tx_state.GetResult() == TxValidationResult::TX_CONSENSUS || tx_state.GetResult() == TxValidationResult::TX_BAD_SPECIAL);
                return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, tx_state.GetRejectReason(),
                                 strprintf("Special Transaction check failed (tx hash %s) %s", ptr_tx->GetHash().ToString(), tx_state.GetDebugMessage()));
            }
        }

        int64_t nTime3 = GetTimeMicros();
        nTimeLoop += nTime3 - nTime2;
        LogPrint(BCLog::BENCHMARK, "      - Loop: %.2fms [%.2fs]\n", 0.001 * (nTime3 - nTime2), nTimeLoop * 0.000001);

        if (opt_cbTx.has_value()) {
            if (!CheckCreditPoolDiffForBlock(block, pindex, *opt_cbTx, blockSubsidy, state)) {
                LogError("CSpecialTxProcessor: CheckCreditPoolDiffForBlock for block %s failed with %s\n",
                         pindex->GetBlockHash().ToString(), state.ToString());
                return false;
            }
        }

        int64_t nTime4 = GetTimeMicros();
        nTimeCreditPool += nTime4 - nTime3;
        LogPrint(BCLog::BENCHMARK, "      - CheckCreditPoolDiffForBlock: %.2fms [%.2fs]\n", 0.001 * (nTime4 - nTime3),
                 nTimeCreditPool * 0.000001);

        if (!m_qblockman.ProcessBlock(chainstate, block, pindex, state, fJustCheck, fCheckCbTxMerkleRoots)) {
            // pass the state returned by the function above
            return false;
        }

        int64_t nTime5 = GetTimeMicros();
        nTimeQuorum += nTime5 - nTime4;
        LogPrint(BCLog::BENCHMARK, "      - m_qblockman.ProcessBlock: %.2fms [%.2fs]\n", 0.001 * (nTime5 - nTime4),
                 nTimeQuorum * 0.000001);

        CDeterministicMNList mn_list;
        if (DeploymentActiveAt(*pindex, m_consensus_params, Consensus::DEPLOYMENT_DIP0003)) {
            if (!BuildNewListFromBlock(block, pindex->pprev, rules.v24, view, true, state, mn_list)) {
                // pass the state returned by the function above
                return false;
            }
            mn_list.SetBlockHash(pindex->GetBlockHash());

            if (!fJustCheck && !m_dmnman.ProcessBlock(block, pindex, state, mn_list, updatesRet)) {
                // pass the state returned by the function above
                return false;
            }
        }

        int64_t nTime6 = GetTimeMicros();
        nTimeDMN += nTime6 - nTime5;
        LogPrint(BCLog::BENCHMARK, "      - m_dmnman.ProcessBlock: %.2fms [%.2fs]\n", 0.001 * (nTime6 - nTime5),
                 nTimeDMN * 0.000001);

        if (opt_cbTx.has_value()) {
            static int64_t nTimeMerkleMNL = 0;
            static int64_t nTimeMerkleQuorums = 0;
            static int64_t nTimeCbTxCL = 0;

            uint256 calculatedMerkleRootMNL;
            if (!CalcCbTxMerkleRootMNList(calculatedMerkleRootMNL, mn_list.to_sml(), state)) {
                // pass the state returned by the function above
                return false;
            }
            if (calculatedMerkleRootMNL != opt_cbTx->merkleRootMNList) {
                return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "bad-cbtx-mnmerkleroot");
            }

            int64_t nTime6_1 = GetTimeMicros();
            nTimeMerkleMNL += nTime6_1 - nTime6;
            LogPrint(BCLog::BENCHMARK, "      - CalcCbTxMerkleRootMNList: %.2fms [%.2fs]\n",
                     0.001 * (nTime6_1 - nTime6), nTimeMerkleMNL * 0.000001);

            if (opt_cbTx->nVersion >= CCbTx::Version::MERKLE_ROOT_QUORUMS) {
                uint256 calculatedMerkleRootQuorums;
                if (!CalcCbTxMerkleRootQuorums(block, pindex->pprev, m_qblockman, calculatedMerkleRootQuorums, state)) {
                    // pass the state returned by the function above
                    return false;
                }
                if (calculatedMerkleRootQuorums != opt_cbTx->merkleRootQuorums) {
                    return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "bad-cbtx-quorummerkleroot");
                }
            }

            int64_t nTime6_2 = GetTimeMicros();
            nTimeMerkleQuorums += nTime6_2 - nTime6_1;

            LogPrint(BCLog::BENCHMARK, "      - CalcCbTxMerkleRootQuorums: %.2fms [%.2fs]\n",
                     0.001 * (nTime6_2 - nTime6_1), nTimeMerkleQuorums * 0.000001);

            if (!CheckCbTxBestChainlock(*opt_cbTx, pindex, m_consensus_params, chain, m_qman, m_chainlocks, state)) {
                // pass the state returned by the function above
                return false;
            }

            int64_t nTime6_3 = GetTimeMicros();
            nTimeCbTxCL += nTime6_3 - nTime6_2;
            LogPrint(BCLog::BENCHMARK, "      - CheckCbTxBestChainlock: %.2fms [%.2fs]\n",
                     0.001 * (nTime6_3 - nTime6_2), nTimeCbTxCL * 0.000001);
        }

        int64_t nTime7 = GetTimeMicros();

        if (!m_mnhfman.ProcessBlock(block, pindex, fJustCheck, state)) {
            // pass the state returned by the function above
            return false;
        }

        int64_t nTime8 = GetTimeMicros();
        nTimeMnehf += nTime8 - nTime7;
        LogPrint(BCLog::BENCHMARK, "      - m_mnhfman.ProcessBlock: %.2fms [%.2fs]\n", 0.001 * (nTime8 - nTime7),
                 nTimeMnehf * 0.000001);

        if (DeploymentActiveAfter(pindex, m_consensus_params, Consensus::DEPLOYMENT_V19) && bls::bls_legacy_scheme.load()) {
            // NOTE: The block next to the activation is the one that is using new rules.
            // V19 activated just activated, so we must switch to the new rules here.
            bls::bls_legacy_scheme.store(false);
            LogPrintf("CSpecialTxProcessor::%s -- bls_legacy_scheme=%d\n", __func__, bls::bls_legacy_scheme.load());
        }
    } catch (const EvoDbInconsistencyError& e) {
        // Local EvoDB corruption detected below (the node is already
        // aborting): fail with M_ERROR so the block is not marked invalid.
        return state.Error(e.what());
    } catch (const std::exception& e) {
        LogPrintf("CSpecialTxProcessor::%s -- FAILURE! %s\n", __func__, e.what());
        return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "failed-procspectxsinblock");
    }

    return true;
}

bool CSpecialTxProcessor::UndoSpecialTxsInBlock(const Chainstate& chainstate, const CBlock& block, const CBlockIndex* pindex, MNListUpdates& updatesRet)
{
    AssertLockHeld(::cs_main);

    auto bls_legacy_scheme = bls::bls_legacy_scheme.load();

    try {
        if (!DeploymentActiveAt(*pindex, m_consensus_params, Consensus::DEPLOYMENT_V19) && !bls_legacy_scheme) {
            // NOTE: The block next to the activation is the one that is using new rules.
            // Removing the activation block here, so we must switch back to the old rules.
            bls::bls_legacy_scheme.store(true);
            LogPrintf("CSpecialTxProcessor::%s -- bls_legacy_scheme=%d\n", __func__, bls::bls_legacy_scheme.load());
        }

        if (!m_mnhfman.UndoBlock(block, pindex)) {
            return false;
        }

        if (!m_dmnman.UndoBlock(pindex, updatesRet)) {
            return false;
        }

        if (!m_qblockman.UndoBlock(chainstate, block, pindex)) {
            return false;
        }
    } catch (const std::exception& e) {
        bls::bls_legacy_scheme.store(bls_legacy_scheme);
        LogPrintf("CSpecialTxProcessor::%s -- bls_legacy_scheme=%d\n", __func__, bls::bls_legacy_scheme.load());
        LogError("CSpecialTxProcessor::%s -- FAILURE! %s\n", __func__, e.what());
        return false;
    }

    return true;
}

void CSpecialTxProcessor::RemoveMineableCommitmentsForBlock(const CBlock& block, gsl::not_null<const CBlockIndex*> pindex)
{
    m_qblockman.RemoveMineableCommitmentsForBlock(block, pindex);
}

bool CSpecialTxProcessor::CheckCreditPoolDiffForBlock(const CBlock& block, const CBlockIndex* pindex, const CCbTx& cbTx,
                                                      CAmount blockSubsidy, BlockValidationState& state)
{
    AssertLockHeld(::cs_main);

    if (!DeploymentActiveAt(*pindex, m_consensus_params, Consensus::DEPLOYMENT_DIP0008)) return true;
    if (!DeploymentActiveAt(*pindex, m_consensus_params, Consensus::DEPLOYMENT_V20)) return true;

    try {
        const auto creditPoolDiff = GetCreditPoolDiffForBlock(m_cpoolman, block,
                                                              pindex->pprev, m_consensus_params, blockSubsidy, state);
        if (!creditPoolDiff.has_value()) return false;

        const CAmount target_balance{cbTx.creditPoolBalance};
        // But it maybe not included yet in previous block yet; in this case value must be 0
        const CAmount locked_calculated{creditPoolDiff->GetTotalLocked()};
        if (target_balance != locked_calculated) {
            LogPrintf("CSpecialTxProcessor::%s -- mismatched locked amount in CbTx: %lld against re-calculated: %lld\n", __func__, target_balance, locked_calculated);
            return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "bad-cbtx-assetlocked-amount");
        }

    } catch (const EvoDbInconsistencyError& e) {
        // Local EvoDB corruption detected below (the node is already
        // aborting): fail with M_ERROR so the block is not marked invalid.
        return state.Error(e.what());
    } catch (const std::exception& e) {
        LogPrintf("CSpecialTxProcessor::%s -- FAILURE! %s\n", __func__, e.what());
        return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "failed-checkcreditpooldiff");
    }

    return true;
}

template <typename ProTx>
static bool CheckHashSig(const ProTx& proTx, const PKHash& pkhash, TxValidationState& state)
{
    if (std::string strError; !CHashSigner::VerifyHash(::SerializeHash(proTx), ToKeyID(pkhash), proTx.vchSig, strError)) {
        return state.Invalid(TxValidationResult::TX_CONSENSUS, "bad-protx-sig");
    }
    return true;
}

template <typename ProTx>
static bool CheckStringSig(const ProTx& proTx, const PKHash& pkhash, TxValidationState& state)
{
    if (std::string strError;
        !CMessageSigner::VerifyMessage(ToKeyID(pkhash), proTx.vchSig, proTx.MakeSignString(), strError)) {
        return state.Invalid(TxValidationResult::TX_CONSENSUS, "bad-protx-sig");
    }
    return true;
}

template <typename ProTx>
static bool CheckHashSig(const ProTx& proTx, const CBLSPublicKey& pubKey, TxValidationState& state)
{
    if (!proTx.sig.VerifyInsecure(pubKey, ::SerializeHash(proTx))) {
        return state.Invalid(TxValidationResult::TX_CONSENSUS, "bad-protx-sig");
    }
    return true;
}

template <typename ProTx>
std::optional<ProTx> GetValidatedPayload(const CTransaction& tx, gsl::not_null<const CBlockIndex*> pindexPrev,
                                         const Consensus::Params& consensus_params, bool is_v24_active,
                                         TxValidationState& state)
{
    if (tx.nType != ProTx::SPECIALTX_TYPE) {
        state.Invalid(TxValidationResult::TX_CONSENSUS, "bad-protx-type");
        return std::nullopt;
    }

    auto opt_ptx = GetTxPayload<ProTx>(tx);
    if (!opt_ptx) {
        state.Invalid(TxValidationResult::TX_CONSENSUS, "bad-protx-payload");
        return std::nullopt;
    }
    const bool is_v19_active{DeploymentActiveAfter(pindexPrev, consensus_params, Consensus::DEPLOYMENT_V19)};
    if (opt_ptx->nVersion > ProTxVersion::GetMax(is_v19_active, is_v24_active)) {
        state.Invalid(TxValidationResult::TX_CONSENSUS, "bad-protx-version");
        return std::nullopt;
    }
    if (!opt_ptx->IsTriviallyValid(state)) {
        // pass the state returned by the function above
        return std::nullopt;
    }
    return opt_ptx;
}

/**
 * Validates potential changes to masternode state version by ProTx transaction version
 * @param[in]  state_version Current masternode state version
 * @param[in]  tx_version    Proposed transaction version
 * @param[in]  is_v24_active Whether DEPLOYMENT_V24 is active for the block being validated
 * @param[out] state         This may be set to an Error state if any error occurred processing them
 * @returns                  true if version change is valid or DEPLOYMENT_V24 is not active
 */
static bool IsVersionChangeValid(const uint16_t state_version, const uint16_t tx_version, bool is_v24_active,
                                 TxValidationState& state)
{
    if (!is_v24_active) {
        // New restrictions only apply after v24 deployment
        return true;
    }

    if (state_version >= ProTxVersion::BasicBLS && tx_version == ProTxVersion::LegacyBLS) {
        // Don't allow legacy scheme versioned transactions after upgrading to basic scheme
        return state.Invalid(TxValidationResult::TX_CONSENSUS, "bad-protx-version-downgrade");
    }

    return true;
}

bool CheckProRegTx(const CTransaction& tx, gsl::not_null<const CBlockIndex*> pindexPrev, CDeterministicMNManager& dmnman,
                   const CCoinsViewCache& view, const Consensus::Params& consensus_params, SpecialTxRules rules,
                   TxValidationState& state, bool check_sigs)
{
    const auto opt_ptx = GetValidatedPayload<CProRegTx>(tx, pindexPrev, consensus_params, rules.v24, state);
    if (!opt_ptx) {
        // pass the state returned by the function above
        return false;
    }

    // No longer allow legacy scheme masternode registration
    if (rules.v24 && opt_ptx->nVersion < ProTxVersion::BasicBLS) {
        return state.Invalid(TxValidationResult::TX_CONSENSUS, "bad-protx-version-disallowed");
    }

    if (opt_ptx->nType == MnType::Evo && !rules.evo_shares) {
        if (opt_ptx->IsShared()) {
            return state.Invalid(TxValidationResult::TX_BAD_SPECIAL, "bad-protx-shares-evo");
        }
        if (GetOwnerPayouts(*opt_ptx).size() > 1) {
            return state.Invalid(TxValidationResult::TX_BAD_SPECIAL, "bad-protx-payouts-evo");
        }
    }

    // It's allowed to set addr to 0, which will put the MN into PoSe-banned state and require a ProUpServTx to be
    // issues later. If any of both is set, it must be valid however
    if (!CheckProviderNetworkFields(opt_ptx->netInfo, opt_ptx->nType, opt_ptx->nVersion, &opt_ptx->platformNodeID,
                                    opt_ptx->platformP2PPort, opt_ptx->platformHTTPPort, /*allow_empty=*/true,
                                    rules.ipv4_optional, state)) {
        return false;
    }

    CTxDestination collateralTxDest;
    const PKHash* keyForPayloadSig = nullptr;
    COutPoint collateralOutpoint;

    CAmount expectedCollateral = GetMnType(opt_ptx->nType).collat_amount;

    if (!opt_ptx->collateralOutpoint.hash.IsNull()) {
        Coin coin;
        if (!view.GetCoin(opt_ptx->collateralOutpoint, coin) || coin.IsSpent() || coin.out.nValue != expectedCollateral) {
            return state.Invalid(TxValidationResult::TX_BAD_SPECIAL, "bad-protx-collateral");
        }

        if (!ExtractDestination(coin.out.scriptPubKey, collateralTxDest)) {
            return state.Invalid(TxValidationResult::TX_BAD_SPECIAL, "bad-protx-collateral-dest");
        }

        // Extract key from collateral. This only works for P2PK and P2PKH collaterals and will fail for P2SH.
        // Issuer of this ProRegTx must prove ownership with this key by signing the ProRegTx
        keyForPayloadSig = std::get_if<PKHash>(&collateralTxDest);
        if (!keyForPayloadSig) {
            return state.Invalid(TxValidationResult::TX_BAD_SPECIAL, "bad-protx-collateral-pkh");
        }

        collateralOutpoint = opt_ptx->collateralOutpoint;
    } else {
        if (opt_ptx->collateralOutpoint.n >= tx.vout.size()) {
            return state.Invalid(TxValidationResult::TX_BAD_SPECIAL, "bad-protx-collateral-index");
        }
        if (tx.vout[opt_ptx->collateralOutpoint.n].nValue != expectedCollateral) {
            return state.Invalid(TxValidationResult::TX_BAD_SPECIAL, "bad-protx-collateral");
        }

        if (opt_ptx->IsShared()) {
            // Shared collateral must use exactly the template script; there is no destination to extract
            if (!IsSharedCollateralScript(tx.vout[opt_ptx->collateralOutpoint.n].scriptPubKey)) {
                return state.Invalid(TxValidationResult::TX_BAD_SPECIAL, "bad-protx-shares-collateral-script");
            }
        } else if (!ExtractDestination(tx.vout[opt_ptx->collateralOutpoint.n].scriptPubKey, collateralTxDest)) {
            return state.Invalid(TxValidationResult::TX_BAD_SPECIAL, "bad-protx-collateral-dest");
        }

        collateralOutpoint = COutPoint(tx.GetHash(), opt_ptx->collateralOutpoint.n);
    }

    // don't allow reuse of collateral key for other keys (don't allow people to put the collateral key onto an online server)
    // this check applies to internal and external collateral, but internal collaterals are not necessarily a P2PKH
    // (shared collateral has no destination at all; refund/reward key reuse is checked in IsShareListTriviallyValid)
    if (!opt_ptx->IsShared() &&
        !IsPayoutListKeySafe(GetOwnerPayouts(*opt_ptx),
                             collateralTxDest, opt_ptx->keyIDOwner, opt_ptx->keyIDVoting,
                             opt_ptx->nVersion >= ProTxVersion::ExtAddr, state)) return false;

    if (pindexPrev) {
        auto mnList = dmnman.GetListForBlock(pindexPrev);

        // only allow reusing of addresses when it's for the same collateral (which replaces the old MN)
        for (const auto& entry : opt_ptx->netInfo->GetEntries()) {
            if (const auto service_opt{entry.GetAddrPort()}) {
                if (mnList.HasUniqueProperty(*service_opt) &&
                    mnList.GetUniquePropertyMN(*service_opt)->collateralOutpoint != collateralOutpoint) {
                    return state.Invalid(TxValidationResult::TX_BAD_SPECIAL, "bad-protx-dup-netinfo-entry");
                }
            } else if (const auto domain_opt{entry.GetDomainPort()}) {
                if (mnList.HasUniqueProperty(*domain_opt) &&
                    mnList.GetUniquePropertyMN(*domain_opt)->collateralOutpoint != collateralOutpoint) {
                    return state.Invalid(TxValidationResult::TX_BAD_SPECIAL, "bad-protx-dup-netinfo-entry");
                }
            } else {
                return state.Invalid(TxValidationResult::TX_BAD_SPECIAL, "bad-protx-netinfo-entry");
            }
        }

        // never allow duplicate keys, even if this ProTx would replace an existing MN. Share owner
        // keys live in the same uniqueness namespace as keyIDOwner, so reuse is blocked in both
        // directions between shared and non-shared masternodes.
        if (opt_ptx->IsShared()) {
            for (const auto& share : opt_ptx->shares) {
                if (mnList.HasUniqueProperty(share.keyIDOwner)) {
                    return state.Invalid(TxValidationResult::TX_BAD_SPECIAL, "bad-protx-dup-key");
                }
            }
            if (mnList.HasUniqueProperty(opt_ptx->pubKeyOperator)) {
                return state.Invalid(TxValidationResult::TX_BAD_SPECIAL, "bad-protx-dup-key");
            }
        } else if (mnList.HasUniqueProperty(opt_ptx->keyIDOwner) || mnList.HasUniqueProperty(opt_ptx->pubKeyOperator)) {
            return state.Invalid(TxValidationResult::TX_BAD_SPECIAL, "bad-protx-dup-key");
        }

        // The check above only sees the operator key under the encoding this payload happens to use,
        // so it misses a key an existing masternode holds under the other one. A ProRegTx never
        // proves ownership of the operator key, so that gap lets anyone claim a masternode's key.
        // Nothing is excluded here: a duplicate key is never allowed, even for a ProTx replacing an
        // existing masternode.
        if (rules.v24 && mnList.HasOperatorKeyUnderAnyScheme(opt_ptx->pubKeyOperator.Get(), /*self=*/uint256())) {
            return state.Invalid(TxValidationResult::TX_BAD_SPECIAL, "bad-protx-dup-key");
        }

        // never allow duplicate platformNodeIds for EvoNodes
        if (opt_ptx->nType == MnType::Evo) {
            if (mnList.HasUniqueProperty(opt_ptx->platformNodeID)) {
                return state.Invalid(TxValidationResult::TX_BAD_SPECIAL, "bad-protx-dup-platformnodeid");
            }
        }

        if (!DeploymentDIP0003Enforced(pindexPrev->nHeight, Params().GetConsensus())) {
            if (opt_ptx->keyIDOwner != opt_ptx->keyIDVoting) {
                return state.Invalid(TxValidationResult::TX_BAD_SPECIAL, "bad-protx-key-not-same");
            }
        }
    }

    if (!CheckInputsHash(tx, *opt_ptx, state)) {
        // pass the state returned by the function above
        return false;
    }

    if (keyForPayloadSig) {
        // collateral is not part of this ProRegTx, so we must verify ownership of the collateral
        if (check_sigs && !CheckStringSig(*opt_ptx, *keyForPayloadSig, state)) {
            // pass the state returned by the function above
            return false;
        }
    } else {
        // collateral is part of this ProRegTx, so we know the collateral is owned by the issuer
        if (!opt_ptx->vchSig.empty()) {
            return state.Invalid(TxValidationResult::TX_CONSENSUS, "bad-protx-sig");
        }
        if (opt_ptx->IsShared() && check_sigs) {
            // every participant must consent to the exact funding inputs, outputs, share table,
            // penalty terms and registrar configuration
            const uint256 consent_hash = opt_ptx->MakeSharedRegConsentHash(tx);
            for (size_t i = 0; i < opt_ptx->shares.size(); i++) {
                if (std::string strError;
                    !CHashSigner::VerifyHashCanonical(consent_hash, opt_ptx->shares[i].keyIDOwner,
                                                      opt_ptx->vchJoinSigs[i], strError)) {
                    return state.Invalid(TxValidationResult::TX_BAD_SPECIAL, "bad-protx-shares-sig");
                }
            }
        }
    }

    return true;
}

bool CheckProUpServTx(const CTransaction& tx, gsl::not_null<const CBlockIndex*> pindexPrev,
                      CDeterministicMNManager& dmnman, const Consensus::Params& consensus_params, SpecialTxRules rules,
                      TxValidationState& state, bool check_sigs)
{
    const auto opt_ptx = GetValidatedPayload<CProUpServTx>(tx, pindexPrev, consensus_params, rules.v24, state);
    if (!opt_ptx) {
        // pass the state returned by the function above
        return false;
    }

    if (!CheckProviderNetworkFields(opt_ptx->netInfo, opt_ptx->nType, opt_ptx->nVersion, &opt_ptx->platformNodeID,
                                    opt_ptx->platformP2PPort, opt_ptx->platformHTTPPort, /*allow_empty=*/false,
                                    rules.ipv4_optional, state)) {
        return false;
    }

    auto mnList = dmnman.GetListForBlock(pindexPrev);
    auto dmn = mnList.GetMN(opt_ptx->proTxHash);
    if (!dmn) {
        return state.Invalid(TxValidationResult::TX_CONSENSUS, "bad-protx-hash");
    }

    // Mirror BuildNewListFromBlock: nType must match the registered MN. Without this check a
    // mempool-accepted ProUpServTx can make CreateNewBlock fail. An out-of-range nType is already
    // rejected by IsTriviallyValid, and dmn->nType is always in range, so no separate
    // IsValidMnType check is reachable here.
    if (opt_ptx->nType != dmn->nType) {
        return state.Invalid(TxValidationResult::TX_CONSENSUS, "bad-protx-type-mismatch");
    }

    if (!IsVersionChangeValid(dmn->pdmnState->nVersion, opt_ptx->nVersion, rules.v24, state)) {
        // pass the state returned by the function above
        return false;
    }

    // From ExtAddr on the Platform ports live in netInfo and are never derived from the scalar fields,
    // so an EvoNode service update has to spell them out
    if (rules.v24 && opt_ptx->nType == MnType::Evo && opt_ptx->nVersion < ProTxVersion::ExtAddr) {
        return state.Invalid(TxValidationResult::TX_CONSENSUS, "bad-protx-version-disallowed");
    }

    if (rules.v24) {
        CDeterministicMNState new_state{*dmn->pdmnState};
        BlockValidationState migration_state;
        if (!ApplyProUpServTx(new_state, *opt_ptx, rules.v24, migration_state)) {
            return state.Invalid(TxValidationResult::TX_BAD_SPECIAL, migration_state.GetRejectReason());
        }
    }

    // A service update carries no operator key, but raising a legacy masternode to the basic scheme
    // re-encodes its stored key, moving it to the basic-scheme unique-property slot. If another
    // masternode already holds that key under either encoding, the re-key in UpdateMN() would throw
    // out of block assembly, so reject the migration cleanly here.
    if (rules.v24 && IsSchemeMigration(dmn->pdmnState->nVersion, opt_ptx->nVersion) &&
        mnList.HasOperatorKeyUnderAnyScheme(dmn->pdmnState->pubKeyOperator.Get(), /*self=*/opt_ptx->proTxHash)) {
        return state.Invalid(TxValidationResult::TX_BAD_SPECIAL, "bad-protx-dup-key");
    }

    // don't allow updating to addresses already used by other MNs
    for (const auto& entry : opt_ptx->netInfo->GetEntries()) {
        if (const auto service_opt{entry.GetAddrPort()}) {
            if (mnList.HasUniqueProperty(*service_opt) &&
                mnList.GetUniquePropertyMN(*service_opt)->proTxHash != opt_ptx->proTxHash) {
                return state.Invalid(TxValidationResult::TX_BAD_SPECIAL, "bad-protx-dup-netinfo-entry");
            }
        } else if (const auto domain_opt{entry.GetDomainPort()}) {
            if (mnList.HasUniqueProperty(*domain_opt) &&
                mnList.GetUniquePropertyMN(*domain_opt)->proTxHash != opt_ptx->proTxHash) {
                return state.Invalid(TxValidationResult::TX_BAD_SPECIAL, "bad-protx-dup-netinfo-entry");
            }
        } else {
            return state.Invalid(TxValidationResult::TX_BAD_SPECIAL, "bad-protx-netinfo-entry");
        }
    }

    // don't allow updating to platformNodeIds already used by other EvoNodes
    if (opt_ptx->nType == MnType::Evo) {
        if (mnList.HasUniqueProperty(opt_ptx->platformNodeID) &&
            mnList.GetUniquePropertyMN(opt_ptx->platformNodeID)->proTxHash != opt_ptx->proTxHash) {
            return state.Invalid(TxValidationResult::TX_BAD_SPECIAL, "bad-protx-dup-platformnodeid");
        }
    }

    if (opt_ptx->scriptOperatorPayout != CScript()) {
        if (dmn->nOperatorReward == 0) {
            // don't allow setting operator reward payee in case no operatorReward was set
            return state.Invalid(TxValidationResult::TX_BAD_SPECIAL, "bad-protx-operator-payee");
        }
        if (!opt_ptx->scriptOperatorPayout.IsPayToPublicKeyHash() && !opt_ptx->scriptOperatorPayout.IsPayToScriptHash()) {
            return state.Invalid(TxValidationResult::TX_BAD_SPECIAL, "bad-protx-operator-payee");
        }
    }

    // we can only check the signature if pindexPrev != nullptr and the MN is known
    if (!CheckInputsHash(tx, *opt_ptx, state)) {
        // pass the state returned by the function above
        return false;
    }
    if (check_sigs && !CheckHashSig(*opt_ptx, dmn->pdmnState->pubKeyOperator.Get(), state)) {
        // pass the state returned by the function above
        return false;
    }

    return true;
}

bool CheckProUpRegTx(const CTransaction& tx, gsl::not_null<const CBlockIndex*> pindexPrev, CDeterministicMNManager& dmnman,
                     const CCoinsViewCache& view, const Consensus::Params& consensus_params, SpecialTxRules rules,
                     TxValidationState& state, bool check_sigs)
{
    const auto opt_ptx = GetValidatedPayload<CProUpRegTx>(tx, pindexPrev, consensus_params, rules.v24, state);
    if (!opt_ptx) {
        // pass the state returned by the function above
        return false;
    }

    auto mnList = dmnman.GetListForBlock(pindexPrev);
    auto dmn = mnList.GetMN(opt_ptx->proTxHash);
    if (!dmn) {
        return state.Invalid(TxValidationResult::TX_CONSENSUS, "bad-protx-hash");
    }

    // Shared masternodes have no registrar owner key; they are updated via ProUpShareTx and
    // ProUpSharedRegTx exclusively
    if (dmn->pdmnState->IsShared()) {
        return state.Invalid(TxValidationResult::TX_BAD_SPECIAL, "bad-protx-shared-mn");
    }

    if (!IsVersionChangeValid(dmn->pdmnState->nVersion, opt_ptx->nVersion, rules.v24, state)) {
        // pass the state returned by the function above
        return false;
    }

    // This update moves the masternode's operator key to a new unique-property slot when it either
    // rotates the key or crosses the legacy->basic scheme boundary (which re-encodes the key). Reject
    // if that target slot is already held by another masternode -- under either encoding -- so the
    // re-key in UpdateMN() cannot collide and throw out of block assembly. Scoped to those two cases
    // so a pre-existing cross-scheme pair's non-migrating routine update is not blocked.
    if (rules.v24) {
        const bool key_changed{!(opt_ptx->pubKeyOperator == dmn->pdmnState->pubKeyOperator)};
        const bool migrating{IsSchemeMigration(dmn->pdmnState->nVersion, opt_ptx->nVersion)};
        if ((key_changed || migrating) &&
            mnList.HasOperatorKeyUnderAnyScheme(opt_ptx->pubKeyOperator.Get(), /*self=*/opt_ptx->proTxHash)) {
            return state.Invalid(TxValidationResult::TX_BAD_SPECIAL, "bad-protx-dup-key");
        }

        // A key change clears the stored addresses, leaving nothing to migrate. Otherwise the
        // existing addresses are carried into the target version's format, which must succeed or
        // BuildNewListFromBlock would reject the block this transaction is placed in.
        if (!key_changed) {
            CDeterministicMNState new_state{*dmn->pdmnState};
            BlockValidationState migration_state;
            if (!SetStateVersion(new_state, std::max<uint16_t>(dmn->pdmnState->nVersion, opt_ptx->nVersion), dmn->nType,
                                 migration_state)) {
                return state.Invalid(TxValidationResult::TX_BAD_SPECIAL, migration_state.GetRejectReason());
            }
        }
    }

    const auto owner_payouts = GetOwnerPayouts(*opt_ptx);
    if (!IsPayoutListTriviallyValid(owner_payouts, dmn->pdmnState->keyIDOwner, opt_ptx->keyIDVoting, state)) return false;
    if (dmn->nType == MnType::Evo && !rules.evo_shares && owner_payouts.size() > 1) {
        return state.Invalid(TxValidationResult::TX_BAD_SPECIAL, "bad-protx-payouts-evo");
    }

    Coin coin;
    if (!view.GetCoin(dmn->collateralOutpoint, coin) || coin.IsSpent()) {
        // this should never happen (there would be no dmn otherwise)
        return state.Invalid(TxValidationResult::TX_CONSENSUS, "bad-protx-collateral");
    }

    // don't allow reuse of collateral key for other keys (don't allow people to put the collateral key onto an online server)
    CTxDestination collateralTxDest;
    if (!ExtractDestination(coin.out.scriptPubKey, collateralTxDest)) {
        return state.Invalid(TxValidationResult::TX_CONSENSUS, "bad-protx-collateral-dest");
    }
    const bool check_payout_collateral_reuse{
        std::max<uint16_t>(dmn->pdmnState->nVersion, opt_ptx->nVersion) >= ProTxVersion::ExtAddr};
    if (!IsPayoutListKeySafe(owner_payouts, collateralTxDest, dmn->pdmnState->keyIDOwner, opt_ptx->keyIDVoting,
                             check_payout_collateral_reuse, state)) return false;

    if (mnList.HasUniqueProperty(opt_ptx->pubKeyOperator)) {
        auto otherDmn = mnList.GetUniquePropertyMN(opt_ptx->pubKeyOperator);
        if (opt_ptx->proTxHash != otherDmn->proTxHash) {
            return state.Invalid(TxValidationResult::TX_BAD_SPECIAL, "bad-protx-dup-key");
        }
    }
    // Cross-scheme duplicates for this update are rejected earlier (see the collision guard after the
    // version-change check), which also covers the key-less migration case.

    if (!DeploymentDIP0003Enforced(pindexPrev->nHeight, Params().GetConsensus())) {
        if (dmn->pdmnState->keyIDOwner != opt_ptx->keyIDVoting) {
            return state.Invalid(TxValidationResult::TX_BAD_SPECIAL, "bad-protx-key-not-same");
        }
    }

    if (!CheckInputsHash(tx, *opt_ptx, state)) {
        // pass the state returned by the function above
        return false;
    }
    if (check_sigs && !CheckHashSig(*opt_ptx, PKHash(dmn->pdmnState->keyIDOwner), state)) {
        // pass the state returned by the function above
        return false;
    }

    return true;
}

bool CheckProUpRevTx(const CTransaction& tx, gsl::not_null<const CBlockIndex*> pindexPrev,
                     CDeterministicMNManager& dmnman, const Consensus::Params& consensus_params, bool is_v24_active,
                     TxValidationState& state, bool check_sigs)
{
    const auto opt_ptx = GetValidatedPayload<CProUpRevTx>(tx, pindexPrev, consensus_params, is_v24_active, state);
    if (!opt_ptx) {
        // pass the state returned by the function above
        return false;
    }

    auto mnList = dmnman.GetListForBlock(pindexPrev);
    auto dmn = mnList.GetMN(opt_ptx->proTxHash);
    if (!dmn) {
        return state.Invalid(TxValidationResult::TX_CONSENSUS, "bad-protx-hash");
    }

    if (!IsVersionChangeValid(dmn->pdmnState->nVersion, opt_ptx->nVersion, is_v24_active, state)) {
        // pass the state returned by the function above
        return false;
    }

    if (!CheckInputsHash(tx, *opt_ptx, state)) {
        // pass the state returned by the function above
        return false;
    }
    if (check_sigs && !CheckHashSig(*opt_ptx, dmn->pdmnState->pubKeyOperator.Get(), state)) {
        // pass the state returned by the function above
        return false;
    }

    return true;
}

bool CheckProUpShareTx(const CTransaction& tx, gsl::not_null<const CBlockIndex*> pindexPrev,
                       CDeterministicMNManager& dmnman, const Consensus::Params& consensus_params, bool is_v24_active,
                       TxValidationState& state, bool check_sigs)
{
    const auto opt_ptx = GetValidatedPayload<CProUpShareTx>(tx, pindexPrev, consensus_params, is_v24_active, state);
    if (!opt_ptx) {
        // pass the state returned by the function above
        return false;
    }
    if (!is_v24_active) {
        return state.Invalid(TxValidationResult::TX_CONSENSUS, "bad-proupshare-too-early");
    }

    const auto mnList = dmnman.GetListForBlock(pindexPrev);
    const auto dmn = mnList.GetMN(opt_ptx->proTxHash);
    if (!dmn) {
        return state.Invalid(TxValidationResult::TX_BAD_SPECIAL, "bad-proupshare-hash");
    }
    const auto& mnState = *dmn->pdmnState;
    if (!mnState.IsShared()) {
        return state.Invalid(TxValidationResult::TX_BAD_SPECIAL, "bad-proupshare-not-shared");
    }
    if (opt_ptx->shareIndex >= mnState.shares.size()) {
        return state.Invalid(TxValidationResult::TX_BAD_SPECIAL, "bad-proupshare-index");
    }

    // The new reward script is subject to the same payee-reuse restrictions as at registration
    CTxDestination dest;
    if (!ExtractDestination(opt_ptx->scriptReward, dest)) {
        return state.Invalid(TxValidationResult::TX_BAD_SPECIAL, "bad-proupshare-payee-dest");
    }
    if (dest == CTxDestination(PKHash(mnState.keyIDVoting))) {
        return state.Invalid(TxValidationResult::TX_BAD_SPECIAL, "bad-proupshare-payee-reuse");
    }
    for (const auto& share : mnState.shares) {
        if (dest == CTxDestination(PKHash(share.keyIDOwner))) {
            return state.Invalid(TxValidationResult::TX_BAD_SPECIAL, "bad-proupshare-payee-reuse");
        }
    }

    if (!CheckInputsHash(tx, *opt_ptx, state)) {
        // pass the state returned by the function above
        return false;
    }
    if (check_sigs) {
        if (std::string strError;
            !CHashSigner::VerifyHashCanonical(::SerializeHash(*opt_ptx),
                                              mnState.shares[opt_ptx->shareIndex].keyIDOwner, opt_ptx->vchSig,
                                              strError)) {
            return state.Invalid(TxValidationResult::TX_BAD_SPECIAL, "bad-proupshare-sig");
        }
    }

    return true;
}

bool CheckProUpSharedRegTx(const CTransaction& tx, gsl::not_null<const CBlockIndex*> pindexPrev,
                           CDeterministicMNManager& dmnman, const Consensus::Params& consensus_params,
                           bool is_v24_active, TxValidationState& state, bool check_sigs)
{
    const auto opt_ptx = GetValidatedPayload<CProUpSharedRegTx>(tx, pindexPrev, consensus_params, is_v24_active,
                                                                state);
    if (!opt_ptx) {
        // pass the state returned by the function above
        return false;
    }
    if (!is_v24_active) {
        return state.Invalid(TxValidationResult::TX_CONSENSUS, "bad-proupsharedreg-too-early");
    }

    const auto mnList = dmnman.GetListForBlock(pindexPrev);
    const auto dmn = mnList.GetMN(opt_ptx->proTxHash);
    if (!dmn) {
        return state.Invalid(TxValidationResult::TX_BAD_SPECIAL, "bad-proupsharedreg-hash");
    }
    const auto& mnState = *dmn->pdmnState;
    if (!mnState.IsShared()) {
        return state.Invalid(TxValidationResult::TX_BAD_SPECIAL, "bad-proupsharedreg-not-shared");
    }

    // A shared registrar update requires unanimity: one signature per share, in share order
    if (opt_ptx->vchSigs.size() != mnState.shares.size()) {
        return state.Invalid(TxValidationResult::TX_BAD_SPECIAL, "bad-proupsharedreg-sig-count");
    }

    // Same rule as the ordinary registrar path: the legacy and basic encodings of one BLS key
    // occupy different unique-property slots, so probe both
    if (mnList.HasOperatorKeyUnderAnyScheme(opt_ptx->pubKeyOperator.Get(), /*self=*/opt_ptx->proTxHash)) {
        return state.Invalid(TxValidationResult::TX_BAD_SPECIAL, "bad-protx-dup-key");
    }

    // The new voting key is subject to the same payee-separation rule as at registration
    if (!IsShareListVotingKeySafe(mnState.shares, opt_ptx->keyIDVoting)) {
        return state.Invalid(TxValidationResult::TX_BAD_SPECIAL, "bad-proupsharedreg-payee-reuse");
    }

    if (!CheckInputsHash(tx, *opt_ptx, state)) {
        // pass the state returned by the function above
        return false;
    }
    if (check_sigs) {
        const uint256 payload_hash = ::SerializeHash(*opt_ptx);
        for (size_t i = 0; i < mnState.shares.size(); i++) {
            if (std::string strError; !CHashSigner::VerifyHashCanonical(payload_hash, mnState.shares[i].keyIDOwner,
                                                                        opt_ptx->vchSigs[i], strError)) {
                return state.Invalid(TxValidationResult::TX_BAD_SPECIAL, "bad-proupsharedreg-sig");
            }
        }
    }

    return true;
}

bool CheckProDisTxForList(const CTransaction& tx, const CProDisTx& ptx, const CDeterministicMNList& mnList,
                          int nSpendHeight, TxValidationState& state, bool check_sigs)
{
    const auto dmn = mnList.GetMN(ptx.proTxHash);
    if (!dmn) {
        return state.Invalid(TxValidationResult::TX_BAD_SPECIAL, "bad-prodis-hash");
    }
    const auto& mnState = *dmn->pdmnState;
    if (!mnState.IsShared()) {
        return state.Invalid(TxValidationResult::TX_BAD_SPECIAL, "bad-prodis-not-shared");
    }
    const auto& shares = mnState.shares;

    // Exactly one input: the masternode's collateral outpoint, with an empty scriptSig (the
    // empty-scriptSig rule keeps the txid non-malleable; the template script needs no witness)
    if (tx.vin.size() != 1 || tx.vin[0].prevout != dmn->collateralOutpoint || !tx.vin[0].scriptSig.empty()) {
        return state.Invalid(TxValidationResult::TX_BAD_SPECIAL, "bad-prodis-input");
    }
    if (ptx.actorIndex >= shares.size()) {
        return state.Invalid(TxValidationResult::TX_BAD_SPECIAL, "bad-prodis-actor");
    }

    // The signature count defines the mode: exactly one (unilateral, by the actor) or exactly one
    // per share in share order (unanimous). shares.size() >= 2, so the two cannot coincide.
    if (ptx.vchSigs.size() != 1 && ptx.vchSigs.size() != shares.size()) {
        return state.Invalid(TxValidationResult::TX_BAD_SPECIAL, "bad-prodis-sig-count");
    }
    const bool unanimous{ptx.vchSigs.size() == shares.size()};

    // requiredPenalty is non-increasing in nSpendHeight, which makes ProDisTx validity monotone:
    // once valid, always valid (see the DIP's "Output rules" section)
    const bool early{nSpendHeight - mnState.nRegisteredHeight < static_cast<int64_t>(mnState.nEarlyPeriodBlocks)};
    const CAmount required_penalty{(unanimous || !early) ? 0 : mnState.nEarlyPenalty};

    // One output per non-actor share, paying its refund script, in share order, plus optionally
    // one final actor output. A zero-value actor output must be omitted. No other outputs.
    const size_t non_actor_count{shares.size() - 1};
    if (tx.vout.size() != non_actor_count && tx.vout.size() != non_actor_count + 1) {
        return state.Invalid(TxValidationResult::TX_BAD_SPECIAL, "bad-prodis-payee-count");
    }

    CAmount non_actor_total{0};
    for (size_t i = 0; i < shares.size(); i++) {
        if (i != ptx.actorIndex) non_actor_total += shares[i].amount;
    }

    size_t out_idx{0};
    CAmount bonus_total{0};
    for (size_t i = 0; i < shares.size(); i++) {
        if (i == ptx.actorIndex) continue;
        const auto& out = tx.vout[out_idx++];
        if (out.scriptPubKey != shares[i].scriptRefund) {
            return state.Invalid(TxValidationResult::TX_BAD_SPECIAL, "bad-prodis-payee");
        }
        // Minimum-based rules: bonus[i] >= floor(P * amount[i] / W). The floor is element-wise
        // monotone in P, which is what preserves monotone validity; overpaying is always valid.
        const CAmount bonus{out.nValue - shares[i].amount};
        if (bonus < ProRataFloor(required_penalty, shares[i].amount, non_actor_total)) {
            return state.Invalid(TxValidationResult::TX_BAD_SPECIAL, "bad-prodis-penalty-floor");
        }
        bonus_total += bonus;
    }
    if (tx.vout.size() == non_actor_count + 1) {
        const auto& actor_out = tx.vout.back();
        if (actor_out.scriptPubKey != shares[ptx.actorIndex].scriptRefund) {
            return state.Invalid(TxValidationResult::TX_BAD_SPECIAL, "bad-prodis-payee");
        }
        if (actor_out.nValue == 0) {
            return state.Invalid(TxValidationResult::TX_BAD_SPECIAL, "bad-prodis-actor-output-zero");
        }
    }
    // The per-recipient floors discard sub-duff remainders; the bonus sum rule forces the
    // remainder onto some non-actor output
    if (bonus_total < required_penalty) {
        return state.Invalid(TxValidationResult::TX_BAD_SPECIAL, "bad-prodis-penalty-sum");
    }
    // Everything not paid to the outputs is fee, which by value conservation comes from the
    // actor's share. The fee ceiling (both modes) and the unilateral bonus ceiling close the two
    // paths by which a stolen share owner key could drain the actor's principal beyond the
    // consented penalty; both ceilings are height-independent, so validity stays monotone. A
    // unanimous dissolution carries every owner's signature over the exact outputs, so bonuses
    // beyond the configured penalty are consented and remain valid.
    CAmount out_total{0};
    for (const auto& out : tx.vout) {
        out_total += out.nValue;
    }
    if (non_actor_total + shares[ptx.actorIndex].amount - out_total > CProDisTx::MAX_FEE) {
        return state.Invalid(TxValidationResult::TX_BAD_SPECIAL, "bad-prodis-fee");
    }
    if (!unanimous && bonus_total > mnState.nEarlyPenalty) {
        return state.Invalid(TxValidationResult::TX_BAD_SPECIAL, "bad-prodis-bonus");
    }

    if (check_sigs) {
        const uint256 sign_hash{ptx.MakeSignHash(tx, static_cast<uint8_t>(ptx.vchSigs.size()))};
        if (unanimous) {
            for (size_t i = 0; i < shares.size(); i++) {
                if (std::string strError; !CHashSigner::VerifyHashCanonical(sign_hash, shares[i].keyIDOwner,
                                                                            ptx.vchSigs[i], strError)) {
                    return state.Invalid(TxValidationResult::TX_BAD_SPECIAL, "bad-prodis-sig");
                }
            }
        } else {
            if (std::string strError; !CHashSigner::VerifyHashCanonical(sign_hash, shares[ptx.actorIndex].keyIDOwner,
                                                                        ptx.vchSigs[0], strError)) {
                return state.Invalid(TxValidationResult::TX_BAD_SPECIAL, "bad-prodis-sig");
            }
        }
    }

    return true;
}

bool CheckProDisTx(const CTransaction& tx, gsl::not_null<const CBlockIndex*> pindexPrev,
                   CDeterministicMNManager& dmnman, const Consensus::Params& consensus_params, bool is_v24_active,
                   TxValidationState& state, bool check_sigs)
{
    const auto opt_ptx = GetValidatedPayload<CProDisTx>(tx, pindexPrev, consensus_params, is_v24_active, state);
    if (!opt_ptx) {
        // pass the state returned by the function above
        return false;
    }
    if (!is_v24_active) {
        return state.Invalid(TxValidationResult::TX_CONSENSUS, "bad-prodis-too-early");
    }

    // The masternode must exist at the previous block: registering and dissolving a shared
    // masternode in the same block is deliberately invalid (as is any same-block
    // ProUpShareTx/ProUpSharedRegTx for a just-registered masternode). Everything a dissolution
    // is validated against (refund scripts, amounts, registration height) is immutable after
    // registration, so this pindexPrev-based check is complete and blocks need no re-validation
    // against the evolving list.
    const auto mnList = dmnman.GetListForBlock(pindexPrev);
    if (!mnList.GetMN(opt_ptx->proTxHash)) {
        return state.Invalid(TxValidationResult::TX_BAD_SPECIAL, "bad-prodis-hash");
    }
    return CheckProDisTxForList(tx, *opt_ptx, mnList, pindexPrev->nHeight + 1, state, check_sigs);
}

bool CheckSharedCollateralSpends(const CTransaction& tx, const CCoinsViewCache& view, TxValidationState& state)
{
    if (tx.IsCoinBase()) return true;
    for (const auto& in : tx.vin) {
        // The template is anyone-can-spend at the script layer, so this consensus-level rule is
        // the only thing restricting spends: only a ProDisTx may consume a template output. The
        // ProDisTx's own validation then ties the outpoint to its masternode. Pre-activation
        // template outputs belong to no masternode and are permanently frozen by this rule.
        const Coin& coin = view.AccessCoin(in.prevout);
        if (!coin.IsSpent() && IsSharedCollateralScript(coin.out.scriptPubKey)) {
            if (!tx.IsSpecialTxVersion() || tx.nType != TRANSACTION_PROVIDER_DISSOLVE) {
                return state.Invalid(TxValidationResult::TX_CONSENSUS, "bad-shared-collateral-spend");
            }
        }
    }
    return true;
}

bool CheckSharedCollateralTemplateOutputs(const CTransaction& tx, TxValidationState& state)
{
    for (uint32_t i = 0; i < tx.vout.size(); i++) {
        if (!IsSharedCollateralScript(tx.vout[i].scriptPubKey)) continue;
        // An output paying the template script is only valid as the collateral output of a shared
        // registration. Anywhere else it would either escape the spend covenant or freeze funds.
        if (!tx.IsSpecialTxVersion() || tx.nType != TRANSACTION_PROVIDER_REGISTER) {
            return state.Invalid(TxValidationResult::TX_CONSENSUS, "bad-shared-collateral-create");
        }
        const auto opt_ptx = GetTxPayload<CProRegTx>(tx);
        if (!opt_ptx || !opt_ptx->IsShared() || !opt_ptx->collateralOutpoint.hash.IsNull() ||
            opt_ptx->collateralOutpoint.n != i) {
            return state.Invalid(TxValidationResult::TX_CONSENSUS, "bad-shared-collateral-create");
        }
    }
    return true;
}

bool IsStandardSpecialTx(const CTransaction& tx, std::string& reason)
{
    if (!tx.IsSpecialTxVersion()) return true;

    if (tx.nType != TRANSACTION_ASSET_LOCK) return true;

    // Each input is referenced by Platform's funding state transition; beyond this
    // many inputs that state transition exceeds Platform's ~20 kB size limit.
    static constexpr size_t MAX_STANDARD_ASSET_LOCK_INPUTS{100};
    if (tx.vin.size() > MAX_STANDARD_ASSET_LOCK_INPUTS) {
        reason = "assetlocktx-too-many-inputs";
        return false;
    }

    constexpr int max_tx_size_for_platform = 20480;
    if (tx.GetTotalSize() > max_tx_size_for_platform) {
        reason = "assetlocktx-too-big";
        return false;
    }

    if (const auto opt_assetLockTx = GetTxPayload<CAssetLockPayload>(tx);
        opt_assetLockTx.has_value() && opt_assetLockTx->getVersion() >= 2) {
        reason = "assetlocktx-version-2";
        return false;
    }

    return true;
}
