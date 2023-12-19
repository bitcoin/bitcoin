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
#include <span>

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
#define Span std::span

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
#define AsBytes std::as_bytes
#define AsWritableBytes std::as_writable_bytes

template <typename V>
Span<const std::byte> MakeByteSpan(const V& v) noexcept
{
    return AsBytes(Span{v});
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
template <typename T, size_t N> constexpr auto UCharSpanCast(Span<T, N> s) -> Span<typename std::remove_pointer<decltype(UCharCast(s.data()))>::type> { return {UCharCast(s.data()), s.size()}; }

/** Like the Span constructor, but for (const) unsigned char member types only. Only works for (un)signed char containers. */
template <typename V> constexpr auto MakeUCharSpan(V&& v) -> decltype(UCharSpanCast(Span{std::forward<V>(v)})) { return UCharSpanCast(Span{std::forward<V>(v)}); }

#endif // BITCOIN_SPAN_H
