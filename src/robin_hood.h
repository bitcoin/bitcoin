//                 ______  _____                 ______                _________
//  ______________ ___  /_ ___(_)_______         ___  /_ ______ ______ ______  /
//  __  ___/_  __ \__  __ \__  / __  __ \        __  __ \_  __ \_  __ \_  __  /
//  _  /    / /_/ /_  /_/ /_  /  _  / / /        _  / / // /_/ // /_/ // /_/ /
//  /_/     \____/ /_.___/ /_/   /_/ /_/ ________/_/ /_/ \____/ \____/ \__,_/
//                                      _/_____/
//
// robin_hood::unordered_map for C++11
// version 3.4.0
// https://github.com/martinus/robin-hood-hashing
//
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.
// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2019 Martin Ankerl <http://martin.ankerl.com>
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#ifndef ROBIN_HOOD_H_INCLUDED
#define ROBIN_HOOD_H_INCLUDED

// see https://semver.org/
#define ROBIN_HOOD_VERSION_MAJOR 3 // for incompatible API changes
#define ROBIN_HOOD_VERSION_MINOR 4 // for adding functionality in a backwards-compatible manner
#define ROBIN_HOOD_VERSION_PATCH 0 // for backwards-compatible bug fixes

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>

// #define ROBIN_HOOD_LOG_ENABLED
#ifdef ROBIN_HOOD_LOG_ENABLED
#    include <iostream>
#    define ROBIN_HOOD_LOG(x) std::cout << __FUNCTION__ << "@" << __LINE__ << ": " << x << std::endl
#else
#    define ROBIN_HOOD_LOG(x)
#endif

// #define ROBIN_HOOD_TRACE_ENABLED
#ifdef ROBIN_HOOD_TRACE_ENABLED
#    include <iostream>
#    define ROBIN_HOOD_TRACE(x) \
        std::cout << __FUNCTION__ << "@" << __LINE__ << ": " << x << std::endl
#else
#    define ROBIN_HOOD_TRACE(x)
#endif

// all non-argument macros should use this facility. See
// https://www.fluentcpp.com/2019/05/28/better-macros-better-flags/
#define ROBIN_HOOD(x) ROBIN_HOOD_PRIVATE_DEFINITION_##x()

// mark unused members with this macro
#define ROBIN_HOOD_UNUSED(identifier)

// bitness
#if SIZE_MAX == UINT32_MAX
#    define ROBIN_HOOD_PRIVATE_DEFINITION_BITNESS() 32
#elif SIZE_MAX == UINT64_MAX
#    define ROBIN_HOOD_PRIVATE_DEFINITION_BITNESS() 64
#else
#    error Unsupported bitness
#endif

// endianess
#ifdef _WIN32
#    define ROBIN_HOOD_PRIVATE_DEFINITION_LITTLE_ENDIAN() 1
#    define ROBIN_HOOD_PRIVATE_DEFINITION_BIG_ENDIAN() 0
#else
#    define ROBIN_HOOD_PRIVATE_DEFINITION_LITTLE_ENDIAN() \
        (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__)
#    define ROBIN_HOOD_PRIVATE_DEFINITION_BIG_ENDIAN() (__BYTE_ORDER__ == __ORDER_BIG_ENDIAN__)
#endif

// inline
#ifdef _WIN32
#    define ROBIN_HOOD_PRIVATE_DEFINITION_NOINLINE() __declspec(noinline)
#else
#    define ROBIN_HOOD_PRIVATE_DEFINITION_NOINLINE() __attribute__((noinline))
#endif

// exceptions
#if !defined(__cpp_exceptions) && !defined(__EXCEPTIONS) && !defined(_CPPUNWIND)
#    define ROBIN_HOOD_PRIVATE_DEFINITION_HAS_EXCEPTIONS() 0
#else
#    define ROBIN_HOOD_PRIVATE_DEFINITION_HAS_EXCEPTIONS() 1
#endif

// count leading/trailing bits
#ifdef _WIN32
#    if ROBIN_HOOD(BITNESS) == 32
#        define ROBIN_HOOD_PRIVATE_DEFINITION_BITSCANFORWARD() _BitScanForward
#    else
#        define ROBIN_HOOD_PRIVATE_DEFINITION_BITSCANFORWARD() _BitScanForward64
#    endif
#    include <intrin.h>
#    pragma intrinsic(ROBIN_HOOD(BITSCANFORWARD))
#    define ROBIN_HOOD_COUNT_TRAILING_ZEROES(x)                                       \
        [](size_t mask) noexcept->int {                                               \
            unsigned long index;                                                      \
            return ROBIN_HOOD(BITSCANFORWARD)(&index, mask) ? static_cast<int>(index) \
                                                            : ROBIN_HOOD(BITNESS);    \
        }                                                                             \
        (x)
#else
#    if ROBIN_HOOD(BITNESS) == 32
#        define ROBIN_HOOD_PRIVATE_DEFINITION_CTZ() __builtin_ctzl
#        define ROBIN_HOOD_PRIVATE_DEFINITION_CLZ() __builtin_clzl
#    else
#        define ROBIN_HOOD_PRIVATE_DEFINITION_CTZ() __builtin_ctzll
#        define ROBIN_HOOD_PRIVATE_DEFINITION_CLZ() __builtin_clzll
#    endif
#    define ROBIN_HOOD_COUNT_LEADING_ZEROES(x) ((x) ? ROBIN_HOOD(CLZ)(x) : ROBIN_HOOD(BITNESS))
#    define ROBIN_HOOD_COUNT_TRAILING_ZEROES(x) ((x) ? ROBIN_HOOD(CTZ)(x) : ROBIN_HOOD(BITNESS))
#endif

// fallthrough
#ifndef __has_cpp_attribute // For backwards compatibility
#    define __has_cpp_attribute(x) 0
#endif
#if __has_cpp_attribute(clang::fallthrough)
#    define ROBIN_HOOD_PRIVATE_DEFINITION_FALLTHROUGH() [[clang::fallthrough]]
#elif __has_cpp_attribute(gnu::fallthrough)
#    define ROBIN_HOOD_PRIVATE_DEFINITION_FALLTHROUGH() [[gnu::fallthrough]]
#else
#    define ROBIN_HOOD_PRIVATE_DEFINITION_FALLTHROUGH()
#endif

// likely/unlikely
#if defined(_WIN32)
#    define ROBIN_HOOD_LIKELY(condition) condition
#    define ROBIN_HOOD_UNLIKELY(condition) condition
#else
#    define ROBIN_HOOD_LIKELY(condition) __builtin_expect(condition, 1)
#    define ROBIN_HOOD_UNLIKELY(condition) __builtin_expect(condition, 0)
#endif

// workaround missing "is_trivially_copyable" in g++ < 5.0
// See https://stackoverflow.com/a/31798726/48181
#if defined(__GNUC__) && __GNUC__ < 5
#    define ROBIN_HOOD_IS_TRIVIALLY_COPYABLE(...) __has_trivial_copy(__VA_ARGS__)
#else
#    define ROBIN_HOOD_IS_TRIVIALLY_COPYABLE(...) std::is_trivially_copyable<__VA_ARGS__>::value
#endif

// helpers for C++ versions, see https://gcc.gnu.org/onlinedocs/cpp/Standard-Predefined-Macros.html
#define ROBIN_HOOD_PRIVATE_DEFINITION_CXX() __cplusplus
#define ROBIN_HOOD_PRIVATE_DEFINITION_CXX98() 199711L
#define ROBIN_HOOD_PRIVATE_DEFINITION_CXX11() 201103L
#define ROBIN_HOOD_PRIVATE_DEFINITION_CXX14() 201402L
#define ROBIN_HOOD_PRIVATE_DEFINITION_CXX17() 201703L

#if ROBIN_HOOD(CXX) >= ROBIN_HOOD(CXX17)
#    define ROBIN_HOOD_PRIVATE_DEFINITION_NODISCARD() [[nodiscard]]
#else
#    define ROBIN_HOOD_PRIVATE_DEFINITION_NODISCARD()
#endif

