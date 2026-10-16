// Copyright (c) 2026-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <memusage.h>
#include <random.h>
#include <test/util/setup_common.h>
#include <tinyformat.h>

#include <boost/test/unit_test.hpp>

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <vector>

#ifdef __GLIBC__
#include <malloc.h>
#endif

#if defined(__SANITIZE_ADDRESS__) || defined(__SANITIZE_THREAD__)
static constexpr bool SANITIZER_ALLOCATOR{true};
#elif defined(__has_feature)
#if __has_feature(address_sanitizer) || __has_feature(memory_sanitizer) || __has_feature(thread_sanitizer)
static constexpr bool SANITIZER_ALLOCATOR{true};
#else
static constexpr bool SANITIZER_ALLOCATOR{false};
#endif
#else
static constexpr bool SANITIZER_ALLOCATOR{false};
#endif

// Set to true to measure the usage of every allocation size up to MAX_TEST_SIZE instead of a random sample, and to
// print the results in a form meant for writing MallocUsage() code. This can take minutes.
static constexpr bool ALL_SIZES{false};

namespace {

/** Largest allocation size tested. On Windows, larger blocks often get their own heap segment (which can be as small as
 *  1 MiB, so this happens from just under 512 KiB on) or VirtualAlloc() region, so the distances between them reflect
 *  reserved address space rather than their usage. */
#ifdef WIN32
constexpr size_t MAX_TEST_SIZE{size_t{256} << 10};
#else
constexpr size_t MAX_TEST_SIZE{size_t{4} << 20};
#endif

/** Granularity of the memory that allocators give large blocks (the smallest common page size). */
constexpr size_t PAGE{4096};

/** The smallest distance between neighbouring addresses (sorted) that accounts for at least 10% of all neighbouring
 *  pairs, or the most common distance if there is none. Blocks carved from fresh memory are adjacent, while blocks
 *  reusing free space in a fragmented heap are further apart (possibly in a regular pattern, which can even
 *  dominate), and the 10% threshold ignores occasional closer pairs from allocators that place blocks at varying
 *  offsets within their slots. */
size_t DominantGap(const std::vector<uintptr_t>& addrs)
{
    std::vector<uintptr_t> gaps;
    for (size_t i{1}; i < addrs.size(); ++i) gaps.push_back(addrs[i] - addrs[i - 1]);
    std::sort(gaps.begin(), gaps.end());
    size_t most_common{0}, most_common_count{0};
    for (size_t i{0}, j; i < gaps.size(); i = j) {
        for (j = i; j < gaps.size() && gaps[j] == gaps[i]; ++j) {}
        if (10 * (j - i) >= gaps.size()) return gaps[i];
        if (j - i > most_common_count) {
            most_common = gaps[i];
            most_common_count = j - i;
        }
    }
    return most_common;
}

/** Measure how much memory one malloc(size) consumes, from the addresses of many blocks of that size (see
 *  DominantGap()). Out-of-band allocator metadata is not seen. */
size_t MeasureMallocUsage(size_t size)
{
    const size_t count{std::clamp<size_t>((size_t{64} << 20) / size, 32, 16384)};
    std::vector<void*> blocks;
    std::vector<uintptr_t> addrs;
    blocks.reserve(count);
    addrs.reserve(count);
    // Nothing in this loop may allocate other than the malloc() calls being measured.
    for (size_t i{0}; i < count; ++i) {
        void* block{std::malloc(size)};
        if (block) *static_cast<volatile unsigned char*>(block) = 1;
        blocks.push_back(block);
        addrs.push_back(reinterpret_cast<uintptr_t>(block));
    }
    for (void* block : blocks) std::free(block);
    BOOST_REQUIRE(std::find(blocks.begin(), blocks.end(), nullptr) == blocks.end());

    std::sort(addrs.begin(), addrs.end());
    const size_t gap{DominantGap(addrs)};
    if (gap < PAGE) return gap;

    // Some allocators (e.g. musl) give large blocks page-granular memory, but place each block at a varying offset
    // within it, so neighbouring blocks are somewhat closer or further apart than the memory they use. Such an
    // allocator gives a different address, less than a page away, when a block is freed and allocated again.
    bool varying_offsets{false};
    for (int i{0}; i < 4 && !varying_offsets; ++i) {
        void* block{std::malloc(size)};
        const auto first{reinterpret_cast<uintptr_t>(block)};
        std::free(block);
        block = std::malloc(size);
        const auto second{reinterpret_cast<uintptr_t>(block)};
        std::free(block);
        varying_offsets = first != second && std::max(first, second) - std::min(first, second) < PAGE;
    }
    if (!varying_offsets) return gap;

    // Then look for a run of neighbours that all lie within less than a page of a grid with a multiple of pages as
    // spacing, and return that spacing. Blocks that are really spaced by something else drift away from any such
    // grid (though possibly not within a short run, so this is only done when offsets vary).
    size_t begin{0}, end{1};
    for (size_t b{0}, e; b < addrs.size(); b = e) {
        for (e = b + 1; e < addrs.size() && addrs[e] - addrs[e - 1] + PAGE > gap && addrs[e] - addrs[e - 1] < gap + PAGE; ++e) {}
        if (e - b > end - begin) {
            begin = b;
            end = e;
        }
    }
    if (end - begin < 2) return gap;
    const size_t spacing{((addrs[end - 1] - addrs[begin]) / (end - begin - 1) + PAGE / 2) / PAGE * PAGE};
    intptr_t lo{0}, hi{0};
    for (size_t i{begin}; i < end; ++i) {
        const auto deviation{static_cast<intptr_t>(addrs[i] - addrs[begin] - (i - begin) * spacing)};
        lo = std::min(lo, deviation);
        hi = std::max(hi, deviation);
    }
    return spacing > 0 && hi - lo < intptr_t{PAGE} ? spacing : gap;
}

/** Whether MallocUsage() is close enough to a measured usage. It may be at most a factor 1+1/16 below the
 *  measured usage, and up to a factor 1+1/8 above it, as it can include out-of-band allocator metadata that the
 *  measurement does not see (up to 1/8 for musl's smallest size class). */
bool IsCloseEnough(size_t model, size_t usage)
{
    return 17 * model >= 16 * usage && 8 * model <= 9 * usage;
}

/** Random allocation sizes below MAX_TEST_SIZE: for every power of two 2^b, 16 >> (b / 4) (but at least one) sizes in
 *  [2^b, 2^(b+1)), so that small sizes are tested more densely than large ones. */
std::set<size_t> RandomSizes(FastRandomContext& rng)
{
    std::set<size_t> sizes;
    for (size_t base{1}, b{0}; base < MAX_TEST_SIZE; base *= 2, ++b) {
        const size_t count{std::max<size_t>(1, 16 >> (b / 4))};
        if (count >= base) {
            for (size_t size{base}; size < 2 * base; ++size) sizes.insert(size);
        } else {
            for (size_t i{0}; i < count; ++i) sizes.insert(base + rng.randrange(base));
        }
    }
    return sizes;
}

/** A range of allocation sizes with the same usage. */
struct SizeClass {
    size_t lo, hi, usage;
};

/** Measure the usage of every size in [1, MAX_TEST_SIZE], as a list of size classes. This assumes that the usage does
 *  not decrease with the size, and finds the boundaries between classes by searching, which takes one or a few
 *  measurements per class. */
std::vector<SizeClass> MeasureAllSizes()
{
    std::map<size_t, size_t> cache;
    const auto measure{[&](size_t size) {
        auto it{cache.find(size)};
        if (it == cache.end()) it = cache.emplace(size, MeasureMallocUsage(size)).first;
        return it->second;
    }};
    std::vector<SizeClass> classes;
    size_t width{1};
    for (size_t lo{1}; lo <= MAX_TEST_SIZE;) {
        const size_t usage{measure(lo)};
        // The largest size with this usage lies in [good, bad).
        size_t good{lo}, bad{MAX_TEST_SIZE + 1};
        // Classes are often as wide as the previous one, so try that first.
        const size_t guess{std::min(MAX_TEST_SIZE, lo + width - 1)};
        if (measure(guess) == usage) {
            good = guess;
            if (guess < MAX_TEST_SIZE && measure(guess + 1) != usage) bad = guess + 1;
        } else {
            bad = guess;
        }
        for (size_t step{width}; bad == MAX_TEST_SIZE + 1 && good < MAX_TEST_SIZE; step *= 2) {
            const size_t probe{std::min(MAX_TEST_SIZE, good + step)};
            (measure(probe) == usage ? good : bad) = probe;
        }
        while (bad - good > 1) {
            const size_t mid{good + (bad - good) / 2};
            (measure(mid) == usage ? good : bad) = mid;
        }
        classes.push_back({lo, good, usage});
        width = good - lo + 1;
        lo = good + 1;
    }
    return classes;
}

/** Describe size classes in a form meant for writing MallocUsage() code: one line per range of sizes, giving its
 *  usage either as a number or, for runs of at least three equally wide classes whose usage exceeds their largest
 *  size by the same amount, as a rounding expression ("25-131048: round_up(size + 8, 16)"). */
std::string DescribeSizeClasses(const std::vector<SizeClass>& classes)
{
    std::string ret;
    for (size_t i{0}; i < classes.size();) {
        const size_t width{classes[i].hi - classes[i].lo + 1};
        const int64_t excess{int64_t(classes[i].usage) - int64_t(classes[i].hi)};
        size_t j{i + 1};
        while (j < classes.size() && classes[j].hi - classes[j].lo + 1 == width && int64_t(classes[j].usage) - int64_t(classes[j].hi) == excess) ++j;
        if (j - i >= 3) {
            // usage == round_up(size + excess - rem, width) + rem, where rem aligns the usage values.
            const int64_t rem{int64_t(classes[i].usage % width)}, add{excess - rem};
            ret += strprintf("  %u-%u: round_up(size%s, %u)%s\n", classes[i].lo, classes[j - 1].hi,
                             add ? strprintf(" %c %d", add < 0 ? '-' : '+', std::abs(add)) : "", width, rem ? strprintf(" + %d", rem) : "");
            i = j;
        } else {
            ret += strprintf("  %u-%u: %u\n", classes[i].lo, classes[i].hi, classes[i].usage);
            ++i;
        }
    }
    return ret;
}

} // namespace

