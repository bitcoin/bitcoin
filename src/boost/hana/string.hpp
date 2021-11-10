/*!
@file
Defines `boost::hana::string`.

@copyright Louis Dionne 2013-2017
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE.md or copy at http://boost.org/LICENSE_1_0.txt)
 */

#ifndef BOOST_HANA_STRING_HPP
#define BOOST_HANA_STRING_HPP

#include <boost/hana/fwd/string.hpp>

#include <boost/hana/bool.hpp>
#include <boost/hana/concept/constant.hpp>
#include <boost/hana/config.hpp>
#include <boost/hana/core/make.hpp>
#include <boost/hana/detail/algorithm.hpp>
#include <boost/hana/detail/operators/adl.hpp>
#include <boost/hana/detail/operators/comparable.hpp>
#include <boost/hana/detail/operators/iterable.hpp>
#include <boost/hana/detail/operators/orderable.hpp>
#include <boost/hana/fwd/at.hpp>
#include <boost/hana/fwd/contains.hpp>
#include <boost/hana/fwd/core/tag_of.hpp>
#include <boost/hana/fwd/core/to.hpp>
#include <boost/hana/fwd/drop_front.hpp>
#include <boost/hana/fwd/equal.hpp>
#include <boost/hana/fwd/find.hpp>
#include <boost/hana/fwd/front.hpp>
#include <boost/hana/fwd/hash.hpp>
#include <boost/hana/fwd/is_empty.hpp>
#include <boost/hana/fwd/length.hpp>
#include <boost/hana/fwd/less.hpp>
#include <boost/hana/fwd/plus.hpp>
#include <boost/hana/fwd/unpack.hpp>
#include <boost/hana/fwd/zero.hpp>
#include <boost/hana/if.hpp>
#include <boost/hana/integral_constant.hpp>
#include <boost/hana/optional.hpp>
#include <boost/hana/type.hpp>

#include <utility>
#include <cstddef>
#include <type_traits>


BOOST_HANA_NAMESPACE_BEGIN
    //////////////////////////////////////////////////////////////////////////
    // string<>
    //////////////////////////////////////////////////////////////////////////
    //! @cond
    namespace detail {
        template <char ...s>
        constexpr char const string_storage[sizeof...(s) + 1] = {s..., '\0'};
    }

    template <char ...s>
    struct string
        : detail::operators::adl<string<s...>>
        , detail::iterable_operators<string<s...>>
    {
        static constexpr char const* c_str() {
            return &detail::string_storage<s...>[0];
        }
    };
    //! @endcond

    template <char ...s>
    struct tag_of<string<s...>> {
        using type = string_tag;
    };

    //////////////////////////////////////////////////////////////////////////
    // make<string_tag>
    //////////////////////////////////////////////////////////////////////////
    template <>
    struct make_impl<string_tag> {
        template <typename ...Chars>
        static constexpr auto apply(Chars const& ...) {
            return hana::string<hana::value<Chars>()...>{};
        }
    };

    //////////////////////////////////////////////////////////////////////////
    // BOOST_HANA_STRING
    //////////////////////////////////////////////////////////////////////////
    namespace string_detail {
        template <typename S, std::size_t ...N>
        constexpr string<S::get()[N]...>
        prepare_impl(S, std::index_sequence<N...>)
        { return {}; }

        template <typename S>
        constexpr decltype(auto) prepare(S s) {
            return prepare_impl(s,
                std::make_index_sequence<sizeof(S::get()) - 1>{});
        }
    }

#define BOOST_HANA_STRING(s)                                                \
    (::boost::hana::string_detail::prepare([]{                              \
        struct tmp {                                                        \
            static constexpr decltype(auto) get() { return s; }             \
        };                                                                  \
        return tmp{};                                                       \
    }()))                                                                   \
/**/

#ifdef BOOST_HANA_CONFIG_ENABLE_STRING_UDL
    //////////////////////////////////////////////////////////////////////////
    // _s user-defined literal
    //////////////////////////////////////////////////////////////////////////
    namespace literals {
        template <typename CharT, CharT ...s>
        constexpr auto operator"" _s() {
            static_assert(std::is_same<CharT, char>::value,
            "hana::string: Only narrow string literals are supported with "
            "the _s string literal right now. See https://github.com/boostorg/hana/issues/80 "
            "if you need support for fancier types of compile-time strings.");
            return hana::string_c<s...>;
        }
    }