namespace robin_hood {

#if ROBIN_HOOD(CXX) >= ROBIN_HOOD(CXX14)
#    define ROBIN_HOOD_STD std
#else

// c++11 compatibility layer
namespace ROBIN_HOOD_STD {
template <class T>
struct alignment_of
    : std::integral_constant<std::size_t, alignof(typename std::remove_all_extents<T>::type)> {};

template <class T, T... Ints>
class integer_sequence {
public:
    using value_type = T;
    static_assert(std::is_integral<value_type>::value, "not integral type");
    static constexpr std::size_t size() noexcept {
        return sizeof...(Ints);
    }
};
template <std::size_t... Inds>
using index_sequence = integer_sequence<std::size_t, Inds...>;

namespace detail_ {
template <class T, T Begin, T End, bool>
struct IntSeqImpl {
    using TValue = T;
    static_assert(std::is_integral<TValue>::value, "not integral type");
    static_assert(Begin >= 0 && Begin < End, "unexpected argument (Begin<0 || Begin<=End)");

    template <class, class>
    struct IntSeqCombiner;

    template <TValue... Inds0, TValue... Inds1>
    struct IntSeqCombiner<integer_sequence<TValue, Inds0...>, integer_sequence<TValue, Inds1...>> {
        using TResult = integer_sequence<TValue, Inds0..., Inds1...>;
    };

    using TResult =
        typename IntSeqCombiner<typename IntSeqImpl<TValue, Begin, Begin + (End - Begin) / 2,
                                                    (End - Begin) / 2 == 1>::TResult,
                                typename IntSeqImpl<TValue, Begin + (End - Begin) / 2, End,
                                                    (End - Begin + 1) / 2 == 1>::TResult>::TResult;
};

template <class T, T Begin>
struct IntSeqImpl<T, Begin, Begin, false> {
    using TValue = T;
    static_assert(std::is_integral<TValue>::value, "not integral type");
    static_assert(Begin >= 0, "unexpected argument (Begin<0)");
    using TResult = integer_sequence<TValue>;
};

template <class T, T Begin, T End>
struct IntSeqImpl<T, Begin, End, true> {
    using TValue = T;
    static_assert(std::is_integral<TValue>::value, "not integral type");
    static_assert(Begin >= 0, "unexpected argument (Begin<0)");
    using TResult = integer_sequence<TValue, Begin>;
};
} // namespace detail_

template <class T, T N>
using make_integer_sequence = typename detail_::IntSeqImpl<T, 0, N, (N - 0) == 1>::TResult;

template <std::size_t N>
using make_index_sequence = make_integer_sequence<std::size_t, N>;

template <class... T>
using index_sequence_for = make_index_sequence<sizeof...(T)>;

} // namespace ROBIN_HOOD_STD

#endif

namespace detail {

// umul
#if defined(__SIZEOF_INT128__)
#    define ROBIN_HOOD_PRIVATE_DEFINITION_HAS_UMUL128() 1
#    if defined(__GNUC__) || defined(__clang__)
#        pragma GCC diagnostic push
#        pragma GCC diagnostic ignored "-Wpedantic"
using uint128_t = unsigned __int128;
#        pragma GCC diagnostic pop
#    endif
inline uint64_t umul128(uint64_t a, uint64_t b, uint64_t* high) noexcept {
    auto result = static_cast<uint128_t>(a) * static_cast<uint128_t>(b);
    *high = static_cast<uint64_t>(result >> 64U);
    return static_cast<uint64_t>(result);
}
#elif (defined(_WIN32) && ROBIN_HOOD(BITNESS) == 64)
#    define ROBIN_HOOD_PRIVATE_DEFINITION_HAS_UMUL128() 1
#    include <intrin.h> // for __umulh
#    pragma intrinsic(__umulh)
#    ifndef _M_ARM64
#        pragma intrinsic(_umul128)
#    endif
inline uint64_t umul128(uint64_t a, uint64_t b, uint64_t* high) noexcept {
#    ifdef _M_ARM64
    *high = __umulh(a, b);
    return ((uint64_t)(a)) * (b);
#    else
    return _umul128(a, b, high);
#    endif
}
#else
#    define ROBIN_HOOD_PRIVATE_DEFINITION_HAS_UMUL128() 0
#endif

// This cast gets rid of warnings like "cast from 'uint8_t*' {aka 'unsigned char*'} to
// 'uint64_t*' {aka 'long unsigned int*'} increases required alignment of target type". Use with
// care!
template <typename T>
inline T reinterpret_cast_no_cast_align_warning(void* ptr) noexcept {
    return reinterpret_cast<T>(ptr);
}

template <typename T>
inline T reinterpret_cast_no_cast_align_warning(void const* ptr) noexcept {
    return reinterpret_cast<T>(ptr);
}

// make sure this is not inlined as it is slow and dramatically enlarges code, thus making other
// inlinings more difficult. Throws are also generally the slow path.
template <typename E, typename... Args>
ROBIN_HOOD(NOINLINE)
#if ROBIN_HOOD(HAS_EXCEPTIONS)
void doThrow(Args&&... args) {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
    throw E(std::forward<Args>(args)...);
}
#else
void doThrow(Args&&... ROBIN_HOOD_UNUSED(args) /*unused*/) {
    abort();
}
#endif

template <typename E, typename T, typename... Args>
T* assertNotNull(T* t, Args&&... args) {
    if (ROBIN_HOOD_UNLIKELY(nullptr == t)) {
        doThrow<E>(std::forward<Args>(args)...);
    }
    return t;
}

template <typename T>
inline T unaligned_load(void const* ptr) noexcept {
    // using memcpy so we don't get into unaligned load problems.
    // compiler should optimize this very well anyways.
    T t;
    std::memcpy(&t, ptr, sizeof(T));
    return t;
}

// Allocates bulks of memory for objects of type T. This deallocates the memory in the destructor,
// and keeps a linked list of the allocated memory around. Overhead per allocation is the size of a
// pointer.
template <typename T, size_t MinNumAllocs = 4, size_t MaxNumAllocs = 256>
class BulkPoolAllocator {
public:
    BulkPoolAllocator() noexcept = default;

    // does not copy anything, just creates a new allocator.
    BulkPoolAllocator(const BulkPoolAllocator& ROBIN_HOOD_UNUSED(o) /*unused*/) noexcept
        : mHead(nullptr)
        , mListForFree(nullptr) {}

    BulkPoolAllocator(BulkPoolAllocator&& o) noexcept
        : mHead(o.mHead)
        , mListForFree(o.mListForFree) {
        o.mListForFree = nullptr;
        o.mHead = nullptr;
    }

    BulkPoolAllocator& operator=(BulkPoolAllocator&& o) noexcept {
        reset();
        mHead = o.mHead;
        mListForFree = o.mListForFree;
        o.mListForFree = nullptr;
        o.mHead = nullptr;
        return *this;
    }

    BulkPoolAllocator&
    operator=(const BulkPoolAllocator& ROBIN_HOOD_UNUSED(o) /*unused*/) noexcept {
        // does not do anything
        return *this;
    }

    ~BulkPoolAllocator() noexcept {
        reset();
    }

    // Deallocates all allocated memory.
    void reset() noexcept {
        while (mListForFree) {
            T* tmp = *mListForFree;
            free(mListForFree);
            mListForFree = reinterpret_cast_no_cast_align_warning<T**>(tmp);
        }
        mHead = nullptr;
    }

    // allocates, but does NOT initialize. Use in-place new constructor, e.g.
    //   T* obj = pool.allocate();
    //   ::new (static_cast<void*>(obj)) T();
    T* allocate() {
        T* tmp = mHead;
        if (!tmp) {
            tmp = performAllocation();
        }

        mHead = *reinterpret_cast_no_cast_align_warning<T**>(tmp);
        return tmp;
    }

    // does not actually deallocate but puts it in store.
    // make sure you have already called the destructor! e.g. with
    //  obj->~T();
    //  pool.deallocate(obj);
    void deallocate(T* obj) noexcept {
        *reinterpret_cast_no_cast_align_warning<T**>(obj) = mHead;
        mHead = obj;
    }

