/*!
@file
Forward declares `boost::hana::string`.

@copyright Louis Dionne 2013-2017
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE.md or copy at http://boost.org/LICENSE_1_0.txt)
 */

#ifndef BOOST_HANA_FWD_STRING_HPP
#define BOOST_HANA_FWD_STRING_HPP

#include <boost/hana/config.hpp>
#include <boost/hana/fwd/core/make.hpp>
#include <boost/hana/fwd/core/to.hpp>


BOOST_HANA_NAMESPACE_BEGIN
#ifdef BOOST_HANA_DOXYGEN_INVOKED
    //! @ingroup group-datatypes
    //! Compile-time string.
    //!
    //! Conceptually, a `hana::string` is like a tuple holding
    //! `integral_constant`s of underlying type `char`. However, the
    //! interface of `hana::string` is not as rich as that of a tuple,
    //! because a string can only hold compile-time characters as opposed
    //! to any kind of object.
    //!
    //! Compile-time strings are used for simple purposes like being keys in a
    //! `hana::map` or tagging the members of a `Struct`. However, you might
    //! find that `hana::string` does not provide enough functionality to be
    //! used as a full-blown compile-time string implementation (e.g. regexp
    //! matching or substring finding). Indeed, providing a comprehensive
    //! string interface is a lot of job, and it is out of the scope of the
    //! library for the time being.
    //!
    //!
    //! @note
    //! The representation of `hana::string` is implementation-defined.
    //! In particular, one should not take for granted that the template
    //! parameters are `char`s. The proper way to access the contents of
    //! a `hana::string` as character constants is to use `hana::unpack`,
    //! `.c_str()` or `hana::to<char const*>`, as documented below. More
    //! details [in the tutorial](@ref tutorial-containers-types).
    //!
    //!
    //! Modeled concepts
    //! ----------------
    //! For most purposes, a `hana::string` is functionally equivalent to a
    //! tuple holding `Constant`s of underlying type `char`.
    //!
    //! 1. `Comparable`\n
    //! Two strings are equal if and only if they have the same number of
    //! characters and characters at corresponding indices are equal.
    //! @include example/string/comparable.cpp
    //!
    //! 2. `Orderable`\n
    //! The total order implemented for `Orderable` is the usual
    //! lexicographical comparison of strings.
    //! @include example/string/orderable.cpp
    //!
    //! 3. `Monoid`\n
    //! Strings form a monoid under concatenation, with the neutral element
    //! being the empty string.
    //! @include example/string/monoid.cpp
    //!
    //! 4. `Foldable`\n
    //! Folding a string is equivalent to folding the sequence of its
    //! characters.
    //! @include example/string/foldable.cpp
    //!
    //! 5. `Iterable`\n
    //! Iterating over a string is equivalent to iterating over the sequence
    //! of its characters. Also note that `operator[]` can be used instead of
    //! the `at` function.
    //! @include example/string/iterable.cpp
    //!
    //! 6. `Searchable`\n
    //! Searching through a string is equivalent to searching through the
    //! sequence of its characters.
    //! @include example/string/searchable.cpp
    //!
    //! 7. `Hashable`\n
    //! The hash of a compile-time string is a type uniquely representing
    //! that string.
    //! @include example/string/hashable.cpp
    //!
    //!
    //! Conversion to `char const*`
    //! ---------------------------
    //! A `hana::string` can be converted to a `constexpr` null-delimited
    //! string of type `char const*` by using the `c_str()` method or
    //! `hana::to<char const*>`. This makes it easy to turn a compile-time
    //! string into a runtime string. However, note that this conversion is
    //! not an embedding, because `char const*` does not model the same
    //! concepts as `hana::string` does.
    //! @include example/string/to.cpp
    //!
    //! Conversion from any Constant holding a `char const*`
    //! ----------------------------------------------------
    //! A `hana::string` can be created from any `Constant` whose underlying
    //! value is convertible to a `char const*` by using `hana::to`. The
    //! contents of the `char const*` are used to build the content of the
    //! `hana::string`.
    //! @include example/string/from_c_str.cpp
    //!
    //! Rationale for `hana::string` not being a `Constant` itself
    //! ----------------------------------------------------------
    //! The underlying type held by a `hana::string` could be either `char const*`
    //! or some other constexpr-enabled string-like container. In the first case,
    //! `hana::string` can not be a `Constant` because the models of several
    //! concepts would not be respected by the underlying type, causing `value`
    //! not to be structure-preserving. Providing an underlying value of
    //! constexpr-enabled string-like container type like `std::string_view`
    //! would be great, but that's a bit complicated for the time being.
    template <typename implementation_defined>
    struct string {
        // Default-construct a `hana::string`; no-op since `hana::string` is stateless.
        constexpr string() = default;

        // Copy-construct a `hana::string`; no-op since `hana::string` is stateless.
        constexpr string(string const&) = default;

        //! Equivalent to `hana::equal`
        template <typename X, typename Y>
        friend constexpr auto operator==(X&& x, Y&& y);

        //! Equivalent to `hana::not_equal`
        template <typename X, typename Y>
        friend constexpr auto operator!=(X&& x, Y&& y);

        //! Equivalent to `hana::less`
        template <typename X, typename Y>
        friend constexpr auto operator<(X&& x, Y&& y);

        //! Equivalent to `hana::greater`
        template <typename X, typename Y>
        friend constexpr auto operator>(X&& x, Y&& y);

        //! Equivalent to `hana::less_equal`
        template <typename X, typename Y>
        friend constexpr auto operator<=(X&& x, Y&& y);

        //! Equivalent to `hana::greater_equal`
        template <typename X, typename Y>
        friend constexpr auto operator>=(X&& x, Y&& y);

        //! Performs concatenation; equivalent to `hana::plus`
        template <typename X, typename Y>
        friend constexpr auto operator+(X&& x, Y&& y);

        //! Equivalent to `hana::at`
        template <typename N>
        constexpr decltype(auto) operator[](N&& n);

        //! Returns a null-delimited C-style string.
        static constexpr char const* c_str();
    };
