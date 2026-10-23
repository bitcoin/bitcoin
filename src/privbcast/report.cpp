// Copyright (c) 2026-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <privbcast/report.h>

#include <netaddress.h>
#include <privbcast/assign.h>
#include <privbcast/attempt.h>
#include <privbcast/params.h>
#include <privbcast/plan.h>
#include <util/check.h>
#include <util/strencodings.h>

#include <univalue.h>

#include <cassert>
#include <chrono>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace privbcast {
namespace {

/** An offset from job start, or null for an event that did not happen. */
UniValue Time(const std::optional<std::chrono::milliseconds>& time)
{
    return time ? UniValue{time->count()} : UniValue{};
}

UniValue Endpoints(const std::vector<CService>& endpoints)
{
    UniValue list{UniValue::VARR};
    for (const CService& endpoint : endpoints) list.push_back(endpoint.ToStringAddrPort());
    return list;
}

std::string StratumName(Stratum stratum)
{
    switch (stratum) {
    case Stratum::Prompt: return "prompt";
    case Stratum::Mid: return "mid";
    case Stratum::Late: return "late";
    } // no default case, so the compiler can warn about missing cases
    assert(false);
}

std::string OutcomeName(Outcome outcome)
{
    switch (outcome) {
    case Outcome::NotAnnounced: return "not_announced";
    case Outcome::AnnouncedNotRequested: return "announced_not_requested";
    case Outcome::TxWrittenNoPong: return "tx_written_no_pong";
    case Outcome::PongReceived: return "pong_received";
    case Outcome::PostAnnouncementFailure: return "post_announcement_failure";
    } // no default case, so the compiler can warn about missing cases
    assert(false);
}

UniValue AttemptToUniValue(const AttemptReport& attempt, bool package)
{
    UniValue obj{UniValue::VOBJ};
    obj.pushKV("endpoint", attempt.candidate.endpoint.ToStringAddrPort());
    obj.pushKV("provenance", attempt.candidate.provenance);
    obj.pushKV("outcome", OutcomeName(attempt.outcome));
    obj.pushKV("reason", attempt.reason);
    obj.pushKV("scheduled_start_ms", attempt.scheduled_start.count());
    obj.pushKV("started_ms", attempt.started.count());
    obj.pushKV("connected_ms", Time(attempt.times.connected));
    obj.pushKV("peer_version", attempt.peer_version ? UniValue{*attempt.peer_version} : UniValue{});
    obj.pushKV("peer_user_agent", attempt.peer_user_agent ? UniValue{SanitizeString(*attempt.peer_user_agent)} : UniValue{});
    obj.pushKV("inv_handed_ms", Time(attempt.times.inv_handed));
    obj.pushKV("inv_written_ms", Time(attempt.times.inv_written));
    obj.pushKV("getdata_ms", Time(attempt.times.getdata));
    obj.pushKV("tx_written_ms", Time(attempt.times.tx_written));
    if (package) {
        obj.pushKV("parent_getdata_ms", Time(attempt.times.parent_getdata));
        obj.pushKV("parent_tx_written_ms", Time(attempt.times.parent_tx_written));
        obj.pushKV("parent_hold_expired_ms", Time(attempt.times.parent_hold_expired));
    }
    obj.pushKV("ping_written_ms", Time(attempt.times.ping_written));
    obj.pushKV("pong_ms", Time(attempt.times.pong));
    obj.pushKV("ended_ms", Time(attempt.times.ended));
    obj.pushKV("extra_requests", attempt.extra_requests);
    obj.pushKV("bytes_sent", attempt.bytes_sent);
    obj.pushKV("bytes_recv", attempt.bytes_recv);
    return obj;
}

UniValue SlotToUniValue(const SlotReport& slot, bool package)
{
    UniValue obj{UniValue::VOBJ};
    obj.pushKV("slot", slot.schedule.index);
    obj.pushKV("stratum", StratumName(slot.schedule.stratum));
    UniValue scheduled{UniValue::VARR};
    for (const std::chrono::milliseconds start : slot.schedule.scheduled) scheduled.push_back(start.count());
    obj.pushKV("scheduled_ms", std::move(scheduled));
    obj.pushKV("scheduled_end_ms", slot.schedule.scheduled_end.count());
    obj.pushKV("empty_opportunities", slot.empty_opportunities);
    obj.pushKV("missed_opportunities", slot.missed_opportunities);
    obj.pushKV("interrupted", slot.interrupted);
    UniValue attempts{UniValue::VARR};
    for (const AttemptReport& attempt : slot.attempts) attempts.push_back(AttemptToUniValue(attempt, package));
    obj.pushKV("attempts", std::move(attempts));
    return obj;
}

} // namespace

UniValue ToUniValue(const DiscoveryReport& discovery, bool candidates)
{
    UniValue obj{UniValue::VOBJ};
    obj.pushKV("duration_ms", discovery.duration.count());
    UniValue seeds{UniValue::VARR};
    for (const SeedSummary& seed : discovery.seeds) {
        UniValue entry{UniValue::VOBJ};
        entry.pushKV("name", seed.name);
        entry.pushKV("queries", seed.queries);
        entry.pushKV("skipped", seed.skipped);
        entry.pushKV("answers", seed.answers);
        entry.pushKV("accepted", seed.accepted);
        if (candidates) entry.pushKV("candidates", Endpoints(seed.candidates));
        seeds.push_back(std::move(entry));
    }
    obj.pushKV("seeds", std::move(seeds));
    obj.pushKV("duplicates", discovery.duplicates);
    obj.pushKV("rejected", discovery.rejected);
    obj.pushKV("exit_path_candidates", discovery.exit_path_candidates);
    return obj;
}

UniValue ToUniValue(const Report& report)
{
    UniValue obj{UniValue::VOBJ};
    obj.pushKV("txid", report.txid.GetHex());
    obj.pushKV("wtxid", report.wtxid.GetHex());
    // Package mode only (Extension).
    const bool package{report.parent_txid.has_value()};
    if (package) {
        obj.pushKV("parent_txid", report.parent_txid->GetHex());
        obj.pushKV("parent_wtxid", Assert(report.parent_wtxid)->GetHex());
    }
    obj.pushKV("chain", report.chain);
    obj.pushKV("discovery", ToUniValue(report.discovery, /*candidates=*/false));
    UniValue slots{UniValue::VARR};
    for (const SlotReport& slot : report.slots) slots.push_back(SlotToUniValue(slot, package));
    obj.pushKV("slots", std::move(slots));
    UniValue summary{UniValue::VOBJ};
    summary.pushKV("connections", report.summary.connections);
    summary.pushKV("announcements_handed", report.summary.announcements_handed);
    summary.pushKV("announcements_written", report.summary.announcements_written);
    summary.pushKV("tx_written", report.summary.tx_written);
    summary.pushKV("pongs", report.summary.pongs);
    if (package) summary.pushKV("parents_served", report.summary.parents_served);
    summary.pushKV("slots_completed", report.summary.slots_completed);
    summary.pushKV("interrupted", report.summary.interrupted);
    summary.pushKV("error", report.summary.error ? UniValue{*report.summary.error} : UniValue{});
    summary.pushKV("duration_ms", report.summary.duration.count());
    obj.pushKV("summary", std::move(summary));
    return obj;
}

int ExitStatus(const Report& report)
{
    if (report.summary.announcements_written > 0) return EXIT_ANNOUNCED;
    return report.summary.error ? EXIT_JOB_FAILED : EXIT_NOT_ANNOUNCED;
}

} // namespace privbcast