    // Adds an already allocated block of memory to the allocator. This allocator is from now on
    // responsible for freeing the data (with free()). If the provided data is not large enough to
    // make use of, it is immediately freed. Otherwise it is reused and freed in the destructor.
    void addOrFree(void* ptr, const size_t numBytes) noexcept {
        // calculate number of available elements in ptr
        if (numBytes < ALIGNMENT + ALIGNED_SIZE) {
            // not enough data for at least one element. Free and return.
            free(ptr);
        } else {
            add(ptr, numBytes);
        }
    }

    void swap(BulkPoolAllocator<T, MinNumAllocs, MaxNumAllocs>& other) noexcept {
        using std::swap;
        swap(mHead, other.mHead);
        swap(mListForFree, other.mListForFree);
    }

private:
    // iterates the list of allocated memory to calculate how many to alloc next.
    // Recalculating this each time saves us a size_t member.
    // This ignores the fact that memory blocks might have been added manually with addOrFree. In
    // practice, this should not matter much.
    ROBIN_HOOD(NODISCARD) size_t calcNumElementsToAlloc() const noexcept {
        auto tmp = mListForFree;
        size_t numAllocs = MinNumAllocs;

        while (numAllocs * 2 <= MaxNumAllocs && tmp) {
            auto x = reinterpret_cast<T***>(tmp);
            tmp = *x;
            numAllocs *= 2;
        }

        return numAllocs;
    }

    // WARNING: Underflow if numBytes < ALIGNMENT! This is guarded in addOrFree().
    void add(void* ptr, const size_t numBytes) noexcept {
        const size_t numElements = (numBytes - ALIGNMENT) / ALIGNED_SIZE;

        auto data = reinterpret_cast<T**>(ptr);

        // link free list
        auto x = reinterpret_cast<T***>(data);
        *x = mListForFree;
        mListForFree = data;

        // create linked list for newly allocated data
        auto const headT =
            reinterpret_cast_no_cast_align_warning<T*>(reinterpret_cast<char*>(ptr) + ALIGNMENT);

        auto const head = reinterpret_cast<char*>(headT);

        // Visual Studio compiler automatically unrolls this loop, which is pretty cool
        for (size_t i = 0; i < numElements; ++i) {
            *reinterpret_cast_no_cast_align_warning<char**>(head + i * ALIGNED_SIZE) =
                head + (i + 1) * ALIGNED_SIZE;
        }

        // last one points to 0
        *reinterpret_cast_no_cast_align_warning<T**>(head + (numElements - 1) * ALIGNED_SIZE) =
            mHead;
        mHead = headT;
    }

    // Called when no memory is available (mHead == 0).
    // Don't inline this slow path.
    ROBIN_HOOD(NOINLINE) T* performAllocation() {
        size_t const numElementsToAlloc = calcNumElementsToAlloc();

        // alloc new memory: [prev |T, T, ... T]
        // std::cout << (sizeof(T*) + ALIGNED_SIZE * numElementsToAlloc) << " bytes" << std::endl;
        size_t const bytes = ALIGNMENT + ALIGNED_SIZE * numElementsToAlloc;
        add(assertNotNull<std::bad_alloc>(malloc(bytes)), bytes);
        return mHead;
    }

    // enforce byte alignment of the T's
#if ROBIN_HOOD(CXX) >= ROBIN_HOOD(CXX14)
    static constexpr size_t ALIGNMENT =
        (std::max)(std::alignment_of<T>::value, std::alignment_of<T*>::value);
#else
    static const size_t ALIGNMENT =
        (ROBIN_HOOD_STD::alignment_of<T>::value > ROBIN_HOOD_STD::alignment_of<T*>::value)
            ? ROBIN_HOOD_STD::alignment_of<T>::value
            : +ROBIN_HOOD_STD::alignment_of<T*>::value; // the + is for walkarround
#endif

    static constexpr size_t ALIGNED_SIZE = ((sizeof(T) - 1) / ALIGNMENT + 1) * ALIGNMENT;

    static_assert(MinNumAllocs >= 1, "MinNumAllocs");
    static_assert(MaxNumAllocs >= MinNumAllocs, "MaxNumAllocs");
    static_assert(ALIGNED_SIZE >= sizeof(T*), "ALIGNED_SIZE");
    static_assert(0 == (ALIGNED_SIZE % sizeof(T*)), "ALIGNED_SIZE mod");
    static_assert(ALIGNMENT >= sizeof(T*), "ALIGNMENT");

    T* mHead{nullptr};
    T** mListForFree{nullptr};
};

template <typename T, size_t MinSize, size_t MaxSize, bool IsFlatMap>
struct NodeAllocator;

// dummy allocator that does nothing
template <typename T, size_t MinSize, size_t MaxSize>
struct NodeAllocator<T, MinSize, MaxSize, true> {

