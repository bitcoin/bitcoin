// Copyright (c) 2018-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_SPAN_H
#define BITCOIN_SPAN_H

#include <cassert>
#include <cstddef>
#include <span>
#include <type_traits>
#include <utility>

#ifdef DEBUG
#define CONSTEXPR_IF_NOT_DEBUG
#define ASSERT_IF_DEBUG(x) assert((x))
#else
#define CONSTEXPR_IF_NOT_DEBUG constexpr
#define ASSERT_IF_DEBUG(x)
#endif

#if defined(__clang__)
#if __has_attribute(lifetimebound)
#define SPAN_ATTR_LIFETIMEBOUND [[clang::lifetimebound]]
#else
#define SPAN_ATTR_LIFETIMEBOUND
#endif
#else
#define SPAN_ATTR_LIFETIMEBOUND
#endif

/** A Span is an object that can refer to a contiguous sequence of objects.
 *
 * This file implements a subset of C++20's std::span.  It can be considered
 * temporary compatibility code until C++20 and is designed to be a
 * self-contained abstraction without depending on other project files. For this
 * reason, Clang lifetimebound is defined here instead of including
 * <attributes.h>, which also defines it.
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
    std::size_t m_size{0};

    template <class T>
    struct is_Span_int : public std::false_type {};
    template <class T>
    struct is_Span_int<Span<T>> : public std::true_type {};
    template <class T>
    struct is_Span : public is_Span_int<typename std::remove_cv<T>::type>{};


public:
    constexpr Span() noexcept : m_data(nullptr) {}

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
    template <typename V>
    constexpr Span(V& other SPAN_ATTR_LIFETIMEBOUND,
        typename std::enable_if<!is_Span<V>::value &&
                                std::is_convertible<typename std::remove_pointer<decltype(std::declval<V&>().data())>::type (*)[], C (*)[]>::value &&
                                std::is_convertible<decltype(std::declval<V&>().size()), std::size_t>::value, std::nullptr_t>::type = nullptr)
        : m_data(other.data()), m_size(other.size()){}

    template <typename V>
    constexpr Span(const V& other SPAN_ATTR_LIFETIMEBOUND,
        typename std::enable_if<!is_Span<V>::value &&
                                std::is_convertible<typename std::remove_pointer<decltype(std::declval<const V&>().data())>::type (*)[], C (*)[]>::value &&
                                std::is_convertible<decltype(std::declval<const V&>().size()), std::size_t>::value, std::nullptr_t>::type = nullptr)
        : m_data(other.data()), m_size(other.size()){}

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
    constexpr std::size_t size_bytes() const noexcept { return sizeof(C) * m_size; }
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

    template <typename O> friend class Span;
};

// Return result of calling .data() method on type T. This is used to be able to
// write template deduction guides for the single-parameter Span constructor
// below that will work if the value that is passed has a .data() method, and if
// the data method does not return a void pointer.
//
// It is important to check for the void type specifically below, so the
// deduction guides can be used in SFINAE contexts to check whether objects can
// be converted to spans. If the deduction guides did not explicitly check for
// void, and an object was passed that returned void* from data (like
// std::vector<bool>), the template deduction would succeed, but the Span<void>
// object instantiation would fail, resulting in a hard error, rather than a
// SFINAE error.
// https://stackoverflow.com/questions/68759148/sfinae-to-detect-the-explicitness-of-a-ctad-deduction-guide
// https://stackoverflow.com/questions/16568986/what-happens-when-you-call-data-on-a-stdvectorbool
template<typename T>
using DataResult = std::remove_pointer_t<decltype(std::declval<T&>().data())>;

// Deduction guides for Span
// For the pointer/size based and iterator based constructor:
template <typename T, typename EndOrSize> Span(T*, EndOrSize) -> Span<T>;
// For the array constructor:
template <typename T, std::size_t N> Span(T (&)[N]) -> Span<T>;
// For the temporaries/rvalue references constructor, only supporting const output.
template <typename T> Span(T&&) -> Span<std::enable_if_t<!std::is_lvalue_reference_v<T> && !std::is_void_v<DataResult<T&&>>, const DataResult<T&&>>>;
// For (lvalue) references, supporting mutable output.
template <typename T> Span(T&) -> Span<std::enable_if_t<!std::is_void_v<DataResult<T&>>, DataResult<T&>>>;

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

// From C++20 as_bytes and as_writeable_bytes
template <typename T>
Span<const std::byte> AsBytes(Span<T> s) noexcept
{
    return {reinterpret_cast<const std::byte*>(s.data()), s.size_bytes()};
}
template <typename T>
Span<std::byte> AsWritableBytes(Span<T> s) noexcept
{
    return {reinterpret_cast<std::byte*>(s.data()), s.size_bytes()};
}

template <typename V>
Span<const std::byte> MakeByteSpan(V&& v) noexcept
{
    return AsBytes(Span{std::forward<V>(v)});
}
template <typename V>
Span<std::byte> MakeWritableByteSpan(V&& v) noexcept
{
    return AsWritableBytes(Span{std::forward<V>(v)});
}

// Helper functions to safely cast basic byte pointers to unsigned char pointers.
inline unsigned char* UCharCast(char* c) { return reinterpret_cast<unsigned char*>(c); }
inline unsigned char* UCharCast(unsigned char* c) { return c; }
inline unsigned char* UCharCast(signed char* c) { return reinterpret_cast<unsigned char*>(c); }
inline unsigned char* UCharCast(std::byte* c) { return reinterpret_cast<unsigned char*>(c); }
inline const unsigned char* UCharCast(const char* c) { return reinterpret_cast<const unsigned char*>(c); }
inline const unsigned char* UCharCast(const unsigned char* c) { return c; }
inline const unsigned char* UCharCast(const signed char* c) { return reinterpret_cast<const unsigned char*>(c); }
inline const unsigned char* UCharCast(const std::byte* c) { return reinterpret_cast<const unsigned char*>(c); }
// Helper concept for the basic byte types.
template <typename B>
concept BasicByte = requires { UCharCast(std::span<B>{}.data()); };

// Helper function to safely convert a Span to a Span<[const] unsigned char>.
template <typename T> constexpr auto UCharSpanCast(Span<T> s) -> Span<typename std::remove_pointer<decltype(UCharCast(s.data()))>::type> { return {UCharCast(s.data()), s.size()}; }

/** Like the Span constructor, but for (const) unsigned char member types only. Only works for (un)signed char containers. */
template <typename V> constexpr auto MakeUCharSpan(V&& v) -> decltype(UCharSpanCast(Span{std::forward<V>(v)})) { return UCharSpanCast(Span{std::forward<V>(v)}); }

#endif // BITCOIN_SPAN_H
