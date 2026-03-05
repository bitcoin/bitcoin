// Copyright (c) 2026 The Syscoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <llmq/quorums_btccheckpoints.h>

#include <llmq/quorums.h>
#include <llmq/quorums_commitment.h>
#include <llmq/quorums_utils.h>
#include <chain.h>
#include <chainparams.h>
#include <common/args.h>
#include <common/run_command.h>
#include <evo/deterministicmns.h>
#include <logging.h>
#include <masternode/activemasternode.h>
#include <masternode/masternodesync.h>
#include <net_processing.h>
#include <node/blockstorage.h>
#include <primitives/block.h>
#include <util/strencodings.h>
#include <util/string.h>
#include <util/time.h>
#include <validation.h>
#include <univalue.h>

#include <algorithm>

namespace llmq
{

static const std::string BTCCHECK_REQUESTID_PREFIX = "btcc";
static constexpr size_t MAX_SIGNED_REQUESTIDS = 4096;
static constexpr int64_t DEFAULT_BTC_HEADER_MIN_CONFIRMATIONS{1};

CBTCCheckpointsHandler* btcCheckpointsHandler{nullptr};

static bool ParseHexUint256Strict(const UniValue& v, uint256& out)
{
    if (!v.isStr()) return false;
    const std::string s = v.get_str();
    if (!IsHex(s) || s.size() != 64) return false;
    out.SetHex(s);
    return true;
}

static bool GetObjectInt64(const UniValue& obj, const char* key, int64_t& out)
{
    if (!obj.isObject()) return false;
    const UniValue& v = obj.find_value(key);
    if (!v.isNum()) return false;
    out = v.getInt<int64_t>();
    return true;
}

static int32_t GetExpectedBTCCheckpointHeight(const ChainstateManager& chainman) EXCLUSIVE_LOCKS_REQUIRED(cs_main)
{
    // Strict height-window gating (absolute schedule):
    // - Emit exactly one checkpoint per BTCCHECK_PERIOD blocks
    // - Sign at heights where height % BTCCHECK_PERIOD == BTCCHECK_SIGN_OFFSET
    const int tip = chainman.ActiveHeight();
    const int start = Params().GetConsensus().nCLReceiptStartBlock;
    static constexpr int PERIOD{BTCCHECK_PERIOD};
    static constexpr int SIGN_OFFSET{BTCCHECK_SIGN_OFFSET}; // within [0, PERIOD)

    // Compute the greatest height h <= tip such that h % PERIOD == SIGN_OFFSET.
    // Enforce activation: don't return pre-activation heights.
    if (tip < SIGN_OFFSET) return -1;
    const int h = tip - ((tip - SIGN_OFFSET) % PERIOD);
    if (h < start) return -1;
    return h;
}

static bool RunBTCHeaderRPCCommand(const std::vector<std::string>& method_and_args, UniValue& out, std::string& err)
{
    if (method_and_args.empty()) {
        err = "btcheadercmd-empty-method";
        return false;
    }

    const bool managed = gArgs.GetBoolArg("-btcheadermanaged", DEFAULT_BTC_HEADER_MANAGED);
    if (managed) {
        std::vector<std::string> command_args;
        if (!GetManagedBTCHeaderRPCCommandArgs(command_args)) {
            err = "btcheadercmd-not-set";
            return false;
        }
        command_args.insert(command_args.end(), method_and_args.begin(), method_and_args.end());
        try {
            out = RunCommandParseJSON(command_args);
            return true;
        } catch (const std::exception& e) {
            err = e.what();
            return false;
        }
    }

    const std::string cmd = gArgs.GetArg("-btcheadercmd", "");
    if (cmd.empty()) {
        err = "btcheadercmd-not-set";
        return false;
    }
    std::string method_tail;
    for (const std::string& arg : method_and_args) {
        if (!method_tail.empty()) method_tail += " ";
        method_tail += arg;
    }
    try {
        out = RunCommandParseJSON(cmd + " " + method_tail);
        return true;
    } catch (const std::exception& e) {
        err = e.what();
        return false;
    }
}

static bool GetLatestOnChainBTCPREVCommitment(const ChainstateManager& chainman, int32_t sign_height, uint256& out_hash) EXCLUSIVE_LOCKS_REQUIRED(cs_main)
{
    out_hash.SetNull();
    const int start = Params().GetConsensus().nCLReceiptStartBlock;
    for (int32_t h = sign_height - BTCCHECK_PERIOD; h >= start; h -= BTCCHECK_PERIOD) {
        const CBlockIndex* pindex = chainman.ActiveChain()[h];
        if (!pindex) continue;
        if (!pindex->btcpPrevCommitment.IsNull()) {
            out_hash = pindex->btcpPrevCommitment;
            return true;
        }
    }
    return false;
}

CBTCHeaderPolicyWatchdog::CBTCHeaderPolicyWatchdog(ChainstateManager& _chainman) :
    chainman(_chainman)
{
}

bool CBTCHeaderPolicyWatchdog::ProbeChainInfo(UniValue& out, std::string& err) const
{
    if (!RunBTCHeaderRPCCommand({"getblockchaininfo"}, out, err)) {
        return false;
    }
    if (!out.isObject()) {
        err = "btc-chaininfo-not-object";
        return false;
    }
    return true;
}

bool CBTCHeaderPolicyWatchdog::ParseChainInfo(const UniValue& chainInfo, bool& ibd, int64_t& tipHeight, std::string& err) const
{
    const UniValue& ibdV = chainInfo.find_value("initialblockdownload");
    if (!ibdV.isBool()) {
        err = "btc-chaininfo-missing-ibd";
        return false;
    }
    ibd = ibdV.get_bool();

    if (!GetObjectInt64(chainInfo, "headers", tipHeight)) {
        // Some backends may not expose "headers"; fallback to "blocks".
        if (!GetObjectInt64(chainInfo, "blocks", tipHeight)) {
            err = "btc-chaininfo-missing-height";
            return false;
        }
    }
    return true;
}

bool CBTCHeaderPolicyWatchdog::AttemptManagedRestart(const std::string& restartReason, std::string& denyReason)
{
    const int64_t now = GetTime();
    const int64_t cooldown = std::max<int64_t>(1, gArgs.GetIntArg("-btcheaderwatchdogrestartcooldown", DEFAULT_BTC_HEADER_WATCHDOG_RESTART_COOLDOWN));
    const int64_t reindex_after = std::max<int64_t>(0, gArgs.GetIntArg("-btcheaderwatchdogreindexafter", DEFAULT_BTC_HEADER_WATCHDOG_REINDEX_AFTER));
    bool force_reindex{false};
    {
        LOCK(cs);
        if (lastRestartTime > 0 && (now - lastRestartTime) < cooldown) {
            denyReason = strprintf("btcheader-watchdog-restart-cooldown(wait=%d reason=%s)",
                                   cooldown - (now - lastRestartTime), restartReason);
            return false;
        }
        if (reindex_after > 0 && !reindexAttemptedInFailureStreak && consecutiveRestartFailures >= reindex_after) {
            force_reindex = true;
            reindexAttemptedInFailureStreak = true;
        }
        lastRestartTime = now;
    }

    if (!chainman.ActiveChainstate().RestartBTCHeaderNode(force_reindex)) {
        int32_t failures_now{0};
        {
            LOCK(cs);
            ++consecutiveRestartFailures;
            failures_now = consecutiveRestartFailures;
        }
        denyReason = strprintf("btcheader-watchdog-restart-failed(reindex=%d failures=%d): %s",
                               force_reindex ? 1 : 0, failures_now, restartReason);
        return false;
    }

    {
        LOCK(cs);
        // Force a fresh backend probe after successful restart.
        consecutiveRestartFailures = 0;
        reindexAttemptedInFailureStreak = false;
        lastProbeTime = 0;
        lastProbeHealthy = true;
        lastProbeReason.clear();
        lastProgressTime = now;
        lastTipHeight = -1;
    }

    LogPrint(BCLog::CHAINLOCKS,
             "CBTCHeaderPolicyWatchdog -- restarted managed BTC header node (reindex=%d reason=%s)\n",
             force_reindex ? 1 : 0, restartReason);
    return true;
}

bool CBTCHeaderPolicyWatchdog::CheckAndRecover(std::string& denyReason)
{
    denyReason.clear();

    if (!gArgs.GetBoolArg("-btcheaderwatchdog", DEFAULT_BTC_HEADER_WATCHDOG)) {
        return true;
    }

    const bool enforce_on_demand = gArgs.GetBoolArg("-btcheaderpolicyondemand", DEFAULT_BTC_HEADER_POLICY_ON_DEMAND);
    if (Params().MineBlocksOnDemand() && !enforce_on_demand) {
        return true;
    }

    const bool managed = gArgs.GetBoolArg("-btcheadermanaged", DEFAULT_BTC_HEADER_MANAGED);
    if (managed) {
        std::string runningReason;
        if (!chainman.ActiveChainstate().IsManagedBTCHeaderNodeRunning(runningReason)) {
            if (!AttemptManagedRestart("process-not-running:" + runningReason, denyReason)) {
                LOCK(cs);
                lastProbeTime = GetTime();
                lastProbeHealthy = false;
                lastProbeReason = denyReason;
                return false;
            }
        }
    }

    const int64_t now = GetTime();
    const int64_t probeInterval = std::max<int64_t>(1, gArgs.GetIntArg("-btcheaderwatchdogprobeinterval", DEFAULT_BTC_HEADER_WATCHDOG_PROBE_INTERVAL));
    {
        LOCK(cs);
        if (lastProbeTime > 0 && (now - lastProbeTime) < probeInterval) {
            if (!lastProbeHealthy) {
                denyReason = lastProbeReason;
            }
            return lastProbeHealthy;
        }
    }

    UniValue chainInfo;
    std::string err;
    if (!ProbeChainInfo(chainInfo, err)) {
        if (managed) {
            if (!AttemptManagedRestart("rpc-unreachable", denyReason)) {
                LOCK(cs);
                lastProbeTime = now;
                lastProbeHealthy = false;
                lastProbeReason = denyReason;
                return false;
            }
            if (!ProbeChainInfo(chainInfo, err)) {
                denyReason = "btcheader-watchdog-rpc-unreachable-after-restart: " + err;
                LOCK(cs);
                lastProbeTime = now;
                lastProbeHealthy = false;
                lastProbeReason = denyReason;
                return false;
            }
        } else {
            denyReason = "btcheader-watchdog-external-unreachable: " + err;
            LOCK(cs);
            lastProbeTime = now;
            lastProbeHealthy = false;
            lastProbeReason = denyReason;
            return false;
        }
    }

    bool ibd{false};
    int64_t tipHeight{-1};
    if (!ParseChainInfo(chainInfo, ibd, tipHeight, err)) {
        denyReason = "btcheader-watchdog-chaininfo-parse-failed: " + err;
        LOCK(cs);
        lastProbeTime = now;
        lastProbeHealthy = false;
        lastProbeReason = denyReason;
        return false;
    }

    bool stalled{false};
    int64_t stalledFor{0};
    const int64_t stallTimeout = std::max<int64_t>(0, gArgs.GetIntArg("-btcheaderwatchdogstalltimeout", DEFAULT_BTC_HEADER_WATCHDOG_STALL_TIMEOUT));
    {
        LOCK(cs);
        if (lastProgressTime == 0) {
            lastProgressTime = now;
        }
        if (tipHeight > lastTipHeight) {
            lastTipHeight = tipHeight;
            lastProgressTime = now;
        }
        if (!ibd) {
            lastProgressTime = now;
        } else if (stallTimeout > 0) {
            stalledFor = now - lastProgressTime;
            stalled = stalledFor >= stallTimeout;
        }
    }

    if (stalled) {
        if (managed) {
            if (!AttemptManagedRestart(strprintf("ibd-stalled(headers=%d stalled=%d)", tipHeight, stalledFor), denyReason)) {
                LOCK(cs);
                lastProbeTime = now;
                lastProbeHealthy = false;
                lastProbeReason = denyReason;
                return false;
            }
        } else {
            denyReason = strprintf("btcheader-watchdog-external-stalled(headers=%d stalled=%d)", tipHeight, stalledFor);
            LOCK(cs);
            lastProbeTime = now;
            lastProbeHealthy = false;
            lastProbeReason = denyReason;
            return false;
        }
    }

    {
        LOCK(cs);
        if (managed) {
            consecutiveRestartFailures = 0;
            reindexAttemptedInFailureStreak = false;
        }
        lastProbeTime = now;
        lastProbeHealthy = true;
        lastProbeReason.clear();
    }
    return true;
}

bool CBTCCheckpointSig::IsNull() const
{
    return nHeight == -1 && sysHash == uint256();
}

std::string CBTCCheckpointSig::ToString() const
{
    return strprintf("CBTCCheckpointSig(nHeight=%d, sysHash=%s, signers: hex=%s size=%d count=%d)",
                     nHeight, sysHash.ToString(),
                     CLLMQUtils::ToHexStr(signers), signers.size(), std::count(signers.begin(), signers.end(), true));
}

CBTCCheckpointsHandler::CBTCCheckpointsHandler(CConnman& _connman, PeerManager& _peerman, ChainstateManager& _chainman) :
    chainman(_chainman),
    connman(_connman),
    peerman(_peerman),
    btcheaderWatchdog(_chainman)
{
}

void CBTCCheckpointsHandler::Start()
{
    quorumSigningManager->RegisterRecoveredSigsListener(this);
}

void CBTCCheckpointsHandler::Stop()
{
    quorumSigningManager->UnregisterRecoveredSigsListener(this);
}

bool CBTCCheckpointsHandler::RunBTCHeaderCommand(const std::vector<std::string>& method_and_args, UniValue& out, std::string& err) const
{
    return RunBTCHeaderRPCCommand(method_and_args, out, err);
}

bool CBTCCheckpointsHandler::CheckBTCHeaderSigningPolicy(const uint256& btcHash, int32_t sysHeight, int32_t& btcHeightOut, std::string& denyReason)
{
    btcHeightOut = -1;

    const bool enforce_on_demand = gArgs.GetBoolArg("-btcheaderpolicyondemand", DEFAULT_BTC_HEADER_POLICY_ON_DEMAND);
    // Mine-blocks-on-demand setups keep policy disabled unless explicitly enabled.
    if (Params().MineBlocksOnDemand() && !enforce_on_demand) {
        return true;
    }

    if (btcHash.IsNull()) {
        denyReason = "btcprev-null";
        return false;
    }

    UniValue chainInfo;
    std::string err;
    if (!RunBTCHeaderCommand({"getblockchaininfo"}, chainInfo, err)) {
        denyReason = "btc-chaininfo-failed: " + err;
        return false;
    }
    if (!chainInfo.isObject()) {
        denyReason = "btc-chaininfo-not-object";
        return false;
    }

    const UniValue& ibd = chainInfo.find_value("initialblockdownload");
    if (!ibd.isBool()) {
        denyReason = "btc-chaininfo-missing-ibd";
        return false;
    }
    if (ibd.get_bool()) {
        denyReason = "btc-node-ibd";
        return false;
    }

    uint256 bestHash;
    if (!ParseHexUint256Strict(chainInfo.find_value("bestblockhash"), bestHash)) {
        denyReason = "btc-chaininfo-badhash";
        return false;
    }

    UniValue bestHeader;
    if (!RunBTCHeaderCommand({"getblockheader", bestHash.ToString(), "true"}, bestHeader, err)) {
        denyReason = "btc-bestheader-failed: " + err;
        return false;
    }

    int64_t tipHeight{-1};
    int64_t tipTime{0};
    if (!GetObjectInt64(bestHeader, "height", tipHeight) || !GetObjectInt64(bestHeader, "time", tipTime)) {
        denyReason = "btc-bestheader-missing-fields";
        return false;
    }

    const int64_t now = GetTime();
    int64_t tipNoProgressAge{0};
    bool tipProgressBypassEligible{false};
    {
        LOCK(cs);
        if (lastPolicyTipProgressTime == 0) {
            // Initialize progress tracker from the first observed tip.
            lastPolicyObservedTipHeight = tipHeight;
            lastPolicyTipProgressTime = now;
        } else {
            tipProgressBypassEligible = true;
            if (tipHeight > lastPolicyObservedTipHeight) {
                lastPolicyObservedTipHeight = tipHeight;
                lastPolicyTipProgressTime = now;
            }
        }
        tipNoProgressAge = std::max<int64_t>(0, now - lastPolicyTipProgressTime);
    }

    const int64_t tipMaxNoProgress = std::max<int64_t>(0, gArgs.GetIntArg("-btcheadertipmaxnoprogress", DEFAULT_BTC_HEADER_TIP_MAX_NO_PROGRESS));
    if (tipMaxNoProgress > 0 && tipNoProgressAge > tipMaxNoProgress) {
        denyReason = strprintf("btc-tip-no-progress(age=%d tip=%d)", tipNoProgressAge, tipHeight);
        return false;
    }

    const int64_t tipMaxAge = std::max<int64_t>(0, gArgs.GetIntArg("-btcheadertipmaxage", DEFAULT_BTC_HEADER_TIP_MAX_AGE));
    if (tipMaxAge > 0) {
        const int64_t age = now - tipTime;
        if (age > tipMaxAge) {
            // When local clock skew makes header timestamps appear old, allow if
            // the header tip is still progressing within the no-progress window.
            const bool allow_by_progress = tipProgressBypassEligible &&
                                           tipMaxNoProgress > 0 &&
                                           tipNoProgressAge <= tipMaxNoProgress;
            if (!allow_by_progress) {
                denyReason = strprintf("btc-tip-stale(age=%d)", age);
                return false;
            }
        }
    }

    UniValue candidateHeader;
    if (!RunBTCHeaderCommand({"getblockheader", btcHash.ToString(), "true"}, candidateHeader, err)) {
        denyReason = "btc-candidate-header-failed: " + err;
        return false;
    }

    int64_t confirmations{0};
    int64_t candidateHeight{-1};
    if (!GetObjectInt64(candidateHeader, "confirmations", confirmations) || !GetObjectInt64(candidateHeader, "height", candidateHeight)) {
        denyReason = "btc-candidate-header-missing-fields";
        return false;
    }
    if (confirmations < DEFAULT_BTC_HEADER_MIN_CONFIRMATIONS) {
        denyReason = "btc-candidate-unconfirmed";
        return false;
    }
    if (candidateHeight > tipHeight) {
        denyReason = "btc-candidate-height-ahead-of-tip";
        return false;
    }

    const int64_t maxLagBlocks = std::max<int64_t>(0, gArgs.GetIntArg("-btcheadermaxlagblocks", DEFAULT_BTC_HEADER_MAX_LAG_BLOCKS));
    const int64_t lagBlocks = tipHeight - candidateHeight;
    if (maxLagBlocks > 0 && lagBlocks > maxLagBlocks) {
        denyReason = strprintf("btc-candidate-too-old(lag=%d max=%d)", lagBlocks, maxLagBlocks);
        return false;
    }

    const int64_t recentForkDepth = std::max<int64_t>(0, gArgs.GetIntArg("-btcheaderrecentforkdepth", DEFAULT_BTC_HEADER_RECENT_FORK_DEPTH));
    if (recentForkDepth > 0) {
        // Recent fork heuristic: if any non-active, valid-looking competing tip is near
        // the active tip, pause signing until the fork risk cools down.
        UniValue chainTips;
        if (!RunBTCHeaderCommand({"getchaintips"}, chainTips, err)) {
            denyReason = "btc-chaintips-failed: " + err;
            return false;
        }
        if (!chainTips.isArray()) {
            denyReason = "btc-chaintips-not-array";
            return false;
        }
        for (const UniValue& tip : chainTips.getValues()) {
            if (!tip.isObject()) continue;
            const UniValue& statusV = tip.find_value("status");
            if (!statusV.isStr()) continue;
            const std::string status = statusV.get_str();
            if (status == "active" || status == "invalid") continue;
            if (status != "valid-fork" && status != "valid-headers" && status != "headers-only") continue;

            int64_t forkTipHeight{-1};
            if (!GetObjectInt64(tip, "height", forkTipHeight)) continue;
            if (forkTipHeight >= tipHeight - recentForkDepth) {
                denyReason = strprintf("btc-recent-fork(status=%s height=%d tip=%d depth=%d)",
                                       status, forkTipHeight, tipHeight, recentForkDepth);
                return false;
            }
        }
    }

    // Continuity guard: never move backward relative to the latest BTC hash this node
    // already queued for signing.
    uint256 prevSignedBTCHash;
    int32_t prevSignedBTCHeight{-1};
    {
        LOCK(cs);
        prevSignedBTCHash = lastSignedBTCHash;
        prevSignedBTCHeight = lastSignedBTCHeight;
    }
    if (!prevSignedBTCHash.IsNull()) {
        int64_t prevHeight = prevSignedBTCHeight;
        if (prevHeight < 0) {
            UniValue prevHeader;
            if (!RunBTCHeaderCommand({"getblockheader", prevSignedBTCHash.ToString(), "true"}, prevHeader, err)) {
                denyReason = "btc-prev-signed-header-lookup-failed: " + err;
                return false;
            }
            if (!GetObjectInt64(prevHeader, "height", prevHeight)) {
                denyReason = "btc-prev-signed-header-missing-height";
                return false;
            }
        }

        // Stronger continuity: the previously signed hash must remain active at its recorded height.
        UniValue activeHashAtPrevHeightV;
        if (!RunBTCHeaderCommand({"getblockhash", strprintf("%d", prevHeight)}, activeHashAtPrevHeightV, err)) {
            denyReason = "btc-prev-signed-active-chain-lookup-failed: " + err;
            return false;
        }
        uint256 activeHashAtPrevHeight;
        if (!ParseHexUint256Strict(activeHashAtPrevHeightV, activeHashAtPrevHeight)) {
            denyReason = "btc-prev-signed-active-chain-badhash";
            return false;
        }
        if (activeHashAtPrevHeight != prevSignedBTCHash) {
            {
                LOCK(cs);
                // Rebaseline once to recover liveness after a detected BTC reorg.
                if (lastSignedBTCHash == prevSignedBTCHash) {
                    lastSignedBTCHash = activeHashAtPrevHeight;
                    lastSignedBTCHeight = static_cast<int32_t>(prevHeight);
                }
            }
            denyReason = strprintf("btc-prev-signed-not-active-chain(height=%d)", prevHeight);
            return false;
        }
        if (candidateHeight < prevHeight) {
            denyReason = strprintf("btc-non-monotonic-height(prev=%d cand=%d)", prevHeight, candidateHeight);
            return false;
        }
        if (candidateHeight == prevHeight && btcHash != prevSignedBTCHash) {
            denyReason = "btc-same-height-different-hash";
            return false;
        }
    }

    btcHeightOut = static_cast<int32_t>(candidateHeight);
    (void)sysHeight;
    return true;
}

bool CBTCCheckpointsHandler::AlreadyHave(const uint256& hash) const
{
    LOCK(cs);
    return seenBTCCheckpointSigs.count(hash) != 0;
}

void CBTCCheckpointsHandler::Cleanup()
{
    if (!masternodeSync.IsBlockchainSynced()) {
        return;
    }

    {
        LOCK(cs);
        if (TicksSinceEpoch<std::chrono::milliseconds>(SystemClock::now()) - lastCleanupTime < CLEANUP_INTERVAL) {
            return;
        }
    }

    LOCK(cs);

    for (auto it = seenBTCCheckpointSigs.begin(); it != seenBTCCheckpointSigs.end(); ) {
        if (TicksSinceEpoch<std::chrono::milliseconds>(SystemClock::now()) - it->second >= CLEANUP_SEEN_TIMEOUT) {
            it = seenBTCCheckpointSigs.erase(it);
        } else {
            ++it;
        }
    }
    for (auto it = sigChecked.begin(); it != sigChecked.end(); ) {
        if (TicksSinceEpoch<std::chrono::milliseconds>(SystemClock::now()) - it->second >= CLEANUP_SEEN_TIMEOUT) {
            it = sigChecked.erase(it);
        } else {
            ++it;
        }
    }

    // prune height-indexed maps behind the best candidate window
    const int32_t bestHeight = bestCandidates.empty() ? -1 : bestCandidates.rbegin()->first;
    if (bestHeight >= 0) {
        const int32_t pruneBelow = std::max<int32_t>(0, bestHeight - RECENT_BTCCHECKPOINTS_MAX);
        for (auto it = recentBTCCheckpoints.begin(); it != recentBTCCheckpoints.end();) {
            if (it->first < pruneBelow) {
                it = recentBTCCheckpoints.erase(it);
            } else {
                ++it;
            }
        }
        for (auto it = bestCandidates.begin(); it != bestCandidates.end();) {
            if (it->first < pruneBelow) {
                it = bestCandidates.erase(it);
            } else {
                ++it;
            }
        }
        for (auto it = bestShares.begin(); it != bestShares.end();) {
            if (it->first < pruneBelow) {
                it = bestShares.erase(it);
            } else {
                ++it;
            }
        }
    }

    lastCleanupTime = TicksSinceEpoch<std::chrono::milliseconds>(SystemClock::now());
}

bool CBTCCheckpointsHandler::GetBTCCheckpointByHash(const uint256& hash, CBTCCheckpointSig& ret) const
{
    LOCK(cs);

    for (const auto& pair : bestCandidates) {
        if (pair.second && ::SerializeHash(*pair.second) == hash) {
            ret = *pair.second;
            return true;
        }
    }

    for (const auto& pair : bestShares) {
        for (const auto& pair2 : pair.second) {
            if (pair2.second && ::SerializeHash(*pair2.second) == hash) {
                ret = *pair2.second;
                return true;
            }
        }
    }

    for (const auto& pair : recentBTCCheckpoints) {
        if (::SerializeHash(pair.second) == hash) {
            ret = pair.second;
            return true;
        }
    }

    auto itp = pendingVerifiedBTCCheckpointSigs.find(hash);
    if (itp != pendingVerifiedBTCCheckpointSigs.end()) {
        ret = itp->second.first;
        return true;
    }

    return false;
}

bool CBTCCheckpointsHandler::VerifyAggregatedBTCCheckpointNoCache(const CBTCCheckpointSig& btcsig, const CBlockIndex* pindexScan) const
{
    const auto& consensus = Params().GetConsensus();
    const auto& llmqParams = consensus.llmqTypeChainLocks;
    const auto& signingActiveQuorumCount = llmqParams.signingActiveQuorumCount;

    if (btcsig.signers.size() != (size_t)signingActiveQuorumCount) return false;
    if (std::count(btcsig.signers.begin(), btcsig.signers.end(), true) < (signingActiveQuorumCount / 2 + 1)) return false;

    const auto quorums_scanned = llmq::quorumManager->ScanQuorums(pindexScan, signingActiveQuorumCount);
    if (quorums_scanned.empty()) return false;

    const uint256 msgHash = btcsig.sysHash;

    std::vector<uint256> hashes;
    std::vector<CBLSPublicKey> quorumPublicKeys;
    for (size_t i = 0; i < quorums_scanned.size(); ++i) {
        const CQuorumCPtr& quorum = quorums_scanned[i];
        if (quorum == nullptr) return false;
        if (!btcsig.signers[i]) continue;
        quorumPublicKeys.emplace_back(quorum->qc->quorumPublicKey);
        const uint256 requestId = ::SerializeHash(std::make_tuple(BTCCHECK_REQUESTID_PREFIX, btcsig.nHeight, quorum->qc->quorumHash));
        const uint256 signHash = llmq::BuildSignHash(quorum->qc->quorumHash, requestId, msgHash);
        hashes.emplace_back(signHash);
    }
    return btcsig.sig.VerifyInsecureAggregated(quorumPublicKeys, hashes);
}

void CBTCCheckpointsHandler::ProcessMessage(CNode* pfrom, const std::string& msg_type, CDataStream& vRecv)
{
    if (msg_type != NetMsgType::BTCCSIG) {
        return;
    }

    CBTCCheckpointSig btccsig;
    vRecv >> btccsig;
    const uint256 hash = ::SerializeHash(btccsig);
    const size_t signers_count = std::count(btccsig.signers.begin(), btccsig.signers.end(), true);

    const NodeId from = pfrom->GetId();
    PeerRef peer = peerman.GetPeerRef(from);
    if (peer) {
        peerman.AddKnownTx(*peer, hash);
    }
    {
        LOCK(cs_main);
        peerman.ReceivedResponse(from, hash);
    }
    auto forget_tx_hash = [&]() {
        LOCK(cs_main);
        peerman.ForgetTxHash(from, hash);
    };

    Cleanup();

    {
        LOCK(cs);
        if (seenBTCCheckpointSigs.count(hash) != 0) {
            // We already processed this object.
            // Must not call forget_tx_hash() under cs (it locks cs_main).
            // Defer handling outside the cs scope to avoid lock-order inversion.
            // (ChainLocks uses LOCK2(cs_main, cs) ordering elsewhere.)
            // Fall through to handle after unlocking.
            goto already_seen;
        }
    }
    goto not_seen;
already_seen:
    forget_tx_hash();
    return;
not_seen:
    const CBlockIndex* pindexScan{nullptr};
    int32_t expectedHeight{-1};
    {
        LOCK(cs_main);
        expectedHeight = GetExpectedBTCCheckpointHeight(chainman);
        pindexScan = chainman.m_blockman.LookupBlockIndex(btccsig.sysHash);
    }
    if (pindexScan == nullptr || pindexScan->nHeight != btccsig.nHeight) {
        forget_tx_hash();
        return;
    }
    {
        LOCK(cs_main);
        if (btccsig.nHeight < 0 || btccsig.nHeight > chainman.ActiveHeight()) {
            forget_tx_hash();
            return;
        }
        if (chainman.ActiveChain()[btccsig.nHeight] != pindexScan) {
            // Don't accept/relay checkpoint sigs for non-active forks.
            forget_tx_hash();
            return;
        }
        if (expectedHeight == -1) {
            forget_tx_hash();
            return;
        }
    }

    const auto& llmqParams = Params().GetConsensus().llmqTypeChainLocks;
    if (btccsig.signers.size() != (size_t)llmqParams.signingActiveQuorumCount) {
        forget_tx_hash();
        return;
    }

    // Drop old material once we have a newer checkpoint (ChainLocks parity: "existing height" gating).
    {
        LOCK(cs);
        if (!bestCandidates.empty() && bestCandidates.rbegin()->first > btccsig.nHeight) {
            // Must not call forget_tx_hash() under cs (it locks cs_main).
            goto older_height;
        }
    }
    goto not_older;
older_height:
    forget_tx_hash();
    return;
not_older:

    if (signers_count == 1) {
        // share
        std::pair<int, CQuorumCPtr> ret;
        if (!VerifyBTCCheckpointShare(btccsig, pindexScan, uint256(), ret, hash)) {
            forget_tx_hash();
            return;
        }
        if (btccsig.nHeight != expectedHeight) {
            AddPendingVerifiedBTCCheckpointSig(hash, btccsig);
            forget_tx_hash();
            return;
        }
        {
            LOCK(cs);
            seenBTCCheckpointSigs.emplace(hash, TicksSinceEpoch<std::chrono::milliseconds>(SystemClock::now()));
        }
        bool agg{false};
        uint256 agg_hash;
        {
            LOCK(cs);
            bestShares[btccsig.nHeight].emplace(ret.second, std::make_shared<const CBTCCheckpointSig>(btccsig));
            agg = TryUpdateBestBTCCheckpoint(pindexScan);
            if (agg) {
                auto it = bestCandidates.find(btccsig.nHeight);
                if (it != bestCandidates.end() && it->second) {
                    agg_hash = ::SerializeHash(*it->second);
                    seenBTCCheckpointSigs.emplace(agg_hash, TicksSinceEpoch<std::chrono::milliseconds>(SystemClock::now()));
                }
            }
        }
        if (agg && !agg_hash.IsNull()) {
            peerman.RelayInv(CInv(MSG_BTCCSIG, agg_hash));
        } else {
            // Relay partial shares to full nodes only, mirroring ChainLocks behavior.
            LOCK(cs_main);
            connman.ForEachNode([&](CNode* pnode) EXCLUSIVE_LOCKS_REQUIRED(::cs_main) {
                AssertLockHeld(::cs_main);
                const bool fSPV{pnode->m_bloom_filter_loaded.load()};
                if (!fSPV && pnode->CanRelay()) {
                    PeerRef peer = peerman.GetPeerRef(pnode->GetId());
                    if (peer) {
                        peerman.PushTxInventoryOther(*peer, CInv(MSG_BTCCSIG, hash));
                    }
                }
            });
        }
        // Always clear request bookkeeping on handled messages (ChainLocks parity).
        forget_tx_hash();
        return;
    }

    // aggregated
    if (!VerifyAggregatedBTCCheckpoint(btccsig, pindexScan)) {
        forget_tx_hash();
        return;
    }
    if (btccsig.nHeight != expectedHeight) {
        AddPendingVerifiedBTCCheckpointSig(hash, btccsig);
        forget_tx_hash();
        return;
    }
    {
        LOCK(cs);
        seenBTCCheckpointSigs.emplace(hash, TicksSinceEpoch<std::chrono::milliseconds>(SystemClock::now()));
    }

    bool accepted{false};
    {
        LOCK(cs);
        auto it = bestCandidates.find(btccsig.nHeight);
        if (it == bestCandidates.end() || it->second == nullptr || it->second->sysHash != btccsig.sysHash) {
            bestCandidates[btccsig.nHeight] = std::make_shared<const CBTCCheckpointSig>(btccsig);
            AddRecentBTCCheckpoint(btccsig);
            accepted = true;
        }
    }

    if (accepted) {
        peerman.RelayInv(CInv(MSG_BTCCSIG, hash));
    }
    // Always clear request bookkeeping on handled messages (ChainLocks parity).
    forget_tx_hash();
}

void CBTCCheckpointsHandler::TrySignBTCCheckpointTip()
{
    if (!fMasternodeMode) return;
    if (!masternodeSync.IsBlockchainSynced()) return;

    const CBlockIndex* pindex{nullptr};
    uint256 btcPrevHash;
    uint256 chainBaselineBTCHash;
    {
        TRY_LOCK(cs_main, lockMain);
        if (!lockMain) {
            return;
        }
        const int start = Params().GetConsensus().nCLReceiptStartBlock;
        if (chainman.ActiveHeight() < start) {
            return;
        }
        // We want a propagation buffer between signing and mining the carrier block.
        // Sign at epoch offset +BTCCHECK_SIGN_OFFSET and embed at +BTCCHECK_CARRIER_OFFSET (buffer BTCCHECK_PROP_BUFFER).
        const int nActiveHeight = GetExpectedBTCCheckpointHeight(chainman);
        if (nActiveHeight == -1) return;
        // Don't sign a height whose carrier block already passed; it can't be embedded anymore.
        // This can happen after restarts or if a node comes online late.
        if (chainman.ActiveHeight() >= nActiveHeight + BTCCHECK_PROP_BUFFER) {
            return;
        }
        pindex = chainman.ActiveChain()[nActiveHeight];
        if (!pindex || !pindex->pprev || !pindex->IsValid(BLOCK_VALID_SCRIPTS)) {
            return;
        }
        btcPrevHash = pindex->btcpPrevCommitment;
        (void)GetLatestOnChainBTCPREVCommitment(chainman, nActiveHeight, chainBaselineBTCHash);
    }

    if (!chainBaselineBTCHash.IsNull()) {
        LOCK(cs);
        if (lastSignedBTCHash.IsNull()) {
            lastSignedBTCHash = chainBaselineBTCHash;
            lastSignedBTCHeight = -1;
        }
    }

    std::string watchdogDenyReason;
    if (!btcheaderWatchdog.CheckAndRecover(watchdogDenyReason)) {
        bool shouldLog{false};
        {
            LOCK(cs);
            if (lastPolicyRejectHeight != pindex->nHeight || lastPolicyRejectReason != watchdogDenyReason) {
                lastPolicyRejectHeight = pindex->nHeight;
                lastPolicyRejectReason = watchdogDenyReason;
                shouldLog = true;
            }
        }
        if (shouldLog) {
            LogPrint(BCLog::CHAINLOCKS,
                     "CBTCCheckpointsHandler -- skip btcc sign at sysHeight=%d btcPrev=%s reason=%s\n",
                     pindex->nHeight, btcPrevHash.ToString(), watchdogDenyReason);
        }
        return;
    }

    int32_t btcHeight{-1};
    std::string policyDenyReason;
    if (!CheckBTCHeaderSigningPolicy(btcPrevHash, pindex->nHeight, btcHeight, policyDenyReason)) {
        bool shouldLog{false};
        {
            LOCK(cs);
            if (lastPolicyRejectHeight != pindex->nHeight || lastPolicyRejectReason != policyDenyReason) {
                lastPolicyRejectHeight = pindex->nHeight;
                lastPolicyRejectReason = policyDenyReason;
                shouldLog = true;
            }
        }
        if (shouldLog) {
            LogPrint(BCLog::CHAINLOCKS,
                     "CBTCCheckpointsHandler -- skip btcc sign at sysHeight=%d btcPrev=%s reason=%s\n",
                     pindex->nHeight, btcPrevHash.ToString(), policyDenyReason);
        }
        return;
    }

    // Keep internal state bounded. We only need a recent window for aggregation/replay.
    // This is intentionally simple (no timers) to minimize behavioral changes.
    {
        const int32_t pruneBelow = std::max<int32_t>(0, pindex->nHeight - RECENT_BTCCHECKPOINTS_MAX - 16);
        LOCK(cs);
        for (auto it = bestShares.begin(); it != bestShares.end();) {
            if (it->first < pruneBelow) {
                it = bestShares.erase(it);
            } else {
                ++it;
            }
        }
        for (auto it = bestCandidates.begin(); it != bestCandidates.end();) {
            if (it->first < pruneBelow) {
                it = bestCandidates.erase(it);
            } else {
                ++it;
            }
        }
    }

    CBTCCheckpointSig want;
    want.nHeight = pindex->nHeight;
    want.sysHash = pindex->GetBlockHash();

    // CL-like: sign msgHash = sysHash, while requestId binds height and quorum.
    const uint256 msgHash = want.sysHash;

    const auto& consensus = Params().GetConsensus();
    const auto& llmqParams = consensus.llmqTypeChainLocks;
    const auto& signingActiveQuorumCount = llmqParams.signingActiveQuorumCount;

    const auto quorums_scanned = llmq::quorumManager->ScanQuorums(pindex, signingActiveQuorumCount);
    if (quorums_scanned.empty()) {
        return;
    }

    auto proTxHash = WITH_LOCK(activeMasternodeInfoCs, return activeMasternodeInfo.proTxHash);

    LOCK(cs);
    if (lastSignedHeight == want.nHeight && lastSignedSysHash == want.sysHash) {
        auto it = bestCandidates.find(want.nHeight);
        if (it != bestCandidates.end() && it->second && it->second->sysHash == want.sysHash) {
            // Suppress redundant signing only after this exact target was already aggregated.
            return;
        }
    }
    bool queued_sign = false;
    for (size_t i = 0; i < quorums_scanned.size(); ++i) {
        const CQuorumCPtr& quorum = quorums_scanned[i];
        if (quorum == nullptr) return;
        if (!quorum->IsValidMember(proTxHash)) continue;

        const uint256 requestId = ::SerializeHash(std::make_tuple(BTCCHECK_REQUESTID_PREFIX, want.nHeight, quorum->qc->quorumHash));
        mapSignedRequestIds.emplace(requestId, std::make_pair(want.nHeight, want.sysHash));
        while (mapSignedRequestIds.size() > MAX_SIGNED_REQUESTIDS) {
            mapSignedRequestIds.erase(mapSignedRequestIds.begin());
        }
        quorumSigningManager->AsyncSignIfMember(requestId, msgHash, quorum->qc->quorumHash);
        queued_sign = true;
    }
    if (queued_sign) {
        lastSignedHeight = want.nHeight;
        lastSignedSysHash = want.sysHash;
        lastSignedBTCHash = btcPrevHash;
        lastSignedBTCHeight = btcHeight;
        lastPolicyRejectHeight = -1;
        lastPolicyRejectReason.clear();
    }
}

bool CBTCCheckpointsHandler::VerifyBTCCheckpointShare(const CBTCCheckpointSig& btcsig, const CBlockIndex* pindexScan, const uint256& idIn, std::pair<int, CQuorumCPtr>& ret, const uint256& hash) const
{
    {
        LOCK(cs);
        if (sigChecked.find(hash) != sigChecked.end()) {
            // Cache hits must still populate ret for callers that key state by
            // signer index/quorum pointer.
            const auto& consensus = Params().GetConsensus();
            const auto& llmqParams = consensus.llmqTypeChainLocks;
            const auto& signingActiveQuorumCount = llmqParams.signingActiveQuorumCount;

            const auto quorums_scanned = llmq::quorumManager->ScanQuorums(pindexScan, signingActiveQuorumCount);
            if (quorums_scanned.empty()) return false;

            for (size_t i = 0; i < quorums_scanned.size(); ++i) {
                const CQuorumCPtr& quorum = quorums_scanned[i];
                if (quorum == nullptr) return false;

                const uint256 requestId = ::SerializeHash(std::make_tuple(BTCCHECK_REQUESTID_PREFIX, btcsig.nHeight, quorum->qc->quorumHash));
                if (!idIn.IsNull() && requestId != idIn) continue;
                if (idIn.IsNull()) {
                    // For p2p-delivered shares, determine quorum by signer bit (must be exactly one).
                    if (btcsig.signers.size() != (size_t)signingActiveQuorumCount) return false;
                    if (!btcsig.signers[i]) continue;
                }
                ret = std::make_pair((int)i, quorum);
                return true;
            }
            return false;
        }
    }
    const auto& consensus = Params().GetConsensus();
    const auto& llmqParams = consensus.llmqTypeChainLocks;
    const auto& signingActiveQuorumCount = llmqParams.signingActiveQuorumCount;

    const auto quorums_scanned = llmq::quorumManager->ScanQuorums(pindexScan, signingActiveQuorumCount);
    if (quorums_scanned.empty()) return false;

    const uint256 msgHash = btcsig.sysHash;

    for (size_t i = 0; i < quorums_scanned.size(); ++i) {
        const CQuorumCPtr& quorum = quorums_scanned[i];
        if (quorum == nullptr) return false;

        const uint256 requestId = ::SerializeHash(std::make_tuple(BTCCHECK_REQUESTID_PREFIX, btcsig.nHeight, quorum->qc->quorumHash));
        if (!idIn.IsNull() && requestId != idIn) continue;
        if (idIn.IsNull()) {
            // For p2p-delivered shares, determine quorum by signer bit (must be exactly one).
            if (btcsig.signers.size() != (size_t)signingActiveQuorumCount) return false;
            if (!btcsig.signers[i]) continue;
        }

        const uint256 signHash = llmq::BuildSignHash(quorum->qc->quorumHash, requestId, msgHash);
        if (!btcsig.sig.VerifyInsecure(quorum->qc->quorumPublicKey, signHash)) {
            return false;
        }

        if (idIn.IsNull() && !quorumSigningManager->HasRecoveredSigForId(requestId)) {
            // Mirror ChainLocks behavior: reconstruct recovered sig from validated share.
            // This avoids double verification and keeps signing-manager caches consistent
            // even if we learned the share via BTCCSIG gossip instead of QSIGREC.
            auto rs = std::make_shared<CRecoveredSig>(quorum->qc->quorumHash, requestId, msgHash, btcsig.sig);
            quorumSigningManager->PushReconstructedRecoveredSig(rs);
        }

        ret = std::make_pair((int)i, quorum);
        {
            LOCK(cs);
            sigChecked.emplace(hash, TicksSinceEpoch<std::chrono::milliseconds>(SystemClock::now()));
        }
        return true;
    }
    return false;
}

bool CBTCCheckpointsHandler::TryUpdateBestBTCCheckpoint(const CBlockIndex* pindexScan)
{
    const int32_t height = pindexScan->nHeight;
    auto it2 = bestShares.find(height);
    if (it2 == bestShares.end()) return false;

    const auto& llmqParams = Params().GetConsensus().llmqTypeChainLocks;
    const size_t threshold = llmqParams.signingActiveQuorumCount / 2 + 1;

    std::vector<CBLSSignature> sigs;
    CBTCCheckpointSig agg;

    for (const auto& pair : it2->second) {
        const auto& share = pair.second;
        if (share->sysHash != pindexScan->GetBlockHash()) {
            continue;
        }

        sigs.emplace_back(share->sig);
        if (agg.IsNull()) {
            agg = *share;
        } else {
            std::transform(agg.signers.begin(), agg.signers.end(), share->signers.begin(), agg.signers.begin(), std::logical_or<bool>());
        }
        if (sigs.size() >= threshold) {
            agg.sig = CBLSSignature::AggregateInsecure(sigs);
            bestCandidates[height] = std::make_shared<const CBTCCheckpointSig>(agg);
            AddRecentBTCCheckpoint(agg);
            return true;
        }
    }

    return false;
}

void CBTCCheckpointsHandler::AddRecentBTCCheckpoint(const CBTCCheckpointSig& btcsig)
{
    // Match ChainLocks semantics: newer aggregates can replace older ones at same height.
    recentBTCCheckpoints[btcsig.nHeight] = btcsig;
    while ((int32_t)recentBTCCheckpoints.size() > RECENT_BTCCHECKPOINTS_MAX) {
        // prune lowest height entry (map is ascending by height)
        recentBTCCheckpoints.erase(recentBTCCheckpoints.begin());
    }
}

void CBTCCheckpointsHandler::AddPendingVerifiedBTCCheckpointSig(const uint256& hash, const CBTCCheckpointSig& btccsig)
{
    const int64_t now = TicksSinceEpoch<std::chrono::milliseconds>(SystemClock::now());
    LOCK(cs);
    // Mark as seen once we've fully verified it (prevents re-processing spam).
    seenBTCCheckpointSigs.emplace(hash, now);
    pendingVerifiedBTCCheckpointSigs[hash] = std::make_pair(btccsig, now);

    // Bound memory; these are fully verified objects so we keep eviction simple.
    while (pendingVerifiedBTCCheckpointSigs.size() > PENDING_VERIFIED_BTCCSIG_MAX) {
        auto oldest = pendingVerifiedBTCCheckpointSigs.end();
        for (auto it = pendingVerifiedBTCCheckpointSigs.begin(); it != pendingVerifiedBTCCheckpointSigs.end(); ++it) {
            if (oldest == pendingVerifiedBTCCheckpointSigs.end() || it->second.second < oldest->second.second) {
                oldest = it;
            }
        }
        if (oldest == pendingVerifiedBTCCheckpointSigs.end()) break;
        pendingVerifiedBTCCheckpointSigs.erase(oldest);
    }
}

void CBTCCheckpointsHandler::AcceptVerifiedBTCCSig(const CBTCCheckpointSig& btccsig, const uint256& hash, const CBlockIndex* pindexScan)
{
    Assert(pindexScan != nullptr);

    // Cheap re-check #1: still active chain, matching sysHash at that height.
    {
        LOCK(cs_main);
        if (btccsig.nHeight < 0 || btccsig.nHeight > chainman.ActiveHeight()) return;
        if (chainman.ActiveChain()[btccsig.nHeight] != pindexScan) return;
        if (pindexScan->GetBlockHash() != btccsig.sysHash) return;
    }

    const auto& llmqParams = Params().GetConsensus().llmqTypeChainLocks;
    if (btccsig.signers.size() != (size_t)llmqParams.signingActiveQuorumCount) return;

    const size_t signers_count = std::count(btccsig.signers.begin(), btccsig.signers.end(), true);
    if (signers_count == 1) {
        // Share: determine quorum by signer bit and store without re-verifying BLS.
        const auto quorums_scanned = llmq::quorumManager->ScanQuorums(pindexScan, llmqParams.signingActiveQuorumCount);
        if (quorums_scanned.empty()) return;

        CQuorumCPtr quorum{nullptr};
        for (size_t i = 0; i < quorums_scanned.size() && i < btccsig.signers.size(); ++i) {
            if (!btccsig.signers[i]) continue;
            quorum = quorums_scanned[i];
            break;
        }
        if (quorum == nullptr) return;

        bool agg{false};
        uint256 agg_hash;
        {
            LOCK(cs);
            bestShares[btccsig.nHeight].emplace(quorum, std::make_shared<const CBTCCheckpointSig>(btccsig));
            agg = TryUpdateBestBTCCheckpoint(pindexScan);
            if (agg) {
                auto it = bestCandidates.find(btccsig.nHeight);
                if (it != bestCandidates.end() && it->second) {
                    agg_hash = ::SerializeHash(*it->second);
                    seenBTCCheckpointSigs.emplace(agg_hash, TicksSinceEpoch<std::chrono::milliseconds>(SystemClock::now()));
                }
            }
        }
        if (agg && !agg_hash.IsNull()) {
            peerman.RelayInv(CInv(MSG_BTCCSIG, agg_hash));
        }
        return;
    }

    // Aggregate: store and relay.
    bool accepted{false};
    {
        LOCK(cs);
        auto it = bestCandidates.find(btccsig.nHeight);
        if (it == bestCandidates.end() || it->second == nullptr || it->second->sysHash != btccsig.sysHash) {
            bestCandidates[btccsig.nHeight] = std::make_shared<const CBTCCheckpointSig>(btccsig);
            AddRecentBTCCheckpoint(btccsig);
            accepted = true;
        }
    }
    if (accepted) {
        peerman.RelayInv(CInv(MSG_BTCCSIG, hash));
    }
}

void CBTCCheckpointsHandler::ProcessPendingVerifiedBTCCheckpointSigs()
{
    Cleanup();

    int32_t expectedHeight{-1};
    {
        LOCK(cs_main);
        expectedHeight = GetExpectedBTCCheckpointHeight(chainman);
    }
    if (expectedHeight == -1) return;

    const int64_t now = TicksSinceEpoch<std::chrono::milliseconds>(SystemClock::now());
    std::vector<std::pair<uint256, CBTCCheckpointSig>> to_process;
    {
        LOCK(cs);
        for (auto it = pendingVerifiedBTCCheckpointSigs.begin(); it != pendingVerifiedBTCCheckpointSigs.end();) {
            if (now - it->second.second >= PENDING_VERIFIED_BTCCSIG_TIMEOUT) {
                it = pendingVerifiedBTCCheckpointSigs.erase(it);
                continue;
            }
            if (it->second.first.nHeight == expectedHeight) {
                to_process.emplace_back(it->first, it->second.first);
                it = pendingVerifiedBTCCheckpointSigs.erase(it);
                continue;
            }
            ++it;
        }
    }
    if (to_process.empty()) return;

    for (const auto& item : to_process) {
        const uint256& hash = item.first;
        const CBTCCheckpointSig& btccsig = item.second;
        const CBlockIndex* pindexScan{nullptr};
        {
            LOCK(cs_main);
            pindexScan = chainman.m_blockman.LookupBlockIndex(btccsig.sysHash);
            if (pindexScan == nullptr || pindexScan->nHeight != btccsig.nHeight) {
                continue;
            }
        }
        AcceptVerifiedBTCCSig(btccsig, hash, pindexScan);
    }
}

void CBTCCheckpointsHandler::HandleNewRecoveredSig(const CRecoveredSig& recoveredSig)
{
    Cleanup();

    CBTCCheckpointSig share;
    bool haveSignedTarget{false};
    {
        LOCK(cs);
        auto it = mapSignedRequestIds.find(recoveredSig.getId());
        if (it != mapSignedRequestIds.end()) {
            haveSignedTarget = true;
            share.nHeight = it->second.first;
            share.sysHash = it->second.second;
            mapSignedRequestIds.erase(it);
        }
    }

    const CBlockIndex* pindexScan;
    int32_t expectedHeight{-1};
    {
        LOCK(cs_main);
        expectedHeight = GetExpectedBTCCheckpointHeight(chainman);
        pindexScan = chainman.m_blockman.LookupBlockIndex(recoveredSig.getMsgHash());
    }
    if (!haveSignedTarget) {
        // ChainLocks parity: recovered-sig aggregation path is only for ids this node signed.
        // Non-signers should learn BTCC through MSG_BTCCSIG objects.
        return;
    }
    if (pindexScan == nullptr) return;
    {
        LOCK(cs_main);
        if (expectedHeight == -1) return;
        if (pindexScan->nHeight < 0 || pindexScan->nHeight > chainman.ActiveHeight()) return;
        if (chainman.ActiveChain()[pindexScan->nHeight] != pindexScan) {
            // Don't accept recovered sigs for non-active forks.
            return;
        }
    }

    if (recoveredSig.getMsgHash() != share.sysHash || pindexScan->nHeight != share.nHeight) return;
    if (share.nHeight != expectedHeight) {
        // Strict height-window acceptance (ChainLocks parity).
        return;
    }

    {
        LOCK(cs);
        // If we already have an aggregated candidate for this height, drop further processing.
        // This mirrors ChainLocks behavior of ignoring superseded/stale material.
        auto itCand = bestCandidates.find(share.nHeight);
        if (itCand != bestCandidates.end() && itCand->second != nullptr && itCand->second->sysHash == share.sysHash) {
            return;
        }
    }

    std::pair<int, CQuorumCPtr> ret;
    share.sig = recoveredSig.sig.Get();
    share.signers.clear();
    const auto& llmqParams = Params().GetConsensus().llmqTypeChainLocks;
    share.signers.resize(llmqParams.signingActiveQuorumCount, false);

    // Verify against the recoveredSig id. The share hash used for caching is computed before the signer bit is set;
    // after verification we re-hash the fully-formed share for correct INV/getdata.
    const uint256 share_hash_pre = ::SerializeHash(share);
    if (!VerifyBTCCheckpointShare(share, pindexScan, recoveredSig.getId(), ret, share_hash_pre)) {
        return;
    }
    share.signers[ret.first] = true;
    const uint256 share_hash = ::SerializeHash(share);
    {
        LOCK(cs);
        // Mark the fully-formed share as seen so AlreadyHave/GetData works with the relayed INV.
        seenBTCCheckpointSigs.emplace(share_hash, TicksSinceEpoch<std::chrono::milliseconds>(SystemClock::now()));
        sigChecked.emplace(share_hash, TicksSinceEpoch<std::chrono::milliseconds>(SystemClock::now()));
    }

    bool agg{false};
    std::string agg_str;
    {
        LOCK(cs);
        bestShares[share.nHeight].emplace(ret.second, std::make_shared<const CBTCCheckpointSig>(share));
        agg = TryUpdateBestBTCCheckpoint(pindexScan);
        if (agg) {
            auto it = bestCandidates.find(share.nHeight);
            if (it != bestCandidates.end() && it->second) {
                agg_str = it->second->ToString();
            }
        }
    }
    if (!agg_str.empty()) {
        LogPrint(BCLog::CHAINLOCKS, "CBTCCheckpointsHandler -- aggregated btcc (%s)\n", agg_str);
    }
    if (agg) {
        CBTCCheckpointSig best;
        {
            LOCK(cs);
            auto it = bestCandidates.find(share.nHeight);
            if (it != bestCandidates.end() && it->second) {
                best = *it->second;
            }
        }
        if (!best.IsNull()) {
            const uint256 hash = ::SerializeHash(best);
            {
                LOCK(cs);
                seenBTCCheckpointSigs.emplace(hash, TicksSinceEpoch<std::chrono::milliseconds>(SystemClock::now()));
            }
            peerman.RelayInv(CInv(MSG_BTCCSIG, hash));
        }
    } else {
        // Relay partial shares to full nodes only, mirroring ChainLocks behavior.
        LOCK(cs_main);
        connman.ForEachNode([&](CNode* pnode) EXCLUSIVE_LOCKS_REQUIRED(::cs_main) {
            AssertLockHeld(::cs_main);
            bool fSPV{pnode->m_bloom_filter_loaded.load()};
            if (!fSPV && pnode->CanRelay()) {
                PeerRef peer = peerman.GetPeerRef(pnode->GetId());
                if (peer) {
                    peerman.PushTxInventoryOther(*peer, CInv(MSG_BTCCSIG, share_hash));
                }
            }
        });
    }
}

CBTCCheckpointSig CBTCCheckpointsHandler::GetMostRecentBTCCheckpoint() const
{
    LOCK(cs);
    if (recentBTCCheckpoints.empty()) {
        return CBTCCheckpointSig();
    }
    return recentBTCCheckpoints.rbegin()->second;
}

CBTCCheckpointSig CBTCCheckpointsHandler::GetBestBTCCheckpoint() const
{
    LOCK(cs);
    if (bestCandidates.empty()) {
        return CBTCCheckpointSig();
    }
    auto it = bestCandidates.rbegin();
    if (!it->second) {
        return CBTCCheckpointSig();
    }
    return *it->second;
}

std::map<CQuorumCPtr, CBTCCheckpointSigCPtr> CBTCCheckpointsHandler::GetBestBTCCheckpointShares() const
{
    LOCK(cs);
    if (bestCandidates.empty()) {
        return {};
    }
    auto it = bestCandidates.rbegin();
    if (!it->second) {
        return {};
    }
    auto jt = bestShares.find(it->first);
    if (jt == bestShares.end()) {
        return {};
    }
    return jt->second;
}

bool CBTCCheckpointsHandler::GetRecentBTCCheckpointByHeight(int32_t nHeight, CBTCCheckpointSig& ret) const
{
    LOCK(cs);
    auto it = recentBTCCheckpoints.find(nHeight);
    if (it == recentBTCCheckpoints.end()) {
        return false;
    }
    ret = it->second;
    return true;
}

bool CBTCCheckpointsHandler::VerifyAggregatedBTCCheckpoint(const CBTCCheckpointSig& btcsig, const CBlockIndex* pindexScan) const
{
    const uint256 hash = ::SerializeHash(btcsig);
    {
        LOCK(cs);
        if (sigChecked.find(hash) != sigChecked.end()) {
            return true;
        }
    }
    const bool ok = VerifyAggregatedBTCCheckpointNoCache(btcsig, pindexScan);
    if (ok) {
        LOCK(cs);
        sigChecked.emplace(hash, TicksSinceEpoch<std::chrono::milliseconds>(SystemClock::now()));
    }
    return ok;
}

} // namespace llmq

