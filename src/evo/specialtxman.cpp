// Copyright (c) 2018-2025 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <evo/specialtxman.h>

#include <chainparams.h>
#include <consensus/amount.h>
#include <consensus/validation.h>
#include <deploymentstatus.h>
#include <hash.h>
#include <primitives/block.h>
#include <util/irange.h>
#include <validation.h>

#include <chainlock/chainlock.h>
#include <chainlock/clsig.h>
#include <evo/assetlocktx.h>
#include <evo/cbtx.h>
#include <evo/creditpool.h>
#include <evo/deterministicmns.h>
#include <evo/mnhftx.h>
#include <evo/simplifiedmns.h>
#include <llmq/blockprocessor.h>
#include <llmq/commitment.h>
#include <llmq/quorums.h>
#include <llmq/utils.h>

static bool CheckCbTxBestChainlock(const CCbTx& cbTx, const CBlockIndex* pindex,
                                   const llmq::CChainLocksHandler& chainlock_handler, BlockValidationState& state)
{
    if (cbTx.nVersion < CCbTx::Version::CLSIG_AND_BALANCE) {
        return true;
    }

    static Mutex cached_mutex;
    static const CBlockIndex* cached_pindex GUARDED_BY(cached_mutex){nullptr};
    static std::optional<std::pair<CBLSSignature, uint32_t>> cached_chainlock GUARDED_BY(cached_mutex){std::nullopt};

    auto best_clsig = chainlock_handler.GetBestChainLock();
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
        int curBlockCoinbaseCLHeight = pindex->nHeight - static_cast<int>(cbTx.bestCLHeightDiff) - 1;
        if (best_clsig.getHeight() == curBlockCoinbaseCLHeight && best_clsig.getSig() == cbTx.bestCLSignature) {
            // matches our best (but outdated) clsig, no need to verify it again
            LOCK(cached_mutex);
            cached_chainlock = std::make_pair(cbTx.bestCLSignature, cbTx.bestCLHeightDiff);
            cached_pindex = pindex;
            return true;
        }
        uint256 curBlockCoinbaseCLBlockHash = pindex->GetAncestor(curBlockCoinbaseCLHeight)->GetBlockHash();
        if (chainlock_handler.VerifyChainLock(
                chainlock::ChainLockSig(curBlockCoinbaseCLHeight, curBlockCoinbaseCLBlockHash, cbTx.bestCLSignature)) !=
            llmq::VerifyRecSigStatus::Valid) {
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

static bool CheckSpecialTxInner(CDeterministicMNManager& dmnman, llmq::CQuorumSnapshotManager& qsnapman,
                                const ChainstateManager& chainman, const llmq::CQuorumManager& qman,
                                const CTransaction& tx, const CBlockIndex* pindexPrev, const CCoinsViewCache& view,
                                const std::optional<CRangesSet>& indexes, bool check_sigs, TxValidationState& state)
    EXCLUSIVE_LOCKS_REQUIRED(::cs_main)
{
    AssertLockHeld(::cs_main);

    if (!tx.HasExtraPayloadField())
        return true;

    const auto& consensusParams = Params().GetConsensus();
    if (!DeploymentActiveAfter(pindexPrev, consensusParams, Consensus::DEPLOYMENT_DIP0003)) {
        return state.Invalid(TxValidationResult::TX_BAD_SPECIAL, "bad-tx-type");
    }

    try {
        switch (tx.nType) {
        case TRANSACTION_PROVIDER_REGISTER:
            return CheckProRegTx(dmnman, tx, pindexPrev, state, view, check_sigs);
        case TRANSACTION_PROVIDER_UPDATE_SERVICE:
            return CheckProUpServTx(dmnman, tx, pindexPrev, state, check_sigs);
        case TRANSACTION_PROVIDER_UPDATE_REGISTRAR:
            return CheckProUpRegTx(dmnman, tx, pindexPrev, state, view, check_sigs);
        case TRANSACTION_PROVIDER_UPDATE_REVOKE:
            return CheckProUpRevTx(dmnman, tx, pindexPrev, state, check_sigs);
        case TRANSACTION_COINBASE: {
            if (!tx.IsCoinBase()) {
                return state.Invalid(TxValidationResult::TX_CONSENSUS, "bad-cbtx-invalid");
            }
            if (const auto opt_cbTx = GetTxPayload<CCbTx>(tx)) {
                return CheckCbTx(*opt_cbTx, pindexPrev, state);
            } else {
                return state.Invalid(TxValidationResult::TX_CONSENSUS, "bad-cbtx-payload");
            }
        }
        case TRANSACTION_QUORUM_COMMITMENT:
            return llmq::CheckLLMQCommitment(dmnman, qsnapman, chainman, tx, pindexPrev, state);
        case TRANSACTION_MNHF_SIGNAL:
            return CheckMNHFTx(chainman, qman, tx, pindexPrev, state);
        case TRANSACTION_ASSET_LOCK:
            return CheckAssetLockTx(tx, state);
        case TRANSACTION_ASSET_UNLOCK:
            return CheckAssetUnlockTx(chainman.m_blockman, qman, tx, pindexPrev, indexes, state);
        }
    } catch (const std::exception& e) {
        LogPrintf("%s -- failed: %s\n", __func__, e.what());
        return state.Invalid(TxValidationResult::TX_CONSENSUS, "failed-check-special-tx");
    }

    return state.Invalid(TxValidationResult::TX_BAD_SPECIAL, "bad-tx-type-check");
}

bool CSpecialTxProcessor::CheckSpecialTx(const CTransaction& tx, const CBlockIndex* pindexPrev, const CCoinsViewCache& view, bool check_sigs, TxValidationState& state)
{
    AssertLockHeld(::cs_main);
    return CheckSpecialTxInner(m_dmnman, m_qsnapman, m_chainman, m_qman, tx, pindexPrev, view, std::nullopt, check_sigs,
                               state);
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
                                                const CCoinsViewCache& view, bool debugLogs,
                                                BlockValidationState& state, CDeterministicMNList& mnListRet)
{
    AssertLockHeld(cs_main);

    int nHeight = pindexPrev->nHeight + 1;

    CDeterministicMNList oldList = m_dmnman.GetListForBlock(pindexPrev);
    CDeterministicMNList newList = oldList;
    newList.SetBlockHash(uint256()); // we can't know the final block hash, so better not return a (invalid) block hash
    newList.SetHeight(nHeight);

    auto payee = oldList.GetMNPayee(pindexPrev);

    // we iterate the oldList here and update the newList
    // this is only valid as long these have not diverged at this point, which is the case as long as we don't add
    // code above this loop that modifies newList
    oldList.ForEachMN(false, [&pindexPrev, &newList, this](auto& dmn) {
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

    const bool isMNRewardReallocation{DeploymentActiveAfter(pindexPrev, m_consensus_params, Consensus::DEPLOYMENT_MN_RR)};
    const bool is_v23_deployed{DeploymentActiveAfter(pindexPrev, m_consensus_params, Consensus::DEPLOYMENT_V23)};

    // we skip the coinbase
    for (int i = 1; i < (int)block.vtx.size(); i++) {
        const CTransaction& tx = *block.vtx[i];

        if (!tx.IsSpecialTxVersion()) {
            // only interested in special TXs
            continue;
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

            Coin coin;
            CAmount expectedCollateral = GetMnType(proTx.nType).collat_amount;
            if (!proTx.collateralOutpoint.hash.IsNull() && (!view.GetCoin(dmn->collateralOutpoint, coin) ||
                                                            coin.IsSpent() || coin.out.nValue != expectedCollateral)) {
                // should actually never get to this point as CheckProRegTx should have handled this case.
                // We do this additional check nevertheless to be 100% sure
                return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "bad-protx-collateral");
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
                              nHeight, newList.GetAllMNsCount());
                }
            }

            for (const NetInfoEntry& entry : proTx.netInfo->GetEntries()) {
                if (const auto& service_opt{entry.GetAddrPort()}; service_opt.has_value()) {
                    const CService& service{service_opt.value()};
                    if (newList.HasUniqueProperty(service)) {
                        return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "bad-protx-dup-netinfo-entry");
                    }
                } else {
                    return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "bad-protx-netinfo-entry");
                }
            }
            if (newList.HasUniqueProperty(proTx.keyIDOwner) || newList.HasUniqueProperty(proTx.pubKeyOperator)) {
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

            for (const NetInfoEntry& entry : opt_proTx->netInfo->GetEntries()) {
                if (const auto& service_opt{entry.GetAddrPort()}; service_opt.has_value()) {
                    const CService& service{service_opt.value()};
                    if (newList.HasUniqueProperty(service) &&
                        newList.GetUniquePropertyMN(service)->proTxHash != opt_proTx->proTxHash) {
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
            if (!IsValidMnType(opt_proTx->nType)) {
                return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "bad-protx-type");
            }

            auto newState = std::make_shared<CDeterministicMNState>(*dmn->pdmnState);
            if (is_v23_deployed) {
                // Extended addresses support in v23 means that the version can be updated
                newState->nVersion = opt_proTx->nVersion;
            }
            newState->netInfo = opt_proTx->netInfo;
            newState->scriptOperatorPayout = opt_proTx->scriptOperatorPayout;
            if (opt_proTx->nType == MnType::Evo) {
                newState->platformNodeID = opt_proTx->platformNodeID;
                newState->platformP2PPort = opt_proTx->platformP2PPort;
                newState->platformHTTPPort = opt_proTx->platformHTTPPort;
            }
            if (newState->IsBanned()) {
                // only revive when all keys are set
                if (newState->pubKeyOperator != CBLSLazyPublicKey() && !newState->keyIDVoting.IsNull() &&
                    !newState->keyIDOwner.IsNull()) {
                    newState->Revive(nHeight);
                    if (debugLogs) {
                        LogPrintf("%s -- MN %s revived at height %d\n", __func__, opt_proTx->proTxHash.ToString(), nHeight);
                    }
                }
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
            auto newState = std::make_shared<CDeterministicMNState>(*dmn->pdmnState);
            if (newState->pubKeyOperator != opt_proTx->pubKeyOperator) {
                // reset all operator related fields and put MN into PoSe-banned state in case the operator key changes
                newState->ResetOperatorFields();
                newState->BanIfNotBanned(nHeight);
                // we update pubKeyOperator here, make sure state version matches
                // Make sure we don't accidentally downgrade the state version if using version after basic BLS
                newState->nVersion = newState->nVersion > ProTxVersion::BasicBLS ? newState->nVersion : opt_proTx->nVersion;
                newState->netInfo = NetInfoInterface::MakeNetInfo(newState->nVersion);
                newState->pubKeyOperator = opt_proTx->pubKeyOperator;
            }
            newState->keyIDVoting = opt_proTx->keyIDVoting;
            newState->scriptPayout = opt_proTx->scriptPayout;

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
            newState->ResetOperatorFields();
            newState->BanIfNotBanned(nHeight);
            newState->nRevocationReason = opt_proTx->nReason;

            newList.UpdateMN(opt_proTx->proTxHash, newState);

            if (debugLogs) {
                LogPrintf("%s -- MN %s revoked operator key at height %d: %s\n", __func__,
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

                const auto members = llmq::utils::GetAllQuorumMembers(opt_qc->commitment.llmqType, m_dmnman, m_qsnapman,
                                                                      pQuorumBaseBlockIndex);
                HandleQuorumCommitment(opt_qc->commitment, members, debugLogs, newList);
            }
        }
    }

    // we skip the coinbase
    for (int i = 1; i < (int)block.vtx.size(); i++) {
        const CTransaction& tx = *block.vtx[i];

        // check if any existing MN collateral is spent by this transaction
        for (const auto& in : tx.vin) {
            auto dmn = newList.GetMNByCollateral(in.prevout);
            if (dmn && dmn->collateralOutpoint == in.prevout) {
                newList.RemoveMN(dmn->proTxHash);

                if (debugLogs) {
                    LogPrintf("%s -- MN %s removed from list because collateral was spent. " /* Continued */
                              "collateralOutpoint=%s, nHeight=%d, mapCurMNs.allMNsCount=%d\n",
                              __func__, dmn->proTxHash.ToString(), dmn->collateralOutpoint.ToStringShort(), nHeight,
                              newList.GetAllMNsCount());
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
    newList2.ForEachMN(false, [&](auto& dmn) {
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

    mnListRet = std::move(newList);

    return true;
}

bool CSpecialTxProcessor::ProcessSpecialTxsInBlock(const CBlock& block, const CBlockIndex* pindex, const CCoinsViewCache& view, bool fJustCheck,
                                                   bool fCheckCbTxMerkleRoots, BlockValidationState& state, std::optional<MNListUpdates>& updatesRet)
{
    AssertLockHeld(::cs_main);

    try {
        static int64_t nTimeLoop = 0;
        static int64_t nTimeQuorum = 0;
        static int64_t nTimeDMN = 0;
        static int64_t nTimeMerkleMNL = 0;
        static int64_t nTimeMerkleQuorums = 0;
        static int64_t nTimeCbTxCL = 0;
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
                if (!CheckCbTx(*opt_cbTx, pindex->pprev, tx_state)) {
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
            CCreditPool creditPool{m_cpoolman.GetCreditPool(pindex->pprev, m_consensus_params)};
            LogPrint(BCLog::CREDITPOOL, "CSpecialTxProcessor::%s -- CCreditPool is %s\n", __func__, creditPool.ToString());
            indexes = std::move(creditPool.indexes);
        }

        for (size_t i = 0; i < block.vtx.size(); ++i) {
            // we validated CCbTx above, starts from the 2nd transaction
            if (i == 0 && block.vtx[i]->nType == TRANSACTION_COINBASE) continue;

            const auto ptr_tx = block.vtx[i];
            TxValidationState tx_state;
            // At this moment CheckSpecialTx() may fail by 2 possible ways:
            // consensus failures and "TX_BAD_SPECIAL"
            if (!CheckSpecialTxInner(m_dmnman, m_qsnapman, m_chainman, m_qman, *ptr_tx, pindex->pprev, view, indexes,
                                     fCheckCbTxMerkleRoots, tx_state)) {
                assert(tx_state.GetResult() == TxValidationResult::TX_CONSENSUS || tx_state.GetResult() == TxValidationResult::TX_BAD_SPECIAL);
                return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, tx_state.GetRejectReason(),
                                 strprintf("Special Transaction check failed (tx hash %s) %s", ptr_tx->GetHash().ToString(), tx_state.GetDebugMessage()));
            }
        }

        int64_t nTime3 = GetTimeMicros();
        nTimeLoop += nTime3 - nTime2;
        LogPrint(BCLog::BENCHMARK, "      - Loop: %.2fms [%.2fs]\n", 0.001 * (nTime3 - nTime2), nTimeLoop * 0.000001);

        if (opt_cbTx.has_value()) {
            if (!CheckCreditPoolDiffForBlock(block, pindex, *opt_cbTx, state)) {
                return error("CSpecialTxProcessor: CheckCreditPoolDiffForBlock for block %s failed with %s",
                             pindex->GetBlockHash().ToString(), state.ToString());
            }
        }

        int64_t nTime4 = GetTimeMicros();
        nTimeCreditPool += nTime4 - nTime3;
        LogPrint(BCLog::BENCHMARK, "      - CheckCreditPoolDiffForBlock: %.2fms [%.2fs]\n", 0.001 * (nTime4 - nTime3),
                 nTimeCreditPool * 0.000001);

        if (!m_qblockman.ProcessBlock(block, pindex, state, fJustCheck, fCheckCbTxMerkleRoots)) {
            // pass the state returned by the function above
            return false;
        }

        int64_t nTime5 = GetTimeMicros();
        nTimeQuorum += nTime5 - nTime4;
        LogPrint(BCLog::BENCHMARK, "      - m_qblockman.ProcessBlock: %.2fms [%.2fs]\n", 0.001 * (nTime5 - nTime4),
                 nTimeQuorum * 0.000001);

        CDeterministicMNList mn_list;
        if (DeploymentActiveAt(*pindex, m_consensus_params, Consensus::DEPLOYMENT_DIP0003)) {
            if (!BuildNewListFromBlock(block, pindex->pprev, view, true, state, mn_list)) {
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

            if (!CheckCbTxBestChainlock(*opt_cbTx, pindex, m_clhandler, state)) {
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
    } catch (const std::exception& e) {
        LogPrintf("CSpecialTxProcessor::%s -- FAILURE! %s\n", __func__, e.what());
        return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "failed-procspectxsinblock");
    }

    return true;
}

bool CSpecialTxProcessor::UndoSpecialTxsInBlock(const CBlock& block, const CBlockIndex* pindex, std::optional<MNListUpdates>& updatesRet)
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

        if (!m_qblockman.UndoBlock(block, pindex)) {
            return false;
        }
    } catch (const std::exception& e) {
        bls::bls_legacy_scheme.store(bls_legacy_scheme);
        LogPrintf("CSpecialTxProcessor::%s -- bls_legacy_scheme=%d\n", __func__, bls::bls_legacy_scheme.load());
        return error(strprintf("CSpecialTxProcessor::%s -- FAILURE! %s\n", __func__, e.what()).c_str());
    }

    return true;
}

bool CSpecialTxProcessor::CheckCreditPoolDiffForBlock(const CBlock& block, const CBlockIndex* pindex, const CCbTx& cbTx,
                                                      BlockValidationState& state)
{
    AssertLockHeld(::cs_main);

    if (!DeploymentActiveAt(*pindex, m_consensus_params, Consensus::DEPLOYMENT_DIP0008)) return true;
    if (!DeploymentActiveAt(*pindex, m_consensus_params, Consensus::DEPLOYMENT_V20)) return true;

    try {
        const CAmount blockSubsidy = GetBlockSubsidy(pindex, m_consensus_params);
        const auto creditPoolDiff = GetCreditPoolDiffForBlock(m_cpoolman, m_chainman.m_blockman, m_qman, block,
                                                              pindex->pprev, m_consensus_params, blockSubsidy, state);
        if (!creditPoolDiff.has_value()) return false;

        const CAmount target_balance{cbTx.creditPoolBalance};
        // But it maybe not included yet in previous block yet; in this case value must be 0
        const CAmount locked_calculated{creditPoolDiff->GetTotalLocked()};
        if (target_balance != locked_calculated) {
            LogPrintf("CSpecialTxProcessor::%s -- mismatched locked amount in CbTx: %lld against re-calculated: %lld\n", __func__, target_balance, locked_calculated);
            return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "bad-cbtx-assetlocked-amount");
        }

    } catch (const std::exception& e) {
        LogPrintf("CSpecialTxProcessor::%s -- FAILURE! %s\n", __func__, e.what());
        return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "failed-checkcreditpooldiff");
    }

    return true;
}