#else
    template <char ...s>
    struct string;
#endif

    //! Tag representing a compile-time string.
    //! @relates hana::string
    struct string_tag { };

#ifdef BOOST_HANA_DOXYGEN_INVOKED
    //! Create a compile-time `hana::string` from a parameter pack of `char`
    //! `integral_constant`s.
    //! @relates hana::string
    //!
    //! Given zero or more `integral_constant`s of underlying type `char`,
    //! `make<string_tag>` creates a `hana::string` containing those characters.
    //! This is provided mostly for consistency with the rest of the library,
    //! as `hana::string_c` is more convenient to use in most cases.
    //!
    //!
    //! Example
    //! -------
    //! @include example/string/make.cpp
    template <>
    constexpr auto make<string_tag> = [](auto&& ...chars) {
        return string<implementation_defined>{};
    };
#endif

    //! Alias to `make<string_tag>`; provided for convenience.
    //! @relates hana::string
    BOOST_HANA_INLINE_VARIABLE constexpr auto make_string = make<string_tag>;

    //! Equivalent to `to<string_tag>`; provided for convenience.
    //! @relates hana::string
    BOOST_HANA_INLINE_VARIABLE constexpr auto to_string = to<string_tag>;

    //! Create a compile-time string from a parameter pack of characters.
    //! @relates hana::string
    //!
    //!
    //! Example
    //! -------
    //! @include example/string/string_c.cpp
#ifdef BOOST_HANA_DOXYGEN_INVOKED
    template <char ...s>
    constexpr string<implementation_defined> string_c{};
#else
    template <char ...s>
    BOOST_HANA_INLINE_VARIABLE constexpr string<s...> string_c{};
#endif

    //! Create a compile-time string from a string literal.
    //! @relates hana::string
    //!
    //! This macro is a more convenient alternative to `string_c` for creating
    //! compile-time strings. However, since this macro uses a lambda
    //! internally, it can't be used in an unevaluated context, or where
    //! a constant expression is expected before C++17.
    //!
    //!
    //! Example
    //! -------
    //! @include example/string/macro.cpp
#ifdef BOOST_HANA_DOXYGEN_INVOKED
    auto BOOST_HANA_STRING(s) = see documentation;
    #define BOOST_HANA_STRING(s) see documentation

    // Note:
    // The trick above seems to exploit a bug in Doxygen, which makes the
    // BOOST_HANA_STRING macro appear in the related objects of hana::string
    // (as we want it to).
#else
    // defined in <boost/hana/string.hpp>
#endif

#ifdef BOOST_HANA_CONFIG_ENABLE_STRING_UDL
    namespace literals {
        //! Creates a compile-time string from a string literal.
        //! @relatesalso boost::hana::string
        //!
        //! The string literal is parsed at compile-time and the result is
        //! returned as a `hana::string`. This feature is an extension that
        //! is disabled by default; see below for details.
        //!
        //! @note
        //! Only narrow string literals are supported right now; support for
        //! fancier types of string literals like wide or UTF-XX might be
        //! added in the future if there is a demand for it. See [this issue]
        //! [Hana.issue80] if you need this.
        //!
        //! @warning
        //! This user-defined literal is an extension which requires a special
        //! string literal operator that is not part of the standard yet.
        //! That operator is supported by both Clang and GCC, and several
        //! proposals were made for it to enter C++17. However, since it is
        //! not standard, it is disabled by default and defining the
        //! `BOOST_HANA_CONFIG_ENABLE_STRING_UDL` config macro is required
        //! to get this operator. Hence, if you want to stay safe, just use
        //! the `BOOST_HANA_STRING` macro instead. If you want to be fast and
        //! furious (I do), define `BOOST_HANA_CONFIG_ENABLE_STRING_UDL`.
        //!
        //!
        //! Example
        //! -------
        //! @include example/string/literal.cpp
        //!
        //! [Hana.issue80]: https://github.com/boostorg/hana/issues/80
        template <typename CharT, CharT ...s>
        constexpr auto operator"" _s();
    }
#endif
BOOST_HANA_NAMESPACE_END

#endif // !BOOST_HANA_FWD_STRING_HPP
