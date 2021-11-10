/*!
@file
Defines the `Logical` and `Comparable` models of `boost::hana::integral_constant`.

@copyright Louis Dionne 2013-2017
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE.md or copy at http://boost.org/LICENSE_1_0.txt)
 */

#ifndef BOOST_HANA_BOOL_HPP
#define BOOST_HANA_BOOL_HPP

#include <boost/hana/fwd/bool.hpp>

#include <boost/hana/concept/integral_constant.hpp>
#include <boost/hana/config.hpp>
#include <boost/hana/core/to.hpp>
#include <boost/hana/core/when.hpp>
#include <boost/hana/detail/operators/arithmetic.hpp>
#include <boost/hana/detail/operators/comparable.hpp>
#include <boost/hana/detail/operators/logical.hpp>
#include <boost/hana/detail/operators/orderable.hpp>
#include <boost/hana/eval.hpp>
#include <boost/hana/fwd/core/tag_of.hpp>
#include <boost/hana/fwd/eval_if.hpp>
#include <boost/hana/fwd/if.hpp>
#include <boost/hana/fwd/value.hpp>

#include <cstddef>
#include <type_traits>
#include <utility>


BOOST_HANA_NAMESPACE_BEGIN
    //////////////////////////////////////////////////////////////////////////
    // integral_constant
    //////////////////////////////////////////////////////////////////////////
    //! @cond
    namespace ic_detail {
        template <typename T, T N, typename = std::make_integer_sequence<T, N>>
        struct go;

        template <typename T, T N, T ...i>
        struct go<T, N, std::integer_sequence<T, i...>> {
            using swallow = T[];

            template <typename F>
            static constexpr void with_index(F&& f)
            { (void)swallow{T{}, ((void)f(integral_constant<T, i>{}), i)...}; }

            template <typename F>
            static constexpr void without_index(F&& f)
            { (void)swallow{T{}, ((void)f(), i)...}; }
        };

        template <typename T, T v>
        template <typename F>
        constexpr void with_index_t<T, v>::operator()(F&& f) const
        { go<T, ((void)sizeof(&f), v)>::with_index(static_cast<F&&>(f)); }

        template <typename T, T v>
        template <typename F>
        constexpr void times_t<T, v>::operator()(F&& f) const
        { go<T, ((void)sizeof(&f), v)>::without_index(static_cast<F&&>(f)); }

        // avoid link-time error
        template <typename T, T v>
        constexpr with_index_t<T, v> times_t<T, v>::with_index;
    }

    // avoid link-time error
    template <typename T, T v>
    constexpr ic_detail::times_t<T, v> integral_constant<T, v>::times;

    template <typename T, T v>
    struct tag_of<integral_constant<T, v>> {
        using type = integral_constant_tag<T>;
    };
    //! @endcond

    //////////////////////////////////////////////////////////////////////////
    // Operators
    //////////////////////////////////////////////////////////////////////////
    namespace detail {
        template <typename T>
        struct comparable_operators<integral_constant_tag<T>> {
            static constexpr bool value = true;
        };
        template <typename T>
        struct orderable_operators<integral_constant_tag<T>> {
            static constexpr bool value = true;
        };
        template <typename T>
        struct arithmetic_operators<integral_constant_tag<T>> {
            static constexpr bool value = true;
        };
        template <typename T>
        struct logical_operators<integral_constant_tag<T>> {
            static constexpr bool value = true;
        };
    }

#define BOOST_HANA_INTEGRAL_CONSTANT_BINARY_OP(op)                          \
    template <typename U, U u, typename V, V v>                             \
    constexpr integral_constant<decltype(u op v), (u op v)>                 \
    operator op(integral_constant<U, u>, integral_constant<V, v>)           \
    { return {}; }                                                          \
    /**/

#define BOOST_HANA_INTEGRAL_CONSTANT_UNARY_OP(op)                           \
    template <typename U, U u>                                              \
    constexpr integral_constant<decltype(op u), (op u)>                     \
    operator op(integral_constant<U, u>)                                    \
    { return {}; }                                                          \
    /**/

    // Arithmetic
    BOOST_HANA_INTEGRAL_CONSTANT_UNARY_OP(+)

    // Bitwise
    BOOST_HANA_INTEGRAL_CONSTANT_UNARY_OP(~)
    BOOST_HANA_INTEGRAL_CONSTANT_BINARY_OP(&)
    BOOST_HANA_INTEGRAL_CONSTANT_BINARY_OP(|)
    BOOST_HANA_INTEGRAL_CONSTANT_BINARY_OP(^)
    BOOST_HANA_INTEGRAL_CONSTANT_BINARY_OP(<<)
    BOOST_HANA_INTEGRAL_CONSTANT_BINARY_OP(>>)

