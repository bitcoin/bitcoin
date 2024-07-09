// Copyright (c) 2018-2019 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <llmq/quorums_blockprocessor.h>
#include <llmq/quorums_commitment.h>
#include <llmq/quorums_debug.h>
#include <llmq/quorums_utils.h>

#include <evo/evodb.h>
#include <evo/specialtx.h>

#include <chain.h>
#include <chainparams.h>
#include <consensus/params.h>
#include <consensus/validation.h>
#include <net.h>
#include <net_processing.h>
#include <primitives/block.h>
#include <primitives/transaction.h>
#include <node/blockstorage.h>
#include <validation.h>
#include <saltedhasher.h>
#include <sync.h>
#include <map>
#include <logging.h>
namespace llmq
{

CQuorumBlockProcessor* quorumBlockProcessor;


CQuorumBlockProcessor::CQuorumBlockProcessor(const DBParams& db_commitment_params, PeerManager &_peerman, ChainstateManager& _chainman) : peerman(_peerman), chainman(_chainman), m_commitment_evoDb(db_commitment_params, 10)
{
}

void CQuorumBlockProcessor::ProcessMessage(CNode* pfrom, const std::string& strCommand, CDataStream& vRecv, PeerManager& peerman)
{
    if (strCommand == NetMsgType::QFCOMMITMENT) {
        CFinalCommitment qc;
        vRecv >> qc;

        const uint256& hash = ::SerializeHash(qc);
        PeerRef peer = peerman.GetPeerRef(pfrom->GetId());
        if (peer)
            peerman.AddKnownTx(*peer, hash);
        {
            LOCK(cs_main);
            peerman.ReceivedResponse(pfrom->GetId(), hash);
        }

        if (qc.IsNull()) {
            LogPrint(BCLog::LLMQ, "CQuorumBlockProcessor::%s -- null commitment from peer=%d\n", __func__, pfrom->GetId());
            {
                LOCK(cs_main);
                peerman.ForgetTxHash(pfrom->GetId(), hash);
            }
            if(peer)
                peerman.Misbehaving(*peer, 100, "null commitment from peer");
            return;
        }

        const auto& params = Params().GetConsensus().llmqTypeChainLocks;
        // Verify that quorumHash is part of the active chain and that it's the first block in the DKG interval
        const CBlockIndex* pQuorumBaseBlockIndex;
        {
            LOCK(cs_main);
            pQuorumBaseBlockIndex = chainman.m_blockman.LookupBlockIndex(qc.quorumHash);
            if (!pQuorumBaseBlockIndex) {
                LogPrint(BCLog::LLMQ, "CQuorumBlockProcessor::%s -- unknown block %s in commitment, peer=%d\n", __func__,
                        qc.quorumHash.ToString(), pfrom->GetId());
                // can't really punish the node here, as we might simply be the one that is on the wrong chain or not
                // fully synced
                return;
            }
            if (chainman.ActiveTip()->GetAncestor(pQuorumBaseBlockIndex->nHeight) != pQuorumBaseBlockIndex) {
                LogPrint(BCLog::LLMQ, "CQuorumBlockProcessor::%s -- block %s not in active chain, peer=%d\n", __func__,
                          qc.quorumHash.ToString(), pfrom->GetId());
                // same, can't punish
                return;
            }
            int quorumHeight = pQuorumBaseBlockIndex->nHeight - (pQuorumBaseBlockIndex->nHeight % params.dkgInterval);
            if (quorumHeight != pQuorumBaseBlockIndex->nHeight) {
                LogPrint(BCLog::LLMQ, "CQuorumBlockProcessor::%s -- block %s is not the first block in the DKG interval, peer=%d\n", __func__,
                            qc.quorumHash.ToString(), pfrom->GetId());
                peerman.ForgetTxHash(pfrom->GetId(), hash);
                if(peer)
                    peerman.Misbehaving(*peer, 100, "not in first block of DKG interval");
                return;
            }
            if (pQuorumBaseBlockIndex->nHeight < (chainman.ActiveHeight() - params.dkgInterval)) {
                LogPrint(BCLog::LLMQ, "CQuorumBlockProcessor::%s -- block %s is too old, peer=%d\n", __func__,
                        qc.quorumHash.ToString(), pfrom->GetId());
                  peerman.ForgetTxHash(pfrom->GetId(), hash);
                if(peer)
                    peerman.Misbehaving(*peer, 100, "block of DKG interval too old");
                return;
            }
            if (HasMinedCommitment(qc.quorumHash)) {
                LogPrint(BCLog::LLMQ, "CQuorumBlockProcessor::%s -- commitment for quorum hash[%s], is already mined, peer=%d\n",
                        __func__, qc.quorumHash.ToString(), pfrom->GetId());
                // NOTE: do not punish here
                return;
            }
        }
        bool bReturn = false;
        {
            // Check if we already got a better one locally
            // We do this before verifying the commitment to avoid DoS
            LOCK(minableCommitmentsCs);
            auto it = minableCommitmentsByQuorum.find(qc.quorumHash);
            if (it != minableCommitmentsByQuorum.end()) {
                auto jt = minableCommitments.find(it->second);
                if (jt != minableCommitments.end() && jt->second.CountSigners() <= qc.CountSigners()) {
                        bReturn = true;
                }
            }
        }
        if(bReturn) {
            LOCK(cs_main);
            peerman.ForgetTxHash(pfrom->GetId(), hash);
            return;
        }

       if (!qc.Verify(pQuorumBaseBlockIndex, true)) {
            LogPrint(BCLog::LLMQ, "CQuorumBlockProcessor::%s -- commitment for quorum %s is not valid, peer=%d\n", __func__,
                    qc.quorumHash.ToString(),pfrom->GetId());
            {
                LOCK(cs_main);
                peerman.ForgetTxHash(pfrom->GetId(), hash);
            }
            if(peer)                  
                peerman.Misbehaving(*peer, 100, "invalid commitment for quorum");
            return;
        }

        LogPrint(BCLog::LLMQ, "CQuorumBlockProcessor::%s -- received commitment for quorum %s, validMembers=%d, signers=%d, peer=%d\n", __func__,
                qc.quorumHash.ToString(), qc.CountValidMembers(), qc.CountSigners(), pfrom->GetId());
        {
            LOCK(cs_main);
            peerman.ForgetTxHash(pfrom->GetId(), hash);
        }
        AddMineableCommitment(qc);
    }
}

bool CQuorumBlockProcessor::ProcessBlock(const CBlock& block, const CBlockIndex* pindex, BlockValidationState& state, bool fJustCheck, bool fBLSChecks)
{
    AssertLockHeld(cs_main);
    
    if (CLLMQUtils::IsV19Active(pindex->pprev->nHeight))
        bls::bls_legacy_scheme.store(false);
    bool fDIP0003Active = pindex->nHeight >= Params().GetConsensus().DIP0003Height;
    if (!fBLSChecks || !fDIP0003Active) {
        return true;
    }
    CFinalCommitment qc;
    if (!GetCommitmentsFromBlock(block, pindex->nHeight, qc, state)) {
        return false;
    }

    // The following checks make sure that there is always a (possibly null) commitment while in the mining phase
    // until the first non-null commitment has been mined. After the non-null commitment, no other commitments are
    // allowed, including null commitments.
    // skip these checks when replaying blocks after the crash
    if (!chainman.ActiveTip()) {
        return false;
    }

    
    // does the currently processed block contain a (possibly null) commitment for the current session?
    bool hasCommitmentInNewBlock = !qc.quorumHash.IsNull();
    bool isCommitmentRequired = IsCommitmentRequired(pindex->nHeight);
    
    if (hasCommitmentInNewBlock && !isCommitmentRequired) {

        // If we're either not in the mining phase or a non-null commitment was mined already, reject the block
        return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "bad-qc-not-allowed");
    }

