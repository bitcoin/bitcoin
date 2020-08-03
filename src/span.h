// Copyright (c) 2018-2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_SPAN_H
#define BITCOIN_SPAN_H

#include <type_traits>
#include <cstddef>
#include <algorithm>
#include <assert.h>

#ifdef DEBUG
#define CONSTEXPR_IF_NOT_DEBUG
#define ASSERT_IF_DEBUG(x) assert((x))
#else
#define CONSTEXPR_IF_NOT_DEBUG constexpr
#define ASSERT_IF_DEBUG(x)
#endif

/** A Span is an object that can refer to a contiguous sequence of objects.
 *
 * It implements a subset of C++20's std::span.
 *
 * Things to be aware of when writing code that deals with Spans:
 *
 * - Similar to references themselves, Spans are subject to reference lifetime
 *   issues. The user is responsible for making sure the objects pointed to by
 *   a Span live as long as the Span is used. For example:
 *
 *       std::vector<int> vec{1,2,3,4};
 *       Span<int> sp(vec);
 *       vec.push_back(5);
 *       printf("%i\n", sp.front()); // UB!
 *
 *   may exhibit undefined behavior, as increasing the size of a vector may
 *   invalidate references.
 *
 * - One particular pitfall is that Spans can be constructed from temporaries,
 *   but this is unsafe when the Span is stored in a variable, outliving the
 *   temporary. For example, this will compile, but exhibits undefined behavior:
 *
 *       Span<const int> sp(std::vector<int>{1, 2, 3});
 *       printf("%i\n", sp.front()); // UB!
 *
 *   The lifetime of the vector ends when the statement it is created in ends.
 *   Thus the Span is left with a dangling reference, and using it is undefined.
 *
 * - Due to Span's automatic creation from range-like objects (arrays, and data
 *   types that expose a data() and size() member function), functions that
 *   accept a Span as input parameter can be called with any compatible
 *   range-like object. For example, this works:
*
 *       void Foo(Span<const int> arg);
 *
 *       Foo(std::vector<int>{1, 2, 3}); // Works
 *
 *   This is very useful in cases where a function truly does not care about the
 *   container, and only about having exactly a range of elements. However it
 *   may also be surprising to see automatic conversions in this case.
 *
 *   When a function accepts a Span with a mutable element type, it will not
 *   accept temporaries; only variables or other references. For example:
 *
 *       void FooMut(Span<int> arg);
 *
 *       FooMut(std::vector<int>{1, 2, 3}); // Does not compile
 *       std::vector<int> baz{1, 2, 3};
 *       FooMut(baz); // Works
 *
 *   This is similar to how functions that take (non-const) lvalue references
 *   as input cannot accept temporaries. This does not work either:
 *
 *       void FooVec(std::vector<int>& arg);
 *       FooVec(std::vector<int>{1, 2, 3}); // Does not compile
 *
 *   The idea is that if a function accepts a mutable reference, a meaningful
 *   result will be present in that variable after the call. Passing a temporary
 *   is useless in that context.
 */
template<typename C>
class Span
{
    C* m_data;
    std::size_t m_size;

public:
    constexpr Span() noexcept : m_data(nullptr), m_size(0) {}

    /** Construct a span from a begin pointer and a size.
     *
     * This implements a subset of the iterator-based std::span constructor in C++20,
     * which is hard to implement without std::address_of.
     */
    template <typename T, typename std::enable_if<std::is_convertible<T (*)[], C (*)[]>::value, int>::type = 0>
    constexpr Span(T* begin, std::size_t size) noexcept : m_data(begin), m_size(size) {}

    /** Construct a span from a begin and end pointer.
     *
     * This implements a subset of the iterator-based std::span constructor in C++20,
     * which is hard to implement without std::address_of.
     */
    template <typename T, typename std::enable_if<std::is_convertible<T (*)[], C (*)[]>::value, int>::type = 0>
    CONSTEXPR_IF_NOT_DEBUG Span(T* begin, T* end) noexcept : m_data(begin), m_size(end - begin)
    {
        ASSERT_IF_DEBUG(end >= begin);
    }

    /** Implicit conversion of spans between compatible types.
     *
     *  Specifically, if a pointer to an array of type O can be implicitly converted to a pointer to an array of type
     *  C, then permit implicit conversion of Span<O> to Span<C>. This matches the behavior of the corresponding
     *  C++20 std::span constructor.
     *
     *  For example this means that a Span<T> can be converted into a Span<const T>.
     */
    template <typename O, typename std::enable_if<std::is_convertible<O (*)[], C (*)[]>::value, int>::type = 0>
    constexpr Span(const Span<O>& other) noexcept : m_data(other.m_data), m_size(other.m_size) {}

    /** Default copy constructor. */
    constexpr Span(const Span&) noexcept = default;

    /** Default assignment operator. */
    Span& operator=(const Span& other) noexcept = default;

    /** Construct a Span from an array. This matches the corresponding C++20 std::span constructor. */
    template <int N>
    constexpr Span(C (&a)[N]) noexcept : m_data(a), m_size(N) {}

