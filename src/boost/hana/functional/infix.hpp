/*!
@file
Defines `boost::hana::infix`.

@copyright Louis Dionne 2013-2017
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE.md or copy at http://boost.org/LICENSE_1_0.txt)
 */

#ifndef BOOST_HANA_FUNCTIONAL_INFIX_HPP
#define BOOST_HANA_FUNCTIONAL_INFIX_HPP

#include <boost/hana/config.hpp>
#include <boost/hana/detail/decay.hpp>
#include <boost/hana/functional/partial.hpp>
#include <boost/hana/functional/reverse_partial.hpp>

#include <type_traits>
#include <utility>


BOOST_HANA_NAMESPACE_BEGIN
    //! @ingroup group-functional
    //! Return an equivalent function that can also be applied in infix
    //! notation.
    //!
    //! Specifically, `infix(f)` is an object such that:
    //! @code
    //!     infix(f)(x1, ..., xn) == f(x1, ..., xn)
    //!     x ^infix(f)^ y == f(x, y)
    //! @endcode
    //!
    //! Hence, the returned function can still be applied using the usual
    //! function call syntax, but it also gains the ability to be applied in
    //! infix notation. The infix syntax allows a great deal of expressiveness,
    //! especially when used in combination with some higher order algorithms.
    //! Since `operator^` is left-associative, `x ^infix(f)^ y` is actually
    //! parsed as `(x ^infix(f))^ y`. However, for flexibility, the order in
    //! which both arguments are applied in infix notation does not matter.
    //! Hence, it is always the case that
    //! @code
    //!     (x ^ infix(f)) ^ y == x ^ (infix(f) ^ y)
    //! @endcode
    //!
    //! However, note that applying more than one argument in infix
    //! notation to the same side of the operator will result in a
    //! compile-time assertion:
    //! @code
    //!     (infix(f) ^ x) ^ y; // compile-time assertion
    //!     y ^ (x ^ infix(f)); // compile-time assertion
    //! @endcode
    //!
    //! Additionally, a function created with `infix` may be partially applied
    //! in infix notation. Specifically,
    //! @code
    //!     (x ^ infix(f))(y1, ..., yn) == f(x, y1, ..., yn)
    //!     (infix(f) ^ y)(x1, ..., xn) == f(x1, ..., xn, y)
    //! @endcode
    //!
    //! @internal
    //! ### Rationales
    //! 1. The `^` operator was chosen because it is left-associative and
    //!    has a low enough priority so that most expressions will render
    //!    the expected behavior.
    //! 2. The operator can't be customimzed because that would require more
    //!    sophistication in the implementation; I want to keep it as simple
    //!    as possible. There is also an advantage in having a uniform syntax
    //!    for infix application.
    //! @endinternal
    //!
    //! @param f
    //! The function which gains the ability to be applied in infix notation.
    //! The function must be at least binary; a compile-time error will be
    //! triggered otherwise.
    //!
    //! ### Example
    //! @include example/functional/infix.cpp
#ifdef BOOST_HANA_DOXYGEN_INVOKED
    constexpr auto infix = [](auto f) {
        return unspecified;
    };
#else
    namespace infix_detail {
        // This needs to be in the same namespace as `operator^` so it can be
        // found by ADL.
        template <bool left, bool right, typename F>
        struct infix_t {
            F f;

            template <typename ...X>
            constexpr decltype(auto) operator()(X&& ...x) const&
            { return f(static_cast<X&&>(x)...); }

            template <typename ...X>
            constexpr decltype(auto) operator()(X&& ...x) &
            { return f(static_cast<X&&>(x)...); }

            template <typename ...X>
            constexpr decltype(auto) operator()(X&& ...x) &&
            { return std::move(f)(static_cast<X&&>(x)...); }
        };

        template <bool left, bool right>
        struct make_infix {
            template <typename F>
            constexpr infix_t<left, right, typename detail::decay<F>::type>
            operator()(F&& f) const { return {static_cast<F&&>(f)}; }
        };

        template <bool left, bool right>
        struct Infix;
        struct Object;

        template <typename T>
        struct dispatch { using type = Object; };

        template <bool left, bool right, typename F>
        struct dispatch<infix_t<left, right, F>> {
            using type = Infix<left, right>;
        };

        template <typename, typename>
        struct bind_infix;

        // infix(f) ^ y
        template <>
        struct bind_infix<Infix<false, false>, Object> {
            template <typename F, typename Y>
            static constexpr decltype(auto) apply(F&& f, Y&& y) {
                return make_infix<false, true>{}(
                    hana::reverse_partial(
                        static_cast<F&&>(f), static_cast<Y&&>(y)
                    )
                );
            }
        };

        // (x^infix(f)) ^ y
        template <>
        struct bind_infix<Infix<true, false>, Object> {
            template <typename F, typename Y>
            static constexpr decltype(auto) apply(F&& f, Y&& y) {
                return static_cast<F&&>(f)(static_cast<Y&&>(y));
            }
        };

        // x ^ infix(f)
        template <>
        struct bind_infix<Object, Infix<false, false>> {
            template <typename X, typename F>
            static constexpr decltype(auto) apply(X&& x, F&& f) {
                return make_infix<true, false>{}(
                    hana::partial(static_cast<F&&>(f), static_cast<X&&>(x))
                );
            }
        };

        // x ^ (infix(f)^y)
        template <>
        struct bind_infix<Object, Infix<false, true>> {
            template <typename X, typename F>
            static constexpr decltype(auto) apply(X&& x, F&& f) {
                return static_cast<F&&>(f)(static_cast<X&&>(x));
            }
        };

        template <typename T>
        using strip = typename std::remove_cv<
            typename std::remove_reference<T>::type
        >::type;

        template <typename X, typename Y>
        constexpr decltype(auto) operator^(X&& x, Y&& y) {
            return bind_infix<
                typename dispatch<strip<X>>::type,
                typename dispatch<strip<Y>>::type
            >::apply(static_cast<X&&>(x), static_cast<Y&&>(y));
        }
    } // end namespace infix_detail

    BOOST_HANA_INLINE_VARIABLE constexpr infix_detail::make_infix<false, false> infix{};
#endif
BOOST_HANA_NAMESPACE_END

#endif // !BOOST_HANA_FUNCTIONAL_INFIX_HPP
