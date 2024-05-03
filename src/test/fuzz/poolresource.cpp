// Copyright (c) 2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <span.h>
#include <support/allocators/pool.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <test/util/poolresourcetester.h>
#include <test/util/xoroshiro128plusplus.h>

#include <cstdint>
#include <tuple>
#include <vector>

namespace {

template <std::size_t MAX_BLOCK_SIZE_BYTES, std::size_t ALIGN_BYTES>
class PoolResourceFuzzer
{
    FuzzedDataProvider& m_provider;
    PoolResource<MAX_BLOCK_SIZE_BYTES, ALIGN_BYTES> m_test_resource;
    uint64_t m_sequence{0};
    size_t m_total_allocated{};

    struct Entry {
        Span<std::byte> span;
        size_t alignment;
        uint64_t seed;

        Entry(Span<std::byte> s, size_t a, uint64_t se) : span(s), alignment(a), seed(se) {}
    };

    std::vector<Entry> m_entries;

public:
    PoolResourceFuzzer(FuzzedDataProvider& provider)
        : m_provider{provider},
          m_test_resource{provider.ConsumeIntegralInRange<size_t>(MAX_BLOCK_SIZE_BYTES, 262144)}
    {
    }

    void Allocate(size_t size, size_t alignment)
    {
        assert(size > 0);                           // Must allocate at least 1 byte.
        assert(alignment > 0);                      // Alignment must be at least 1.
        assert((alignment & (alignment - 1)) == 0); // Alignment must be power of 2.
        assert((size & (alignment - 1)) == 0);      // Size must be a multiple of alignment.

        auto span = Span(static_cast<std::byte*>(m_test_resource.Allocate(size, alignment)), size);
        m_total_allocated += size;

        auto ptr_val = reinterpret_cast<std::uintptr_t>(span.data());
        assert((ptr_val & (alignment - 1)) == 0);

        uint64_t seed = m_sequence++;
        RandomContentFill(m_entries.emplace_back(span, alignment, seed));
    }

    void
    Allocate()
    {
        if (m_total_allocated > 0x1000000) return;
        size_t alignment_bits = m_provider.ConsumeIntegralInRange<size_t>(0, 7);
        size_t alignment = size_t{1} << alignment_bits;
        size_t size_bits = m_provider.ConsumeIntegralInRange<size_t>(0, 16 - alignment_bits);
        size_t size = m_provider.ConsumeIntegralInRange<size_t>(size_t{1} << size_bits, (size_t{1} << (size_bits + 1)) - 1U) << alignment_bits;
        Allocate(size, alignment);
    }

    void RandomContentFill(Entry& entry)
    {
        XoRoShiRo128PlusPlus rng(entry.seed);
        auto ptr = entry.span.data();
        auto size = entry.span.size();

        while (size >= 8) {
            auto r = rng();
            std::memcpy(ptr, &r, 8);
            size -= 8;
            ptr += 8;
        }
        if (size > 0) {
            auto r = rng();
            std::memcpy(ptr, &r, size);
        }
    }

    void RandomContentCheck(const Entry& entry)
    {
        XoRoShiRo128PlusPlus rng(entry.seed);
        auto ptr = entry.span.data();
        auto size = entry.span.size();

        std::byte buf[8];
        while (size >= 8) {
            auto r = rng();
            std::memcpy(buf, &r, 8);
            assert(std::memcmp(buf, ptr, 8) == 0);
            size -= 8;
            ptr += 8;
        }
        if (size > 0) {
            auto r = rng();
            std::memcpy(buf, &r, size);
            assert(std::memcmp(buf, ptr, size) == 0);
        }
    }

    void Deallocate(const Entry& entry)
    {
        auto ptr_val = reinterpret_cast<std::uintptr_t>(entry.span.data());
        assert((ptr_val & (entry.alignment - 1)) == 0);
        RandomContentCheck(entry);
        m_total_allocated -= entry.span.size();
        m_test_resource.Deallocate(entry.span.data(), entry.span.size(), entry.alignment);
    }

    void Deallocate()
    {
        if (m_entries.empty()) {
            return;
        }

        size_t idx = m_provider.ConsumeIntegralInRange<size_t>(0, m_entries.size() - 1);
        Deallocate(m_entries[idx]);
        if (idx != m_entries.size() - 1) {
            m_entries[idx] = std::move(m_entries.back());
        }
        m_entries.pop_back();
    }

    void Clear()
    {
        while (!m_entries.empty()) {
            Deallocate();
        }

        PoolResourceTester::CheckAllDataAccountedFor(m_test_resource);
    }

    void Fuzz()
    {
        LIMITED_WHILE(m_provider.ConsumeBool(), 10000)
        {
            CallOneOf(
                m_provider,
                [&] { Allocate(); },
                [&] { Deallocate(); });
        }
        Clear();
    }
};


} // namespace

FUZZ_TARGET(pool_resource)
{
    FuzzedDataProvider provider(buffer.data(), buffer.size());
    CallOneOf(
        provider,
        [&] { PoolResourceFuzzer<128, 1>{provider}.Fuzz(); },
        [&] { PoolResourceFuzzer<128, 2>{provider}.Fuzz(); },
        [&] { PoolResourceFuzzer<128, 4>{provider}.Fuzz(); },
        [&] { PoolResourceFuzzer<128, 8>{provider}.Fuzz(); },

        [&] { PoolResourceFuzzer<8, 8>{provider}.Fuzz(); },
        [&] { PoolResourceFuzzer<16, 16>{provider}.Fuzz(); },

        [&] { PoolResourceFuzzer<256, alignof(max_align_t)>{provider}.Fuzz(); },
        [&] { PoolResourceFuzzer<256, 64>{provider}.Fuzz(); });
}