    // we are not using the data, so just free it.
    void addOrFree(void* ptr, size_t ROBIN_HOOD_UNUSED(numBytes) /*unused*/) noexcept {
        free(ptr);
    }
};

template <typename T, size_t MinSize, size_t MaxSize>
struct NodeAllocator<T, MinSize, MaxSize, false> : public BulkPoolAllocator<T, MinSize, MaxSize> {};

// dummy hash, unsed as mixer when robin_hood::hash is already used
template <typename T>
struct identity_hash {
    constexpr size_t operator()(T const& obj) const noexcept {
        return static_cast<size_t>(obj);
    }
};

// c++14 doesn't have is_nothrow_swappable, and clang++ 6.0.1 doesn't like it either, so I'm making
// my own here.
namespace swappable {
using std::swap;
template <typename T>
struct nothrow {
    static const bool value = noexcept(swap(std::declval<T&>(), std::declval<T&>()));
};

} // namespace swappable

} // namespace detail

struct is_transparent_tag {};

namespace detail {

// A highly optimized hashmap implementation, using the Robin Hood algorithm.
//
// In most cases, this map should be usable as a drop-in replacement for std::unordered_map, but be
// about 2x faster in most cases and require much less allocations.
//
// This implementation uses the following memory layout:
//
// [Node, Node, ... Node | info, info, ... infoSentinel ]
//
// * Node: either a DataNode that directly has the std::pair<key, val> as member,
//   or a DataNode with a pointer to std::pair<key,val>. Which DataNode representation to use
//   depends on how fast the swap() operation is. Heuristically, this is automatically choosen based
//   on sizeof(). there are always 2^n Nodes.
//
// * info: Each Node in the map has a corresponding info byte, so there are 2^n info bytes.
//   Each byte is initialized to 0, meaning the corresponding Node is empty. Set to 1 means the
//   corresponding node contains data. Set to 2 means the corresponding Node is filled, but it
//   actually belongs to the previous position and was pushed out because that place is already
//   taken.
//
// * infoSentinel: Sentinel byte set to 1, so that iterator's ++ can stop at end() without the need
// for a idx
//   variable.
//
// According to STL, order of templates has effect on throughput. That's why I've moved the boolean
// to the front.
// https://www.reddit.com/r/cpp/comments/ahp6iu/compile_time_binary_size_reductions_and_cs_future/eeguck4/
template <bool IsFlatMap, size_t MaxLoadFactor100, typename Key, typename T, typename Hash,
          typename KeyEqual>
class unordered_map
    : public Hash,
      public KeyEqual,
      detail::NodeAllocator<
          std::pair<typename std::conditional<IsFlatMap, Key, Key const>::type, T>, 4, 16384,
          IsFlatMap> {
public:
    using key_type = Key;
    using mapped_type = T;
    using value_type =
        std::pair<typename std::conditional<IsFlatMap, Key, Key const>::type, T>;
    using size_type = size_t;
    using hasher = Hash;
    using key_equal = KeyEqual;
    using Self =
        unordered_map<IsFlatMap, MaxLoadFactor100, key_type, mapped_type, hasher, key_equal>;
    static constexpr bool is_flat_map = IsFlatMap;

private:
    static_assert(MaxLoadFactor100 > 10 && MaxLoadFactor100 < 100,
                  "MaxLoadFactor100 needs to be >10 && < 100");

    // configuration defaults

    // make sure we have 8 elements, needed to quickly rehash mInfo
    static constexpr size_t InitialNumElements = sizeof(uint64_t);
    static constexpr uint32_t InitialInfoNumBits = 5;
    static constexpr uint8_t InitialInfoInc = 1U << InitialInfoNumBits;
    static constexpr uint8_t InitialInfoHashShift = sizeof(size_t) * 8 - InitialInfoNumBits;
    using DataPool = detail::NodeAllocator<value_type, 4, 16384, IsFlatMap>;

    // type needs to be wider than uint8_t.
    //
    // TODO(jamesob): why? advertised as a byte?
    using InfoType = uint32_t;

    // DataNode ////////////////////////////////////////////////////////

    // Primary template for the data node. We have special implementations for small and big
    // objects. For large objects it is assumed that swap() is fairly slow, so we allocate these on
    // the heap so swap merely swaps a pointer.
    template <typename M, bool>
    class DataNode {};

    // big object: allocate on heap.
    template <typename M>
    class DataNode<M, false> {
    public:
        template <typename... Args>
        explicit DataNode(M& map, Args&&... args)
            : mData(map.allocate()) {
            ::new (static_cast<void*>(mData)) value_type(std::forward<Args>(args)...);
        }

        DataNode(M& ROBIN_HOOD_UNUSED(map) /*unused*/, DataNode<M, false>&& n) noexcept
            : mData(std::move(n.mData)) {}

        void destroy(M& map) noexcept {
            // don't deallocate, just put it into list of datapool.
            mData->~value_type();
            map.deallocate(mData);
        }

        void destroyDoNotDeallocate() noexcept {
            mData->~value_type();
        }

        value_type const* operator->() const noexcept {
            return mData;
        }

        value_type* operator->() noexcept {
            return mData;
        }

        const value_type& operator*() const {
            return *mData;
        }

        value_type& operator*() {
            return *mData;
        }

        ROBIN_HOOD(NODISCARD) typename value_type::first_type& getFirst() {
            return mData->first;
        }

        ROBIN_HOOD(NODISCARD) typename value_type::first_type const& getFirst() const {
            return mData->first;
        }

        ROBIN_HOOD(NODISCARD) typename value_type::second_type& getSecond() {
            return mData->second;
        }

        ROBIN_HOOD(NODISCARD) typename value_type::second_type const& getSecond() const {
            return mData->second;
        }

        void swap(DataNode<M, false>& o) noexcept {
            using std::swap;
            swap(mData, o.mData);
        }

    private:
        value_type* mData;
    };

    using Node = DataNode<Self, false>;

    // Cloner //////////////////////////////////////////////////////////

    template <typename M, bool UseMemcpy>
    struct Cloner;

    // fast path: Just copy data, without allocating anything.
    template <typename M>
    struct Cloner<M, true> {
        void operator()(M const& source, M& target) const {
            // std::memcpy(target.mKeyVals, source.mKeyVals,
            //             target.calcNumBytesTotal(target.mMask + 1));
            auto src = reinterpret_cast<char const*>(source.mKeyVals);
            auto tgt = reinterpret_cast<char*>(target.mKeyVals);
            std::copy(src, src + target.calcNumBytesTotal(target.mMask + 1), tgt);
        }
    };

    template <typename M>
    struct Cloner<M, false> {
        void operator()(M const& s, M& t) const {
            std::copy(s.mInfo, s.mInfo + t.calcNumBytesInfo(t.mMask + 1), t.mInfo);

            for (size_t i = 0; i < t.mMask + 1; ++i) {
                if (t.mInfo[i]) {
                    ::new (static_cast<void*>(t.mKeyVals + i)) Node(t, *s.mKeyVals[i]);
                }
            }
        }
    };

    // Destroyer ///////////////////////////////////////////////////////

    template <typename M, bool IsFlatMapAndTrivial>
    struct Destroyer {};

    template <typename M>
    struct Destroyer<M, true> {
        void nodes(M& m) const noexcept {
            m.mNumElements = 0;
        }

        void nodesDoNotDeallocate(M& m) const noexcept {
            m.mNumElements = 0;
        }
    };

    template <typename M>
    struct Destroyer<M, false> {
        void nodes(M& m) const noexcept {
            m.mNumElements = 0;
            // clear also resets mInfo to 0, that's sometimes not necessary.
            for (size_t idx = 0; idx <= m.mMask; ++idx) {
                if (0 != m.mInfo[idx]) {
                    Node& n = m.mKeyVals[idx];
                    n.destroy(m);
                    n.~Node();
                }
            }
        }

        void nodesDoNotDeallocate(M& m) const noexcept {
            m.mNumElements = 0;
            // clear also resets mInfo to 0, that's sometimes not necessary.
            for (size_t idx = 0; idx <= m.mMask; ++idx) {
                if (0 != m.mInfo[idx]) {
                    Node& n = m.mKeyVals[idx];
                    n.destroyDoNotDeallocate();
                    n.~Node();
                }
            }
        }
    };

    // Iter ////////////////////////////////////////////////////////////

    struct fast_forward_tag {};

    // generic iterator for both const_iterator and iterator.
    template <bool IsConst>
    // NOLINTNEXTLINE(hicpp-special-member-functions,cppcoreguidelines-special-member-functions)
    class Iter {
    private:
        using NodePtr = typename std::conditional<IsConst, Node const*, Node*>::type;

    public:
        using difference_type = std::ptrdiff_t;
        using value_type = typename Self::value_type;
        using reference = typename std::conditional<IsConst, value_type const&, value_type&>::type;
        using pointer = typename std::conditional<IsConst, value_type const*, value_type*>::type;
        using iterator_category = std::forward_iterator_tag;

        // default constructed iterator can be compared to itself, but WON'T return true when
        // compared to end().
        Iter() = default;

        // Rule of zero: nothing specified. The conversion constructor is only enabled for iterator
        // to const_iterator, so it doesn't accidentally work as a copy ctor.

        // Conversion constructor from iterator to const_iterator.
        template <bool OtherIsConst,
                  typename = typename std::enable_if<IsConst && !OtherIsConst>::type>
        // NOLINTNEXTLINE(hicpp-explicit-conversions)
        Iter(Iter<OtherIsConst> const& other) noexcept
            : mKeyVals(other.mKeyVals)
            , mInfo(other.mInfo) {}

        Iter(NodePtr valPtr, uint8_t const* infoPtr) noexcept
            : mKeyVals(valPtr)
            , mInfo(infoPtr) {}

        Iter(NodePtr valPtr, uint8_t const* infoPtr,
             fast_forward_tag ROBIN_HOOD_UNUSED(tag) /*unused*/) noexcept
            : mKeyVals(valPtr)
            , mInfo(infoPtr) {
            fastForward();
        }

        template <bool OtherIsConst,
                  typename = typename std::enable_if<IsConst && !OtherIsConst>::type>
        Iter& operator=(Iter<OtherIsConst> const& other) noexcept {
            mKeyVals = other.mKeyVals;
            mInfo = other.mInfo;
            return *this;
        }

        // prefix increment. Undefined behavior if we are at end()!
        Iter& operator++() noexcept {
            mInfo++;
            mKeyVals++;
            fastForward();
            return *this;
        }

        reference operator*() const {
            return **mKeyVals;
        }

        pointer operator->() const {
            return &**mKeyVals;
        }

        template <bool O>
        bool operator==(Iter<O> const& o) const noexcept {
            return mKeyVals == o.mKeyVals;
        }

        template <bool O>
        bool operator!=(Iter<O> const& o) const noexcept {
            return mKeyVals != o.mKeyVals;
        }

    private:
        // fast forward to the next non-free info byte
        void fastForward() noexcept {
            int inc;
            do {
                auto const n = detail::unaligned_load<size_t>(mInfo);
#if ROBIN_HOOD(LITTLE_ENDIAN)
                inc = ROBIN_HOOD_COUNT_TRAILING_ZEROES(n) / 8;
#else
                inc = ROBIN_HOOD_COUNT_LEADING_ZEROES(n) / 8;
#endif
                mInfo += inc;
                mKeyVals += inc;
            } while (inc == static_cast<int>(sizeof(size_t)));
        }

        friend class unordered_map<IsFlatMap, MaxLoadFactor100, key_type, mapped_type, hasher,
                                   key_equal>;
        NodePtr mKeyVals{nullptr};
        uint8_t const* mInfo{nullptr};
    };

    ////////////////////////////////////////////////////////////////////

    // highly performance relevant code.
    // Lower bits are used for indexing into the array (2^n size)
    // The upper 1-5 bits need to be a reasonable good hash, to save comparisons.
    template <typename HashKey>
    void keyToIdx(HashKey&& key, size_t* idx, InfoType* info) const {
        *idx = Hash::operator()(key);
        *info = mInfoInc + static_cast<InfoType>(*idx >> mInfoHashShift);
        *idx &= mMask;
    }

    // forwards the index by one, wrapping around at the end
    void next(InfoType* info, size_t* idx) const noexcept {
        *idx = (*idx + 1) & mMask;
        *info += mInfoInc;
    }

    void nextWhileLess(InfoType* info, size_t* idx) const noexcept {
        // unrolling this by hand did not bring any speedups.
        while (*info < mInfo[*idx]) {
            next(info, idx);
        }
    }

    // Shift everything up by one element. Tries to move stuff around.
    // True if some shifting has occured (entry under idx is a constructed object)
    // Fals if no shift has occured (entry under idx is unconstructed memory)
    void
    shiftUp(size_t idx,
            size_t const insertion_idx) noexcept(std::is_nothrow_move_assignable<Node>::value) {
        while (idx != insertion_idx) {
            size_t prev_idx = (idx - 1) & mMask;
            if (mInfo[idx]) {
                mKeyVals[idx] = std::move(mKeyVals[prev_idx]);
            } else {
                ::new (static_cast<void*>(mKeyVals + idx)) Node(std::move(mKeyVals[prev_idx]));
            }
            mInfo[idx] = static_cast<uint8_t>(mInfo[prev_idx] + mInfoInc);
            if (ROBIN_HOOD_UNLIKELY(mInfo[idx] + mInfoInc > 0xFF)) {
                mMaxNumElementsAllowed = 0;
            }
            idx = prev_idx;
        }
    }

    void shiftDown(size_t idx) noexcept(std::is_nothrow_move_assignable<Node>::value) {
        // until we find one that is either empty or has zero offset.
        // TODO(martinus) we don't need to move everything, just the last one for the same bucket.
        mKeyVals[idx].destroy(*this);

        // until we find one that is either empty or has zero offset.
        size_t nextIdx = (idx + 1) & mMask;
        while (mInfo[nextIdx] >= 2 * mInfoInc) {
            mInfo[idx] = static_cast<uint8_t>(mInfo[nextIdx] - mInfoInc);
            mKeyVals[idx] = std::move(mKeyVals[nextIdx]);
            idx = nextIdx;
            nextIdx = (idx + 1) & mMask;
        }

        mInfo[idx] = 0;
        // don't destroy, we've moved it
        // mKeyVals[idx].destroy(*this);
        mKeyVals[idx].~Node();
    }

    // copy of find(), except that it returns iterator instead of const_iterator.
    template <typename Other>
    ROBIN_HOOD(NODISCARD)
    size_t findIdx(Other const& key) const {
        size_t idx;
        InfoType info;
        keyToIdx(key, &idx, &info);

        do {
            // unrolling this twice gives a bit of a speedup. More unrolling did not help.
            if (info == mInfo[idx] && KeyEqual::operator()(key, mKeyVals[idx].getFirst())) {
                return idx;
            }
            next(&info, &idx);
            if (info == mInfo[idx] && KeyEqual::operator()(key, mKeyVals[idx].getFirst())) {
                return idx;
            }
            next(&info, &idx);
        } while (info <= mInfo[idx]);

        // nothing found!
        return mMask == 0 ? 0 : mMask + 1;
    }

    void cloneData(const unordered_map& o) {
        Cloner<unordered_map, IsFlatMap && ROBIN_HOOD_IS_TRIVIALLY_COPYABLE(Node)>()(o, *this);
    }

    // inserts a keyval that is guaranteed to be new, e.g. when the hashmap is resized.
    // @return index where the element was created
    size_t insert_move(Node&& keyval) {
        // we don't retry, fail if overflowing
        // don't need to check max num elements
        if (0 == mMaxNumElementsAllowed && !try_increase_info()) {
            throwOverflowError();
        }

        size_t idx;
        InfoType info;
        keyToIdx(keyval.getFirst(), &idx, &info);

        // skip forward. Use <= because we are certain that the element is not there.
        while (info <= mInfo[idx]) {
            idx = (idx + 1) & mMask;
            info += mInfoInc;
        }

        // key not found, so we are now exactly where we want to insert it.
        auto const insertion_idx = idx;
        auto const insertion_info = static_cast<uint8_t>(info);
        if (ROBIN_HOOD_UNLIKELY(insertion_info + mInfoInc > 0xFF)) {
            mMaxNumElementsAllowed = 0;
        }

        // find an empty spot
        while (0 != mInfo[idx]) {
            next(&info, &idx);
        }

        auto& l = mKeyVals[insertion_idx];
        if (idx == insertion_idx) {
            ::new (static_cast<void*>(&l)) Node(std::move(keyval));
        } else {
            shiftUp(idx, insertion_idx);
            l = std::move(keyval);
        }

        // put at empty spot
        mInfo[insertion_idx] = insertion_info;

        ++mNumElements;
        return insertion_idx;
    }

public:
    using iterator = Iter<false>;
    using const_iterator = Iter<true>;

    // Creates an empty hash map. Nothing is allocated yet, this happens at the first insert. This
    // tremendously speeds up ctor & dtor of a map that never receives an element. The penalty is
    // payed at the first insert, and not before. Lookup of this empty map works because everybody
    // points to DummyInfoByte::b. parameter bucket_count is dictated by the standard, but we can
    // ignore it.
    explicit unordered_map(size_t ROBIN_HOOD_UNUSED(bucket_count) /*unused*/ = 0,
                           const Hash& h = Hash{},
                           const KeyEqual& equal = KeyEqual{}) noexcept(noexcept(Hash(h)) &&
                                                                        noexcept(KeyEqual(equal)))
        : Hash(h)
        , KeyEqual(equal) {
        ROBIN_HOOD_TRACE(this);
    }

    template <typename Iter>
    unordered_map(Iter first, Iter last, size_t ROBIN_HOOD_UNUSED(bucket_count) /*unused*/ = 0,
                  const Hash& h = Hash{}, const KeyEqual& equal = KeyEqual{})
        : Hash(h)
        , KeyEqual(equal) {
        ROBIN_HOOD_TRACE(this);
        insert(first, last);
    }

    unordered_map(std::initializer_list<value_type> initlist,
                  size_t ROBIN_HOOD_UNUSED(bucket_count) /*unused*/ = 0, const Hash& h = Hash{},
                  const KeyEqual& equal = KeyEqual{})
        : Hash(h)
        , KeyEqual(equal) {
        ROBIN_HOOD_TRACE(this);
        insert(initlist.begin(), initlist.end());
    }

    unordered_map(unordered_map&& o) noexcept
        : Hash(std::move(static_cast<Hash&>(o)))
        , KeyEqual(std::move(static_cast<KeyEqual&>(o)))
        , DataPool(std::move(static_cast<DataPool&>(o))) {
        ROBIN_HOOD_TRACE(this);
        if (o.mMask) {
            mKeyVals = std::move(o.mKeyVals);
            mInfo = std::move(o.mInfo);
            mNumElements = std::move(o.mNumElements);
            mMask = std::move(o.mMask);
            mMaxNumElementsAllowed = std::move(o.mMaxNumElementsAllowed);
            mInfoInc = std::move(o.mInfoInc);
            mInfoHashShift = std::move(o.mInfoHashShift);
            // set other's mask to 0 so its destructor won't do anything
            o.init();
        }
    }

    unordered_map& operator=(unordered_map&& o) noexcept {
        ROBIN_HOOD_TRACE(this);
        if (&o != this) {
            if (o.mMask) {
                // only move stuff if the other map actually has some data
                destroy();
                mKeyVals = std::move(o.mKeyVals);
                mInfo = std::move(o.mInfo);
                mNumElements = std::move(o.mNumElements);
                mMask = std::move(o.mMask);
                mMaxNumElementsAllowed = std::move(o.mMaxNumElementsAllowed);
                mInfoInc = std::move(o.mInfoInc);
                mInfoHashShift = std::move(o.mInfoHashShift);
                Hash::operator=(std::move(static_cast<Hash&>(o)));
                KeyEqual::operator=(std::move(static_cast<KeyEqual&>(o)));
                DataPool::operator=(std::move(static_cast<DataPool&>(o)));

                o.init();

            } else {
                // nothing in the other map => just clear us.
                clear();
            }
        }
        return *this;
    }

    unordered_map(const unordered_map& o)
        : Hash(static_cast<const Hash&>(o))
        , KeyEqual(static_cast<const KeyEqual&>(o))
        , DataPool(static_cast<const DataPool&>(o)) {
        ROBIN_HOOD_TRACE(this);
        if (!o.empty()) {
            // not empty: create an exact copy. it is also possible to just iterate through all
            // elements and insert them, but copying is probably faster.

            mKeyVals = static_cast<Node*>(
                detail::assertNotNull<std::bad_alloc>(malloc(calcNumBytesTotal(o.mMask + 1))));
            // no need for calloc because clonData does memcpy
            mInfo = reinterpret_cast<uint8_t*>(mKeyVals + o.mMask + 1);
            mNumElements = o.mNumElements;
            mMask = o.mMask;
            mMaxNumElementsAllowed = o.mMaxNumElementsAllowed;
            mInfoInc = o.mInfoInc;
            mInfoHashShift = o.mInfoHashShift;
            cloneData(o);
        }
    }

    // Creates a copy of the given map. Copy constructor of each entry is used.
    unordered_map& operator=(unordered_map const& o) {
        ROBIN_HOOD_TRACE(this);
        if (&o == this) {
            // prevent assigning of itself
            return *this;
        }

        // we keep using the old allocator and not assign the new one, because we want to keep the
        // memory available. when it is the same size.
        if (o.empty()) {
            if (0 == mMask) {
                // nothing to do, we are empty too
                return *this;
            }

            // not empty: destroy what we have there
            // clear also resets mInfo to 0, that's sometimes not necessary.
            destroy();
            init();
            Hash::operator=(static_cast<const Hash&>(o));
            KeyEqual::operator=(static_cast<const KeyEqual&>(o));
            DataPool::operator=(static_cast<DataPool const&>(o));

            return *this;
        }

        // clean up old stuff
        Destroyer<Self, IsFlatMap && std::is_trivially_destructible<Node>::value>{}.nodes(*this);

        if (mMask != o.mMask) {
            // no luck: we don't have the same array size allocated, so we need to realloc.
            if (0 != mMask) {
                // only deallocate if we actually have data!
                free(mKeyVals);
            }

            mKeyVals = static_cast<Node*>(
                detail::assertNotNull<std::bad_alloc>(malloc(calcNumBytesTotal(o.mMask + 1))));

            // no need for calloc here because cloneData performs a memcpy.
            mInfo = reinterpret_cast<uint8_t*>(mKeyVals + o.mMask + 1);
            // sentinel is set in cloneData
        }
        Hash::operator=(static_cast<const Hash&>(o));
        KeyEqual::operator=(static_cast<const KeyEqual&>(o));
        DataPool::operator=(static_cast<DataPool const&>(o));
        mNumElements = o.mNumElements;
        mMask = o.mMask;
        mMaxNumElementsAllowed = o.mMaxNumElementsAllowed;
        mInfoInc = o.mInfoInc;
        mInfoHashShift = o.mInfoHashShift;
        cloneData(o);

        return *this;
    }

    // Swaps everything between the two maps.
    void swap(unordered_map& o) {
        ROBIN_HOOD_TRACE(this);
        using std::swap;
        swap(o, *this);
    }

    // Clears all data, without resizing.
    void clear() {
        ROBIN_HOOD_TRACE(this);
        if (empty()) {
            // don't do anything! also important because we don't want to write to DummyInfoByte::b,
            // even though we would just write 0 to it.
            return;
        }

        Destroyer<Self, IsFlatMap && std::is_trivially_destructible<Node>::value>{}.nodes(*this);

        // clear everything except the sentinel
        // std::memset(mInfo, 0, sizeof(uint8_t) * (mMask + 1));
        uint8_t const z = 0;
        std::fill(mInfo, mInfo + (sizeof(uint8_t) * (mMask + 1)), z);

        mInfoInc = InitialInfoInc;
        mInfoHashShift = InitialInfoHashShift;
    }

    // Destroys the map and all it's contents.
    ~unordered_map() {
        ROBIN_HOOD_TRACE(this);
        destroy();
    }

    // Checks if both maps contain the same entries. Order is irrelevant.
    bool operator==(const unordered_map& other) const {
        ROBIN_HOOD_TRACE(this);
        if (other.size() != size()) {
            return false;
        }
        for (auto const& otherEntry : other) {
            auto const myIt = find(otherEntry.first);
            if (myIt == end() || !(myIt->second == otherEntry.second)) {
                return false;
            }
        }

        return true;
    }

    bool operator!=(const unordered_map& other) const {
        ROBIN_HOOD_TRACE(this);
        return !operator==(other);
    }

    mapped_type& operator[](const key_type& key) {
        ROBIN_HOOD_TRACE(this);
        return doCreateByKey(key);
    }

    mapped_type& operator[](key_type&& key) {
        ROBIN_HOOD_TRACE(this);
        return doCreateByKey(std::move(key));
    }

    template <typename Iter>
    void insert(Iter first, Iter last) {
        for (; first != last; ++first) {
            // value_type ctor needed because this might be called with std::pair's
            insert(value_type(*first));
        }
    }

    template <typename... Args>
    std::pair<iterator, bool> emplace(Args&&... args) {
        ROBIN_HOOD_TRACE(this);
        Node n{*this, std::forward<Args>(args)...};
        auto r = doInsert(std::move(n));
        if (!r.second) {
            // insertion not possible: destroy node
            // NOLINTNEXTLINE(bugprone-use-after-move)
            n.destroy(*this);
        }
        return r;
    }

    std::pair<iterator, bool> insert(const value_type& keyval) {
        ROBIN_HOOD_TRACE(this);
        return doInsert(keyval);
    }

    std::pair<iterator, bool> insert(value_type&& keyval) {
        return doInsert(std::move(keyval));
    }

    // Returns 1 if key is found, 0 otherwise.
    size_t count(const key_type& key) const { // NOLINT(modernize-use-nodiscard)
        ROBIN_HOOD_TRACE(this);
        auto kv = mKeyVals + findIdx(key);
        if (kv != reinterpret_cast_no_cast_align_warning<Node*>(mInfo)) {
            return 1;
        }
        return 0;
    }

    // Returns a reference to the value found for key.
    // Throws std::out_of_range if element cannot be found
    mapped_type& at(key_type const& key) {
        ROBIN_HOOD_TRACE(this);
        auto kv = mKeyVals + findIdx(key);
        if (kv == reinterpret_cast_no_cast_align_warning<Node*>(mInfo)) {
            doThrow<std::out_of_range>("key not found");
        }
        return kv->getSecond();
    }

    // Returns a reference to the value found for key.
    // Throws std::out_of_range if element cannot be found
    mapped_type const& at(key_type const& key) const { // NOLINT(modernize-use-nodiscard)
        ROBIN_HOOD_TRACE(this);
        auto kv = mKeyVals + findIdx(key);
        if (kv == reinterpret_cast_no_cast_align_warning<Node*>(mInfo)) {
            doThrow<std::out_of_range>("key not found");
        }
        return kv->getSecond();
    }

    const_iterator find(const key_type& key) const { // NOLINT(modernize-use-nodiscard)
        ROBIN_HOOD_TRACE(this);
        const size_t idx = findIdx(key);
        return const_iterator{mKeyVals + idx, mInfo + idx};
    }

    template <typename OtherKey>
    const_iterator find(const OtherKey& key, is_transparent_tag /*unused*/) const {
        ROBIN_HOOD_TRACE(this);
        const size_t idx = findIdx(key);
        return const_iterator{mKeyVals + idx, mInfo + idx};
    }

    iterator find(const key_type& key) {
        ROBIN_HOOD_TRACE(this);
        const size_t idx = findIdx(key);
        return iterator{mKeyVals + idx, mInfo + idx};
    }

    template <typename OtherKey>
    iterator find(const OtherKey& key, is_transparent_tag /*unused*/) {
        ROBIN_HOOD_TRACE(this);
        const size_t idx = findIdx(key);
        return iterator{mKeyVals + idx, mInfo + idx};
    }

    iterator begin() {
        ROBIN_HOOD_TRACE(this);
        if (empty()) {
            return end();
        }
        return iterator(mKeyVals, mInfo, fast_forward_tag{});
    }
    const_iterator begin() const { // NOLINT(modernize-use-nodiscard)
        ROBIN_HOOD_TRACE(this);
        return cbegin();
    }
    const_iterator cbegin() const { // NOLINT(modernize-use-nodiscard)
        ROBIN_HOOD_TRACE(this);
        if (empty()) {
            return cend();
        }
        return const_iterator(mKeyVals, mInfo, fast_forward_tag{});
    }

    iterator end() {
        ROBIN_HOOD_TRACE(this);
        // no need to supply valid info pointer: end() must not be dereferenced, and only node
        // pointer is compared.
        return iterator{reinterpret_cast_no_cast_align_warning<Node*>(mInfo), nullptr};
    }
    const_iterator end() const { // NOLINT(modernize-use-nodiscard)
        ROBIN_HOOD_TRACE(this);
        return cend();
    }
    const_iterator cend() const { // NOLINT(modernize-use-nodiscard)
        ROBIN_HOOD_TRACE(this);
        return const_iterator{reinterpret_cast_no_cast_align_warning<Node*>(mInfo), nullptr};
    }

    iterator erase(const_iterator pos) {
        ROBIN_HOOD_TRACE(this);
        // its safe to perform const cast here
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast)
        return erase(iterator{const_cast<Node*>(pos.mKeyVals), const_cast<uint8_t*>(pos.mInfo)});
    }