#undef BOOST_HANA_INTEGRAL_CONSTANT_UNARY_OP
#undef BOOST_HANA_INTEGRAL_CONSTANT_BINARY_OP


    //////////////////////////////////////////////////////////////////////////
    // User-defined literal
    //////////////////////////////////////////////////////////////////////////
    namespace ic_detail {

        constexpr int to_int(char c) {
            int result = 0;

            if (c >= 'A' && c <= 'F') {
                result = static_cast<int>(c) - static_cast<int>('A') + 10;
            }
            else if (c >= 'a' && c <= 'f') {
                result = static_cast<int>(c) - static_cast<int>('a') + 10;
            }
            else {
                result = static_cast<int>(c) - static_cast<int>('0');
            }

            return result;
        }

        template<std::size_t N>
        constexpr long long parse(const char (&arr)[N]) {
            long long base = 10;
            std::size_t offset = 0;

            if (N > 2) {
                bool starts_with_zero = arr[0] == '0';
                bool is_hex = starts_with_zero && arr[1] == 'x';
                bool is_binary = starts_with_zero && arr[1] == 'b';

                if (is_hex) {
                    //0xDEADBEEF (hexadecimal)
                    base = 16;
                    offset = 2;
                }
                else if (is_binary) {
                    //0b101011101 (binary)
                    base = 2;
                    offset = 2;
                }
                else if (starts_with_zero) {
                    //012345 (octal)
                    base = 8;
                    offset = 1;
                }
            }

            long long number = 0;
            long long multiplier = 1;

            for (std::size_t i = 0; i < N - offset; ++i) {
                char c = arr[N - 1 - i];
                if (c != '\'') { // skip digit separators
                    number += to_int(c) * multiplier;
                    multiplier *= base;
                }
            }

            return number;
        }
    }

    namespace literals {
        template <char ...c>
        constexpr auto operator"" _c() {
            return hana::llong<ic_detail::parse<sizeof...(c)>({c...})>{};
        }
    }

    //////////////////////////////////////////////////////////////////////////
    // Model of Constant/IntegralConstant
    //////////////////////////////////////////////////////////////////////////
    template <typename T>
    struct IntegralConstant<integral_constant_tag<T>> {
        static constexpr bool value = true;
    };

    template <typename T, typename C>
    struct to_impl<integral_constant_tag<T>, C, when<hana::IntegralConstant<C>::value>>
        : embedding<is_embedded<typename C::value_type, T>::value>
    {
        template <typename N>
        static constexpr auto apply(N const&)
        { return integral_constant<T, N::value>{}; }
    };

    //////////////////////////////////////////////////////////////////////////
    // Optimizations
    //////////////////////////////////////////////////////////////////////////
    template <typename T>
    struct eval_if_impl<integral_constant_tag<T>> {
        template <typename Cond, typename Then, typename Else>
        static constexpr decltype(auto)
        apply(Cond const&, Then&& t, Else&& e) {
            constexpr bool cond = static_cast<bool>(Cond::value);
            return eval_if_impl::apply(hana::bool_<cond>{},
                                       static_cast<Then&&>(t),
                                       static_cast<Else&&>(e));
        }

        template <typename Then, typename Else>
        static constexpr decltype(auto)
        apply(hana::true_ const&, Then&& t, Else&&)
        { return hana::eval(static_cast<Then&&>(t)); }

        template <typename Then, typename Else>
        static constexpr decltype(auto)
        apply(hana::false_ const&, Then&&, Else&& e)
        { return hana::eval(static_cast<Else&&>(e)); }
    };

    template <typename T>
    struct if_impl<integral_constant_tag<T>> {
        template <typename Cond, typename Then, typename Else>
        static constexpr decltype(auto)
        apply(Cond const&, Then&& t, Else&& e) {
            constexpr bool cond = static_cast<bool>(Cond::value);
            return if_impl::apply(hana::bool_<cond>{},
                                  static_cast<Then&&>(t),
                                  static_cast<Else&&>(e));
        }

        //! @todo We could return `Then` instead of `auto` to sometimes save
        //! a copy, but that would break some code that would return a
        //! reference to a `type` object. I think the code that would be
        //! broken should be changed, but more thought needs to be given.
        template <typename Then, typename Else>
        static constexpr auto
        apply(hana::true_ const&, Then&& t, Else&&)
        { return static_cast<Then&&>(t); }

        template <typename Then, typename Else>
        static constexpr auto
        apply(hana::false_ const&, Then&&, Else&& e)
        { return static_cast<Else&&>(e); }
    };
BOOST_HANA_NAMESPACE_END

#endif // !BOOST_HANA_BOOL_HPP
