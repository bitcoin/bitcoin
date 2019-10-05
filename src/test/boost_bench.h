// Copyright (c) 2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_TEST_BOOST_BENCH_H
#define BITCOIN_TEST_BOOST_BENCH_H

#include <chrono>
#include <iostream>
#include <string>

#define BENCHMARK(testname) BOOST_AUTO_TEST_CASE(testname, *::boost::unit_test::label("benchmark") * ::boost::unit_test::disabled())

class Benchmark
{
    using Clock = std::chrono::high_resolution_clock;

    std::string m_name;
    size_t m_batch{1};
    size_t m_iters{1};

public:
    explicit Benchmark(std::string name)
        : m_name(std::move(name)) {}

    Benchmark& Batch(size_t batch)
    {
        m_batch = batch;
        return *this;
    }

    Benchmark& Iters(size_t iters)
    {
        m_iters = iters;
        return *this;
    }

    template <typename Op>
    void Run(Op op) const
    {
        size_t n = m_iters;
        auto before = Clock::now();
        while (n > 0) {
            op();
            --n;
        }
        auto after = Clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::duration<double>>(after - before).count();
        std::cout << (1e9 * elapsed / (m_batch * m_iters)) << " ns for " << m_name << " (" << elapsed << " s total)" << std::endl;
    }
};

#endif
