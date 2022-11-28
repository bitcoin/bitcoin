#include <node/connection_types.h>
#include <node/eviction.h>
#include <node/eviction_impl.h>
#include <optional>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <test/util/net.h>

#include <algorithm>
#include <chrono>
#include <cstdint>

constexpr int MAX_PEERS = 1000;

std::chrono::microseconds DELAYS[256];

struct Initializer {
    Initializer()
    {
        int i = 0;
        // DELAYS[N] for N=0..15 is just N microseconds.
        for (; i < 16; ++i) {
            DELAYS[i] = std::chrono::microseconds{i};
        }
        // DELAYS[N] for N=16..127 has randomly-looking but roughly exponentially increasing values up to
        // 198.416453 seconds.
        for (; i < 128; ++i) {
            int diff_bits = ((i - 10) * 2) / 9;
            uint64_t diff = 1 + (CSipHasher(0, 0).Write(i).Finalize() >> (64 - diff_bits));
            DELAYS[i] = DELAYS[i - 1] + std::chrono::microseconds{diff};
        }
        // DELAYS[N] for N=128..255 are negative delays with the same magnitude as N=0..127.
        for (; i < 256; ++i) {
            DELAYS[i] = -DELAYS[255 - i];
        }
    }
} g_initializer;

class Tester
{
private:
    EvictionManager m_evictionman;

    std::chrono::microseconds m_now;
    std::map<NodeId, NodeEvictionCandidate> m_candidates;
    int m_num_noban_or_outbound{0};

    std::optional<NodeEvictionCandidate> GetMockCandidate(NodeId id) const
    {
        if (auto it = m_candidates.find(id); it != m_candidates.end()) {
            return it->second;
        }
        return std::nullopt;
    }

    template <typename T = std::chrono::seconds>
    T Now() const
    {
        return std::chrono::duration_cast<T>(m_now);
    }

public:
    void AdvanceTime(uint8_t delay)
    {
        m_now += DELAYS[delay];
    }

    void AddCandidate(NodeId id, uint64_t keyed_net_group, bool prefer_evict,
                      bool is_local, Network network,
                      bool noban, ConnectionType conn_type)
    {
        bool already_added{false};
        if (m_evictionman.HasCandidate(id)) {
            already_added = true;
        }

        m_evictionman.AddCandidate(
            id, Now<>(),
            keyed_net_group, prefer_evict,
            is_local, network, noban, conn_type);

        assert(m_evictionman.HasCandidate(id));
        const auto [it, inserted] = m_candidates.emplace(
            id,
            NodeEvictionCandidate{
                .id = id,
                .m_connected = Now<>(),
                .m_min_ping_time = std::chrono::microseconds::max(),
                .m_last_block_time = 0s,
                .m_last_tx_time = 0s,
                .fRelevantServices = false,
                .m_relay_txs = false,
                .fBloomFilter = false,
                .nKeyedNetGroup = keyed_net_group,
                .prefer_evict = prefer_evict,
                .m_is_local = is_local,
                .m_network = network,
                .m_noban = noban,
                .m_conn_type = conn_type,
            });

        assert(inserted != already_added);

        if (inserted) {
            auto candidate = it->second;

            m_num_noban_or_outbound +=
                candidate.m_noban || conn_type != ConnectionType::INBOUND;
        }
    }
    void RemoveCandidate(NodeId id)
    {
        auto candidate{GetMockCandidate(id)};
        bool erased_mock{m_candidates.erase(id) > 0};
        bool erased_actual{m_evictionman.RemoveCandidate(id)};
        assert(erased_actual == erased_mock);
        if (erased_mock) {
            assert(candidate);
            m_num_noban_or_outbound -=
                candidate->m_noban || candidate->m_conn_type != ConnectionType::INBOUND;
        }
    }
    void SelectNodeToEvict(bool remove)
    {
        auto evicted{m_evictionman.SelectNodeToEvict()};
        assert(!evicted || m_candidates.count(*evicted) > 0);

        if (evicted) {
            assert(m_candidates.count(*evicted) > 0);

            // Check that the minimum number of peers for eviction to take
            // place has been reached.
            int min_protected{m_num_noban_or_outbound +
                              /*netgroup*/ 4 + /*ping time*/ 8 +
                              /*tx time*/ 4 +
                              /*novel block time*/ 4};
            assert((int)m_candidates.size() > min_protected);

            if (remove) {
                RemoveCandidate(*evicted);
            }
        }
    }

