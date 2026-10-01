// Copyright (c) 2026-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <privbcast/job.h>

#include <compat/compat.h>
#include <netaddress.h>
#include <primitives/transaction.h>
#include <privbcast/assign.h>
#include <privbcast/attempt.h>
#include <privbcast/discovery.h>
#include <privbcast/params.h>
#include <privbcast/plan.h>
#include <privbcast/report.h>
#include <privbcast/socks5.h>
#include <privbcast/stream.h>
#include <random.h>
#include <tinyformat.h>
#include <util/check.h>
#include <util/log.h>
#include <util/sock.h>
#include <util/time.h>

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <exception>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <utility>
#include <vector>

namespace privbcast {
namespace {

/** The most read from a recipient at a time. */
constexpr size_t READ_SIZE{64 * 1024};

} // namespace

Job::Job(JobInputs inputs, std::unique_ptr<FastRandomContext> rng)
    : m_inputs{std::move(inputs)},
      m_rng{rng ? std::move(rng) : std::make_unique<FastRandomContext>()},
      m_start{m_inputs.start ? *m_inputs.start : NodeClock::now()},
      m_plan{DrawPlan(*m_rng, m_inputs.timing, m_inputs.seeds.dns_seeds)},
      m_discovery{m_plan, m_inputs.proxy, m_inputs.seeds.default_port, *m_rng},
      m_buffer(READ_SIZE)
{
}

Report Job::Run(const std::function<void(const Progress&)>& progress)
{
    Progress passed;
    while (!Step(LOOP_WAIT)) {
        if (!progress || Snapshot() == passed) continue;
        passed = Snapshot();
        progress(passed);
    }
    return TakeReport();
}

bool Job::Step(std::chrono::milliseconds max_wait)
{
    if (m_report) return true;
    std::optional<std::chrono::milliseconds> end;
    try {
        end = Iterate(max_wait);
    } catch (const std::exception& e) {
        // What the attempts and the queries do not contain is a failure of the job (C7).
        end = PlanNow();
        Fail(*end, e.what());
    }
    if (!end) return false;
    Finish(*end);
    return true;
}

std::optional<std::chrono::milliseconds> Job::Iterate(std::chrono::milliseconds max_wait)
{
    std::chrono::milliseconds now{PlanNow()};
    SteadyMs steady_now{Now<SteadyMs>()};
    if (Cancelled()) {
        Cancel(now);
        return now;
    }
    if (!m_started) {
        m_started = true;
        LogDebug(BCLog::PRIVBROADCAST, "%s: started through %s\n", LogName(), m_inputs.proxy.ToString());
        m_discovery.Start(now, steady_now);
    }
    Advance(now, steady_now);
    if (Over()) return now;

    // Wait for what every open stream waits for, until the next deadline at most.
    struct Waiting {
        std::shared_ptr<const Sock> sock;
        /** A discovery stream, or else the stream of this slot's attempt. */
        ProxyStream* query;
        size_t slot;
    };
    std::vector<Waiting> waiting;
    Sock::EventsPerSock events;
    for (ProxyStream* stream : m_discovery.ActiveStreams()) {
        waiting.push_back({stream->GetSock(), stream, 0});
        events.emplace(stream->GetSock(), Sock::Events{stream->WantedEvents()});
    }
    for (size_t i{0}; i < SLOTS; ++i) {
        if (!m_slots[i].running) continue;
        const Running& run{*m_slots[i].running};
        if (run.stream.GetPhase() == ProxyStream::Phase::Closed) continue;
        Sock::Event wanted{run.stream.WantedEvents()};
        if (run.stream.GetPhase() == ProxyStream::Phase::Open && !run.attempt->BytesToSend().empty()) wanted |= Sock::SendEvent;
        waiting.push_back({run.stream.GetSock(), nullptr, i});
        events.emplace(run.stream.GetSock(), Sock::Events{wanted});
    }
    const std::chrono::milliseconds wait{NextWait(now, steady_now, max_wait)};
    if (events.empty()) {
        if (wait > 0ms) UninterruptibleSleep(wait);
    } else if (!events.begin()->first->WaitMany(wait, events)) {
        LogDebug(BCLog::PRIVBROADCAST, "Waiting for the job's sockets failed: %s\n", NetworkErrorString(WSAGetLastError()));
        for (auto& [sock, ev] : events) ev.occurred = 0;
        UninterruptibleSleep(wait);
    }

    now = PlanNow();
    steady_now = Now<SteadyMs>();
    if (Cancelled()) {
        Cancel(now);
        return now;
    }
    for (const Waiting& w : waiting) {
        const auto it{events.find(w.sock)};
        const Sock::Event occurred{it == events.end() ? Sock::Event{0} : it->second.occurred};
        if (w.query) {
            m_discovery.OnStreamEvents(*w.query, occurred, now, steady_now);
        } else {
            OnAttemptEvents(w.slot, occurred, steady_now);
        }
    }
    for (size_t i{0}; i < SLOTS; ++i) Write(i);
    // Let go of the sockets held for the wait, so that a stream closed by now, or reaped below,
    // frees its socket before its slot dials again (D1).
    waiting.clear();
    events.clear();
    // The I/O takes time: the deadlines and dials that follow go by the clocks as they read now
    // (C5, D2).
    now = PlanNow();
    steady_now = Now<SteadyMs>();
    Advance(now, steady_now);
    if (Over()) return now;
    return std::nullopt;
}

Report Job::TakeReport()
{
    Assume(m_report);
    return std::move(m_report).value_or(Report{});
}

Report Job::RunDiscoveryOnly()
{
    Assume(!m_started);
    m_discovery_only = true;
    return Run();
}

std::chrono::milliseconds Job::PlanNow() const
{
    return std::chrono::floor<std::chrono::milliseconds>(NodeClock::now() - m_start);
}

bool Job::Cancelled() const
{
    return m_inputs.cancel && m_inputs.cancel->load();
}

void Job::Advance(std::chrono::milliseconds now, SteadyMs steady_now)
{
    if (!m_assignment) {
        m_discovery.Tick(now, steady_now);
        if (m_discovery.Done()) EndDiscovery();
    }
    if (m_assignment && !m_discovery_only) {
        for (size_t i{0}; i < SLOTS; ++i) AdvanceSlot(i, now, steady_now);
    }
    UpdateProgress();
}

void Job::AdvanceSlot(size_t index, std::chrono::milliseconds now, SteadyMs steady_now)
{
    Slot& slot{m_slots[index]};
    const SlotSchedule& schedule{m_plan.slots[index]};
    const auto& candidates{m_assignment->opportunities[index]};
    // An empty opportunity ends at its scheduled time, whatever its slot does.
    for (size_t k{0}; k < OPPORTUNITIES_PER_SLOT; ++k) {
        if (!candidates[k] && now >= schedule.scheduled[k]) slot.opportunity_ended[k] = true;
    }
    if (slot.ended) return;

    if (slot.running) {
        Running& run{*slot.running};
        try {
            // The attempt's deadlines, then the time of its stream's stage (D2).
            run.attempt->Tick(now);
            if (!run.attempt->Ended() && run.stream.GetPhase() != ProxyStream::Phase::Open) {
                run.stream.OnEvents(0, steady_now);
                if (run.stream.GetPhase() == ProxyStream::Phase::Closed) StreamClosed(run, now);
            }
        } catch (const std::exception& e) {
            AttemptFailed(run, now, e.what());
        }
        if (!run.attempt->Ended()) return;
        Reap(index);
    }

    // Before the announcement point, the slot's next opportunity runs at its scheduled time;
    // one that the loop reaches more than START_GRACE late is missed, never dialled (C4-C6).
    while (!slot.announced && !slot.running && slot.next < OPPORTUNITIES_PER_SLOT) {
        const size_t k{slot.next};
        if (candidates[k] && now < schedule.scheduled[k]) break;
        ++slot.next;
        if (!candidates[k]) continue;
        if (now > schedule.scheduled[k] + m_plan.timing.Scale(START_GRACE)) {
            ++slot.missed;
            slot.opportunity_ended[k] = true;
            LogDebug(BCLog::PRIVBROADCAST, "Slot %d: opportunity %d missed, reached %d ms late\n",
                     index, k, (now - schedule.scheduled[k]).count());
            continue;
        }
        try {
            Dial(index, k, now, steady_now);
        } catch (const std::exception& e) {
            if (slot.running) {
                AttemptFailed(*slot.running, now, e.what());
            } else {
                // Without an attempt to end, the opportunity could not be dialled (C5).
                ++slot.missed;
                slot.opportunity_ended[k] = true;
                LogDebug(BCLog::PRIVBROADCAST, "Slot %d: opportunity %d missed: %s\n", index, k, e.what());
                continue;
            }
        }
        if (slot.running->attempt->Ended()) Reap(index);
    }

    // The slot ends with its announced attempt, the rest of its opportunities dropped (C6), or else
    // once each of its opportunities has ended, an empty one at its scheduled time (D2).
    if (!slot.running && (slot.announced || std::ranges::all_of(slot.opportunity_ended, [](bool ended) { return ended; }))) {
        slot.ended = slot.completed = true;
        LogDebug(BCLog::PRIVBROADCAST, "Slot %d ended at %d ms\n", index, now.count());
    }
}

void Job::EndDiscovery()
{
    const DiscoveryResult result{m_discovery.Result()};
    m_discovery_duration = result.duration;
    m_assignment = Assign(m_plan, result);
    LogDebug(BCLog::PRIVBROADCAST, "Discovery found %d candidates\n", m_assignment->exit_path_candidates);
}

void Job::Dial(size_t index, size_t opportunity, std::chrono::milliseconds now, SteadyMs steady_now)
{
    Slot& slot{m_slots[index]};
    const Candidate& candidate{*m_assignment->opportunities[index][opportunity]};
    // Fresh BIP324 keys, nonces and proxy credentials for every attempt (A4, B2).
    auto attempt{std::make_unique<Attempt>(m_inputs.tx, m_plan.slots[index].scheduled[opportunity], m_plan.timing, *m_rng, m_inputs.keys, m_next_id++)};
    ProxyStream stream{m_inputs.proxy,
                       Socks5Client{Socks5Client::Command::Connect, candidate.endpoint.ToStringAddr(), candidate.endpoint.GetPort(), *m_rng},
                       m_plan.timing, steady_now};
    Running& run{slot.running.emplace(Running{opportunity, candidate, now, std::move(attempt), std::move(stream)})};
    LogDebug(BCLog::PRIVBROADCAST, "Slot %d: dialling %s from %s, opportunity %d\n",
             index, candidate.endpoint.ToStringAddrPort(), candidate.provenance, opportunity);
    run.stream.Open();
    // A stream that could not start, or that closed as it started, ends the attempt (C4, E7).
    if (run.stream.GetPhase() == ProxyStream::Phase::Closed) StreamClosed(run, now);
}

void Job::Reap(size_t index)
{
    Slot& slot{m_slots[index]};
    Running& run{*slot.running};
    run.stream.Close();
    const Attempt& attempt{*run.attempt};
    AttemptReport report;
    report.candidate = run.candidate;
    report.outcome = attempt.GetOutcome();
    report.reason = attempt.Reason();
    report.scheduled_start = m_plan.slots[index].scheduled[run.opportunity];
    report.started = run.started;
    report.times = attempt.Times();
    report.peer_version = attempt.PeerVersion();
    report.peer_user_agent = attempt.PeerUserAgent();
    report.extra_requests = attempt.ExtraRequests();
    report.bytes_sent = attempt.BytesSent();
    report.bytes_recv = attempt.BytesRecv();
    LogDebug(BCLog::PRIVBROADCAST, "Slot %d: attempt to %s ended at %d ms: %s\n",
             index, run.candidate.endpoint.ToStringAddrPort(), report.times.ended.value_or(0ms).count(), report.reason);
    slot.attempts.push_back(std::move(report));
    slot.opportunity_ended[run.opportunity] = true;
    // After the announcement point nothing else in the slot runs (C6).
    if (attempt.Times().inv_handed) slot.announced = true;
    slot.running.reset();
}

void Job::OnAttemptEvents(size_t index, Sock::Event occurred, SteadyMs steady_now)
{
    // The slots before this one may have taken time.
    const std::chrono::milliseconds now{PlanNow()};
    if (!m_slots[index].running) return;
    Running& run{*m_slots[index].running};
    Attempt& attempt{*run.attempt};
    try {
        // Deadlines first: nothing is acted on at or after one (D2).
        attempt.Tick(now);
        if (attempt.Ended()) return;
        ProxyStream& stream{run.stream};
        if (stream.GetPhase() == ProxyStream::Phase::Open) {
            if (occurred & (Sock::RecvEvent | Sock::ErrorEvent)) {
                const size_t received{stream.Recv(m_buffer)};
                if (received > 0) attempt.Received(std::span{m_buffer}.first(received), now);
            }
        } else if (stream.GetPhase() != ProxyStream::Phase::Closed) {
            stream.OnEvents(occurred, steady_now);
            if (stream.GetPhase() == ProxyStream::Phase::Open) {
                attempt.Connected(now, stream.TakeLeftover());
            }
        }
        if (stream.GetPhase() == ProxyStream::Phase::Closed && !attempt.Ended()) StreamClosed(run, now);
    } catch (const std::exception& e) {
        AttemptFailed(run, now, e.what());
    }
}

void Job::Write(size_t index)
{
    if (!m_slots[index].running) return;
    Running& run{*m_slots[index].running};
    Attempt& attempt{*run.attempt};
    try {
        while (!attempt.Ended() && run.stream.GetPhase() == ProxyStream::Phase::Open) {
            const std::span<const uint8_t> bytes{attempt.BytesToSend()};
            if (bytes.empty()) break;
            const size_t sent{run.stream.Send(bytes)};
            if (sent == 0) break;
            // When the write completed, which a slow one makes later than the step's time (C6,
            // Interface/Report).
            attempt.MarkSent(sent, PlanNow());
        }
        if (run.stream.GetPhase() == ProxyStream::Phase::Closed && !attempt.Ended()) StreamClosed(run, PlanNow());
    } catch (const std::exception& e) {
        AttemptFailed(run, PlanNow(), e.what());
    }
}

void Job::StreamClosed(Running& run, std::chrono::milliseconds now)
{
    if (run.stream.PeerClosed()) return run.attempt->ConnectionFailed(now, "peer closed the connection");
    LogDebug(BCLog::PROXY, "Connection to %s through %s failed: %s\n",
             run.candidate.endpoint.ToStringAddrPort(), m_inputs.proxy.ToString(), run.stream.Reason());
    run.attempt->ConnectionFailed(now, strprintf("socket error: %s", run.stream.Reason()));
}

void Job::AttemptFailed(Running& run, std::chrono::milliseconds now, const std::string& what)
{
    LogDebug(BCLog::PRIVBROADCAST, "Attempt to %s failed inside the job: %s\n", run.candidate.endpoint.ToStringAddrPort(), what);
    run.stream.Close();
    run.attempt->ConnectionFailed(now, what);
}

std::chrono::milliseconds Job::NextWait(std::chrono::milliseconds now, SteadyMs steady_now, std::chrono::milliseconds max_wait)
{
    std::chrono::milliseconds wait{max_wait};
    const auto until{[&](std::chrono::milliseconds at) { wait = std::min(wait, at - now); }};
    const auto until_steady{[&](std::optional<SteadyMs> at) {
        if (at) wait = std::min(wait, std::chrono::milliseconds{*at - steady_now});
    }};
    if (!m_assignment) {
        until(m_discovery.WindowEnd());
        for (const ProxyStream* stream : m_discovery.ActiveStreams()) until_steady(stream->NextDeadline());
    } else if (!m_discovery_only) {
        for (size_t i{0}; i < SLOTS; ++i) {
            const Slot& slot{m_slots[i]};
            if (slot.running) {
                if (const auto deadline{slot.running->attempt->NextDeadline()}) until(*deadline);
                until_steady(slot.running->stream.NextDeadline());
            }
            // An opportunity to dial, or an empty one to count as ended.
            for (size_t k{0}; k < OPPORTUNITIES_PER_SLOT; ++k) {
                const std::chrono::milliseconds at{m_plan.slots[i].scheduled[k]};
                if (!slot.opportunity_ended[k] && at > now) until(at);
            }
        }
    }
    return std::max(wait, std::chrono::milliseconds{0});
}

bool Job::Over() const
{
    return m_assignment && (m_discovery_only || std::ranges::all_of(m_slots, &Slot::ended));
}

std::string Job::LogName() const
{
    if (!m_inputs.tx) return "Private broadcast discovery";
    return strprintf("Private broadcast of txid=%s", m_inputs.tx->GetHash().ToString());
}

void Job::Cancel(std::chrono::milliseconds now)
{
    LogDebug(BCLog::PRIVBROADCAST, "%s cancelled\n", LogName());
    m_interrupted = true;
    Stop(now);
}

void Job::Fail(std::chrono::milliseconds now, const std::string& what)
{
    LogDebug(BCLog::PRIVBROADCAST, "%s failed: %s\n", LogName(), what);
    m_error = what;
    Stop(now);
}

void Job::Stop(std::chrono::milliseconds now)
{
    if (!m_assignment) {
        m_discovery.Interrupt(now);
        EndDiscovery();
    }
    // A blocked exchange, or a connect under way, is simply closed.
    for (size_t i{0}; i < SLOTS; ++i) {
        Slot& slot{m_slots[i]};
        if (slot.ended || m_discovery_only) continue;
        if (slot.running) {
            slot.running->attempt->Interrupt(now, m_error.value_or("interrupted"));
            Reap(i);
        }
        slot.interrupted = !m_error;
        slot.ended = true;
    }
}

void Job::Finish(std::chrono::milliseconds now)
{
    UpdateProgress();
    Report report;
    if (m_inputs.tx) {
        report.txid = m_inputs.tx->GetHash();
        report.wtxid = m_inputs.tx->GetWitnessHash();
    }
    report.chain = m_inputs.seeds.chain;

    DiscoveryReport& discovery{report.discovery};
    static_cast<DiscoverySummary&>(discovery) = *m_assignment;
    discovery.duration = m_discovery_duration;

    Summary& summary{report.summary};
    for (size_t i{0}; i < SLOTS; ++i) {
        Slot& slot{m_slots[i]};
        SlotReport& slot_report{report.slots.emplace_back()};
        slot_report.schedule = m_plan.slots[i];
        slot_report.empty_opportunities = static_cast<int>(std::ranges::count_if(m_assignment->opportunities[i], [](const auto& c) { return !c; }));
        slot_report.missed_opportunities = slot.missed;
        slot_report.interrupted = slot.interrupted;
        slot_report.attempts = std::move(slot.attempts);
        if (slot.completed) ++summary.slots_completed;
        for (const AttemptReport& attempt : slot_report.attempts) {
            ++summary.connections;
            if (attempt.times.inv_handed) ++summary.announcements_handed;
            if (attempt.times.inv_written) ++summary.announcements_written;
            if (attempt.times.tx_written) ++summary.tx_written;
            if (attempt.times.pong) ++summary.pongs;
        }
    }
    summary.interrupted = m_interrupted;
    summary.error = m_error;
    summary.duration = now;
    LogDebug(BCLog::PRIVBROADCAST, "%s ended at %d ms: %d connections, %d announcements written\n",
             LogName(), now.count(), summary.connections, summary.announcements_written);
    m_report = std::move(report);
}

void Job::UpdateProgress()
{
    Progress progress;
    progress.discovery_done = m_assignment.has_value();
    for (const Slot& slot : m_slots) {
        progress.opportunities_ended += static_cast<int>(std::ranges::count(slot.opportunity_ended, true));
        for (const AttemptReport& attempt : slot.attempts) {
            ++progress.connections;
            if (attempt.times.inv_written) ++progress.announcements_written;
        }
    }
    m_progress = progress;
}

} // namespace privbcast
