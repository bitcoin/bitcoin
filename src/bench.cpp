/**********************************************************************
 * Copyright (c) 2018 Pieter Wuille, Greg Maxwell, Gleb Naumenko      *
 * Distributed under the MIT software license, see the accompanying   *
 * file LICENSE or http://www.opensource.org/licenses/mit-license.php.*
 **********************************************************************/

#include "../include/minisketch.h"
#include <string.h>
#include <memory>
#include <vector>
#include <chrono>
#include <random>
#include <set>
#include <algorithm>

int main(int argc, char** argv) {
    if (argc < 1 || argc > 4) {
        printf("Usage: %s [syndromes=150] [errors=syndromes] [iters=10]\n", argv[0]);
        return 1;
    }
    int syndromes = argc > 1 ? strtoul(argv[1], NULL, 10) : 150;
    int errors = argc > 2 ? strtoul(argv[2], NULL, 10) : syndromes;
    int iters = argc > 3 ? strtoul(argv[3], NULL, 10) : 10;
    if (syndromes < 0 || syndromes > 1000000) {
        printf("Number of syndromes (%i) out of range 0..1000000\n", syndromes);
        return 1;
    }
    if (errors < 0) {
        printf("Number of errors (%i) is negative(%i)\n", errors, syndromes);
        return 1;
    }
    if (iters < 0 || iters > 1000000000) {
        printf("Number of iterations (%i) out of range 0..1000000000\n", iters);
        return 1;
    }
    uint32_t max_impl = minisketch_implementation_max();
    for (int bits = 2; bits <= 64; ++bits) {
        if (errors > pow(2.0, bits - 1)) continue;
        if (!minisketch_bits_supported(bits)) continue;
        printf("recover[ms]\t% 3i\t", bits);
        for (uint32_t impl = 0; impl <= max_impl; ++impl) {
            std::vector<minisketch*> states;
            std::vector<uint64_t> roots(2 * syndromes);
            std::random_device rng;
            std::uniform_int_distribution<uint64_t> dist(1, (uint64_t(1) << bits) - 1);
            states.resize(iters);
            std::vector<double> benches;
            benches.reserve(iters);
            for (int i = 0; i < iters; ++i) {
                states[i] = minisketch_create(bits, impl, syndromes);
                if (!states[i]) break;
                std::set<uint64_t> done;
                for (int j = 0; j < errors; ++j) {
                    uint64_t r;
                    do {
                        r = dist(rng);
                    } while (done.count(r));
                    done.insert(r);
                    minisketch_add_uint64(states[i], r);
                }
            }
            if (!states[0]) {
                printf("         -\t");
            } else {
                for (auto& state : states) {
                    auto start = std::chrono::steady_clock::now();
                    minisketch_decode(state, 2 * syndromes, roots.data());
                    auto stop = std::chrono::steady_clock::now();
                    std::chrono::duration<double> dur(stop - start);
                    benches.push_back(dur.count());
                }
                std::sort(benches.begin(), benches.end());
                printf("% 10.5f\t", benches[0] * 1000.0);
            }
            for (auto& state : states) {
                minisketch_destroy(state);
            }
        }
        printf("\n");
        printf("create[ns]\t% 3i\t", bits);
        for (uint32_t impl = 0; impl <= max_impl; ++impl) {
            std::vector<minisketch*> states;
            std::random_device rng;
            std::uniform_int_distribution<uint64_t> dist;
            std::vector<uint64_t> data;
            data.resize(errors * 10);
            states.resize(iters);
            std::vector<double> benches;
            benches.reserve(iters);
            for (int i = 0; i < iters; ++i) {
                states[i] = minisketch_create(bits, impl, syndromes);
            }
            for (size_t i = 0; i < data.size(); ++i) {
                data[i] = dist(rng);
            }
            if (!states[0]) {
                printf("         -\t");
            } else {
                for (auto& state : states) {
                    auto start = std::chrono::steady_clock::now();
                    for (auto val : data) {
                        minisketch_add_uint64(state, val);
                    }
                    auto stop = std::chrono::steady_clock::now();
                    std::chrono::duration<double> dur(stop - start);
                    benches.push_back(dur.count());
                }
                std::sort(benches.begin(), benches.end());
                printf("% 10.5f\t", benches[0] * 1000000000.0 / data.size() / syndromes);
            }
            for (auto& state : states) {
                minisketch_destroy(state);
            }
        }
        printf("\n");
    }
    return 0;
}