    // Erases element at pos, returns iterator to the next element.
    iterator erase(iterator pos) {
        ROBIN_HOOD_TRACE(this);
        // we assume that pos always points to a valid entry, and not end().
        auto const idx = static_cast<size_t>(pos.mKeyVals - mKeyVals);

        shiftDown(idx);
        --mNumElements;

        if (*pos.mInfo) {
            // we've backward shifted, return this again
            return pos;
        }

        // no backward shift, return next element
        return ++pos;
    }

    size_t erase(const key_type& key) {
        ROBIN_HOOD_TRACE(this);
        size_t idx;
        InfoType info;
        keyToIdx(key, &idx, &info);

        // check while info matches with the source idx
        do {
            if (info == mInfo[idx] && KeyEqual::operator()(key, mKeyVals[idx].getFirst())) {
                shiftDown(idx);
                --mNumElements;
                return 1;
            }
            next(&info, &idx);
        } while (info <= mInfo[idx]);

        // nothing found to delete
        return 0;
    }

    // reserves space for the specified number of elements. Makes sure the old data fits.
    // exactly the same as reserve(c).
    void rehash(size_t c) {
        reserve(c);
    }

    // reserves space for the specified number of elements. Makes sure the old data fits.
    // Exactly the same as resize(c). Use resize(0) to shrink to fit.
    void reserve(size_t c) {
        ROBIN_HOOD_TRACE(this);
        auto const minElementsAllowed = (std::max)(c, mNumElements);
        auto newSize = InitialNumElements;
        while (calcMaxNumElementsAllowed(newSize) < minElementsAllowed && newSize != 0) {
            newSize *= 2;
        }
        if (ROBIN_HOOD_UNLIKELY(newSize == 0)) {
            throwOverflowError();
        }

        rehashPowerOfTwo(newSize);
    }