BOOST_FIXTURE_TEST_SUITE(memusage_tests, BasicTestingSetup)

BOOST_AUTO_TEST_CASE(malloc_usage)
{
    if constexpr (SANITIZER_ALLOCATOR) {
        BOOST_TEST_MESSAGE("Skipped: MallocUsage() does not model sanitizer allocators");
        return;
    }

    if (ALL_SIZES) {
#ifdef __GLIBC__
        // glibc raises its mmap threshold whenever an mmapped block is freed, which would make the results depend
        // on the order of measurements. Pin it to its default, which is what MallocUsage() models.
        mallopt(M_MMAP_THRESHOLD, 128 * 1024);
#endif
        const std::vector<SizeClass> classes{MeasureAllSizes()};
        std::cout << strprintf("Measured malloc usage of every size in 1-%u (sizes: usage):\n", MAX_TEST_SIZE) << DescribeSizeClasses(classes);
        for (const auto& c : classes) {
            for (size_t size{c.lo}; size <= c.hi; ++size) {
                if (IsCloseEnough(memusage::MallocUsage(size), c.usage)) continue;
                size_t end{size};
                while (end < c.hi && !IsCloseEnough(memusage::MallocUsage(end + 1), c.usage)) ++end;
                BOOST_ERROR(strprintf("MallocUsage() is off for sizes %u-%u: measured usage %u", size, end, c.usage));
                size = end;
            }
        }
    } else {
        for (const size_t size : RandomSizes(m_rng)) {
            const size_t usage{MeasureMallocUsage(size)};
            BOOST_CHECK_MESSAGE(IsCloseEnough(memusage::MallocUsage(size), usage), strprintf("MallocUsage() is off for size %u: measured usage %u", size, usage));
        }
    }
}

BOOST_AUTO_TEST_SUITE_END()
