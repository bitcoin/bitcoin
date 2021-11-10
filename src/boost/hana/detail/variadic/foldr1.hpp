/*!
@file
Defines `boost::hana::detail::variadic::foldr1`.

@copyright Louis Dionne 2013-2017
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE.md or copy at http://boost.org/LICENSE_1_0.txt)
 */

#ifndef BOOST_HANA_DETAIL_VARIADIC_FOLDR1_HPP
#define BOOST_HANA_DETAIL_VARIADIC_FOLDR1_HPP

#include <boost/hana/config.hpp>
#include <boost/hana/core/when.hpp>


BOOST_HANA_NAMESPACE_BEGIN namespace detail { namespace variadic {
    //! @cond
    template <unsigned int n, typename = when<true>>
    struct foldr1_impl;

    template <>
    struct foldr1_impl<1> {
        template <typename F, typename X1>
        static constexpr X1 apply(F&&, X1&& x1)
        { return static_cast<X1&&>(x1); }
    };

    template <>
    struct foldr1_impl<2> {
        template <typename F, typename X1, typename X2>
        static constexpr decltype(auto) apply(F&& f, X1&& x1, X2&& x2) {
            return static_cast<F&&>(f)(static_cast<X1&&>(x1),
                                       static_cast<X2&&>(x2));
        }
    };

    template <>
    struct foldr1_impl<3> {
        template <typename F, typename X1, typename X2, typename X3>
        static constexpr decltype(auto) apply(F&& f, X1&& x1, X2&& x2, X3&& x3) {
            return f(static_cast<X1&&>(x1),
                   f(static_cast<X2&&>(x2),
                     static_cast<X3&&>(x3)));
        }
    };

    template <>
    struct foldr1_impl<4> {
        template <typename F, typename X1, typename X2, typename X3, typename X4>
        static constexpr decltype(auto) apply(F&& f, X1&& x1, X2&& x2, X3&& x3, X4&& x4) {
            return f(static_cast<X1&&>(x1),
                   f(static_cast<X2&&>(x2),
                   f(static_cast<X3&&>(x3),
                     static_cast<X4&&>(x4))));
        }
    };

    template <>
    struct foldr1_impl<5> {
        template <typename F, typename X1, typename X2, typename X3, typename X4, typename X5>
        static constexpr decltype(auto) apply(F&& f, X1&& x1, X2&& x2, X3&& x3, X4&& x4, X5&& x5) {
            return f(static_cast<X1&&>(x1),
                   f(static_cast<X2&&>(x2),
                   f(static_cast<X3&&>(x3),
                   f(static_cast<X4&&>(x4),
                     static_cast<X5&&>(x5)))));
        }
    };

    template <>
    struct foldr1_impl<6> {
        template <typename F, typename X1, typename X2, typename X3, typename X4, typename X5, typename X6>
        static constexpr decltype(auto) apply(F&& f, X1&& x1, X2&& x2, X3&& x3, X4&& x4, X5&& x5, X6&& x6) {
            return f(static_cast<X1&&>(x1),
                   f(static_cast<X2&&>(x2),
                   f(static_cast<X3&&>(x3),
                   f(static_cast<X4&&>(x4),
                   f(static_cast<X5&&>(x5),
                     static_cast<X6&&>(x6))))));
        }
    };

    template <unsigned int n>
    struct foldr1_impl<n, when<(n >= 7) && (n < 14)>> {
        template <typename F, typename X1, typename X2, typename X3, typename X4, typename X5, typename X6, typename X7, typename ...Xn>
        static constexpr decltype(auto)
        apply(F&& f
              , X1&& x1, X2&& x2, X3&& x3, X4&& x4, X5&& x5, X6&& x6, X7&& x7
              , Xn&& ...xn)
        {
            return f(static_cast<X1&&>(x1),
                   f(static_cast<X2&&>(x2),
                   f(static_cast<X3&&>(x3),
                   f(static_cast<X4&&>(x4),
                   f(static_cast<X5&&>(x5),
                   f(static_cast<X6&&>(x6),
                     foldr1_impl<sizeof...(xn) + 1>::apply(f, static_cast<X7&&>(x7), static_cast<Xn&&>(xn)...)))))));
        }
    };

    template <unsigned int n>
    struct foldr1_impl<n, when<(n >= 14) && (n < 28)>> {
        template <
              typename F
            , typename X1, typename X2, typename X3,  typename X4,  typename X5,  typename X6,  typename X7
            , typename X8, typename X9, typename X10, typename X11, typename X12, typename X13, typename X14
            , typename ...Xn
        >
        static constexpr decltype(auto)
        apply(F&& f
              , X1&& x1, X2&& x2, X3&& x3,   X4&& x4,   X5&& x5,   X6&& x6,   X7&& x7
              , X8&& x8, X9&& x9, X10&& x10, X11&& x11, X12&& x12, X13&& x13, X14&& x14
              , Xn&& ...xn)
        {
            return f(static_cast<X1&&>(x1), f(static_cast<X2&&>(x2), f(static_cast<X3&&>(x3),   f(static_cast<X4&&>(x4),   f(static_cast<X5&&>(x5),   f(static_cast<X6&&>(x6),   f(static_cast<X7&&>(x7),
                   f(static_cast<X8&&>(x8), f(static_cast<X9&&>(x9), f(static_cast<X10&&>(x10), f(static_cast<X11&&>(x11), f(static_cast<X12&&>(x12), f(static_cast<X13&&>(x13),
                     foldr1_impl<sizeof...(xn) + 1>::apply(f, static_cast<X14&&>(x14), static_cast<Xn&&>(xn)...))))))))))))));
        }
    };

    template <unsigned int n>
    struct foldr1_impl<n, when<(n >= 28) && (n < 56)>> {
        template <
              typename F
            , typename X1,  typename X2,  typename X3,  typename X4,  typename X5,  typename X6,  typename X7
            , typename X8,  typename X9,  typename X10, typename X11, typename X12, typename X13, typename X14
            , typename X15, typename X16, typename X17, typename X18, typename X19, typename X20, typename X21
            , typename X22, typename X23, typename X24, typename X25, typename X26, typename X27, typename X28
            , typename ...Xn
        >
        static constexpr decltype(auto)
        apply(F&& f
              , X1&& x1,   X2&& x2,   X3&& x3,   X4&& x4,   X5&& x5,   X6&& x6,   X7&& x7
              , X8&& x8,   X9&& x9,   X10&& x10, X11&& x11, X12&& x12, X13&& x13, X14&& x14
              , X15&& x15, X16&& x16, X17&& x17, X18&& x18, X19&& x19, X20&& x20, X21&& x21
              , X22&& x22, X23&& x23, X24&& x24, X25&& x25, X26&& x26, X27&& x27, X28&& x28
              , Xn&& ...xn)
        {
            return f(static_cast<X1&&>(x1),   f(static_cast<X2&&>(x2),   f(static_cast<X3&&>(x3),   f(static_cast<X4&&>(x4),   f(static_cast<X5&&>(x5),   f(static_cast<X6&&>(x6),   f(static_cast<X7&&>(x7),
                   f(static_cast<X8&&>(x8),   f(static_cast<X9&&>(x9),   f(static_cast<X10&&>(x10), f(static_cast<X11&&>(x11), f(static_cast<X12&&>(x12), f(static_cast<X13&&>(x13), f(static_cast<X14&&>(x14),
                   f(static_cast<X15&&>(x15), f(static_cast<X16&&>(x16), f(static_cast<X17&&>(x17), f(static_cast<X18&&>(x18), f(static_cast<X19&&>(x19), f(static_cast<X20&&>(x20), f(static_cast<X21&&>(x21),
                   f(static_cast<X22&&>(x22), f(static_cast<X23&&>(x23), f(static_cast<X24&&>(x24), f(static_cast<X25&&>(x25), f(static_cast<X26&&>(x26), f(static_cast<X27&&>(x27),
                     foldr1_impl<sizeof...(xn) + 1>::apply(f, static_cast<X28&&>(x28), static_cast<Xn&&>(xn)...))))))))))))))))))))))))))));
        }
    };

    template <unsigned int n>
    struct foldr1_impl<n, when<(n >= 56)>> {
        template <
              typename F
            , typename X1,  typename X2,  typename X3,  typename X4,  typename X5,  typename X6,  typename X7
            , typename X8,  typename X9,  typename X10, typename X11, typename X12, typename X13, typename X14
            , typename X15, typename X16, typename X17, typename X18, typename X19, typename X20, typename X21
            , typename X22, typename X23, typename X24, typename X25, typename X26, typename X27, typename X28
            , typename X29, typename X30, typename X31, typename X32, typename X33, typename X34, typename X35
            , typename X36, typename X37, typename X38, typename X39, typename X40, typename X41, typename X42
            , typename X43, typename X44, typename X45, typename X46, typename X47, typename X48, typename X49
            , typename X50, typename X51, typename X52, typename X53, typename X54, typename X55, typename X56
            , typename ...Xn
        >
        static constexpr decltype(auto)
        apply(F&& f
              , X1&& x1,   X2&& x2,   X3&& x3,   X4&& x4,   X5&& x5,   X6&& x6,   X7&& x7
              , X8&& x8,   X9&& x9,   X10&& x10, X11&& x11, X12&& x12, X13&& x13, X14&& x14
              , X15&& x15, X16&& x16, X17&& x17, X18&& x18, X19&& x19, X20&& x20, X21&& x21
              , X22&& x22, X23&& x23, X24&& x24, X25&& x25, X26&& x26, X27&& x27, X28&& x28
              , X29&& x29, X30&& x30, X31&& x31, X32&& x32, X33&& x33, X34&& x34, X35&& x35
              , X36&& x36, X37&& x37, X38&& x38, X39&& x39, X40&& x40, X41&& x41, X42&& x42
              , X43&& x43, X44&& x44, X45&& x45, X46&& x46, X47&& x47, X48&& x48, X49&& x49
              , X50&& x50, X51&& x51, X52&& x52, X53&& x53, X54&& x54, X55&& x55, X56&& x56
              , Xn&& ...xn)
        {
            return f(static_cast<X1&&>(x1),   f(static_cast<X2&&>(x2),   f(static_cast<X3&&>(x3),   f(static_cast<X4&&>(x4),   f(static_cast<X5&&>(x5),   f(static_cast<X6&&>(x6),   f(static_cast<X7&&>(x7),
                   f(static_cast<X8&&>(x8),   f(static_cast<X9&&>(x9),   f(static_cast<X10&&>(x10), f(static_cast<X11&&>(x11), f(static_cast<X12&&>(x12), f(static_cast<X13&&>(x13), f(static_cast<X14&&>(x14),
                   f(static_cast<X15&&>(x15), f(static_cast<X16&&>(x16), f(static_cast<X17&&>(x17), f(static_cast<X18&&>(x18), f(static_cast<X19&&>(x19), f(static_cast<X20&&>(x20), f(static_cast<X21&&>(x21),
                   f(static_cast<X22&&>(x22), f(static_cast<X23&&>(x23), f(static_cast<X24&&>(x24), f(static_cast<X25&&>(x25), f(static_cast<X26&&>(x26), f(static_cast<X27&&>(x27), f(static_cast<X28&&>(x28),
                   f(static_cast<X29&&>(x29), f(static_cast<X30&&>(x30), f(static_cast<X31&&>(x31), f(static_cast<X32&&>(x32), f(static_cast<X33&&>(x33), f(static_cast<X34&&>(x34), f(static_cast<X35&&>(x35),
                   f(static_cast<X36&&>(x36), f(static_cast<X37&&>(x37), f(static_cast<X38&&>(x38), f(static_cast<X39&&>(x39), f(static_cast<X40&&>(x40), f(static_cast<X41&&>(x41), f(static_cast<X42&&>(x42),
                   f(static_cast<X43&&>(x43), f(static_cast<X44&&>(x44), f(static_cast<X45&&>(x45), f(static_cast<X46&&>(x46), f(static_cast<X47&&>(x47), f(static_cast<X48&&>(x48), f(static_cast<X49&&>(x49),
                   f(static_cast<X50&&>(x50), f(static_cast<X51&&>(x51), f(static_cast<X52&&>(x52), f(static_cast<X53&&>(x53), f(static_cast<X54&&>(x54), f(static_cast<X55&&>(x55),
                     foldr1_impl<sizeof...(xn) + 1>::apply(f, static_cast<X56&&>(x56), static_cast<Xn&&>(xn)...))))))))))))))))))))))))))))))))))))))))))))))))))))))));
        }
    };
    //! @endcond

    struct foldr1_t {
        template <typename F, typename X1, typename ...Xn>
        constexpr decltype(auto) operator()(F&& f, X1&& x1, Xn&& ...xn) const {
            return foldr1_impl<sizeof...(xn) + 1>::apply(
                static_cast<F&&>(f), static_cast<X1&&>(x1), static_cast<Xn&&>(xn)...
            );
        }
    };

    BOOST_HANA_INLINE_VARIABLE constexpr foldr1_t foldr1{};

    struct foldr_t {
        template <typename F, typename State, typename ...Xn>
        constexpr decltype(auto) operator()(F&& f, State&& state, Xn&& ...xn) const {
            return foldr1_impl<sizeof...(xn) + 1>::apply(
                static_cast<F&&>(f), static_cast<Xn&&>(xn)..., static_cast<State&&>(state)
            );
        }
    };

    BOOST_HANA_INLINE_VARIABLE constexpr foldr_t foldr{};
}} BOOST_HANA_NAMESPACE_END

#endif // !BOOST_HANA_DETAIL_VARIADIC_FOLDR1_HPP