    size_type size() const noexcept { // NOLINT(modernize-use-nodiscard)
        ROBIN_HOOD_TRACE(this);
        return mNumElements;
    }

    size_type max_size() const noexcept { // NOLINT(modernize-use-nodiscard)
        ROBIN_HOOD_TRACE(this);
        return static_cast<size_type>(-1);
    }

    ROBIN_HOOD(NODISCARD) bool empty() const noexcept {
        ROBIN_HOOD_TRACE(this);
        return 0 == mNumElements;
    }

    float max_load_factor() const noexcept { // NOLINT(modernize-use-nodiscard)
        ROBIN_HOOD_TRACE(this);
        return MaxLoadFactor100 / 100.0F;
    }

    // Average number of elements per bucket. Since we allow only 1 per bucket
    float load_factor() const noexcept { // NOLINT(modernize-use-nodiscard)
        ROBIN_HOOD_TRACE(this);
        return static_cast<float>(size()) / static_cast<float>(mMask + 1);
    }

    ROBIN_HOOD(NODISCARD) size_t mask() const noexcept {
        ROBIN_HOOD_TRACE(this);
        return mMask;
    }

    ROBIN_HOOD(NODISCARD) size_t calcMaxNumElementsAllowed(size_t maxElements) const noexcept {
        if (ROBIN_HOOD_LIKELY(maxElements <= (std::numeric_limits<size_t>::max)() / 100)) {
            return maxElements * MaxLoadFactor100 / 100;
        }

        // we might be a bit inprecise, but since maxElements is quite large that doesn't matter
        return (maxElements / 100) * MaxLoadFactor100;
    }