    if (!hasCommitmentInNewBlock && isCommitmentRequired) {
        // If no non-null commitment was mined for the mining phase yet and the new block does not include
        // a (possibly null) commitment, the block should be rejected.
        return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "bad-qc-missing");
    }
    
    if (!qc.quorumHash.IsNull() && !ProcessCommitment(pindex->nHeight, block.GetHash(), qc, state, fJustCheck, fBLSChecks)) {
        return false;
    }
    return true;
}


bool CQuorumBlockProcessor::ProcessCommitment(int nHeight, const uint256& blockHash, const CFinalCommitment& qc, BlockValidationState& state, bool fJustCheck, bool fBLSChecks)
{
    AssertLockHeld(cs_main);
    uint256 quorumHash = GetQuorumBlockHash(chainman, nHeight);

    // skip `bad-qc-block` checks below when replaying blocks after the crash
    if (!chainman.ActiveTip()) {
        quorumHash = qc.quorumHash;
    }

    if (quorumHash.IsNull()) {
        return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "bad-qc-block-null");
    }
    if (quorumHash != qc.quorumHash) {
        return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "bad-qc-block-mismatch");
    }

    if (qc.IsNull()) {
        if (!qc.VerifyNull()) {
            return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "bad-qc-invalid-null");
        }
        return true;
    }

    if (HasMinedCommitment(quorumHash)) {
        // should not happen as it's already handled in ProcessBlock
        return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "bad-qc-dup");
    }

    if (!IsMiningPhase(nHeight)) {
        // should not happen as it's already handled in ProcessBlock
        return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "bad-qc-height");
    }

    auto pQuorumBaseBlockIndex = chainman.m_blockman.LookupBlockIndex(qc.quorumHash);
    if(!pQuorumBaseBlockIndex) {
        return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "bad-qc-block-index");
    }
    if (!qc.Verify(pQuorumBaseBlockIndex, fBLSChecks)) {
        return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "bad-qc-invalid");
    }

    if (fJustCheck) {
        return true;
    }

    // Store commitment in DB
    m_commitment_evoDb.WriteCache(quorumHash, std::make_pair(qc, blockHash));

    {
        LOCK(minableCommitmentsCs);
        minableCommitmentsByQuorum.erase(quorumHash);
        minableCommitments.erase(::SerializeHash(qc));
    }

    LogPrint(BCLog::LLMQ, "CQuorumBlockProcessor::%s -- processed commitment from block. quorumHash=%s, signers=%s, validMembers=%d, quorumPublicKey=%s\n", __func__,
            quorumHash.ToString(), qc.CountSigners(), qc.CountValidMembers(), qc.quorumPublicKey.ToString());

    return true;
}