#endif

    //////////////////////////////////////////////////////////////////////////
    // Operators
    //////////////////////////////////////////////////////////////////////////
    namespace detail {
        template <>
        struct comparable_operators<string_tag> {
            static constexpr bool value = true;
        };
        template <>
        struct orderable_operators<string_tag> {
            static constexpr bool value = true;
        };
    }

    //////////////////////////////////////////////////////////////////////////
    // to<char const*>
    //////////////////////////////////////////////////////////////////////////
    template <>
    struct to_impl<char const*, string_tag> {
        template <char ...c>
        static constexpr char const* apply(string<c...> const&)
        { return string<c...>::c_str(); }
    };

    //////////////////////////////////////////////////////////////////////////
    // to<string_tag>
    //////////////////////////////////////////////////////////////////////////
    namespace detail {
        constexpr std::size_t cx_strlen(char const* s) {
          std::size_t n = 0u;
          while (*s != '\0')
            ++s, ++n;
          return n;
        }

        template <typename S, std::size_t ...I>
        constexpr hana::string<hana::value<S>()[I]...> expand(std::index_sequence<I...>)
        { return {}; }
    }

    template <typename IC>
    struct to_impl<hana::string_tag, IC, hana::when<
        hana::Constant<IC>::value &&
        std::is_convertible<typename IC::value_type, char const*>::value
    >> {
        template <typename S>
        static constexpr auto apply(S const&) {
            constexpr char const* s = hana::value<S>();
            constexpr std::size_t len = detail::cx_strlen(s);
            return detail::expand<S>(std::make_index_sequence<len>{});
        }
    };

    //////////////////////////////////////////////////////////////////////////
    // Comparable
    //////////////////////////////////////////////////////////////////////////
    template <>
    struct equal_impl<string_tag, string_tag> {
        template <typename S>
        static constexpr auto apply(S const&, S const&)
        { return hana::true_c; }

        template <typename S1, typename S2>
        static constexpr auto apply(S1 const&, S2 const&)
        { return hana::false_c; }
    };

    //////////////////////////////////////////////////////////////////////////
    // Orderable
    //////////////////////////////////////////////////////////////////////////
    template <>
    struct less_impl<string_tag, string_tag> {
        template <char ...s1, char ...s2>
        static constexpr auto
        apply(string<s1...> const&, string<s2...> const&) {
            // We put a '\0' at the end only to avoid empty arrays.
            constexpr char const c_str1[] = {s1..., '\0'};
            constexpr char const c_str2[] = {s2..., '\0'};
            return hana::bool_c<detail::lexicographical_compare(
                c_str1, c_str1 + sizeof...(s1),
                c_str2, c_str2 + sizeof...(s2)
            )>;
        }
    };

    //////////////////////////////////////////////////////////////////////////
    // Monoid
    //////////////////////////////////////////////////////////////////////////
    template <>
    struct plus_impl<string_tag, string_tag> {
        template <char ...s1, char ...s2>
        static constexpr auto
        apply(string<s1...> const&, string<s2...> const&) {
            return string<s1..., s2...>{};
        }
    };

    template <>
    struct zero_impl<string_tag> {
        static constexpr auto apply() {
            return string<>{};
        }
    };

    template <char ...s1, char ...s2>
    constexpr auto operator+(string<s1...> const&, string<s2...> const&) {
        return hana::string<s1..., s2...>{};
    }

    //////////////////////////////////////////////////////////////////////////
    // Foldable
    //////////////////////////////////////////////////////////////////////////
    template <>
    struct unpack_impl<string_tag> {
        template <char ...s, typename F>
        static constexpr decltype(auto) apply(string<s...> const&, F&& f)
        { return static_cast<F&&>(f)(char_<s>{}...); }
    };

    template <>
    struct length_impl<string_tag> {
        template <char ...s>
        static constexpr auto apply(string<s...> const&)
        { return hana::size_c<sizeof...(s)>; }
    };

    //////////////////////////////////////////////////////////////////////////
    // Iterable
    //////////////////////////////////////////////////////////////////////////
    template <>
    struct front_impl<string_tag> {
        template <char x, char ...xs>
        static constexpr auto apply(string<x, xs...> const&)
        { return hana::char_c<x>; }
    };

    template <>
    struct drop_front_impl<string_tag> {
        template <std::size_t N, char ...xs, std::size_t ...i>
        static constexpr auto helper(string<xs...> const&, std::index_sequence<i...>) {
            constexpr char s[] = {xs...};
            return hana::string_c<s[i + N]...>;
        }

        template <char ...xs, typename N>
        static constexpr auto apply(string<xs...> const& s, N const&) {
            return helper<N::value>(s, std::make_index_sequence<
                (N::value < sizeof...(xs)) ? sizeof...(xs) - N::value : 0
            >{});
        }

        template <typename N>
        static constexpr auto apply(string<> const& s, N const&)
        { return s; }
    };

    template <>
    struct is_empty_impl<string_tag> {
        template <char ...s>
        static constexpr auto apply(string<s...> const&)
        { return hana::bool_c<sizeof...(s) == 0>; }
    };

    template <>
    struct at_impl<string_tag> {
        template <char ...s, typename N>
        static constexpr auto apply(string<s...> const&, N const&) {
            // We put a '\0' at the end to avoid an empty array.
            constexpr char characters[] = {s..., '\0'};
            constexpr auto n = N::value;
            return hana::char_c<characters[n]>;
        }
    };

    //////////////////////////////////////////////////////////////////////////
    // Searchable
    //////////////////////////////////////////////////////////////////////////
    template <>
    struct contains_impl<string_tag> {
        template <char ...s, typename C>
        static constexpr auto
        helper(string<s...> const&, C const&, hana::true_) {
            constexpr char const characters[] = {s..., '\0'};
            constexpr char c = hana::value<C>();
            return hana::bool_c<
                detail::find(characters, characters + sizeof...(s), c)
                    != characters + sizeof...(s)
            >;
        }

        template <typename S, typename C>
        static constexpr auto helper(S const&, C const&, hana::false_)
        { return hana::false_c; }

        template <typename S, typename C>
        static constexpr auto apply(S const& s, C const& c)
        { return helper(s, c, hana::bool_c<hana::Constant<C>::value>); }
    };

    template <>
    struct find_impl<string_tag> {
        template <char ...s, typename Char>
        static constexpr auto apply(string<s...> const& str, Char const& c) {
            return hana::if_(contains_impl<string_tag>::apply(str, c),
                hana::just(c),
                hana::nothing
            );
        }
    };

    //////////////////////////////////////////////////////////////////////////
    // Hashable
    //////////////////////////////////////////////////////////////////////////
    template <>
    struct hash_impl<string_tag> {
        template <typename String>
        static constexpr auto apply(String const&) {
            return hana::type_c<String>;
        }
    };
BOOST_HANA_NAMESPACE_END

#endif // !BOOST_HANA_STRING_HPP