    ROBIN_HOOD(NODISCARD) size_t calcNumBytesInfo(size_t numElements) const {
        return numElements + sizeof(uint64_t);
    }

    // calculation ony allowed for 2^n values
    ROBIN_HOOD(NODISCARD) size_t calcNumBytesTotal(size_t numElements) const {
#if ROBIN_HOOD(BITNESS) == 64
        return numElements * sizeof(Node) + calcNumBytesInfo(numElements);
#else
        // make sure we're doing 64bit operations, so we are at least safe against 32bit overflows.
        auto const ne = static_cast<uint64_t>(numElements);
        auto const s = static_cast<uint64_t>(sizeof(Node));
        auto const infos = static_cast<uint64_t>(calcNumBytesInfo(numElements));

        auto const total64 = ne * s + infos;
        auto const total = static_cast<size_t>(total64);

        if (ROBIN_HOOD_UNLIKELY(static_cast<uint64_t>(total) != total64)) {
            throwOverflowError();
        }
        return total;
#endif
    }

private:
    // reserves space for at least the specified number of elements.
    // only works if numBuckets if power of two
    void rehashPowerOfTwo(size_t numBuckets) {
        ROBIN_HOOD_TRACE(this);

        Node* const oldKeyVals = mKeyVals;
        uint8_t const* const oldInfo = mInfo;

        const size_t oldMaxElements = mMask + 1;

        // resize operation: move stuff
        init_data(numBuckets);
        if (oldMaxElements > 1) {
            for (size_t i = 0; i < oldMaxElements; ++i) {
                if (oldInfo[i] != 0) {
                    insert_move(std::move(oldKeyVals[i]));
                    // destroy the node but DON'T destroy the data.
                    oldKeyVals[i].~Node();
                }
            }

            // don't destroy old data: put it into the pool instead
            DataPool::addOrFree(oldKeyVals, calcNumBytesTotal(oldMaxElements));
        }
    }

    ROBIN_HOOD(NOINLINE) void throwOverflowError() const {
#if ROBIN_HOOD(HAS_EXCEPTIONS)
        throw std::overflow_error("robin_hood::map overflow");
#else
        abort();
#endif
    }

    void init_data(size_t max_elements) {
        mNumElements = 0;
        mMask = max_elements - 1;
        mMaxNumElementsAllowed = calcMaxNumElementsAllowed(max_elements);

        // calloc also zeroes everything
        mKeyVals = reinterpret_cast<Node*>(
            detail::assertNotNull<std::bad_alloc>(calloc(1, calcNumBytesTotal(max_elements))));
        mInfo = reinterpret_cast<uint8_t*>(mKeyVals + max_elements);

        // set sentinel
        mInfo[max_elements] = 1;

        mInfoInc = InitialInfoInc;
        mInfoHashShift = InitialInfoHashShift;
    }