    void UpdateMinPingTime(NodeId id, int delay)
    {
        m_evictionman.UpdateMinPingTime(id, DELAYS[delay]);
        if (auto it = m_candidates.find(id); it != m_candidates.end()) {
            it->second.m_min_ping_time = std::min(it->second.m_min_ping_time, DELAYS[delay]);
        }
        GetMinPingTime(id);
    }
    void GetMinPingTime(NodeId id) const
    {
        if (auto it = m_candidates.find(id); it != m_candidates.end()) {
            assert(it->second.m_min_ping_time == m_evictionman.GetMinPingTime(id));
        }
    }

    void UpdateLastBlockTime(NodeId id)
    {
        m_evictionman.UpdateLastBlockTime(id, Now<>());
        if (auto it = m_candidates.find(id); it != m_candidates.end()) {
            it->second.m_last_block_time = Now<>();
        }
        GetLastBlockTime(id);
    }
    void GetLastBlockTime(NodeId id) const
    {
        if (auto it = m_candidates.find(id); it != m_candidates.end()) {
            assert(it->second.m_last_block_time == m_evictionman.GetLastBlockTime(id));
        }
    }

    void UpdateLastTxTime(NodeId id)
    {
        m_evictionman.UpdateLastTxTime(id, Now<>());
        if (auto it = m_candidates.find(id); it != m_candidates.end()) {
            it->second.m_last_tx_time = Now<>();
        }
        GetLastTxTime(id);
    }
    void GetLastTxTime(NodeId id) const
    {
        assert(m_candidates.count(id) != 0 == m_evictionman.HasCandidate(id));
        if (auto it = m_candidates.find(id); it != m_candidates.end()) {
            assert(it->second.m_last_tx_time == m_evictionman.GetLastTxTime(id));
        }
    }

    void UpdateRelevantServices(NodeId id, bool has_relevant_flags)
    {
        m_evictionman.UpdateRelevantServices(id, has_relevant_flags);
    }
    void UpdateLoadedBloomFilter(NodeId id, bool bloom_filter_loaded)
    {
        m_evictionman.UpdateLoadedBloomFilter(id, bloom_filter_loaded);
    }
    void UpdateRelayTxs(NodeId id)
    {
        m_evictionman.UpdateRelayTxs(id);
    }
};

FUZZ_TARGET(evictionman)
{
    FuzzedDataProvider fuzzed_data_provider{buffer.data(), buffer.size()};

    Tester tester;

    LIMITED_WHILE(fuzzed_data_provider.remaining_bytes(), 1000)
    {
        CallOneOf(
            fuzzed_data_provider,
            [&] {
                tester.AdvanceTime(fuzzed_data_provider.ConsumeIntegral<uint8_t>());
            },
            [&] {
                tester.SelectNodeToEvict(fuzzed_data_provider.ConsumeBool());
            },
            [&] {
                tester.AddCandidate(/*id=*/fuzzed_data_provider.ConsumeIntegralInRange(0, MAX_PEERS),
                                    /*keyed_net_group=*/fuzzed_data_provider.ConsumeIntegral<uint64_t>(),
                                    /*prefer_evict=*/fuzzed_data_provider.ConsumeBool(),
                                    /*is_local=*/fuzzed_data_provider.ConsumeBool(),
                                    /*network=*/fuzzed_data_provider.PickValueInArray(ALL_NETWORKS),
                                    /*noban=*/fuzzed_data_provider.ConsumeBool(),
                                    /*conn_type=*/fuzzed_data_provider.PickValueInArray(ALL_CONNECTION_TYPES));
            },
            [&] {
                tester.RemoveCandidate(fuzzed_data_provider.ConsumeIntegralInRange(0, MAX_PEERS));
            },
            [&] {
                tester.UpdateMinPingTime(fuzzed_data_provider.ConsumeIntegralInRange(0, MAX_PEERS),
                                         fuzzed_data_provider.ConsumeIntegral<uint8_t>());
            },
            [&] {
                tester.UpdateLastBlockTime(fuzzed_data_provider.ConsumeIntegralInRange(0, MAX_PEERS));
            },
            [&] {
                tester.UpdateLastTxTime(fuzzed_data_provider.ConsumeIntegralInRange(0, MAX_PEERS));
            },
            [&] {
                tester.UpdateRelevantServices(fuzzed_data_provider.ConsumeIntegralInRange(0, MAX_PEERS),
                                              fuzzed_data_provider.ConsumeBool());
            },
            [&] {
                tester.UpdateLoadedBloomFilter(fuzzed_data_provider.ConsumeIntegralInRange(0, MAX_PEERS),
                                               fuzzed_data_provider.ConsumeBool());
            },
            [&] {
                tester.UpdateRelayTxs(fuzzed_data_provider.ConsumeIntegralInRange(0, MAX_PEERS));
            });
    }
}
