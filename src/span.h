// Copyright (c) 2018-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_SPAN_H
#define BITCOIN_SPAN_H

#include <cassert>
#include <cstddef>
#include <span>
#include <type_traits>
#include <utility>

/** A span is an object that can refer to a contiguous sequence of objects.
 *
 * Things to be aware of when writing code that deals with spans:
 *
 * - Similar to references themselves, spans are subject to reference lifetime
 *   issues. The user is responsible for making sure the objects pointed to by
 *   a span live as long as the span is used. For example:
 *
 *       std::vector<int> vec{1,2,3,4};
 *       std::span<int> sp(vec);
 *       vec.push_back(5);
 *       printf("%i\n", sp.front()); // UB!
 *
 *   may exhibit undefined behavior, as increasing the size of a vector may
 *   invalidate references.
 *
 * - One particular pitfall is that spans can be constructed from temporaries,
 *   but this is unsafe when the span is stored in a variable, outliving the
 *   temporary. For example, this will compile, but exhibits undefined behavior:
 *
 *       std::span<const int> sp(std::vector<int>{1, 2, 3});
 *       printf("%i\n", sp.front()); // UB!
 *
 *   The lifetime of the vector ends when the statement it is created in ends.
 *   Thus the span is left with a dangling reference, and using it is undefined.
 *
 * - Due to spans automatic creation from range-like objects (arrays, and data
 *   types that expose a data() and size() member function), functions that
 *   accept a span as input parameter can be called with any compatible
 *   range-like object. For example, this works:
 *
 *       void Foo(std::span<const int> arg);
 *
 *       Foo(std::vector<int>{1, 2, 3}); // Works
 *
 *   This is very useful in cases where a function truly does not care about the
 *   container, and only about having exactly a range of elements. However it
 *   may also be surprising to see automatic conversions in this case.
 *
 *   When a function accepts a span with a mutable element type, it will not
 *   accept temporaries; only variables or other references. For example:
 *
 *       void FooMut(std::span<int> arg);
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

/** Pop the last element off a span, and return a reference to that element. */
template <typename T>
T& SpanPopBack(std::span<T>& span)
{
    size_t size = span.size();
    T& back = span.back();
    span = span.first(size - 1);
    return back;
}

template <typename V>
auto MakeByteSpan(const V& v) noexcept
{
    return std::as_bytes(std::span{v});
}
template <typename V>
auto MakeWritableByteSpan(V&& v) noexcept
{
    return std::as_writable_bytes(std::span{std::forward<V>(v)});
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

// Helper function to safely convert a span to a span<[const] unsigned char>.
template <typename T, size_t N> constexpr auto UCharSpanCast(std::span<T, N> s) { return std::span<std::remove_pointer_t<decltype(UCharCast(s.data()))>, N>{UCharCast(s.data()), s.size()}; }

/** Like the std::span constructor, but for (const) unsigned char member types only. Only works for (un)signed char containers. */
template <typename V> constexpr auto MakeUCharSpan(const V& v) -> decltype(UCharSpanCast(std::span{v})) { return UCharSpanCast(std::span{v}); }
template <typename V> constexpr auto MakeWritableUCharSpan(V&& v) -> decltype(UCharSpanCast(std::span{std::forward<V>(v)})) { return UCharSpanCast(std::span{std::forward<V>(v)}); }

#endif // BITCOIN_SPAN_H
