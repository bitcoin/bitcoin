# Issue: RPC latency degradation under sustained P2P traffic

## Problem

Under sustained low-priority P2P traffic (tx relay, addr gossip), RPC tail latency can degrade significantly, especially on resource-constrained nodes.

### Root Cause

The single-threaded message handler (`-msghand` thread) processes all P2P messages sequentially without considering RPC queue pressure:

1. **Single-threaded bottleneck** - The msghand thread can saturate a single CPU core at 100%, leaving other cores idle while processing P2P messages
2. **No priority differentiation** - Low-priority P2P messages (INV, TX, ADDR) compete equally with time-sensitive operations
3. **RPC starvation** - When the RPC work queue fills up during heavy P2P processing, new RPC requests are rejected with "Work queue depth exceeded"

### Affected Users

- Wallet operators making frequent RPC calls
- Block explorers and indexing services
- Lightning Network nodes requiring low-latency RPC
- Any node running RPC-heavy workloads alongside P2P relay

### Observed Symptoms

- RPC p95/p99 latency spikes during tx relay floods
- "Work queue depth exceeded" errors under load
- Inconsistent RPC response times

## Proposed Solution

Add an opt-in backpressure mechanism that defers low-priority P2P message processing when the RPC work queue is under pressure.

### Key Design Points

1. **RpcLoadMonitor** - Lock-free state machine tracking RPC queue depth with hysteresis
2. **Message Classification** - Separate low-priority (TX, INV, ADDR) from critical (HEADERS, BLOCK, CMPCTBLOCK)
3. **Defer-to-tail** - Low-priority messages requeued to back of queue, not dropped
4. **Opt-in** - Disabled by default via `-experimental-rpc-priority` flag

### State Machine

```
NORMAL → ELEVATED: queue ≥ 75%
ELEVATED → NORMAL: queue < 50%  (hysteresis)
ELEVATED → CRITICAL: queue ≥ 90%
CRITICAL → ELEVATED: queue < 70%
```

### Preliminary Results

A/B test with P2P INV flood (~108K entries) and concurrent RPC workload:

| Metric | Improvement |
|--------|-------------|
| RPC p95 | -16.79% (better) |
| RPC p99 | -11.11% (better) |
| Throughput | +9.4% |

## Implementation

PR: [link to PR]

### Changes
- New: `src/node/rpc_load_monitor.h` - State machine implementation
- Modified: `src/net_processing.cpp` - Backpressure gate in ProcessMessages()
- Modified: `src/httpserver.cpp` - Queue depth sampling
- New flag: `-experimental-rpc-priority`

### Testing
- 12 unit tests for state machine
- Functional A/B test with metrics

## Questions for Discussion

1. Are the threshold values (75%/90% enter, 50%/70% leave) reasonable defaults?
2. Should message classification be configurable?
3. Is defer-to-tail the right strategy vs. other approaches (drop, rate-limit)?
4. Should this eventually become default behavior?

## Related Discussions

- Mining pools have reported ProcessMessage CPU saturation causing block delays
- Previous requests to split msghand thread into multiple threads
- Analysis of CPU time distribution in ProcessMessages()

This PR provides a lightweight mitigation without requiring architectural changes to the threading model.
