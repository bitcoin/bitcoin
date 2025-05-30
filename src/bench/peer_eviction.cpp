// Copyright (c) 2021-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <bench/bench.h>
#include <netaddress.h>
#include <node/eviction.h>
#include <random.h>
#include <test/util/net.h>

#include <chrono>
#include <functional>
#include <vector>

static void EvictionProtectionCommon(
    benchmark::Bench& bench,
    int num_candidates,
    std::function<void(NodeEvictionCandidate&)> candidate_setup_fn)
{
    using Candidates = std::vector<NodeEvictionCandidate>;
    FastRandomContext random_context{true};

    Candidates candidates{GetRandomNodeEvictionCandidates(num_candidates, random_context)};
    for (auto& c : candidates) {
        candidate_setup_fn(c);
    }


    bench.run([&] {
        // creating a copy has an overhead of about 3%, so it does not influence the benchmark results much.
        auto copy = candidates;
        ProtectEvictionCandidatesByRatio(copy);
    });
}

/* Benchmarks */

static void EvictionProtection0Networks250Candidates(benchmark::Bench& bench)
{
    EvictionProtectionCommon(
        bench,
        /*num_candidates=*/250,
        [](NodeEvictionCandidate& c) {
            c.m_connected = std::chrono::seconds{c.id};
            c.m_network = NET_IPV4;
        });
}

static void EvictionProtection1Networks250Candidates(benchmark::Bench& bench)
{
    EvictionProtectionCommon(
        bench,
        /*num_candidates=*/250,
        [](NodeEvictionCandidate& c) {
            c.m_connected = std::chrono::seconds{c.id};
            c.m_is_local = false;
            if (c.id >= 130 && c.id < 240) { // 110 Tor
                c.m_network = NET_ONION;
            } else {
                c.m_network = NET_IPV4;
            }
        });
}

static void EvictionProtection2Networks250Candidates(benchmark::Bench& bench)
{
    EvictionProtectionCommon(
        bench,
        /*num_candidates=*/250,
        [](NodeEvictionCandidate& c) {
            c.m_connected = std::chrono::seconds{c.id};
            c.m_is_local = false;
            if (c.id >= 90 && c.id < 160) { // 70 Tor
                c.m_network = NET_ONION;
            } else if (c.id >= 170 && c.id < 250) { // 80 I2P
                c.m_network = NET_I2P;
            } else {
                c.m_network = NET_IPV4;
            }
        });
}

static void EvictionProtection3Networks050Candidates(benchmark::Bench& bench)
{
    EvictionProtectionCommon(
        bench,
        /*num_candidates=*/50,
        [](NodeEvictionCandidate& c) {
            c.m_connected = std::chrono::seconds{c.id};
            c.m_is_local = (c.id == 28 || c.id == 47); //  2 localhost
            if (c.id >= 30 && c.id < 47) {             // 17 I2P
                c.m_network = NET_I2P;
            } else if (c.id >= 24 && c.id < 28) { //  4 Tor
                c.m_network = NET_ONION;
            } else {
                c.m_network = NET_IPV4;
            }
        });
}

static void EvictionProtection3Networks100Candidates(benchmark::Bench& bench)
{
    EvictionProtectionCommon(
        bench,
        /*num_candidates=*/100,
        [](NodeEvictionCandidate& c) {
            c.m_connected = std::chrono::seconds{c.id};
            c.m_is_local = (c.id >= 55 && c.id < 60); //  5 localhost
            if (c.id >= 70 && c.id < 80) {            // 10 I2P
                c.m_network = NET_I2P;
            } else if (c.id >= 80 && c.id < 96) { // 16 Tor
                c.m_network = NET_ONION;
            } else {
                c.m_network = NET_IPV4;
            }
        });
}

static void EvictionProtection3Networks250Candidates(benchmark::Bench& bench)
{
    EvictionProtectionCommon(
        bench,
        /*num_candidates=*/250,
        [](NodeEvictionCandidate& c) {
            c.m_connected = std::chrono::seconds{c.id};
            c.m_is_local = (c.id >= 140 && c.id < 160); // 20 localhost
            if (c.id >= 170 && c.id < 180) {            // 10 I2P
                c.m_network = NET_I2P;
            } else if (c.id >= 190 && c.id < 240) { // 50 Tor
                c.m_network = NET_ONION;
            } else {
                c.m_network = NET_IPV4;
            }
        });
}

// Candidate numbers used for the benchmarks:
// -  50 candidates simulates a possible use of -maxconnections
// - 100 candidates approximates an average node with default settings
// - 250 candidates is the number of peers reported by operators of busy nodes

// No disadvantaged networks, with 250 eviction candidates.
BENCHMARK(EvictionProtection0Networks250Candidates, benchmark::PriorityLevel::HIGH);

// 1 disadvantaged network (Tor) with 250 eviction candidates.
BENCHMARK(EvictionProtection1Networks250Candidates, benchmark::PriorityLevel::HIGH);

// 2 disadvantaged networks (I2P, Tor) with 250 eviction candidates.
BENCHMARK(EvictionProtection2Networks250Candidates, benchmark::PriorityLevel::HIGH);

// 3 disadvantaged networks (I2P/localhost/Tor) with 50/100/250 eviction candidates.
BENCHMARK(EvictionProtection3Networks050Candidates, benchmark::PriorityLevel::HIGH);
BENCHMARK(EvictionProtection3Networks100Candidates, benchmark::PriorityLevel::HIGH);
BENCHMARK(EvictionProtection3Networks250Candidates, benchmark::PriorityLevel::HIGH);
