// Copyright (c) 2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <random.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/util.h>
#include <util/bitdeque.h>

#include <deque>
#include <vector>

namespace {

constexpr int LEN_BITS = 16;
constexpr int RANDDATA_BITS = 20;

using bitdeque_type = bitdeque<128>;

//! Deterministic random vector of bools, for begin/end insertions to draw from.
std::vector<bool> RANDDATA;

void InitRandData()
{
    FastRandomContext ctx(true);
    RANDDATA.clear();
    for (size_t i = 0; i < (1U << RANDDATA_BITS) + (1U << LEN_BITS); ++i) {
        RANDDATA.push_back(ctx.randbool());
    }
}

} // namespace

FUZZ_TARGET_INIT(bitdeque, InitRandData)
{
    FuzzedDataProvider provider(buffer.data(), buffer.size());
    FastRandomContext ctx(true);

    size_t maxlen = (1U << provider.ConsumeIntegralInRange<size_t>(0, LEN_BITS)) - 1;
    size_t limitlen = 4 * maxlen;

    std::deque<bool> deq;
    bitdeque_type bitdeq;

    const auto& cdeq = deq;
    const auto& cbitdeq = bitdeq;

    size_t initlen = provider.ConsumeIntegralInRange<size_t>(0, maxlen);
    while (initlen) {
        bool val = ctx.randbool();
        deq.push_back(val);
        bitdeq.push_back(val);
        --initlen;
    }

    LIMITED_WHILE(provider.remaining_bytes() > 0, 900)
    {
        {
            assert(deq.size() == bitdeq.size());
            auto it = deq.begin();
            auto bitit = bitdeq.begin();
            auto itend = deq.end();
            while (it != itend) {
                assert(*it == *bitit);
                ++it;
                ++bitit;
            }
        }

        CallOneOf(provider,
            [&] {
                // constructor()
                deq = std::deque<bool>{};
                bitdeq = bitdeque_type{};
            },
            [&] {
                // clear()
                deq.clear();
                bitdeq.clear();
            },
            [&] {
                // resize()
                auto count = provider.ConsumeIntegralInRange<size_t>(0, maxlen);
                deq.resize(count);
                bitdeq.resize(count);
            },
            [&] {
                // assign(count, val)
                auto count = provider.ConsumeIntegralInRange<size_t>(0, maxlen);
                bool val = ctx.randbool();
                deq.assign(count, val);
                bitdeq.assign(count, val);
            },
            [&] {
                // constructor(count, val)
                auto count = provider.ConsumeIntegralInRange<size_t>(0, maxlen);
                bool val = ctx.randbool();
                deq = std::deque<bool>(count, val);
                bitdeq = bitdeque_type(count, val);
            },
            [&] {
                // constructor(count)
                auto count = provider.ConsumeIntegralInRange<size_t>(0, maxlen);
                deq = std::deque<bool>(count);
                bitdeq = bitdeque_type(count);
            },
            [&] {
                // construct(begin, end)
                auto count = provider.ConsumeIntegralInRange<size_t>(0, maxlen);
                auto rand_begin = RANDDATA.begin() + ctx.randbits(RANDDATA_BITS);
                auto rand_end = rand_begin + count;
                deq = std::deque<bool>(rand_begin, rand_end);
                bitdeq = bitdeque_type(rand_begin, rand_end);
            },
            [&] {
                // assign(begin, end)
                auto count = provider.ConsumeIntegralInRange<size_t>(0, maxlen);
                auto rand_begin = RANDDATA.begin() + ctx.randbits(RANDDATA_BITS);
                auto rand_end = rand_begin + count;
                deq.assign(rand_begin, rand_end);
                bitdeq.assign(rand_begin, rand_end);
            },
            [&] {
                // construct(initializer_list)
                std::initializer_list<bool> ilist{ctx.randbool(), ctx.randbool(), ctx.randbool(), ctx.randbool(), ctx.randbool()};
                deq = std::deque<bool>(ilist);
                bitdeq = bitdeque_type(ilist);
            },
            [&] {
                // assign(initializer_list)
                std::initializer_list<bool> ilist{ctx.randbool(), ctx.randbool(), ctx.randbool()};
                deq.assign(ilist);
                bitdeq.assign(ilist);
            },
            [&] {
                // operator=(const&)
                auto count = provider.ConsumeIntegralInRange<size_t>(0, maxlen);
                bool val = ctx.randbool();
                const std::deque<bool> deq2(count, val);
                deq = deq2;
                const bitdeque_type bitdeq2(count, val);
                bitdeq = bitdeq2;
            },
            [&] {
                // operator=(&&)
                auto count = provider.ConsumeIntegralInRange<size_t>(0, maxlen);
                bool val = ctx.randbool();
                std::deque<bool> deq2(count, val);
                deq = std::move(deq2);
                bitdeque_type bitdeq2(count, val);
                bitdeq = std::move(bitdeq2);
            },
            [&] {
                // deque swap
                auto count = provider.ConsumeIntegralInRange<size_t>(0, maxlen);
                auto rand_begin = RANDDATA.begin() + ctx.randbits(RANDDATA_BITS);
                auto rand_end = rand_begin + count;
                std::deque<bool> deq2(rand_begin, rand_end);
                bitdeque_type bitdeq2(rand_begin, rand_end);
                using std::swap;
                assert(deq.size() == bitdeq.size());
                assert(deq2.size() == bitdeq2.size());
                swap(deq, deq2);
                swap(bitdeq, bitdeq2);
                assert(deq.size() == bitdeq.size());
                assert(deq2.size() == bitdeq2.size());
            },
            [&] {
                // deque.swap
                auto count = provider.ConsumeIntegralInRange<size_t>(0, maxlen);
                auto rand_begin = RANDDATA.begin() + ctx.randbits(RANDDATA_BITS);
                auto rand_end = rand_begin + count;
                std::deque<bool> deq2(rand_begin, rand_end);
                bitdeque_type bitdeq2(rand_begin, rand_end);
                assert(deq.size() == bitdeq.size());
                assert(deq2.size() == bitdeq2.size());
                deq.swap(deq2);
                bitdeq.swap(bitdeq2);
                assert(deq.size() == bitdeq.size());
                assert(deq2.size() == bitdeq2.size());
            },
            [&] {
                // operator=(initializer_list)
                std::initializer_list<bool> ilist{ctx.randbool(), ctx.randbool(), ctx.randbool()};
                deq = ilist;
                bitdeq = ilist;
            },
            [&] {
                // iterator arithmetic
                auto pos1 = provider.ConsumeIntegralInRange<long>(0, cdeq.size());
                auto pos2 = provider.ConsumeIntegralInRange<long>(0, cdeq.size());
                auto it = deq.begin() + pos1;
                auto bitit = bitdeq.begin() + pos1;
                if ((size_t)pos1 != cdeq.size()) assert(*it == *bitit);
                assert(it - deq.begin() == pos1);
                assert(bitit - bitdeq.begin() == pos1);
                if (provider.ConsumeBool()) {
                    it += pos2 - pos1;
                    bitit += pos2 - pos1;
                } else {
                    it -= pos1 - pos2;
                    bitit -= pos1 - pos2;
                }
                if ((size_t)pos2 != cdeq.size()) assert(*it == *bitit);
                assert(deq.end() - it == bitdeq.end() - bitit);
                if (provider.ConsumeBool()) {
                    if ((size_t)pos2 != cdeq.size()) {
                        ++it;
                        ++bitit;
                    }
                } else {
                    if (pos2 != 0) {
                        --it;
                        --bitit;
                    }
                }
                assert(deq.end() - it == bitdeq.end() - bitit);
            },
            [&] {
                // begin() and end()
                assert(deq.end() - deq.begin() == bitdeq.end() - bitdeq.begin());
            },
            [&] {
                // begin() and end() (const)
                assert(cdeq.end() - cdeq.begin() == cbitdeq.end() - cbitdeq.begin());
            },
            [&] {
                // rbegin() and rend()
                assert(deq.rend() - deq.rbegin() == bitdeq.rend() - bitdeq.rbegin());
            },
            [&] {
                // rbegin() and rend() (const)
                assert(cdeq.rend() - cdeq.rbegin() == cbitdeq.rend() - cbitdeq.rbegin());
            },
            [&] {
                // cbegin() and cend()
                assert(cdeq.cend() - cdeq.cbegin() == cbitdeq.cend() - cbitdeq.cbegin());
            },
            [&] {
                // crbegin() and crend()
                assert(cdeq.crend() - cdeq.crbegin() == cbitdeq.crend() - cbitdeq.crbegin());
            },
            [&] {
                // size() and maxsize()
                assert(cdeq.size() == cbitdeq.size());
                assert(cbitdeq.size() <= cbitdeq.max_size());
            },
            [&] {
                // empty
                assert(cdeq.empty() == cbitdeq.empty());
            },
            [&] {
                // at (in range) and flip
                if (!cdeq.empty()) {
                    size_t pos = provider.ConsumeIntegralInRange<size_t>(0, cdeq.size() - 1);
                    auto& ref = deq.at(pos);
                    auto bitref = bitdeq.at(pos);
                    assert(ref == bitref);
                    if (ctx.randbool()) {
                        ref = !ref;
                        bitref.flip();
                    }
                }
            },
            [&] {
                // at (maybe out of range) and bit assign
                size_t pos = provider.ConsumeIntegralInRange<size_t>(0, cdeq.size() + maxlen);
                bool newval = ctx.randbool();
                bool throw_deq{false}, throw_bitdeq{false};
                bool val_deq{false}, val_bitdeq{false};
                try {
                    auto& ref = deq.at(pos);
                    val_deq = ref;
                    ref = newval;
                } catch (const std::out_of_range&) {
                    throw_deq = true;
                }
                try {
                    auto ref = bitdeq.at(pos);
                    val_bitdeq = ref;
                    ref = newval;
                } catch (const std::out_of_range&) {
                    throw_bitdeq = true;
                }
                assert(throw_deq == throw_bitdeq);
                assert(throw_bitdeq == (pos >= cdeq.size()));
                if (!throw_deq) assert(val_deq == val_bitdeq);
            },
            [&] {
                // at (maybe out of range) (const)
                size_t pos = provider.ConsumeIntegralInRange<size_t>(0, cdeq.size() + maxlen);
                bool throw_deq{false}, throw_bitdeq{false};
                bool val_deq{false}, val_bitdeq{false};
                try {
                    auto& ref = cdeq.at(pos);
                    val_deq = ref;
                } catch (const std::out_of_range&) {
                    throw_deq = true;
                }
                try {
                    auto ref = cbitdeq.at(pos);
                    val_bitdeq = ref;
                } catch (const std::out_of_range&) {
                    throw_bitdeq = true;
                }
                assert(throw_deq == throw_bitdeq);
                assert(throw_bitdeq == (pos >= cdeq.size()));
                if (!throw_deq) assert(val_deq == val_bitdeq);
            },
            [&] {
                // operator[]
                if (!cdeq.empty()) {
                    size_t pos = provider.ConsumeIntegralInRange<size_t>(0, cdeq.size() - 1);
                    assert(deq[pos] == bitdeq[pos]);
                    if (ctx.randbool()) {
                        deq[pos] = !deq[pos];
                        bitdeq[pos].flip();
                    }
                }
            },
            [&] {
                // operator[] const
                if (!cdeq.empty()) {
                    size_t pos = provider.ConsumeIntegralInRange<size_t>(0, cdeq.size() - 1);
                    assert(deq[pos] == bitdeq[pos]);
                }
            },
            [&] {
                // front()
                if (!cdeq.empty()) {
                    auto& ref = deq.front();
                    auto bitref = bitdeq.front();
                    assert(ref == bitref);
                    if (ctx.randbool()) {
                        ref = !ref;
                        bitref = !bitref;
                    }
                }
            },
            [&] {
                // front() const
                if (!cdeq.empty()) {
                    auto& ref = cdeq.front();
                    auto bitref = cbitdeq.front();
                    assert(ref == bitref);
                }
            },
            [&] {
                // back() and swap(bool, ref)
                if (!cdeq.empty()) {
                    auto& ref = deq.back();
                    auto bitref = bitdeq.back();
                    assert(ref == bitref);
                    if (ctx.randbool()) {
                        ref = !ref;
                        bitref.flip();
                    }
                }
            },
            [&] {
                // back() const
                if (!cdeq.empty()) {
                    const auto& cdeq = deq;
                    const auto& cbitdeq = bitdeq;
                    auto& ref = cdeq.back();
                    auto bitref = cbitdeq.back();
                    assert(ref == bitref);
                }
            },
            [&] {
                // push_back()
                if (cdeq.size() < limitlen) {
                    bool val = ctx.randbool();
                    if (cdeq.empty()) {
                        deq.push_back(val);
                        bitdeq.push_back(val);
                    } else {
                        size_t pos = provider.ConsumeIntegralInRange<size_t>(0, cdeq.size() - 1);
                        auto& ref = deq[pos];
                        auto bitref = bitdeq[pos];
                        assert(ref == bitref);
                        deq.push_back(val);
                        bitdeq.push_back(val);
                        assert(ref == bitref); // references are not invalidated
                    }
                }
            },
            [&] {
                // push_front()
                if (cdeq.size() < limitlen) {
                    bool val = ctx.randbool();
                    if (cdeq.empty()) {
                        deq.push_front(val);
                        bitdeq.push_front(val);
                    } else {
                        size_t pos = provider.ConsumeIntegralInRange<size_t>(0, cdeq.size() - 1);
                        auto& ref = deq[pos];
                        auto bitref = bitdeq[pos];
                        assert(ref == bitref);
                        deq.push_front(val);
                        bitdeq.push_front(val);
                        assert(ref == bitref); // references are not invalidated
                    }
                }
            },
            [&] {
                // pop_back()
                if (!cdeq.empty()) {
                    if (cdeq.size() == 1) {
                        deq.pop_back();
                        bitdeq.pop_back();
                    } else {
                        size_t pos = provider.ConsumeIntegralInRange<size_t>(0, cdeq.size() - 2);
                        auto& ref = deq[pos];
                        auto bitref = bitdeq[pos];
                        assert(ref == bitref);
                        deq.pop_back();
                        bitdeq.pop_back();
                        assert(ref == bitref); // references to other elements are not invalidated
                    }
                }
            },
            [&] {
                // pop_front()
                if (!cdeq.empty()) {
                    if (cdeq.size() == 1) {
                        deq.pop_front();
                        bitdeq.pop_front();
                    } else {
                        size_t pos = provider.ConsumeIntegralInRange<size_t>(1, cdeq.size() - 1);
                        auto& ref = deq[pos];
                        auto bitref = bitdeq[pos];
                        assert(ref == bitref);
                        deq.pop_front();
                        bitdeq.pop_front();
                        assert(ref == bitref); // references to other elements are not invalidated
                    }
                }
            },
            [&] {
                // erase (in middle, single)
                if (!cdeq.empty()) {
                    size_t before = provider.ConsumeIntegralInRange<size_t>(0, cdeq.size() - 1);
                    size_t after = cdeq.size() - 1 - before;
                    auto it = deq.erase(cdeq.begin() + before);
                    auto bitit = bitdeq.erase(cbitdeq.begin() + before);
                    assert(it == cdeq.begin() + before && it == cdeq.end() - after);
                    assert(bitit == cbitdeq.begin() + before && bitit == cbitdeq.end() - after);
                }
            },
            [&] {
                // erase (at front, range)
                size_t count = provider.ConsumeIntegralInRange<size_t>(0, cdeq.size());
                auto it = deq.erase(cdeq.begin(), cdeq.begin() + count);
                auto bitit = bitdeq.erase(cbitdeq.begin(), cbitdeq.begin() + count);
                assert(it == deq.begin());
                assert(bitit == bitdeq.begin());
            },
            [&] {
                // erase (at back, range)
                size_t count = provider.ConsumeIntegralInRange<size_t>(0, cdeq.size());
                auto it = deq.erase(cdeq.end() - count, cdeq.end());
                auto bitit = bitdeq.erase(cbitdeq.end() - count, cbitdeq.end());
                assert(it == deq.end());
                assert(bitit == bitdeq.end());
            },
            [&] {
                // erase (in middle, range)
                size_t count = provider.ConsumeIntegralInRange<size_t>(0, cdeq.size());
                size_t before = provider.ConsumeIntegralInRange<size_t>(0, cdeq.size() - count);
                size_t after = cdeq.size() - count - before;
                auto it = deq.erase(cdeq.begin() + before, cdeq.end() - after);
                auto bitit = bitdeq.erase(cbitdeq.begin() + before, cbitdeq.end() - after);
                assert(it == cdeq.begin() + before && it == cdeq.end() - after);
                assert(bitit == cbitdeq.begin() + before && bitit == cbitdeq.end() - after);
            },
            [&] {
                // insert/emplace (in middle, single)
                if (cdeq.size() < limitlen) {
                    size_t before = provider.ConsumeIntegralInRange<size_t>(0, cdeq.size());
                    bool val = ctx.randbool();
                    bool do_emplace = provider.ConsumeBool();
                    auto it = deq.insert(cdeq.begin() + before, val);
                    auto bitit = do_emplace ? bitdeq.emplace(cbitdeq.begin() + before, val)
                                            : bitdeq.insert(cbitdeq.begin() + before, val);
                    assert(it == deq.begin() + before);
                    assert(bitit == bitdeq.begin() + before);
                }
            },
            [&] {
                // insert (at front, begin/end)
                if (cdeq.size() < limitlen) {
                    size_t count = provider.ConsumeIntegralInRange<size_t>(0, maxlen);
                    auto rand_begin = RANDDATA.begin() + ctx.randbits(RANDDATA_BITS);
                    auto rand_end = rand_begin + count;
                    auto it = deq.insert(cdeq.begin(), rand_begin, rand_end);
                    auto bitit = bitdeq.insert(cbitdeq.begin(), rand_begin, rand_end);
                    assert(it == cdeq.begin());
                    assert(bitit == cbitdeq.begin());
                }
            },
            [&] {
                // insert (at back, begin/end)
                if (cdeq.size() < limitlen) {
                    size_t count = provider.ConsumeIntegralInRange<size_t>(0, maxlen);
                    auto rand_begin = RANDDATA.begin() + ctx.randbits(RANDDATA_BITS);
                    auto rand_end = rand_begin + count;
                    auto it = deq.insert(cdeq.end(), rand_begin, rand_end);
                    auto bitit = bitdeq.insert(cbitdeq.end(), rand_begin, rand_end);
                    assert(it == cdeq.end() - count);
                    assert(bitit == cbitdeq.end() - count);
                }
            },
            [&] {
                // insert (in middle, range)
                if (cdeq.size() < limitlen) {
                    size_t count = provider.ConsumeIntegralInRange<size_t>(0, maxlen);
                    size_t before = provider.ConsumeIntegralInRange<size_t>(0, cdeq.size());
                    bool val = ctx.randbool();
                    auto it = deq.insert(cdeq.begin() + before, count, val);
                    auto bitit = bitdeq.insert(cbitdeq.begin() + before, count, val);
                    assert(it == deq.begin() + before);
                    assert(bitit == bitdeq.begin() + before);
                }
            },
            [&] {
                // insert (in middle, begin/end)
                if (cdeq.size() < limitlen) {
                    size_t count = provider.ConsumeIntegralInRange<size_t>(0, maxlen);
                    size_t before = provider.ConsumeIntegralInRange<size_t>(0, cdeq.size());
                    auto rand_begin = RANDDATA.begin() + ctx.randbits(RANDDATA_BITS);
                    auto rand_end = rand_begin + count;
                    auto it = deq.insert(cdeq.begin() + before, rand_begin, rand_end);
                    auto bitit = bitdeq.insert(cbitdeq.begin() + before, rand_begin, rand_end);
                    assert(it == deq.begin() + before);
                    assert(bitit == bitdeq.begin() + before);
                }
            }
        );
    }
}