    /** Construct a Span for objects with .data() and .size() (std::string, std::array, std::vector, ...).
     *
     * This implements a subset of the functionality provided by the C++20 std::span range-based constructor.
     *
     * To prevent surprises, only Spans for constant value types are supported when passing in temporaries.
     * Note that this restriction does not exist when converting arrays or other Spans (see above).
     */
    template <typename V, typename std::enable_if<(std::is_const<C>::value || std::is_lvalue_reference<V>::value) && std::is_convertible<typename std::remove_pointer<decltype(std::declval<V&>().data())>::type (*)[], C (*)[]>::value && std::is_convertible<decltype(std::declval<V&>().size()), std::size_t>::value, int>::type = 0>
    constexpr Span(V&& v) noexcept : m_data(v.data()), m_size(v.size()) {}

    constexpr C* data() const noexcept { return m_data; }
    constexpr C* begin() const noexcept { return m_data; }
    constexpr C* end() const noexcept { return m_data + m_size; }
    CONSTEXPR_IF_NOT_DEBUG C& front() const noexcept
    {
        ASSERT_IF_DEBUG(size() > 0);
        return m_data[0];
    }
    CONSTEXPR_IF_NOT_DEBUG C& back() const noexcept
    {
        ASSERT_IF_DEBUG(size() > 0);
        return m_data[m_size - 1];
    }
    constexpr std::size_t size() const noexcept { return m_size; }
    constexpr bool empty() const noexcept { return size() == 0; }
    CONSTEXPR_IF_NOT_DEBUG C& operator[](std::size_t pos) const noexcept
    {
        ASSERT_IF_DEBUG(size() > pos);
        return m_data[pos];
    }
    CONSTEXPR_IF_NOT_DEBUG Span<C> subspan(std::size_t offset) const noexcept
    {
        ASSERT_IF_DEBUG(size() >= offset);
        return Span<C>(m_data + offset, m_size - offset);
    }
    CONSTEXPR_IF_NOT_DEBUG Span<C> subspan(std::size_t offset, std::size_t count) const noexcept
    {
        ASSERT_IF_DEBUG(size() >= offset + count);
        return Span<C>(m_data + offset, count);
    }
    CONSTEXPR_IF_NOT_DEBUG Span<C> first(std::size_t count) const noexcept
    {
        ASSERT_IF_DEBUG(size() >= count);
        return Span<C>(m_data, count);
    }
    CONSTEXPR_IF_NOT_DEBUG Span<C> last(std::size_t count) const noexcept
    {
         ASSERT_IF_DEBUG(size() >= count);
         return Span<C>(m_data + m_size - count, count);
    }

    friend constexpr bool operator==(const Span& a, const Span& b) noexcept { return a.size() == b.size() && std::equal(a.begin(), a.end(), b.begin()); }
    friend constexpr bool operator!=(const Span& a, const Span& b) noexcept { return !(a == b); }
    friend constexpr bool operator<(const Span& a, const Span& b) noexcept { return std::lexicographical_compare(a.begin(), a.end(), b.begin(), b.end()); }
    friend constexpr bool operator<=(const Span& a, const Span& b) noexcept { return !(b < a); }
    friend constexpr bool operator>(const Span& a, const Span& b) noexcept { return (b < a); }
    friend constexpr bool operator>=(const Span& a, const Span& b) noexcept { return !(a < b); }

    template <typename O> friend class Span;
};

// MakeSpan helps constructing a Span of the right type automatically.
/** MakeSpan for arrays: */
template <typename A, int N> Span<A> constexpr MakeSpan(A (&a)[N]) { return Span<A>(a, N); }
/** MakeSpan for temporaries / rvalue references, only supporting const output. */
template <typename V> constexpr auto MakeSpan(V&& v) -> typename std::enable_if<!std::is_lvalue_reference<V>::value, Span<const typename std::remove_pointer<decltype(v.data())>::type>>::type { return std::forward<V>(v); }
/** MakeSpan for (lvalue) references, supporting mutable output. */
template <typename V> constexpr auto MakeSpan(V& v) -> Span<typename std::remove_pointer<decltype(v.data())>::type> { return v; }

/** Pop the last element off a span, and return a reference to that element. */
template <typename T>
T& SpanPopBack(Span<T>& span)
{
    size_t size = span.size();
    ASSERT_IF_DEBUG(size > 0);
    T& back = span[size - 1];
    span = Span<T>(span.data(), size - 1);
    return back;
}

// Helper functions to safely cast to unsigned char pointers.
inline unsigned char* UCharCast(char* c) { return (unsigned char*)c; }
inline unsigned char* UCharCast(unsigned char* c) { return c; }
inline const unsigned char* UCharCast(const char* c) { return (unsigned char*)c; }
inline const unsigned char* UCharCast(const unsigned char* c) { return c; }

// Helper function to safely convert a Span to a Span<[const] unsigned char>.
template <typename T> constexpr auto UCharSpanCast(Span<T> s) -> Span<typename std::remove_pointer<decltype(UCharCast(s.data()))>::type> { return {UCharCast(s.data()), s.size()}; }

/** Like MakeSpan, but for (const) unsigned char member types only. Only works for (un)signed char containers. */
template <typename V> constexpr auto MakeUCharSpan(V&& v) -> decltype(UCharSpanCast(MakeSpan(std::forward<V>(v)))) { return UCharSpanCast(MakeSpan(std::forward<V>(v))); }

#endif
