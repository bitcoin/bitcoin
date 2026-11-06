// Copyright (c) 2015-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_MEMUSAGE_H
#define BITCOIN_MEMUSAGE_H

#include <indirectmap.h>
#include <prevector.h>
#include <support/allocators/pool.h>

#include <algorithm>
#include <array>
#include <bit>
#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <list>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>

#if defined(__linux__) || defined(__APPLE__)
#include <unistd.h>
#endif

namespace memusage
{

/** Round x up to a multiple of a, which must be a power of two. */
static constexpr size_t round_up(size_t x, size_t a) { return (x + a - 1) & ~(a - 1); }

/** Compute the total memory used by allocating alloc bytes. */
static size_t MallocUsage(size_t alloc);

/** Dynamic memory usage for built-in types is zero. */
static inline size_t DynamicUsage(const int8_t& v) { return 0; }
static inline size_t DynamicUsage(const uint8_t& v) { return 0; }
static inline size_t DynamicUsage(const int16_t& v) { return 0; }
static inline size_t DynamicUsage(const uint16_t& v) { return 0; }
static inline size_t DynamicUsage(const int32_t& v) { return 0; }
static inline size_t DynamicUsage(const uint32_t& v) { return 0; }
static inline size_t DynamicUsage(const int64_t& v) { return 0; }
static inline size_t DynamicUsage(const uint64_t& v) { return 0; }
static inline size_t DynamicUsage(const float& v) { return 0; }
static inline size_t DynamicUsage(const double& v) { return 0; }
template<typename X> static inline size_t DynamicUsage(X * const &v) { return 0; }
template<typename X> static inline size_t DynamicUsage(const X * const &v) { return 0; }

/** Compute the memory used for dynamically allocated but owned data structures.
 *  For generic data types, this is *not* recursive. DynamicUsage(vector<vector<int> >)
 *  will compute the memory used for the vector<int>'s, but not for the ints inside.
 *  This is for efficiency reasons, as these functions are intended to be fast. If
 *  application data structures require more accurate inner accounting, they should
 *  iterate themselves, or use more efficient caching + updating on modification.
 */

static inline size_t MallocUsage(size_t alloc)
{
    // Containers do not allocate for zero elements.
    if (alloc == 0) return 0;
#if defined(__GLIBC__)
    // glibc's ptmalloc2 (all Linux release builds). Every chunk carries a size_t header, is a
    // multiple of MALLOC_ALIGNMENT and at least MINSIZE bytes. Chunks of 128 KiB or more are
    // mmapped as whole pages (glibc raises this threshold at runtime when such chunks are freed,
    // which is not modelled). Verified exactly against glibc 2.43 on x86_64, i386, aarch64,
    // armhf, armel, riscv64, ppc64, ppc64le, ppc, s390x, loongarch64, alpha, hppa, m68k, sh4
    // and sparc64.
    constexpr size_t header{sizeof(size_t)};
#if defined(__i386__) || (defined(__powerpc__) && !defined(__powerpc64__))
    constexpr size_t align{16};
#else
    constexpr size_t align{2 * sizeof(size_t)};
#endif
    constexpr size_t min_chunk{round_up(4 * sizeof(size_t), align)};
    const size_t chunk{std::max(min_chunk, round_up(alloc + header, align))};
    if (chunk < 128 * 1024) return chunk;
    static const size_t page{static_cast<size_t>(sysconf(_SC_PAGESIZE))};
    return round_up(chunk + header, page);
#elif defined(__linux__) && !defined(__BIONIC__) && !defined(__UCLIBC__)
    // musl's mallocng (musl cannot be detected directly; other Linux libcs are excluded above).
    // Slots come in 48 size classes and carry a 4-byte header. Slots are grouped, and every group
    // has a 16-byte header plus out-of-band metadata (a 40-byte struct meta on 64-bit, 24 bytes on
    // 32-bit). Small groups are themselves slots of a larger class; larger ones are mmapped as
    // whole pages. COST is the resulting steady-state cost per slot (many live allocations of the
    // class), rounded to the nearest byte. Verified against RSS measurements of musl 1.2.5 on
    // x86_64 (within 0.2%). Requests of 131052 bytes or more are mmapped individually.
    static constexpr std::array<uint32_t, 48> SLOT{
        16, 32, 48, 64, 80, 96, 112, 128, 144, 160, 192, 240, 288, 320, 400, 496,
        576, 672, 800, 1008, 1152, 1344, 1632, 2032, 2336, 2720, 3264, 4080, 4672, 5440, 6544, 8176,
        9344, 10912, 13088, 16368, 18704, 21824, 26192, 32752, 37440, 43680, 52416, 65520, 74880, 87360, 104832, 131056};
    static constexpr std::array<uint32_t, 48> COST{sizeof(void*) == 8 ? std::array<uint32_t, 48>{
        18, 34, 53, 67, 84, 100, 116, 133, 148, 172, 207, 257, 294, 343, 412, 513,
        587, 684, 821, 1025, 1172, 1367, 1640, 2049, 2342, 2732, 3279, 4097, 4683, 5463, 6556, 8193,
        9364, 10924, 13109, 16385, 18726, 21847, 26216, 32769, 37451, 43692, 52431, 65539, 74901, 87385, 104862, 131077} : std::array<uint32_t, 48>{
        18, 34, 52, 67, 83, 99, 115, 133, 147, 172, 206, 257, 293, 342, 411, 513,
        586, 684, 820, 1025, 1171, 1366, 1640, 2049, 2341, 2732, 3278, 4097, 4682, 5462, 6555, 8193,
        9363, 10924, 13108, 16385, 18725, 21846, 26216, 32769, 37450, 43692, 52430, 65538, 74900, 87383, 104860, 131075}};
    constexpr size_t header{4}, group_header{16}, meta{sizeof(void*) == 8 ? 41 : 24}, mmap_threshold{131052};
    if (alloc < mmap_threshold) {
        // Find the class like musl's size_to_class(): 16-byte steps up to 160 bytes, then four classes for
        // every doubling of the size, the right one being at most two comparisons away.
        const size_t slot{alloc + header};
        if (slot <= 160) return COST[(slot - 1) >> 4];
        size_t i = (28 - std::countl_zero(static_cast<uint32_t>((slot + 15) >> 4))) * 4 + 8;
        if (slot > SLOT[i + 1]) i += 2;
        if (slot > SLOT[i]) ++i;
        return COST[i];
    }
    static const size_t page{static_cast<size_t>(sysconf(_SC_PAGESIZE))};
    return round_up(alloc + header + group_header, page) + meta;
#elif defined(__APPLE__)
    // macOS's libmalloc keeps its metadata out of band, and rounds requests up to a quantum that
    // depends on the size range: 16 bytes for tiny (and nano) allocations up to 1008 bytes, 512
    // bytes for small ones up to 32 KiB, and 32 KiB for medium ones up to 8 MiB. Larger ones are
    // mapped as whole pages. Checked against measurements on macOS 15 (arm64).
    if (alloc <= 1008) return round_up(alloc, 16);
    if (alloc <= 32 * 1024) return round_up(alloc, 512);
    if (alloc <= 8 * 1024 * 1024) return round_up(alloc, 32 * 1024);
    static const size_t page{static_cast<size_t>(sysconf(_SC_PAGESIZE))};
    return round_up(alloc, page);
#elif defined(_WIN64)
    // The Windows heap, which the UCRT's malloc uses. Its blocks have an (effectively) 8-byte
    // header and a 16-byte granularity, but many live allocations of up to 16 KiB come from its
    // low-fragmentation heap, whose buckets get coarser with the size. Blocks over 0xFF000 bytes
    // (including the header) get their own VirtualAlloc() region; count its committed pages. The
    // rest was checked against measurements on Windows Server 2022, up to 512 KiB.
    if (alloc <= 1032) return round_up(alloc + 8, 16);
    if (alloc <= 16392) return round_up(alloc - 8, alloc <= 2056 ? 64 : alloc <= 4104 ? 128 : alloc <= 8200 ? 256 : 512) + 16;
    if (alloc + 8 <= 0xFF000) return round_up(alloc + 8, 16);
    return round_up(alloc + 64, 4096);
#else
    // Not yet measured on other platforms (BSDs); keep the historical estimate, which was measured
    // on glibc 2.19.
    if (sizeof(void*) == 8) return round_up(alloc + 16, 16);
    return round_up(alloc + 8, 8);
#endif
}

// STL data structures

template<typename X>
struct stl_tree_node
{
private:
    int color;
    void* parent;
    void* left;
    void* right;
    X x;
};

struct stl_shared_counter
{
    /* Various platforms use different sized counters here.
     * Conservatively assume that they won't be larger than size_t. */
    void* class_type;
    size_t use_count;
    size_t weak_count;
};

template<typename T, typename Allocator>
static inline size_t DynamicUsage(const std::vector<T, Allocator>& v)
{
    return MallocUsage(v.capacity() * sizeof(T));
}

static inline size_t DynamicUsage(const std::string& s)
{
    const char* s_ptr = reinterpret_cast<const char*>(&s);
    // Don't count the dynamic memory used for string, if it resides in the
    // "small string" optimization area (which stores data inside the object itself, up to some
    // size; 15 bytes in modern libstdc++).
    if (!std::less{}(s.data(), s_ptr) && !std::greater{}(s.data() + s.size(), s_ptr + sizeof(s))) {
        return 0;
    }
    return MallocUsage(s.capacity());
}

template<unsigned int N, typename X, typename S, typename D>
static inline size_t DynamicUsage(const prevector<N, X, S, D>& v)
{
    return MallocUsage(v.allocated_memory());
}

template<typename X, typename Y>
static inline size_t DynamicUsage(const std::set<X, Y>& s)
{
    return MallocUsage(sizeof(stl_tree_node<X>)) * s.size();
}

template<typename X, typename Y>
static inline size_t IncrementalDynamicUsage(const std::set<X, Y>& s)
{
    return MallocUsage(sizeof(stl_tree_node<X>));
}

template<typename X, typename Y, typename Z>
static inline size_t DynamicUsage(const std::map<X, Y, Z>& m)
{
    return MallocUsage(sizeof(stl_tree_node<std::pair<const X, Y> >)) * m.size();
}

template<typename X, typename Y, typename Z>
static inline size_t IncrementalDynamicUsage(const std::map<X, Y, Z>& m)
{
    return MallocUsage(sizeof(stl_tree_node<std::pair<const X, Y> >));
}

// indirectmap has underlying map with pointer as key

template<typename X, typename Y>
static inline size_t DynamicUsage(const indirectmap<X, Y>& m)
{
    return MallocUsage(sizeof(stl_tree_node<std::pair<const X*, Y> >)) * m.size();
}

template<typename X, typename Y>
static inline size_t IncrementalDynamicUsage(const indirectmap<X, Y>& m)
{
    return MallocUsage(sizeof(stl_tree_node<std::pair<const X*, Y> >));
}

template<typename X>
static inline size_t DynamicUsage(const std::unique_ptr<X>& p)
{
    return p ? MallocUsage(sizeof(X)) : 0;
}

template<typename X>
static inline size_t DynamicUsage(const std::shared_ptr<X>& p)
{
    // A shared_ptr can either use a single continuous memory block for both
    // the counter and the storage (when using std::make_shared), or separate.
    // We can't observe the difference, however, so assume the worst.
    return p ? MallocUsage(sizeof(X)) + MallocUsage(sizeof(stl_shared_counter)) : 0;
}

template<typename X>
struct list_node
{
private:
    void* ptr_next;
    void* ptr_prev;
    X x;
};

template<typename X>
static inline size_t DynamicUsage(const std::list<X>& l)
{
    return MallocUsage(sizeof(list_node<X>)) * l.size();
}

template<typename X>
struct unordered_node : private X
{
private:
    void* ptr;
#if defined(_LIBCPP_VERSION)
    // libc++ stores the hash in every node.
    size_t hash;
#elif defined(_MSVC_STL_VERSION)
    // MSVC's STL keeps the elements in a doubly-linked list.
    void* prev;
#endif
};

template<typename X, typename Y>
static inline size_t DynamicUsage(const std::unordered_set<X, Y>& s)
{
    return MallocUsage(sizeof(unordered_node<X>)) * s.size() + MallocUsage(sizeof(void*) * s.bucket_count());
}

template<typename X, typename Y, typename Z>
static inline size_t DynamicUsage(const std::unordered_map<X, Y, Z>& m)
{
    return MallocUsage(sizeof(unordered_node<std::pair<const X, Y> >)) * m.size() + MallocUsage(sizeof(void*) * m.bucket_count());
}

template <class Key, class T, class Hash, class Pred, std::size_t MAX_BLOCK_SIZE_BYTES, std::size_t ALIGN_BYTES>
static inline size_t DynamicUsage(const std::unordered_map<Key,
                                                           T,
                                                           Hash,
                                                           Pred,
                                                           PoolAllocator<std::pair<const Key, T>,
                                                                         MAX_BLOCK_SIZE_BYTES,
                                                                         ALIGN_BYTES>>& m)
{
    auto* pool_resource = m.get_allocator().resource();

    // The allocated chunks are stored in a std::list. Size per node should
    // therefore be 3 pointers: next, previous, and a pointer to the chunk.
    size_t estimated_list_node_size = MallocUsage(sizeof(void*) * 3);
    size_t usage_resource = estimated_list_node_size * pool_resource->NumAllocatedChunks();
    size_t usage_chunks = MallocUsage(pool_resource->ChunkSizeBytes()) * pool_resource->NumAllocatedChunks();
    return usage_resource + usage_chunks + MallocUsage(sizeof(void*) * m.bucket_count());
}

} // namespace memusage

#endif // BITCOIN_MEMUSAGE_H