bool CQuorumBlockProcessor::UndoBlock(const CBlock& block, const CBlockIndex* pindex)
{
    AssertLockHeld(cs_main);
    if (!CLLMQUtils::IsV19Active(pindex->pprev->nHeight))
        bls::bls_legacy_scheme.store(true);
    CFinalCommitment qc;
    BlockValidationState dummy;
    if (!GetCommitmentsFromBlock(block, pindex->nHeight, qc, dummy)) {
        return false;
    }

    if (qc.IsNull()) {
        return true;
    }

    m_commitment_evoDb.EraseCache(qc.quorumHash);

    // if a reorg happened, we should allow to mine this commitment later
    AddMineableCommitment(qc);
    return true;
}

bool CQuorumBlockProcessor::GetCommitmentsFromBlock(const CBlock& block, const uint32_t& nHeight, CFinalCommitment& ret, BlockValidationState& state)
{
    auto& consensus = Params().GetConsensus();
    bool fDIP0003Active = nHeight >= (uint32_t)consensus.DIP0003Height;
    if (block.vtx[0]->nVersion == SYSCOIN_TX_VERSION_MN_QUORUM_COMMITMENT) {
        if (!fDIP0003Active) {
            return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "bad-qc-premature");
        }
        CFinalCommitmentTxPayload qc;
        if (!GetTxPayload(*block.vtx[0], qc)) {
            // should not happen as it was verified before processing the block
            return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "bad-qc-payload");
        }
        ret = std::move(qc.commitment);
    }
    return true;
}

bool CQuorumBlockProcessor::IsMiningPhase(int nHeight)
{
    const auto& params = Params().GetConsensus().llmqTypeChainLocks;
    int phaseIndex = nHeight % params.dkgInterval;
    if (phaseIndex >= params.dkgMiningWindowStart && phaseIndex <= params.dkgMiningWindowEnd) {
        return true;
    }
    return false;
}

bool CQuorumBlockProcessor::IsCommitmentRequired(int nHeight) const
{
    AssertLockHeld(cs_main);
    uint256 quorumHash = GetQuorumBlockHash(chainman, nHeight);
    // perform extra check for quorumHash.IsNull as the quorum hash is unknown for the first block of a session
    // this is because the currently processed block's hash will be the quorumHash of this session
    bool isMiningPhase = !quorumHash.IsNull() && IsMiningPhase(nHeight);

    // did we already mine a non-null commitment for this session?
    bool hasMinedCommitment = !quorumHash.IsNull() && HasMinedCommitment(quorumHash);
    return isMiningPhase && !hasMinedCommitment;
}