    template <typename Arg>
    mapped_type& doCreateByKey(Arg&& key) {
        while (true) {
            size_t idx;
            InfoType info;
            keyToIdx(key, &idx, &info);
            nextWhileLess(&info, &idx);

            // while we potentially have a match. Can't do a do-while here because when mInfo is 0
            // we don't want to skip forward
            while (info == mInfo[idx]) {
                if (KeyEqual::operator()(key, mKeyVals[idx].getFirst())) {
                    // key already exists, do not insert.
                    return mKeyVals[idx].getSecond();
                }
                next(&info, &idx);
            }

            // unlikely that this evaluates to true
            if (ROBIN_HOOD_UNLIKELY(mNumElements >= mMaxNumElementsAllowed)) {
                increase_size();
                continue;
            }

            // key not found, so we are now exactly where we want to insert it.
            auto const insertion_idx = idx;
            auto const insertion_info = info;
            if (ROBIN_HOOD_UNLIKELY(insertion_info + mInfoInc > 0xFF)) {
                mMaxNumElementsAllowed = 0;
            }

            // find an empty spot
            while (0 != mInfo[idx]) {
                next(&info, &idx);
            }

            auto& l = mKeyVals[insertion_idx];
            if (idx == insertion_idx) {
                // put at empty spot. This forwards all arguments into the node where the object is
                // constructed exactly where it is needed.
                ::new (static_cast<void*>(&l))
                    Node(*this, std::piecewise_construct,
                         std::forward_as_tuple(std::forward<Arg>(key)), std::forward_as_tuple());
            } else {
                shiftUp(idx, insertion_idx);
                l = Node(*this, std::piecewise_construct,
                         std::forward_as_tuple(std::forward<Arg>(key)), std::forward_as_tuple());
            }

            // mKeyVals[idx].getFirst() = std::move(key);
            mInfo[insertion_idx] = static_cast<uint8_t>(insertion_info);

            ++mNumElements;
            return mKeyVals[insertion_idx].getSecond();
        }
    }

    // This is exactly the same code as operator[], except for the return values
    template <typename Arg>
    std::pair<iterator, bool> doInsert(Arg&& keyval) {
        while (true) {
            size_t idx;
            InfoType info;
            keyToIdx(keyval.getFirst(), &idx, &info);
            nextWhileLess(&info, &idx);

            // while we potentially have a match
            while (info == mInfo[idx]) {
                if (KeyEqual::operator()(keyval.getFirst(), mKeyVals[idx].getFirst())) {
                    // key already exists, do NOT insert.
                    // see http://en.cppreference.com/w/cpp/container/unordered_map/insert
                    return std::make_pair<iterator, bool>(iterator(mKeyVals + idx, mInfo + idx),
                                                          false);
                }
                next(&info, &idx);
            }

            // unlikely that this evaluates to true
            if (ROBIN_HOOD_UNLIKELY(mNumElements >= mMaxNumElementsAllowed)) {
                increase_size();
                continue;
            }

            // key not found, so we are now exactly where we want to insert it.
            auto const insertion_idx = idx;
            auto const insertion_info = info;
            if (ROBIN_HOOD_UNLIKELY(insertion_info + mInfoInc > 0xFF)) {
                mMaxNumElementsAllowed = 0;
            }

            // find an empty spot
            while (0 != mInfo[idx]) {
                next(&info, &idx);
            }

            auto& l = mKeyVals[insertion_idx];
            if (idx == insertion_idx) {
                ::new (static_cast<void*>(&l)) Node(*this, std::forward<Arg>(keyval));
            } else {
                shiftUp(idx, insertion_idx);
                l = Node(*this, std::forward<Arg>(keyval));
            }

            // put at empty spot
            mInfo[insertion_idx] = static_cast<uint8_t>(insertion_info);

            ++mNumElements;
            return std::make_pair(iterator(mKeyVals + insertion_idx, mInfo + insertion_idx), true);
        }
    }

    bool try_increase_info() {
        ROBIN_HOOD_LOG("mInfoInc=" << mInfoInc << ", numElements=" << mNumElements
                                   << ", maxNumElementsAllowed="
                                   << calcMaxNumElementsAllowed(mMask + 1));
        if (mInfoInc <= 2) {
            // need to be > 2 so that shift works (otherwise undefined behavior!)
            return false;
        }
        // we got space left, try to make info smaller
        mInfoInc = static_cast<uint8_t>(mInfoInc >> 1U);

        // remove one bit of the hash, leaving more space for the distance info.
        // This is extremely fast because we can operate on 8 bytes at once.
        ++mInfoHashShift;
        auto const data = reinterpret_cast_no_cast_align_warning<uint64_t*>(mInfo);
        auto const numEntries = (mMask + 1) / 8;

        for (size_t i = 0; i < numEntries; ++i) {
            data[i] = (data[i] >> 1U) & UINT64_C(0x7f7f7f7f7f7f7f7f);
        }
        mMaxNumElementsAllowed = calcMaxNumElementsAllowed(mMask + 1);
        return true;
    }

    void increase_size() {
        // nothing allocated yet? just allocate InitialNumElements
        if (0 == mMask) {
            init_data(InitialNumElements);
            return;
        }

        auto const maxNumElementsAllowed = calcMaxNumElementsAllowed(mMask + 1);
        if (mNumElements < maxNumElementsAllowed && try_increase_info()) {
            return;
        }

        ROBIN_HOOD_LOG("mNumElements=" << mNumElements << ", maxNumElementsAllowed="
                                       << maxNumElementsAllowed << ", load="
                                       << (static_cast<double>(mNumElements) * 100.0 /
                                           (static_cast<double>(mMask) + 1)));
        // it seems we have a really bad hash function! don't try to resize again
        if (mNumElements * 2 < calcMaxNumElementsAllowed(mMask + 1)) {
            throwOverflowError();
        }

        rehashPowerOfTwo((mMask + 1) * 2);
    }

    void destroy() {
        if (0 == mMask) {
            // don't deallocate!
            return;
        }

        Destroyer<Self, IsFlatMap && std::is_trivially_destructible<Node>::value>{}
            .nodesDoNotDeallocate(*this);
        free(mKeyVals);
    }

    void init() noexcept {
        mKeyVals = reinterpret_cast<Node*>(&mMask);
        mInfo = reinterpret_cast<uint8_t*>(&mMask);
        mNumElements = 0;
        mMask = 0;
        mMaxNumElementsAllowed = 0;
        mInfoInc = InitialInfoInc;
        mInfoHashShift = InitialInfoHashShift;
    }

    // members are sorted so no padding occurs
    Node* mKeyVals = reinterpret_cast<Node*>(&mMask);    // 8 byte  8
    uint8_t* mInfo = reinterpret_cast<uint8_t*>(&mMask); // 8 byte 16
    size_t mNumElements = 0;                             // 8 byte 24
    size_t mMask = 0;                                    // 8 byte 32
    size_t mMaxNumElementsAllowed = 0;                   // 8 byte 40
    InfoType mInfoInc = InitialInfoInc;                  // 4 byte 44
    InfoType mInfoHashShift = InitialInfoHashShift;      // 4 byte 48
                                                         // 16 byte 56 if NodeAllocator
};

} // namespace detail

template <typename Key, typename T, typename Hash,
          typename KeyEqual = std::equal_to<Key>, size_t MaxLoadFactor100 = 80>
using unordered_flat_map = detail::unordered_map<true, MaxLoadFactor100, Key, T, Hash, KeyEqual>;

template <typename Key, typename T, typename Hash,
          typename KeyEqual = std::equal_to<Key>, size_t MaxLoadFactor100 = 80>
using unordered_node_map = detail::unordered_map<false, MaxLoadFactor100, Key, T, Hash, KeyEqual>;

template <typename Key, typename T, typename Hash,
          typename KeyEqual = std::equal_to<Key>, size_t MaxLoadFactor100 = 80>
using unordered_map =
    detail::unordered_map<sizeof(std::pair<Key, T>) <= sizeof(size_t) * 6 &&
                              std::is_nothrow_move_constructible<std::pair<Key, T>>::value &&
                              std::is_nothrow_move_assignable<std::pair<Key, T>>::value,
                          MaxLoadFactor100, Key, T, Hash, KeyEqual>;

} // namespace robin_hood

#endif