// WARNING: This method returns uint256() on the first block of the DKG interval (because the block hash is not known yet)
uint256 CQuorumBlockProcessor::GetQuorumBlockHash(ChainstateManager& chainman, int nHeight)
{
    AssertLockHeld(cs_main);
    auto& params = Params().GetConsensus().llmqTypeChainLocks;
    int quorumStartHeight = nHeight - (nHeight % params.dkgInterval);
    uint256 quorumBlockHash;
    if (!GetBlockHash(chainman, quorumBlockHash, quorumStartHeight)) {
        return {};
    }
    return quorumBlockHash;
}

bool CQuorumBlockProcessor::HasMinedCommitment(const uint256& quorumHash) const
{
    return m_commitment_evoDb.ExistsCache(quorumHash);
}

CFinalCommitmentPtr CQuorumBlockProcessor::GetMinedCommitment(const uint256& quorumHash, uint256& retMinedBlockHash) const
{
    std::pair<CFinalCommitment, uint256> p;
    if (!m_commitment_evoDb.ReadCache(quorumHash, p)) {
        return nullptr;
    }
    retMinedBlockHash = p.second;
    return std::make_unique<CFinalCommitment>(p.first);
}

bool CQuorumBlockProcessor::HasMineableCommitment(const uint256& hash) const
{
    LOCK(minableCommitmentsCs);
    return minableCommitments.count(hash) != 0;
}

void CQuorumBlockProcessor::AddMineableCommitment(const CFinalCommitment& fqc)
{
    const uint256 commitmentHash = ::SerializeHash(fqc);

    const bool relay = [&]() {
        LOCK(minableCommitmentsCs);

        auto k = fqc.quorumHash;
        auto [itInserted, successfullyInserted] = minableCommitmentsByQuorum.try_emplace(k, commitmentHash);
        if (successfullyInserted) {
            minableCommitments.try_emplace(commitmentHash, fqc);
            return true;
        } else {
            auto& insertedQuorumHash = itInserted->second;
            const auto& oldFqc = minableCommitments.at(insertedQuorumHash);
            if (fqc.CountSigners() > oldFqc.CountSigners()) {
                // new commitment has more signers, so override the known one
                insertedQuorumHash = commitmentHash;
                minableCommitments.erase(insertedQuorumHash);
                minableCommitments.try_emplace(commitmentHash, fqc);
                return true;
            }
        }
        return false;
    }();

    // We only relay the new commitment if it's new or better then the old one
    if (relay) {
        CInv inv(MSG_QUORUM_FINAL_COMMITMENT, commitmentHash);
        peerman.RelayTransactionOther(inv);
    }
}

bool CQuorumBlockProcessor::GetMineableCommitmentByHash(const uint256& commitmentHash, llmq::CFinalCommitment& ret)
{
    LOCK(minableCommitmentsCs);
    auto it = minableCommitments.find(commitmentHash);
    if (it == minableCommitments.end()) {
        return false;
    }
    ret = it->second;
    return true;
}

// Will return false if no commitment should be mined
// Will return true and a null commitment if no minable commitment is known and none was mined yet
bool CQuorumBlockProcessor::GetMinableCommitment(int nHeight, CFinalCommitment& ret)
{
    AssertLockHeld(cs_main);
    if (!IsCommitmentRequired(nHeight)) {
        // no commitment required
        return false;
    }
    bool basic_bls_enabled = CLLMQUtils::IsV19Active(nHeight);
    uint256 quorumHash = GetQuorumBlockHash(chainman, nHeight);
    if (quorumHash.IsNull()) {
        return false;
    }
    {
        LOCK(minableCommitmentsCs);

        auto it = minableCommitmentsByQuorum.find(quorumHash);
        if (it == minableCommitmentsByQuorum.end()) {
            // null commitment required
            ret = CFinalCommitment(quorumHash);
            ret.nVersion = CFinalCommitment::GetVersion(basic_bls_enabled);
            return true;
        }
        ret = minableCommitments.at(it->second);
    }

    return true;
}
bool CQuorumBlockProcessor::FlushCacheToDisk() {
    return m_commitment_evoDb.FlushCacheToDisk();
}

} // namespace llmq
